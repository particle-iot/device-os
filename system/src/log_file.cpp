#include "log_file.h"

#if HAL_PLATFORM_LOG_FILE

#include "system_task.h"
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
#include "scope_guard.h"
#include "check.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <cstdio>

// Set to a non-zero value to enable logging from ISRs
#define LOG_FILE_FROM_ISR 0

namespace particle::system {

namespace {

const auto LOG_FILE1 = "/sys/log.1";
const auto LOG_FILE2 = "/sys/log.2";

const size_t DEFAULT_LOG_FILE_SIZE = 50000;

const size_t LOG_FILE_BUFFER_SIZE = 2 * 1024;

// Paths used by earlier versions of this feature. Removed on startup so that they don't
// linger in the filesystem after an update
const auto OLD_LOG_FILE1 = "/sys/bootlog.1";
const auto OLD_LOG_FILE2 = "/sys/bootlog.2";

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
        auto fileName = rotated ? LOG_FILE2 : LOG_FILE1;
        int r = file.open(fileName, LFS_O_RDONLY);
        if (r < 0 && (r != SYSTEM_ERROR_FILESYSTEM_NOENT || rotated)) {
            return r;
        }
        if (r != SYSTEM_ERROR_FILESYSTEM_NOENT) {
            size_t lastFileSize = CHECK(file.size());
            if (lastFileSize < maxSize && rotated) {
                // Open the previous file
                CHECK(file.open(LOG_FILE1, LFS_O_RDONLY));

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
            CHECK(file_.open(LOG_FILE2, LFS_O_RDONLY));

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
        int r = fs::stat(LOG_FILE2, &info);
        if (r >= 0 && info.type == LFS_TYPE_REG) {
            rotated = true;
        } else if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r >= 0 && info.type != LFS_TYPE_REG)) {
            remove = true;
        }

