#include "boot_log.h"

#if HAL_PLATFORM_BOOT_LOG

#include "system_threading.h"
#include "system_config.h"
#include "interrupts_hal.h"
#include "filesystem.h"
#include "file_util.h"
#include "stream.h"
#include "ringbuffer.h"
#include "atomic_section.h"
#include "platform_headers.h" // For retained_system
#include "str_util.h"
#include "str_compat.h"
#include "logging.h"
#include "check.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdio>

// Set to a non-zero value to enable logging from ISRs
#define BOOT_LOG_FROM_ISR 0

namespace particle::system {

namespace {

const auto BOOT_LOG_FILE1 = "/sys/bootlog.1";
const auto BOOT_LOG_FILE2 = "/sys/bootlog.2";

const size_t DEFAULT_BOOT_LOG_SIZE = 50000;

const size_t BOOT_LOG_BUFFER_SIZE = 2 * 1024;

// Note: The caller must acquire the filesystem lock before calling the methods of this class
class RotatingLogReader: public InputStream {
public:
    RotatingLogReader() :
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
class RotatingLog {
public:
    explicit RotatingLog(size_t maxSize) :
            maxSize_(maxSize),
            rotated_(false) {
    }

    int init() {
        bool rotated = false;
        bool remove = false;

        lfs_info info = {};
        int r = fs::stat(BOOT_LOG_FILE2, &info);
        if (r >= 0 && info.type == LFS_TYPE_REG) {
            rotated = true;
        } else if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r >= 0 && info.type != LFS_TYPE_REG)) {
            remove = true;
        }

        if (!remove) {
            int r = fs::stat(BOOT_LOG_FILE1, &info);
            if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r == SYSTEM_ERROR_FILESYSTEM_NOENT && rotated) ||
                    (r >= 0 && info.type != LFS_TYPE_REG)) {
                remove = true;
            }
        }

        if (remove) {
            CHECK(rmrf(BOOT_LOG_FILE2));
            CHECK(rmrf(BOOT_LOG_FILE1));
            rotated = false;
        }

        rotated_ = rotated;
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

    int sync() {
        CHECK(file_.sync());
        return 0;
    }

    int close() {
        CHECK(file_.close());
        return 0;
    }

