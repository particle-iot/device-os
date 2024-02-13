/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <string>
#include <mutex>
#include <memory>

#include "device_config.h"
#include "sparse_buffer.h"

#include "exflash_hal.h"
#include "flash_mal.h"

#include "endian_util.h"
#include "system_error.h"
#include "logging.h"

using namespace particle;

namespace fs = std::filesystem;

namespace {

class ExternalFlash {
public:
    void read(uintptr_t addr, uint8_t* data, size_t size) const {
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        auto s = buf_.read(addr, size);
        std::memcpy(data, s.data(), size);
    }

    void write(uintptr_t addr, const uint8_t* data, size_t size) {
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        // Read the contents of the affected region
        std::string s = buf_.read(addr, size);
        uint8_t* d = (uint8_t*)s.data();
        for (size_t i = 0; i < size; ++i) {
            d[i] &= data[i]; // Pretend this is flash memory
        }
        // Write the changes
        buf_.write(addr, s);
        bufferChanged();
    }

    void erase(uintptr_t addr, size_t blockCount, size_t blockSize) {
        if (!blockSize) {
            return;
        }
        addr = addr / blockSize * blockSize;
        size_t size = blockSize * blockCount;
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        buf_.erase(addr, size);
        bufferChanged();
    }

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.lock();
    }

    static ExternalFlash* instance() {
        static ExternalFlash f;
        return &f;
    }

private:
    SparseBuffer buf_;
    std::string tempFile_;
    std::string persistFile_;

    mutable std::recursive_mutex mutex_;

    ExternalFlash() :
            buf_(0xff /* fill */) {
        if (!deviceConfig.flash_file.empty()) {
            persistFile_ = deviceConfig.flash_file;
            if (fs::exists(persistFile_)) {
                loadBuffer(buf_, persistFile_);
            }
            fs::path p(persistFile_);
            tempFile_ = p.parent_path().append('~' + p.filename().string());
        }
    }

    void bufferChanged() {
        if (!persistFile_.empty()) {
            saveBuffer(buf_, tempFile_);
            fs::rename(tempFile_, persistFile_);
        }
    }

    static void loadBuffer(SparseBuffer& buf, const std::string& file) {
        std::ifstream f;
        f.exceptions(std::ios::badbit | std::ios::failbit);
        f.open(file, std::ios::binary);
        size_t segCount = readUint32(f);
        for (size_t i = 0; i < segCount; ++i) {
            size_t offs = readUint32(f);
            size_t size = readUint32(f);
            std::string s;
            s.resize(size);
            f.read(s.data(), size);
            buf.write(offs, s);
        }
    }

    static void saveBuffer(const SparseBuffer& buf, const std::string& file) {
        std::ofstream f;
        f.exceptions(std::ios::badbit | std::ios::failbit);
        f.open(file, std::ios::binary | std::ios::trunc);
        auto& seg = buf.segments();
        writeUint32(f, seg.size());
        for (auto it = seg.begin(); it != seg.end(); ++it) {
            writeUint32(f, it->first);
            writeUint32(f, it->second.size());
            f.write(it->second.data(), it->second.size());
        }
        f.close();
    }

    static uint32_t readUint32(std::ifstream& f) {
        uint32_t v = 0;
        f.read((char*)&v, sizeof(v));
        return littleEndianToNative(v);
    }

    static void writeUint32(std::ofstream& f, uint32_t val) {
        val = nativeToLittleEndian(val);
        f.write((const char*)&val, sizeof(val));
    }
};

} // namespace

int hal_exflash_init(void) {
    return 0;
}

int hal_exflash_uninit(void) {
    return 0;
}

int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    try {
        ExternalFlash::instance()->read(addr, data_buf, data_size);
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "hal_exflash_read() failed: %s", e.what());
        return SYSTEM_ERROR_IO;
    }
}

int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    try {
        ExternalFlash::instance()->write(addr, data_buf, data_size);
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "hal_exflash_write() failed: %s", e.what());
        return SYSTEM_ERROR_IO;
    }
}

int hal_exflash_erase_sector(uintptr_t addr, size_t num_sectors) {
    try {
        ExternalFlash::instance()->erase(addr, num_sectors, sFLASH_PAGESIZE);
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "hal_exflash_erase_sector() failed: %s", e.what());
        return SYSTEM_ERROR_IO;
    }
}

int hal_exflash_lock(void) {
    ExternalFlash::instance()->lock();
    return 0;
}

int hal_exflash_unlock(void) {
    ExternalFlash::instance()->unlock();
    return 0;
}