        if (!remove) {
            int r = fs::stat(LOG_FILE1, &info);
            if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r == SYSTEM_ERROR_FILESYSTEM_NOENT && rotated) ||
                    (r >= 0 && info.type != LFS_TYPE_REG)) {
                remove = true;
            }
        }

        if (remove) {
            CHECK(rmrf(LOG_FILE2));
            CHECK(rmrf(LOG_FILE1));
            rotated = false;
        }

        rotated_ = rotated;
        return 0;
    }

    int write(const char* data, size_t size) {
        size_t totalBytesWritten = 0;

        if (!file_.isOpen()) {
            auto fileName = rotated_ ? LOG_FILE2 : LOG_FILE1;
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
                CHECK(fs::rename(LOG_FILE2, LOG_FILE1));
            }
            CHECK(file_.open(LOG_FILE2, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND));

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

    // Closes the file and deletes its contents
    int clear() {
        CHECK(file_.close());
        CHECK(rmrf(LOG_FILE2));
        CHECK(rmrf(LOG_FILE1));
        rotated_ = false;
        return 0;
    }

    size_t maxSize() const {
        return maxSize_;
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

// Wrapper for the category filter. Copied as a whole so that it can be updated while the logging
// functions are running
struct Category {
    char name[20];
};

struct LogFileConfig {
    Category category;
    size_t maxSize;
    unsigned magic;
    int level;
    bool enabled;
};

class LogFile {
public:
    LogFile() :
            bufMem_(),
            bytesDropped_(0),
            category_(),
            minLevel_(0),
            printing_(false) {
    }

    // Can be called repeatedly to reconfigure the log
    int init(const LogFileConfig& conf) {
        fs::FsLock lock;

        CHECK(fs::mount());

        if (!bufMem_) {
            std::unique_ptr<char[]> bufMem(new(std::nothrow) char[LOG_FILE_BUFFER_SIZE]);
            if (!bufMem) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            buf_.init(bufMem.get(), LOG_FILE_BUFFER_SIZE);
            bufMem_ = std::move(bufMem);
        }
        if (!log_ || log_->maxSize() != conf.maxSize) {
            std::unique_ptr<RotatingLog> log(new(std::nothrow) RotatingLog(conf.maxSize));
            if (!log) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            CHECK(log->init());
            // The buffered data is preserved and will be written to the new log
            log_ = std::move(log);
        }
        ATOMIC_BLOCK() {
            category_ = conf.category;
            minLevel_ = conf.level;
        }
        return 0;
    }

    // Discards the buffered data, deletes the contents of the log and releases the resources
    void destroy() {
        fs::FsLock lock;

        ATOMIC_BLOCK() {
            buf_.init(nullptr, 0);
            bytesDropped_ = 0;
        }
        bufMem_.reset();
        if (log_) {
            log_->clear();
        }
        log_.reset();
    }

    // Discards the buffered data and deletes the contents of the log. Writing to the log continues
    int clear() {
        fs::FsLock lock;

        if (!log_) {
            return 0;
        }
        ATOMIC_BLOCK() {
            buf_.reset();
            bytesDropped_ = 0;
        }
        CHECK(log_->clear());
        return 0;
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
        int r = flush();
        CHECK(log_->close());
        CHECK(r);
        return 0;
    }

    int printLog(int level, const char* category) {
        fs::FsLock lock;

        if (!log_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        CHECK(flushBuffer());

        // Printing the log generates log messages of its own. Suppress them for the duration of the
        // operation so that they don't end up in the log
        printing_ = true;
        SCOPE_GUARD({
            printing_ = false;
        });

        std::unique_ptr<InputStream> in;
        CHECK(log_->openForRead(in));

        char buf[256];
        for (;;) {
            size_t bytesAvail = CHECK(in->availForRead());
            if (!bytesAvail) {
                break;
            }
            size_t n = std::min(bytesAvail, sizeof(buf));
            n = CHECK(in->read(buf, n));
            log_write(level, category, buf, n, nullptr /* reserved */);
        }

        in.reset();
        return 0;
    }

    bool isEnabledForLevel(int level, const char* category) const {
        if (printing_) {
            return false;
        }
        Category cat;
        int minLevel = 0;
        ATOMIC_BLOCK() {
            cat = category_;
            minLevel = minLevel_;
        }
        if (level < minLevel) {
            return false;
        }
        if (category && !startsWith(category, cat.name)) {
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
    // Copy of the current configuration. The logging functions run without the filesystem lock, so
    // they can't access the global configuration object directly
    Category category_;
    int minLevel_;
    // Set while the contents of the log are being printed. Written under the filesystem lock and
    // read without it: a stale read can only cause a message to be dropped
    bool printing_;

    void append(const Chunk* chunks, size_t count) {
        size_t size = 0;
        for (size_t i = 0; i < count; ++i) {
            size += chunks[i].size;
        }
        bool flush = canFlush();
        for (;;) {
            ATOMIC_BLOCK() {
                // The message is stored either in its entirety or not at all
                if ((ssize_t)size <= buf_.space()) {
                    for (size_t i = 0; i < count; ++i) {
                        buf_.put(chunks[i].data, chunks[i].size);
                    }
                    return;
                } else if (!flush) {
                    bytesDropped_ += size;
                    return;
                }
            }
            flushBuffer();
            flush = false;
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

    static bool canFlush() {
        if (hal_interrupt_is_isr()) {
            return false;
        }
        if (SYSTEM_THREAD_CURRENT()) {
            return true;
        }
        return !SystemThread.isStarted() && main_thread_current(nullptr /* reserved */);
    }
};

const auto LOG_FILE_CONFIG_MAGIC = 0xe62a3e5eu;

retained_system LogFileConfig g_logFileConfig = {};
std::atomic<bool> g_logFileEnabled;

// Note: The instance is assigned in enableLogFile() and never reset. Resetting it would create a
// race between the logging functions and the log file's deinitialization
std::unique_ptr<LogFile> g_logFile;

// Note: The caller must acquire the filesystem lock before calling this function
int enableLogFile(const LogFileConfig& conf) {
    if (!g_logFile) {
        std::unique_ptr<LogFile> log(new(std::nothrow) LogFile());
        if (!log) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        g_logFile = std::move(log);
    }
    CHECK(g_logFile->init(conf));
    g_logFileEnabled.store(true, std::memory_order_release);
    return 0;
}

} // namespace

// Called by the system thread
int initLogFile() {
    fs::FsLock lock;

    if (g_logFileConfig.magic != LOG_FILE_CONFIG_MAGIC || !g_logFileConfig.enabled) {
        return 0;
    }
    CHECK(fs::mount());
    rmrf(OLD_LOG_FILE2);
    rmrf(OLD_LOG_FILE1);

    CHECK(enableLogFile(g_logFileConfig));
    return 0;
}

// Called by the system thread
int flushLogFile() {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return 0;
    }
    return g_logFile->flush();
}

// Can be called from any thread, e.g. through system_reset()
void closeLogFile() {
    // Stop writing to the log but don't flush it unless this is the system or app thread that has
    // a large enough stack for file IO
    g_logFileEnabled.exchange(false, std::memory_order_acq_rel);

    if (g_logFile && (SYSTEM_THREAD_CURRENT() || APPLICATION_THREAD_CURRENT())) {
        g_logFile->close();
    }
}

void logFileMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !LOG_FILE_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    g_logFile->logMessage(msg, level, category, attrs);
}

void writeLogFile(const char* data, size_t size, int level, const char* category) {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !LOG_FILE_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    g_logFile->write(data, size, level, category);
}

bool isLogFileEnabledForLevel(int level, const char* category) {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return false;
    }
#if !LOG_FILE_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return false;
    }
#endif
    return g_logFile->isEnabledForLevel(level, category);
}

bool isLogFileEnabled() {
    return g_logFileEnabled.load(std::memory_order_relaxed);
}

} // namespace particle::system

using namespace particle;
using namespace particle::system;

void system_enable_log_file(const system_log_file_config* config) {
    fs::FsLock lock;

    LogFileConfig newConfig = {};
    newConfig.magic = LOG_FILE_CONFIG_MAGIC;
    newConfig.enabled = true;

    if (config) {
        if (config->category) {
            strlcpy(newConfig.category.name, config->category, sizeof(newConfig.category.name));
        }
        newConfig.maxSize = config->max_size ? config->max_size : DEFAULT_LOG_FILE_SIZE;
        newConfig.level = config->level;
    } else {
        newConfig.maxSize = DEFAULT_LOG_FILE_SIZE;
        newConfig.level = LOG_LEVEL_ALL;
    }

    // Apply the configuration to the current session as well
    if (enableLogFile(newConfig) < 0) {
        return;
    }
    g_logFileConfig = newConfig;
}

void system_disable_log_file(void* reserved) {
    fs::FsLock lock;

    // Stop writing to the log without flushing it
    g_logFileEnabled.store(false, std::memory_order_release);

    if (g_logFile) {
        g_logFile->destroy();
    }

    LogFileConfig newConfig = {};
    newConfig.magic = LOG_FILE_CONFIG_MAGIC;
    newConfig.enabled = false;

    g_logFileConfig = newConfig;
}

void system_flush_log_file(void* reserved) {
    flushLogFile();
}

void system_print_log_file(const system_log_file_print_options* opts) {
    fs::FsLock lock;

    if (!g_logFile) {
        return;
    }
    int level = LOG_LEVEL_INFO;
    auto category = "app";
    if (opts) {
        if (opts->level > 0) {
            level = opts->level;
        }
        if (opts->category) {
            category = opts->category;
        }
    }
    if (level < LOG_LEVEL_NONE) {
        g_logFile->printLog(level, category);
    }
}

void system_clear_log_file(void* reserved) {
    fs::FsLock lock;

    if (g_logFile) {
        g_logFile->clear();
    }
}

#endif // HAL_PLATFORM_LOG_FILE
