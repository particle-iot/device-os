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
        auto strm = openFile();
        strm.seekg(0, std::ios::end);
        size_t fileSize = strm.tellg();
        std::unique_ptr<char[]> buf(new char[size]);
        std::memset(buf.get(), 0xff, size);
        if (addr < fileSize) {
            strm.seekg(addr);
            size_t n = std::min(size, fileSize - addr);
            strm.read(buf.get(), n);
        }
        std::memcpy(data, buf.get(), size);
    }

    void write(uintptr_t addr, const uint8_t* data, size_t size) {
        if (!size) {
            return;
        }
        if (addr + size > EXTERNAL_FLASH_SIZE) {
            throw std::runtime_error("Invalid address");
        }
        std::lock_guard lock(mutex_);
        auto strm = openFile();
        strm.seekg(0, std::ios::end);
        size_t fileSize = strm.tellg();
        if (addr > fileSize) {
            // Fill the file with 0xff up to the destination offset
            strm.seekp(fileSize);
            fillBytes(strm, addr - fileSize);
            fileSize = addr;
        }
        // Read the current contents of the affected region
        std::unique_ptr<uint8_t[]> buf(new uint8_t[size]);
        std::memset(buf.get(), 0xff, size);
        strm.seekg(addr);
        size_t n = std::min(size, fileSize - addr);
        strm.read((char*)buf.get(), n);
        // Mimic the flash memory behavior
        for (size_t i = 0; i < size; ++i) {
            buf[i] &= data[i];
        }
        // Write the resulting data
        strm.seekp(addr);
        strm.write((const char*)buf.get(), size);
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
        auto strm = openFile();
        strm.seekg(0, std::ios::end);
        size_t fileSize = strm.tellg();
        // Do not write past the current end of file
        if (addr < fileSize) {
            strm.seekp(addr);
            fillBytes(strm, fileSize - addr);
        }
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
    std::string fileName_;
    bool usingTempFile_;

    mutable std::recursive_mutex mutex_;

    ExternalFlash() :
            usingTempFile_(false) {
        if (!deviceConfig.ext_flash_file.empty()) {
            fileName_ = fs::absolute(deviceConfig.ext_flash_file);
        } else {
            fileName_ = temp_file_name("ext_flash_", ".bin");
            usingTempFile_ = true;
            // TODO: Delete the file on abnormal program termination
        }
    }

    ~ExternalFlash() {
        if (usingTempFile_) {
            deleteFile();
        }
    }

    std::fstream openFile() {
        std::fstream strm;
        strm.exceptions(std::ios::badbit | std::ios::failbit);
        auto flags = std::ios::in | std::ios::out | std::ios::binary;
        if (!fs::exists(fileName_)) {
            flags |= std::ios::trunc;
        }
        strm.open(fileName_, flags);
        return strm;
    }

    void deleteFile() {
        std::error_code ec;
        fs::remove(fileName_, ec);
        if (ec) {
            LOG(ERROR, "Failed to remove file: %s", fileName_.c_str());
        }
    }

    static void fillBytes(std::fstream& strm, size_t count) {
        char buf[4096];
        std::memset(buf, 0xff, sizeof(buf));
        size_t offs = 0;
        while (offs < count) {
            size_t n = std::min(count - offs, sizeof(buf));
            strm.write(buf, n);
            offs += n;
        }
    }
};

} // namespace

int hal_exflash_init(void) {
    return 0;
}

int hal_exflash_uninit(void) {
    return 0;
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

int hal_exflash_erase_block(uintptr_t addr, size_t num_blocks) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
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

int hal_exflash_copy_sector(uintptr_t src_addr, size_t dest_addr, size_t data_size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_lock(void) {
    ExternalFlash::instance()->lock();
    return 0;
}

int hal_exflash_unlock(void) {
    ExternalFlash::instance()->unlock();
    return 0;
}

int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_sleep(bool sleep, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
