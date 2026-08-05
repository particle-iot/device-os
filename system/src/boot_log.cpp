#include "boot_log.h"
#include "filesystem.h"
#include "file_util.h"
#include "stream.h"
#include "platform_headers.h" // For retained_system
#include "str_compat.h"
#include "logging.h"
#include "check.h"

#include <algorithm>
#include <mutex>
#include <memory>
#include <cstring>
#include <cstdarg>

namespace particle::system {

namespace {

class RotatingLogReader: public InputStream {
public:
    RotatingLogReader(size_t maxSize, const char* fileName1, const char* fileName2, bool hasSecond, bool writeSecond) :
            fileName1_(fileName1),
            fileName2_(fileName2),
            maxSize_(maxSize),
            bytesAvail_(0),
            hasSecond_(hasSecond),
            readSecond_(false),
            writeSecond_(writeSecond) {
    }

    ~RotatingLogReader() {
        fs::FsLock lock;

        file_.close();
    }

    int init() {
        fs::FsLock lock;

        fs::File file;
        size_t bytesAvail = 0;
        bool readSecond = writeSecond_;

        // Open the latest file
        auto fileName = writeSecond_ ? fileName2_ : fileName1_;
        int r = file.open(fileName, LFS_O_RDONLY);
        if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            return r;
        }
        if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            size_t lastFileSize = CHECK(file.size());
            if (lastFileSize < maxSize_ && hasSecond_) {
                // Open the previous file
                auto fileName = writeSecond_ ? fileName1_ : fileName2_;
                CHECK(file.open(fileName, LFS_O_RDONLY)); // Must exist
                size_t prevFileSize = CHECK(file.size());

                size_t prevFileAvail = std::min(maxSize_ - lastFileSize, prevFileSize);
                CHECK(file.seek(prevFileAvail, LFS_SEEK_END));

                bytesAvail = prevFileAvail + lastFileSize;
                readSecond = !writeSecond_;
            } else if (lastFileSize > maxSize_) {
                CHECK(file.seek(lastFileSize - maxSize_));
                bytesAvail = maxSize_;
            } else {
                bytesAvail = lastFileSize;
            }
        } // else: Log is empty

        file_ = std::move(file);
        bytesAvail_ = bytesAvail;
        readSecond_ = readSecond;
        return 0;
    }

    int read(char* data, size_t size) override {
        fs::FsLock lock;

        if (!bytesAvail_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size_t bytesToRead = std::min(size, bytesAvail_);
        size_t totalBytesRead = CHECK(file_.read(data, bytesToRead));
        if (totalBytesRead < bytesToRead) {
            if (!hasSecond_) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            // Open the latest file
            auto fileName = readSecond_ ? fileName1_ : fileName2_;
            CHECK(file_.open(fileName, LFS_O_RDONLY));

            bytesToRead -= totalBytesRead;
            size_t bytesRead = CHECK(file_.read(data + totalBytesRead, bytesToRead));
            if (bytesRead != bytesToRead) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            totalBytesRead += bytesRead;
            hasSecond_ = false;
        }
        
        bytesAvail_ -= totalBytesRead;
        if (!bytesAvail_) {
            file_.close();
        }
        return totalBytesRead;
    }

    int skip(size_t size) override {
        fs::FsLock lock;

        size_t n = CHECK(read(nullptr, size));
        return n;
    }

    int availForRead() override {
        fs::FsLock lock;

        return bytesAvail_;
    }

    int peek(char* data, size_t size) override {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    int seek(size_t offset) override {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

private:
    fs::File file_;
    const char* fileName1_;
    const char* fileName2_;
    size_t maxSize_;
    size_t bytesAvail_;
    bool hasSecond_;
    bool readSecond_;
    bool writeSecond_;
};

class RotatingLog {
public:
    RotatingLog(size_t maxSize, const char* fileName1, const char* fileName2) :
            fileName1_(fileName1),
            fileName2_(fileName2),
            maxSize_(maxSize),
            hasSecond_(false),
            writeSecond_(false) {
    }

    ~RotatingLog() {
        fs::FsLock lock;

        file_.close();
    }

    int init() {
        fs::FsLock lock;

        bool hasSecond = false;
        bool writeSecond = false;

        lfs_info info = {};
        int r = fs::stat(fileName1_, &info);
        if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            return r;
        }
        if (r == SYSTEM_ERROR_FILESYSTEM_NOENT || info.type != LFS_TYPE_REG) {
            if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
                rmrf(fileName1_); // Log file is not a regular file
            }
            // For consistency, make sure the second file doesn't exist too
            rmrf(fileName2_);
        }

        r = fs::stat(fileName2_, &info);
        if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            return r;
        }
        if (r != SYSTEM_ERROR_FILESYSTEM_NOENT && info.type == LFS_TYPE_REG) {
            hasSecond = true;
            if (info.size < maxSize_) {
                writeSecond = true;
            }
        } else if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            rmrf(fileName2_); // Log file is not a regular file
        }

