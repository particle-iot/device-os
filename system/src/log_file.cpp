#include "log_file.h"

#if HAL_PLATFORM_LOG_FILE

#include "system_task.h"
#include "system_threading.h"
#include "system_config.h"
#include "dct_hal.h"
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
#include <atomic>
#include <memory>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include "log_file_config.pb.h"

// Set to a non-zero value to enable logging from ISRs
#define LOG_FROM_ISR 0

#define PB_INTERNAL(_name) particle_firmware_##_name

namespace particle::system {

namespace {

const auto LOG_FILE1 = "/sys/log.1";
const auto LOG_FILE2 = "/sys/log.2";
const auto LOG_CONFIG_FILE = "/sys/log_config";

const size_t DEFAULT_LOG_FILE_SIZE = 50000;
const size_t DEFAULT_LOG_BUFFER_SIZE = 2 * 1024;

int removeLogFiles() {
    int result = 0;
    int r = rmrf(LOG_FILE2);
    if (r < 0) {
        result = r;
    }
    r = rmrf(LOG_FILE1);
    if (r < 0 && result >= 0) {
        result = r;
    }
    return result;
}

inline bool isSystemOrMainThread() {
    return !hal_interrupt_is_isr() && (SYSTEM_THREAD_CURRENT() || (!SystemThread.isStarted() &&
            main_thread_current(nullptr /* reserved */)));
}

inline bool isAppThread() {
    return !hal_interrupt_is_isr() && APPLICATION_THREAD_CURRENT();
}

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

        size_t bytesToRead = std::min(size, bytesAvail_);
        size_t totalBytesRead = 0;
        if (data) {
            totalBytesRead = CHECK(file_.read(data, bytesToRead));
        } else {
            totalBytesRead = std::min<size_t>(bytesToRead, CHECK(file_.size()) - CHECK(file_.tell()));
            CHECK(file_.seek(totalBytesRead, LFS_SEEK_CUR));
        }
        if (totalBytesRead < bytesToRead) {
            if (readingLast_) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            // Open the latest file
            CHECK(file_.open(LOG_FILE2, LFS_O_RDONLY));

            bytesToRead -= totalBytesRead;
            size_t bytesRead = 0;
            if (data) {
                bytesRead = CHECK(file_.read(data + totalBytesRead, bytesToRead));
            } else {
                bytesRead = std::min<size_t>(bytesToRead, CHECK(file_.size()) - CHECK(file_.tell()));
                CHECK(file_.seek(bytesRead, LFS_SEEK_CUR));
            }
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

    int skip(size_t size) override {
        size_t n = CHECK(read(nullptr /* data */, size));
        return n;
    }

    int availForRead() override {
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
        bool clear = false;

        lfs_info info = {};
        int r = fs::stat(LOG_FILE2, &info);
        if (r >= 0 && info.type == LFS_TYPE_REG) {
            rotated = true;
        } else if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r >= 0 && info.type != LFS_TYPE_REG)) {
            clear = true;
        }

        if (!clear) {
            int r = fs::stat(LOG_FILE1, &info);
            if ((r < 0 && r != SYSTEM_ERROR_FILESYSTEM_NOENT) || (r == SYSTEM_ERROR_FILESYSTEM_NOENT && rotated) ||
                    (r >= 0 && info.type != LFS_TYPE_REG)) {
                clear = true;
            }
        }

        if (clear) {
            CHECK(removeLogFiles());
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
        if (file_.isOpen()) {
            CHECK(file_.sync());
        }
        return 0;
    }

    int close() {
        CHECK(file_.close());
        return 0;
    }

    int openForRead(std::unique_ptr<InputStream>& stream) {
        if (file_.isOpen()) {
            CHECK(file_.sync());
        }

        std::unique_ptr<RotatingLogReader> reader(new(std::nothrow) RotatingLogReader());
        if (!reader) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(reader->init(maxSize_, rotated_));

        stream = std::move(reader);
        return 0;
    }

    void reset() {
        file_.close();
        rotated_ = false;
    }

private:
    fs::File file_;
    size_t maxSize_;
    bool rotated_;
};

struct LogFilter {
    char category[32];
    int level;

