#include "boot_log.h"

#if HAL_PLATFORM_BOOT_LOG

#include "system_threading.h"
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

namespace particle::system {

namespace {

const auto BOOT_LOG_FILE1 = "/sys/bootlog.1";
const auto BOOT_LOG_FILE2 = "/sys/bootlog.2";

const size_t DEFAULT_BOOT_LOG_SIZE = 50000;

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

    int sync() {
        CHECK(file_.sync());
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

// A piece of log data to be appended to the buffer
struct Chunk {
    const char* data;
    size_t size;
};

retained_system BootLogConfig g_retainedConfig = {};

// Copy of the retained configuration. The logging functions run without the filesystem lock, so
// they can't access the retained memory which is updated under that lock
BootLogConfig g_config = {};

std::unique_ptr<BootLog> g_bootLog;
std::atomic<bool> g_bootLogEnabled;

services::RingBuffer<char> g_buffer;
std::unique_ptr<char[]> g_bufferData;
size_t g_droppedBytes = 0;

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

inline bool isBootLogEnabled() {
    return g_bootLogEnabled.load(std::memory_order_relaxed);
}

bool isBootLogEnabledForLevel(int level, const char* category) {
    if (!isBootLogEnabled()) {
        return false;
    }
    if (level < g_config.level) {
        return false;
    }
    if (category && !startsWith(category, g_config.category)) {
        return false;
    }
    return true;
}

void appendToBootLog(const Chunk* chunks, size_t count) {
    size_t size = 0;
    for (size_t i = 0; i < count; ++i) {
        size += chunks[i].size;
    }
    ATOMIC_BLOCK() {
        // The message is stored either in its entirety or not at all
        if (g_buffer.space() < (ssize_t)size) {
            g_droppedBytes += size;
            return;
        }
        for (size_t i = 0; i < count; ++i) {
            g_buffer.put(chunks[i].data, chunks[i].size);
        }
    }
}

// Note: The caller must acquire the filesystem lock before calling this function
int flushBufferedData() {
    auto log = g_bootLog.get();
    if (!log) { // Sanity check
        return 0;
    }
    size_t bytesAvail = 0;
    ATOMIC_BLOCK() {
        ssize_t n = g_buffer.data();
        if (n > 0) {
            bytesAvail = n;
        }
    }
    bool written = false;
    while (bytesAvail > 0) {
        size_t size = 0;
        const char* data = nullptr;
        ATOMIC_BLOCK() {
            size = std::min(g_buffer.consumable(), bytesAvail);
            if (size > 0) {
                data = g_buffer.consume(size);
            }
        }
        if (!data) {
            break;
        }
        // The data is written to the file with the interrupts enabled. The consumed region is not
        // released until it's committed below, so it can't be overwritten by the logging functions
        int r = log->write(data, size);
        ATOMIC_BLOCK() {
            g_buffer.consumeCommit(size);
        }
        CHECK(r);
        bytesAvail -= size;
        written = true;
    }
    size_t dropped = 0;
    ATOMIC_BLOCK() {
        dropped = g_droppedBytes;
        g_droppedBytes = 0;
    }
    if (dropped > 0) {
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "[bootlog] %u bytes dropped\r\n", (unsigned)dropped);
        if (n > 0) {
            CHECK(log->write(buf, std::min((size_t)n, sizeof(buf) - 1)));
            written = true;
        }
    }
    if (written) {
        CHECK(log->sync());
    }
    return 0;
}

} // namespace

int initBootLog() {
    // Use the filesystem lock for serializing access to the log file
    fs::FsLock lock;

    if (g_retainedConfig.magic != BOOT_LOG_CONFIG_MAGIC || !g_retainedConfig.enabled) {
        return 0;
    }
    CHECK(fs::mount());

    std::unique_ptr<char[]> buf(new(std::nothrow) char[HAL_PLATFORM_BOOT_LOG_BUFFER_SIZE]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    std::unique_ptr<BootLog> log(new(std::nothrow) BootLog(g_retainedConfig.maxSize));
    if (!log) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(log->init());

    g_buffer.init(buf.get(), HAL_PLATFORM_BOOT_LOG_BUFFER_SIZE);
    g_bufferData = std::move(buf);
    g_bootLog = std::move(log);
    g_config = g_retainedConfig;

    g_bootLogEnabled.store(true, std::memory_order_relaxed);
    return 0;
}

// Must be called from the system or application thread
int flushBootLog() {
    if (!isBootLogEnabled()) {
        return 0;
    }
    fs::FsLock lock;

    CHECK(flushBufferedData());
    return 0;
}

// Can be called from an ISR in system_reset()
void closeBootLog() {
    bool wasEnabled = g_bootLogEnabled.exchange(false, std::memory_order_relaxed);
    if (!wasEnabled || hal_interrupt_is_isr()) {
        return;
    }

    fs::FsLock lock;

    // Note: the ISR check above can't be omitted. os_thread_is_current() compares the current task
    // handle, which in an ISR is the handle of the interrupted task
    if (SYSTEM_THREAD_CURRENT() || APPLICATION_THREAD_CURRENT()) {
        flushBufferedData();
    }
    if (g_bootLog) {
        g_bootLog->close(); // Flush the data
    }
}

// Called by the logging service
void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs) {
#if !HAL_PLATFORM_BOOT_LOG_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    if (!isBootLogEnabledForLevel(level, category)) {
        return;
    }
    char timeBuf[12] = {};
    size_t timeSize = 0;
    if (attrs && attrs->has_time) {
        int n = snprintf(timeBuf, sizeof(timeBuf), "%010u ", (unsigned)attrs->time);
        if (n > 0) {
            timeSize = std::min((size_t)n, sizeof(timeBuf) - 1);
        }
    }
    auto levelName = log_level_name(level, nullptr /* reserved */);
    const Chunk chunks[] = {
        { timeBuf, timeSize },
        { "[", category ? 1u : 0u },
        { category, category ? std::strlen(category) : 0u },
        { "] ", category ? 2u : 0u },
        { levelName, std::strlen(levelName) },
        { ": ", 2 },
        { msg, msg ? std::strlen(msg) : 0u },
        { "\r\n", 2 }
    };
    appendToBootLog(chunks, sizeof(chunks) / sizeof(chunks[0]));
}

// Called by the logging service
void writeBootLog(const char* data, size_t size, int level, const char* category) {
#if !HAL_PLATFORM_BOOT_LOG_ISR
    if (hal_interrupt_is_isr()) {
        return;
    }
#endif
    if (!isBootLogEnabledForLevel(level, category)) {
        return;
    }
    const Chunk chunk = { data, size };
    appendToBootLog(&chunk, 1);
}

// Called by the logging service
bool isBootLogEnabled(int level, const char* category) {
#if !HAL_PLATFORM_BOOT_LOG_ISR
    if (hal_interrupt_is_isr()) {
        return false;
    }
#endif
    return isBootLogEnabledForLevel(level, category);
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

    g_retainedConfig = newConfig;
}

void system_flush_boot_log(int level, const char* category, void* reserved) {
    fs::FsLock lock;

    closeBootLog();

    if (level < LOG_LEVEL_NONE) {
        printBootLog(level, category ? category : "app");
    }

    g_bootLog.reset();
    ATOMIC_BLOCK() {
        g_buffer.init(nullptr, 0);
        g_droppedBytes = 0;
    }
    g_bufferData.reset();

    rmrf(BOOT_LOG_FILE2);
    rmrf(BOOT_LOG_FILE1);
}

#endif // HAL_PLATFORM_BOOT_LOG
