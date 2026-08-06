#include "boot_log.h"
#include "interrupts_hal.h"
#include "filesystem.h"
#include "file_util.h"
#include "stream.h"
#include "platform_headers.h" // For retained_system
#include "str_util.h"
#include "str_compat.h"
#include "logging.h"
#include "scope_guard.h"
#include "check.h"

#include <algorithm>
#include <memory>
#include <cstring>

namespace particle::system {

namespace {

const auto BOOT_LOG_FILE1 = "/sys/bootlog.1";
const auto BOOT_LOG_FILE2 = "/sys/bootlog.2";

const size_t DEFAULT_BOOT_LOG_SIZE = 100000;

class BootLogReader: public InputStream {
public:
    BootLogReader() :
            bytesAvail_(0),
            readingLast_(false) {
    }

    int init(size_t maxSize, bool rotated) {
        fs::File file;
        size_t bytesAvail = 0;
        bool readingLast = true;

        // Open the latest file
        auto fileName = rotated ? BOOT_LOG_FILE2 : BOOT_LOG_FILE1;
        int r = file.open(fileName, LFS_O_RDONLY);
        if (r < 0 && (r != SYSTEM_ERROR_FILESYSTEM_NOENT || rotated)) {
            return r;
        }
        if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            size_t lastFileSize = CHECK(file.size());
            if (lastFileSize < maxSize && rotated) {
                // Open the previous file
                CHECK(file.open(BOOT_LOG_FILE1, LFS_O_RDONLY));

                size_t prevFileSize = CHECK(file.size());
                size_t prevFileAvail = std::min(maxSize - lastFileSize, prevFileSize);
                CHECK(file.seek(prevFileSize - prevFileAvail));
                bytesAvail = prevFileAvail + lastFileSize;

                readingLast = false;
            } else if (lastFileSize > maxSize) {
                CHECK(file.seek(lastFileSize - maxSize));
                bytesAvail = maxSize;
            } else {
                bytesAvail = lastFileSize;
            }
        } // else: Log is empty

        file_ = std::move(file);
        bytesAvail_ = bytesAvail;
        readingLast_ = readingLast;
        return 0;
    }

    int read(char* data, size_t size) override {
        if (!bytesAvail_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size_t totalBytesRead = 0;

        size_t bytesToRead = std::min(size, bytesAvail_);
        size_t bytesRead = CHECK(file_.read(data, bytesToRead));
        totalBytesRead += bytesRead;

        if (totalBytesRead < bytesToRead) {
            if (readingLast_) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            // Open the latest file
            CHECK(file_.open(BOOT_LOG_FILE2, LFS_O_RDONLY));

            bytesToRead -= totalBytesRead;
            bytesRead = CHECK(file_.read(data + totalBytesRead, bytesToRead));
            if (bytesRead != bytesToRead) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            totalBytesRead += bytesRead;
            readingLast_ = true;
        }

        bytesAvail_ -= totalBytesRead;
        if (!bytesAvail_) {
            file_.close();
        }
        return totalBytesRead;
    }

    int availForRead() override {
        return bytesAvail_;
    }

    int peek(char* data, size_t size) override {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    int skip(size_t size) override {
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
    size_t bytesAvail_;
    bool readingLast_;
};

// Note: The caller must acquire the filesystem lock before calling the methods of this class
class BootLog {
public:
    explicit BootLog(size_t maxSize) :
            maxSize_(maxSize),
            rotated_(false) {
    }

    int init() {
        lfs_info info = {};
        int r = fs::stat(BOOT_LOG_FILE2, &info);
        if (r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            return r;
        }
        if (r >= 0) {
            rotated_ = true;
        }
        return 0;
    }

    int write(const char* data, size_t size) {
        size_t totalBytesWritten = 0;

        if (!file_.isOpen()) {
            auto fileName = rotated_ ? BOOT_LOG_FILE2 : BOOT_LOG_FILE1;
            CHECK(file_.open(fileName, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND));
        }

        size_t fileSize = CHECK(file_.size());
        size_t bytesToWrite = (fileSize < maxSize_) ? std::min(size, maxSize_ - fileSize) : 0;
        size_t bytesWritten = CHECK(file_.write(data, bytesToWrite));
        if (bytesWritten != bytesToWrite) {
            return SYSTEM_ERROR_FILESYSTEM_IO;
        }
        totalBytesWritten += bytesWritten;

        if (totalBytesWritten < size) {
            // Rotate the log
            CHECK(file_.close());
            if (rotated_) {
                CHECK(fs::rename(BOOT_LOG_FILE2, BOOT_LOG_FILE1));
            }
            CHECK(file_.open(BOOT_LOG_FILE2, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND));

            bytesToWrite = size - totalBytesWritten;
            bytesWritten = CHECK(file_.write(data + totalBytesWritten, bytesToWrite));
            if (bytesWritten != bytesToWrite) {
                return SYSTEM_ERROR_FILESYSTEM_IO;
            }
            totalBytesWritten += bytesWritten;

            rotated_ = true;
        }
        return totalBytesWritten;
    }

    int write(const char* str) {
        size_t n = CHECK(write(str, std::strlen(str)));
        return n;
    }

    int printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        char buf[32];
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n < 0) {
            return SYSTEM_ERROR_IO;
        }
        if ((size_t)n < sizeof(buf)) {
            write(buf, n);
        } else {
            char buf[n + 1]; // Use a larger buffer
            va_start(args, fmt);
            n = vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            if (n < 0) {
                return SYSTEM_ERROR_IO;
            }
            write(buf, n);
        }
        return n;
    }

    int openForRead(std::unique_ptr<InputStream>& stream) {
        CHECK(file_.close()); // Flush the data

        std::unique_ptr<BootLogReader> reader(new(std::nothrow) BootLogReader());
        if (!reader) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(reader->init(maxSize_, rotated_));

        stream = std::move(reader);
        return 0;
    }

    int close() {
        CHECK(file_.close());
        return 0;
    }

private:
    fs::File file_;
    size_t maxSize_;
    bool rotated_;
};

struct BootLogConfig {
    char category[20];
    size_t maxSize;
    unsigned magic;
    int level;
    bool enabled;
};

const auto BOOT_LOG_CONFIG_MAGIC = 0xe62a3e5eu;

retained_system BootLogConfig g_bootLogConfig = {};
std::unique_ptr<BootLog> g_bootLog;
std::atomic<bool> g_bootLogEnabled;
bool g_bootLogWriting = false;

int printBootLog(int level, const char* category) {
    if (!g_bootLog) { // Sanity check
        return 0;
    }
    std::unique_ptr<InputStream> in;
    CHECK(g_bootLog->openForRead(in));

    char buf[256];
    for (;;) {
        size_t bytesAvail = CHECK(in->availForRead());
        if (!bytesAvail) {
            break;
        }
        size_t n = std::min(bytesAvail, sizeof(buf));
        CHECK(in->read(buf, n));
        log_write(level, category, buf, n, nullptr /* reserved */);
    }

    in.reset();
    return 0;
}

bool isBootLogEnabledForLevel(int level, const char* category) {
    if (level < g_bootLogConfig.level) {
        return false;
    }
    if (category && !startsWith(category, g_bootLogConfig.category)) {
        return false;
    }
    return true;
}

} // namespace

int initBootLog() {
    // Use the filesystem lock for serializing access to the boot log
    fs::FsLock lock;

    if (g_bootLogConfig.magic != BOOT_LOG_CONFIG_MAGIC) {
        g_bootLogConfig = BootLogConfig();
        g_bootLogConfig.magic = BOOT_LOG_CONFIG_MAGIC;
    }
    if (!g_bootLogConfig.enabled) {
        return 0;
    }

    CHECK(fs::mount());

    std::unique_ptr<BootLog> log(new(std::nothrow) BootLog(g_bootLogConfig.maxSize));
    if (!log) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(log->init());
    g_bootLog = std::move(log);

    g_bootLogEnabled.store(true, std::memory_order_release);
    return 0;
}

void stopWritingBootLog() {
    bool wasEnabled = g_bootLogEnabled.exchange(false, std::memory_order_acq_rel);
    if (!wasEnabled || hal_interrupt_is_isr()) {
        return;
    }
    fs::FsLock lock;

    if (g_bootLog) {
        // Flush the data
        g_bootLog->close();
    }
}

void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
    if (hal_interrupt_is_isr() || !isBootLogEnabled()) {
        return;
    }
    fs::FsLock lock;