    LogFilter() :
            category(),
            level(LOG_LEVEL_ALL) {
    }
};

struct LogFileConfig {
    LogFilter filter;
    size_t maxSize;
    size_t bufferSize;
    int flags;

    LogFileConfig() :
            maxSize(DEFAULT_LOG_FILE_SIZE),
            bufferSize(DEFAULT_LOG_BUFFER_SIZE),
            flags(0) {
    }
};

class LogFile {
public:
    LogFile() :
            buf_(),
            printing_(false),
            bytesDropped_(0) {
    }

    // Can be called repeatedly to reconfigure the log
    int init(const LogFileConfig& conf) {
        fs::FsLock lock;

        CHECK(fs::mount());

        std::unique_ptr<char[]> bufMem(new(std::nothrow) char[conf.bufferSize]);
        if (!bufMem) {
            return SYSTEM_ERROR_NO_MEMORY;
        }

        std::unique_ptr<RotatingLog> log(new(std::nothrow) RotatingLog(conf.maxSize));
        if (!log) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(log->init());

        ATOMIC_BLOCK() {
            buf_.init(bufMem.get(), conf.bufferSize);
            filter_ = conf.filter;
            bytesDropped_ = 0;
        }
        bufMem_ = std::move(bufMem);
        log_ = std::move(log);
        return 0;
    }

    void destroy() {
        fs::FsLock lock;

        ATOMIC_BLOCK() {
            buf_ = services::RingBuffer<char>(); // Clear the reference to the underlying buffer
        }
        bufMem_.reset();
        log_.reset();
    }

    int logMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
        if (!isEnabledForLevel(level, category) || isPrinting()) {
            return 0;
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
        CHECK(append(chunks, sizeof(chunks) / sizeof(chunks[0])));
        return 0;
    }

    int write(const char* data, size_t size, int level, const char* category) {
        if (!isEnabledForLevel(level, category) || isPrinting()) {
            return 0;
        }
        Chunk chunk = { data, size };
        CHECK(append(&chunk, 1));
        return size;
    }

    int flush() {
        fs::FsLock lock;

        CHECK(flushBuffer());
        return 0;
    }

    int clear() {
        fs::FsLock lock;

        ATOMIC_BLOCK() {
            buf_.reset();
            bytesDropped_ = 0;
        }
        if (log_) {
            log_->reset();
        }
        CHECK(removeLogFiles());
        return 0;
    }

    int printLog(size_t size, int level, const char* category) {
        fs::FsLock lock;

        // Printing the log generates log messages of its own. Suppress them for the duration of the
        // operation so that they don't end up in the log
        printing_ = true;
        SCOPE_GUARD({
            printing_ = false;
        });

        size_t n = CHECK(readLog(size, [=](const char* data, size_t size) {
            log_write(level, category, data, size, nullptr /* reserved */);
            return 0;
        }));
        return n;
    }