    int openForRead(std::unique_ptr<InputStream>& stream) {
        CHECK(file_.close()); // Flush the data

        std::unique_ptr<RotatingLogReader> reader(new(std::nothrow) RotatingLogReader());
        if (!reader) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(reader->init(maxSize_, rotated_));

        stream = std::move(reader);
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

class BootLog {
public:
    BootLog() :
            bufMem_(),
            bytesDropped_(0),
            category_(nullptr),
            minLevel_(0) {
    }

    int init(const BootLogConfig& conf) {
        fs::FsLock lock;

        CHECK(fs::mount());

        std::unique_ptr<char[]> bufMem(new(std::nothrow) char[BOOT_LOG_BUFFER_SIZE]);
        if (!bufMem) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        std::unique_ptr<RotatingLog> log(new(std::nothrow) RotatingLog(conf.maxSize));
        if (!log) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(log->init());

        buf_.init(bufMem.get(), BOOT_LOG_BUFFER_SIZE);
        bufMem_ = std::move(bufMem);
        log_ = std::move(log);
        category_ = conf.category;
        minLevel_ = conf.level;
        return 0;
    }

    void destroy() {
        fs::FsLock lock;

        ATOMIC_BLOCK() {
            buf_.init(nullptr, 0);
            bytesDropped_ = 0;
        }
        bufMem_.reset();
        log_.reset();
    }

    void logMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
        if (!isEnabledForLevel(level, category)) {
            return;
        }
        char timeBuf[12] = {};
        size_t timeLen = 0;
        if (attrs && attrs->has_time) {
            int n = std::snprintf(timeBuf, sizeof(timeBuf), "%010u ", (unsigned)attrs->time);
            if (n > 0 && (size_t)n < sizeof(timeBuf)) {
                timeLen = n;
            }
        }
        auto levelName = log_level_name(level, nullptr /* reserved */);
        Chunk chunks[] = {
            { timeBuf, timeLen },
            { "[", category ? 1u : 0u },
            { category, category ? std::strlen(category) : 0u },
            { "] ", category ? 2u : 0u },
            { levelName, std::strlen(levelName) },
            { ": ", 2u },
            { msg, msg ? std::strlen(msg) : 0u },
            { "\r\n", 2u }
        };
        append(chunks, sizeof(chunks) / sizeof(chunks[0]));
    }

    void write(const char* data, size_t size, int level, const char* category) {
        if (!isEnabledForLevel(level, category)) {
            return;
        }
        Chunk chunk = { data, size };
        append(&chunk, 1);
    }

    int flush() {
        fs::FsLock lock;

        if (!log_) {
            return 0;
        }
        CHECK(flushBuffer());
        return 0;
    }

    int close() {
        fs::FsLock lock;

        if (!log_) {
            return 0;
        }
        CHECK(flush());
        CHECK(log_->close());
        return 0;
    }

    int printLog(int level, const char* category) {
        fs::FsLock lock;

        if (!log_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        std::unique_ptr<InputStream> in;
        CHECK(log_->openForRead(in));

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

    bool isEnabledForLevel(int level, const char* category) const {
        if (level < minLevel_) {
            return false;
        }
        if (category && !startsWith(category, category_)) {
            return false;
        }
        return true;
    }

private:
    // A piece of log data to be appended to the buffer
    struct Chunk {
        const char* data;
        size_t size;
    };

    std::unique_ptr<RotatingLog> log_;
    std::unique_ptr<char[]> bufMem_;
    services::RingBuffer<char> buf_;
    size_t bytesDropped_;
    // Read-only copy of the current configuration. The logging functions run without a lock, so they
    // can't access the global configuration object directly
    const char* category_;
    int minLevel_;

    void append(const Chunk* chunks, size_t count) {
        size_t size = 0;
        for (size_t i = 0; i < count; ++i) {
            size += chunks[i].size;
        }
        ATOMIC_BLOCK() {
            // The message is stored either in its entirety or not at all
            if (buf_.space() < (ssize_t)size) {
                bytesDropped_ += size;
                return;
            }
            for (size_t i = 0; i < count; ++i) {
                buf_.put(chunks[i].data, chunks[i].size);
            }
        }
    }

    int flushBuffer() {
        size_t bytesAvail = 0;
        size_t bytesDropped = 0;
        ATOMIC_BLOCK() {
            ssize_t n = buf_.data();
            if (n > 0) {
                bytesAvail = n;
            }
            bytesDropped = bytesDropped_;
            bytesDropped_ = 0;
        }
        bool needSync = false;
        while (bytesAvail > 0) {
            size_t size = 0;
            const char* data = nullptr;
            ATOMIC_BLOCK() {
                size = std::min(buf_.consumable(), bytesAvail);
                if (size > 0) {
                    data = buf_.consume(size);
                }
            }
            if (!data) {
                break;
            }
            int r = log_->write(data, size);
            ATOMIC_BLOCK() {
                buf_.consumeCommit(size);
            }
            CHECK(r);
            bytesAvail -= size;
            needSync = true;
        }
        if (bytesDropped > 0) {
            char buf[64];
            int n = std::snprintf(buf, sizeof(buf), "...dropped %u bytes of log data\r\n", (unsigned)bytesDropped);
            if (n > 0) {
                CHECK(log_->write(buf, std::min((size_t)n, sizeof(buf) - 1)));
                needSync = true;
            }
        }
        if (needSync) {
            CHECK(log_->sync());
        }
        return 0;
    }
};

const auto BOOT_LOG_CONFIG_MAGIC = 0xe62a3e5eu;

retained_system BootLogConfig g_bootLogConfig = {};
std::atomic<bool> g_bootLogEnabled;

// Note: The instance is assigned in initBootLog() and never reset. Resetting it would create a race
// between the logging functions and the boot log's deinitialization
std::unique_ptr<BootLog> g_bootLog;

} // namespace

// Called by the system thread
int initBootLog() {
    fs::FsLock lock;

    if (g_bootLogConfig.magic != BOOT_LOG_CONFIG_MAGIC || !g_bootLogConfig.enabled) {
        return 0;
    }
    std::unique_ptr<BootLog> log(new(std::nothrow) BootLog());
    if (!log) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(log->init(g_bootLogConfig));

    g_bootLog = std::move(log);
    g_bootLogEnabled.store(true, std::memory_order_release);
    return 0;
}

// Called by the system thread
int flushBootLog() {
    if (!g_bootLogEnabled.load(std::memory_order_acquire)) {
        return 0;
    }
    return g_bootLog->flush();
}

// Can be called from any thread, e.g. through system_reset()
void closeBootLog() {
    // Disable the boot log but don't flush it unless this is the system or app thread that has
    // a large enough stack for file IO
    g_bootLogEnabled.exchange(false, std::memory_order_acq_rel);

    if (g_bootLog && (SYSTEM_THREAD_CURRENT() || APPLICATION_THREAD_CURRENT())) {
        g_bootLog->close();
    }
}

void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
    if (!g_bootLogEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !BOOT_LOG_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    g_bootLog->logMessage(msg, level, category, attrs);
}

void writeBootLog(const char* data, size_t size, int level, const char* category) {
    if (!g_bootLogEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !BOOT_LOG_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    g_bootLog->write(data, size, level, category);
}

bool isBootLogEnabled(int level, const char* category) {
    if (!g_bootLogEnabled.load(std::memory_order_acquire)) {
        return false;
    }
#if !BOOT_LOG_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return false;
    }
#endif
    return g_bootLog->isEnabledForLevel(level, category);
}

} // namespace particle::system

using namespace particle;
using namespace particle::system;

void system_enable_boot_log(bool enabled, const system_boot_log_config* config, void* reserved) {
    fs::FsLock lock;

    if (!enabled) {
        closeBootLog();
    }

    BootLogConfig newConfig = {};
    newConfig.magic = BOOT_LOG_CONFIG_MAGIC;
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

    closeBootLog();

    if (g_bootLog) {
        if (level < LOG_LEVEL_NONE) {
            g_bootLog->printLog(level, category ? category : "system.boot");
        }
        g_bootLog->destroy();
    }

    rmrf(BOOT_LOG_FILE2);
    rmrf(BOOT_LOG_FILE1);
}

#endif // HAL_PLATFORM_BOOT_LOG