    if (g_bootLogWriting || !isBootLogEnabledForLevel(level, category)) {
        return;
    }
    g_bootLogWriting = true;
    SCOPE_GUARD({
        g_bootLogWriting = false;
    });

    auto log = g_bootLog.get();
    if (!log) { // Sanity check
        return;
    }

    if (attrs && attrs->has_time) {
        log->printf("%010u ", (unsigned)attrs->time);
    }
    if (category) {
        log->write("[");
        log->write(category);
        log->write("] ");
    }
    log->write(log_level_name(level, nullptr /* reserved */));
    log->write(": ");
    if (msg) {
        log->write(msg);
    }
    log->write("\r\n");
}

void writeBootLog(const char* data, size_t size, int level, const char* category) {
    if (hal_interrupt_is_isr() || !isBootLogEnabled()) {
        return;
    }
    fs::FsLock lock;

    if (g_bootLogWriting || !isBootLogEnabledForLevel(level, category)) {
        return;
    }
    g_bootLogWriting = true;
    SCOPE_GUARD({
        g_bootLogWriting = false;
    });

    if (g_bootLog) {
        g_bootLog->write(data, size);
    }
}

bool isBootLogEnabled(int level, const char* category) {
    if (hal_interrupt_is_isr() || !isBootLogEnabled()) {
        return false;
    }
    fs::FsLock lock;

    if (!isBootLogEnabledForLevel(level, category)) {
        return false;
    }
    return true;
}

bool isBootLogEnabled() {
    return g_bootLogEnabled.load(std::memory_order_acquire);
}

} // namespace particle::system

using namespace particle;
using namespace particle::system;

void system_enable_boot_log(bool enabled, const system_boot_log_config* config, void* reserved) {
    fs::FsLock lock;

    if (!enabled) {
        stopWritingBootLog();
    }

    BootLogConfig newConfig = {};
    newConfig.enabled = enabled;

    if (enabled) {
        if (config) {
            if (config->category) {
                strlcpy(newConfig.category, config->category, sizeof(newConfig.category));
            }
            newConfig.maxSize = config->max_size ? config->max_size : DEFAULT_BOOT_LOG_SIZE;
            newConfig.level = config->level;
        } else {
            newConfig.maxSize = DEFAULT_BOOT_LOG_SIZE;
            newConfig.level = LOG_LEVEL_ALL;
        }
    }
    g_bootLogConfig = newConfig;
}

void system_flush_boot_log(int level, const char* category, void* reserved) {
    fs::FsLock lock;

    stopWritingBootLog();

    if (level < LOG_LEVEL_NONE) {
        if (!category) {
            category = "app";
        }
        printBootLog(level, category);
    }

    g_bootLog.reset();
    rmrf(BOOT_LOG_FILE1);
    rmrf(BOOT_LOG_FILE2);
}
