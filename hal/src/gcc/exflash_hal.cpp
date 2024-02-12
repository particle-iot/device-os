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
#include "filesystem_util.h"

#include "exflash_hal.h"
#include "flash_mal.h"

#include "logging.h"
#include "system_error.h"

using namespace particle;

namespace fs = std::filesystem;

namespace {

class ExternalFlash {
public:
    void read(uintptr_t addr, uint8_t* data, size_t size) {
        if (!size) {
            return;
        }
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        auto s = buf_.read(addr, size);
        std::memcpy(data, s.data(), size);
    }

    void write(uintptr_t addr, const uint8_t* data, size_t size) {
        if (!size) {
            return;
        }
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        // Read the contents of the affected region
        std::string s = buf_.read(addr, size);
        uint8_t* d = (uint8_t*)s.data();
        for (size_t i = 0; i < size; ++i) {
            d[i] &= data[i]; // Mimic the flash memory behavior
        }
        // Write the changes
        buf_.write(addr, s);
        saveBuffer();
    }

    void erase(uintptr_t addr, size_t blockCount, size_t blockSize) {
        if (!blockCount || !blockSize) {
            return;
        }
        addr = addr / blockSize * blockSize;
        size_t size = blockSize * blockCount;
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        buf_.erase(addr, size);
        saveBuffer();
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
            tempFile_ = temp_file_name("flash_", ".bin");
        }
    }

    void saveBuffer() {
        if (!persistFile_.empty()) {
            saveBuffer(buf_, tempFile_);
            fs::rename(tempFile_, persistFile_);
        }
    }

    static void loadBuffer(SparseBuffer& buf, const std::string& file) {
        std::ifstream f;
        f.exceptions(std::ios::badbit | std::ios::failbit);
        f.open(file, std::ios::binary);
        size_t maxOffs = 0;
        while (!f.eof()) {
            size_t offs = readVarint(f) + maxOffs;
            size_t size = readVarint(f);
            std::string s;
            s.resize(size);
            f.read(s.data(), size);
            buf.write(offs, s);
            maxOffs = offs + size;
        }
    }

    static void saveBuffer(const SparseBuffer& buf, const std::string& file) {
        std::ofstream f;
        f.exceptions(std::ios::badbit | std::ios::failbit);
        f.open(file, std::ios::binary | std::ios::trunc);
        size_t maxOffs = 0;
        auto& seg = buf.segments();
        for (auto it = seg.begin(); it != seg.end(); ++it) {
            writeVarint(f, it->first - maxOffs); // Use delta encoding
            writeVarint(f, it->second.size());
            f.write(it->second.data(), it->second.size());
            maxOffs = it->first + it->second.size();
        }
        f.close();
    }

    static uint32_t readVarint(std::ifstream& f) {
        char buf[maxUnsignedVarintSize<uint32_t>()];
        size_t n = 0;
        uint8_t b = 0;
        do {
            f.read((char*)&b, 1);
            if (n >= sizeof(buf)) {
                throw std::runtime_error("Decoding error");
            }
            buf[n++] = b;
        } while (b & 0x80);
        uint32_t v = 0;
        int r = decodeUnsignedVarint(buf, n, &v);
        if (r != (int)n) {
            throw std::runtime_error("Decoding error");
        }
        return v;
    }

    static void writeVarint(std::ofstream& f, uint32_t val) {
        char buf[maxUnsignedVarintSize<uint32_t>()];
        int r = encodeUnsignedVarint(buf, sizeof(buf), val);
        if (r <= 0 || r > (int)sizeof(buf)) {
            throw std::runtime_error("Encoding error");
        }
        f.write(buf, r);
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