        hasSecond_ = hasSecond;
        writeSecond_ = writeSecond;
        return 0;
    }

    int write(const char* data, size_t size) {
        fs::FsLock lock;

        if (!file_.isOpen()) {
            auto fileName = writeSecond_ ? fileName2_ : fileName1_;
            CHECK(file_.open(fileName, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND));
        }

        size_t fileSize = CHECK(file_.size());
        size_t bytesToWrite = (fileSize < maxSize_) ? std::min(size, maxSize_ - fileSize) : 0;
        size_t bytesWritten = CHECK(file_.write(data, bytesToWrite));
        if (bytesWritten != bytesToWrite) {
            return SYSTEM_ERROR_FILESYSTEM_IO;
        }
        if (bytesWritten == size) {
            return size;
        }

        // Rotate the log
        CHECK(file_.close());
        writeSecond_ = !writeSecond_;
        auto fileName = writeSecond_ ? fileName2_ : fileName1_;
        CHECK(file_.open(fileName, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND)); // Note LFS_O_TRUNC
        if (writeSecond_) {
            hasSecond_ = true;
        }

        bytesToWrite = size - bytesWritten;
        bytesWritten = CHECK(file_.write(data + bytesWritten, bytesToWrite));
        if (bytesWritten != bytesToWrite) {
            return SYSTEM_ERROR_FILESYSTEM_IO;
        }
        return size;
    }

    int printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        char buf[32];
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, args);
        if (n < 0) {
            return SYSTEM_ERROR_IO;
        }
        va_end(args);
        if ((size_t)n >= sizeof(buf)) {
            char buf[n + 1]; // Use a larger buffer
            va_start(args, fmt);
            n = vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            if (n > 0) {
                write(buf, n);
            }
        } else if (n > 0) {
            write(buf, n);
        }
        return n;
    }

    int clear() {
        fs::FsLock lock;

        file_.close();
        rmrf(fileName1_);
        rmrf(fileName2_);

        hasSecond_ = false;
        writeSecond_ = false;
        return 0;
    }

    int openForRead(std::unique_ptr<InputStream>& stream) {
        fs::FsLock lock;

        std::unique_ptr<RotatingLogReader> reader(new(std::nothrow) RotatingLogReader(maxSize_, fileName1_,
                fileName2_, hasSecond_, writeSecond_));
        if (!reader) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(reader->init());

        stream = std::move(reader);
        return 0;
    }

private:
    fs::File file_;
    const char* fileName1_;
    const char* fileName2_;
    size_t maxSize_;
    bool hasSecond_;
    bool writeSecond_;
};

struct BootLogConfig {
    char category[20];
    size_t maxSize;
    int level;
    bool enabled;
};

const auto BOOT_LOG_FILE1 = "/sys/bootlog.1";
const auto BOOT_LOG_FILE2 = "/sys/bootlog.2";

const size_t DEFAULT_BOOT_LOG_SIZE = 100000;

retained_system BootLogConfig bootLogConfig = {};
std::unique_ptr<RotatingLog> bootLog;

void bootLogMessageCallback(const char* msg, int level, const char* category, const LogAttributes* attr, void* reserved) {
    fs::FsLock lock;

    if (!bootLog) {
        return;
    }
    bootLog->write(msg, std::strlen(msg));
    bootLog->write("\r\n", 2);
}

void bootLogWriteCallback(const char* data, size_t size, int level, const char* category, void* reserved) {
    fs::FsLock lock;

    if (!bootLog) {
        return;
    }
    bootLog->write(data, size);
}

int bootLogEnabledCallback(int level, const char* category, void* reserved) {
    fs::FsLock lock;

    if (!bootLog) {
        return 0;
    }
    return 1;
}

} // namespace

int initBootLog() {
    fs::FsLock lock;

    if (!bootLogConfig.enabled) {
        return 0;
    }

    CHECK(fs::mount());

    std::unique_ptr<RotatingLog> log(new(std::nothrow) RotatingLog(bootLogConfig.maxSize, BOOT_LOG_FILE1, BOOT_LOG_FILE2));
    if (!log) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(log->init());
    bootLog = std::move(log);

    log_set_callbacks(bootLogMessageCallback, bootLogWriteCallback, bootLogEnabledCallback, nullptr /* reserved */);
    return 0;
}

void stopWritingBootLog() {
}

} // namespace particle::system

using namespace particle;
using namespace particle::system;

void system_enable_boot_log(bool enabled, const system_boot_log_config* config, void* reserved) {
    fs::FsLock lock;

    if (!enabled) {
        bootLog.reset();
    }

    bootLogConfig = BootLogConfig();
    bootLogConfig.enabled = enabled;
    if (enabled) {
        if (config) {
            if (config->category) {
                strlcpy(bootLogConfig.category, config->category, sizeof(bootLogConfig.category));
            }
            bootLogConfig.maxSize = config->max_size ? config->max_size : DEFAULT_BOOT_LOG_SIZE;
            bootLogConfig.level = config->level;
        } else {
            bootLogConfig.maxSize = DEFAULT_BOOT_LOG_SIZE;
            bootLogConfig.level = LOG_LEVEL_ALL;
        }
    }
}

void system_flush_boot_log(int level, const char* category, void* reserved) {
    fs::FsLock lock;

    if (!bootLog) {
        return;
    }

    std::unique_ptr<InputStream> in;
    int r = bootLog->openForRead(in);
    if (r < 0) {
        return;
    }

    if (level < LOG_LEVEL_NONE) {
        char buf[256];
        int bytesAvail = 0;
        while ((bytesAvail = in->availForRead()) > 0) {
            size_t n = std::min<size_t>(bytesAvail, sizeof(buf));
            int r = in->read(buf, n);
            if (r < 0) {
                return;
            }
            log_write(level, category, buf, n, nullptr /* reserved */);
        }
    }

    bootLog->clear();
}