    template<typename F>
    int readLog(size_t size, F&& fn) {
        fs::FsLock lock;

        if (!log_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        CHECK(flushBuffer());

        std::unique_ptr<InputStream> in;
        CHECK(log_->openForRead(in));

        size_t bytesAvail = CHECK(in->availForRead());
        if (size > 0 && bytesAvail > size) {
            CHECK(in->skip(bytesAvail - size));
        }

        char buf[256];
        size_t bytesRead = 0;
        while ((bytesAvail = CHECK(in->availForRead())) > 0) {
            size_t n = std::min(bytesAvail, sizeof(buf));
            n = CHECK(in->read(buf, n));
            // TODO: Release the lock before calling the user callback
            CHECK(fn(buf, n));
            bytesRead += n;
        }
        return bytesRead;
    }

    int getSize() {
        fs::FsLock lock;

        if (!log_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        CHECK(flushBuffer());

        std::unique_ptr<InputStream> in;
        CHECK(log_->openForRead(in));
        size_t size = CHECK(in->availForRead());
        return size;
    }

    bool isEnabledForLevel(int level, const char* category) const {
        LogFilter filter;
        ATOMIC_BLOCK() {
            filter = filter_;
        }
        if (level < filter.level) {
            return false;
        }
        if (filter.category[0] && (!category || !startsWith(category, filter.category))) {
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
    LogFilter filter_;
    std::atomic<bool> printing_;
    size_t bytesDropped_;

    int append(const Chunk* chunks, size_t count) {
        size_t size = 0;
        for (size_t i = 0; i < count; ++i) {
            size += chunks[i].size;
        }
        bool canFlush = isSystemOrMainThread();
        for (;;) {
            ATOMIC_BLOCK() {
                // The message is stored either in its entirety or not at all
                if ((ssize_t)size <= buf_.space()) {
                    for (size_t i = 0; i < count; ++i) {
                        buf_.put(chunks[i].data, chunks[i].size);
                    }
                    return 0;
                } else if (!canFlush) {
                    bytesDropped_ += size;
                    return 0;
                }
            }
            fs::FsLock lock;
            // If the filesystem lock acquired above is not the outermost one, do not attempt to write
            // to the filesystem as we might potentially be within a LittleFS call already
            if (filesystem_lock_depth(nullptr /* fs */) == 1) {
                CHECK(flushBuffer());
            }
            canFlush = false;
        }
        // Unreachable
    }

    int flushBuffer() {
        if (!log_) {
            return 0;
        }
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

    bool isPrinting() const {
        return printing_.load(std::memory_order_relaxed);
    }
};

// Note: The instance is assigned in enableLogFile() and never reset. Resetting it would create a
// race between the logging functions and the log's deinitialization
std::unique_ptr<LogFile> g_logFile;
std::atomic<bool> g_logFileEnabled;
LogFileConfig g_logFileConfig;

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

int loadLogFileConfig(LogFileConfig& config) {
    fs::File file;
    CHECK(file.open(LOG_CONFIG_FILE, LFS_O_RDONLY));

    PB_INTERNAL(LogFileConfig) pbConf = {};
    CHECK(decodeProtobufFromFile(file.handle(), &PB_INTERNAL(LogFileConfig_msg), &pbConf));

    LogFileConfig conf;
    ::strlcpy(conf.filter.category, pbConf.category, sizeof(conf.filter.category));
    if (pbConf.level > 0) {
        conf.filter.level = pbConf.level;
    }
    if (pbConf.max_size > 0) {
        conf.maxSize = pbConf.max_size;
    }
    if (pbConf.buffer_size > 0) {
        conf.bufferSize = pbConf.buffer_size;
    }

    config = conf;
    return 0;
}

int saveLogFileConfig(const LogFileConfig& conf) {
    fs::File file;
    char tempPath[fs::TEMP_PATH_BUF_SIZE] = {};
    CHECK(createTempFile(file, tempPath, sizeof(tempPath), LFS_O_WRONLY));

    PB_INTERNAL(LogFileConfig) pbConf = {};
    ::strlcpy(pbConf.category, conf.filter.category, sizeof(pbConf.category));
    pbConf.level = conf.filter.level;
    pbConf.max_size = conf.maxSize;
    pbConf.buffer_size = conf.bufferSize;

    CHECK(encodeProtobufToFile(file.handle(), &PB_INTERNAL(LogFileConfig_msg), &pbConf));
    CHECK(file.close());
    CHECK(fs::rename(tempPath, LOG_CONFIG_FILE));
    return 0;
}

int readLogFileDctFlag(bool& enabled) {
    uint8_t val = 0;
    int r = dct_read_app_data_copy(DCT_LOG_FILE_ENABLED_OFFSET, &val, sizeof(val));
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    enabled = (val == 0x01); // Can be 0xff if not initialized
    return 0;
}

int writeLogFileDctFlag(bool enabled) {
    uint8_t val = enabled ? 0x01 : 0xff;
    int r = dct_write_app_data(&val, DCT_LOG_FILE_ENABLED_OFFSET, sizeof(val));
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

} // namespace

// Called by the system thread
int initLogFile() {
    fs::FsLock lock;

    CHECK(fs::mount());

    bool enabled = false;
    CHECK(readLogFileDctFlag(enabled));
    if (!enabled) {
        return 0;
    }

    LogFileConfig conf;
    CHECK(loadLogFileConfig(conf));
    CHECK(enableLogFile(conf));
    g_logFileConfig = conf;

    LOG_PRINT(INFO, "~~~~~~~~~~\r\n");
    return 0;
}

// Called by the system thread
int flushLogFile() {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return 0;
    }
    CHECK(g_logFile->flush());
    return 0;
}

// Can be called from any thread, e.g. through system_reset()
int closeLogFile() {
    // Stop writing to the log but don't flush it unless this is the system or app thread that has
    // a large enough stack for file IO
    g_logFileEnabled.exchange(false, std::memory_order_acq_rel);

    if (!isSystemOrMainThread() && !isAppThread()) {
        return 0;
    }
    fs::FsLock lock;

    if (g_logFile) {
        int r = g_logFile->flush();
        g_logFile->destroy();
        CHECK(r);
    }
    return 0;
}

// Called by the logging service
void logMessageToFile(const char* msg, int level, const char* category, const LogAttributes* attrs) {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !LOG_FROM_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    g_logFile->logMessage(msg, level, category, attrs);
}

void writeToLogFile(const char* data, size_t size, int level, const char* category) {
    if (!g_logFileEnabled.load(std::memory_order_acquire)) {
        return;
    }
#if !LOG_FROM_ISR
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
#if !LOG_FROM_ISR
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

int system_enable_log_file(int flags, const system_log_file_options* opts) {
    fs::FsLock lock;

    LogFileConfig newConf;
    newConf.flags = flags;

    if (opts) {
        if (opts->category) {
            ::strlcpy(newConf.filter.category, opts->category, sizeof(newConf.filter.category));
        }
        if (opts->level > 0) {
            newConf.filter.level = std::min<int>(opts->level, LOG_LEVEL_NONE /* 70 */);
        }
        if (opts->max_size > 0) {
            newConf.maxSize = opts->max_size;
        }
        if (opts->buffer_size > 0) {
            newConf.bufferSize = opts->buffer_size;
        }
    }

    if (!g_logFileEnabled.load(std::memory_order_relaxed) ||
            std::memcmp(&newConf, &g_logFileConfig, sizeof(LogFileConfig)) != 0) {
        CHECK(enableLogFile(newConf));
        g_logFileConfig = newConf;

        if (!(flags & SYSTEM_LOG_FILE_NO_PERSIST)) {
            CHECK(saveLogFileConfig(newConf));
            CHECK(writeLogFileDctFlag(true /* enabled */));
        }
    }
    return 0;
}

void system_disable_log_file(int flags, void* reserved) {
    fs::FsLock lock;

    g_logFileEnabled.store(false, std::memory_order_relaxed);

    if (g_logFile) {
        if (flags & SYSTEM_LOG_FILE_NO_CLEAR) {
            g_logFile->flush();
        }
        g_logFile->destroy();
    }
    if (!(flags & SYSTEM_LOG_FILE_NO_CLEAR)) {
        removeLogFiles();
    }
    if (!(flags & SYSTEM_LOG_FILE_NO_PERSIST)) {
        writeLogFileDctFlag(false /* enabled */);
        rmrf(LOG_CONFIG_FILE);
    }
    g_logFileConfig = LogFileConfig();
}

int system_print_log_file(size_t size, int level, const char* category, void* reserved) {
    fs::FsLock lock;

    if (!g_logFile) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (level <= 0) {
        level = LOG_LEVEL_TRACE;
    }
    if (!category) {
        category = "";
    }
    size_t n = CHECK(g_logFile->printLog(size, level, category));
    return n;
}

int system_read_log_file(size_t size, system_read_log_file_callback callback, void* arg, void* reserved) {
    fs::FsLock lock;

    if (!g_logFile) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t count = 0;
    if (callback) {
        count = CHECK(g_logFile->readLog(size, [=](const char* data, size_t size) {
            return callback(data, size, arg);
        }));
    } else {
        count = CHECK(g_logFile->getSize());
        if (size > 0 && size < count) {
            count = size;
        }
    }
    return count;
}

int system_clear_log_file(void* reserved) {
    fs::FsLock lock;

    if (g_logFile) {
        CHECK(g_logFile->clear());
    } else {
        CHECK(removeLogFiles());
    }
    return 0;
}

#endif // HAL_PLATFORM_LOG_FILE
