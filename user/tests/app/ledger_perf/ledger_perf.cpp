#define LOG_CHECKED_ERRORS 1 // Log errors caught by the CHECK() macro

#include <functional>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <cmath>

#include "application.h"

#include "random.h"
#include "scope_guard.h"
#include "check.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

namespace {

const auto LEDGER_NAME = "test";

const size_t SMALL_DATA_SIZE = 100;
const size_t MEDIUM_DATA_SIZE = 1000;
const size_t LARGE_DATA_SIZE = 10000;

static_assert(SMALL_DATA_SIZE <= LARGE_DATA_SIZE && MEDIUM_DATA_SIZE <= LARGE_DATA_SIZE &&
        LARGE_DATA_SIZE <= LEDGER_MAX_DATA_SIZE);

const auto REPEAT_COUNT = 100;

const auto DESCR_COLUMN_WIDTH = 62;
const auto MIN_MAX_AVG_COLUMN_WIDTH = 7;

const auto MAX_TICKS = std::numeric_limits<system_tick_t>::max();

struct Stats {
    system_tick_t open;
    system_tick_t close;
    system_tick_t readOrWrite;
    system_tick_t getInstance;
    system_tick_t release;
    system_tick_t openReadOrWriteClose;
    system_tick_t total;
};

const SerialLogHandler logHandler(LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_ALL }
});

char inBuffer[LARGE_DATA_SIZE];
char outBuffer[LARGE_DATA_SIZE];

int readFromLedger(char* data, size_t size, Stats& stats, const std::function<int()>& onStreamOpen = nullptr) {
    int r = 0;
    // Get the ledger
    ledger_instance* ledger = nullptr;
    auto t1 = millis();
    r = ledger_get_instance(&ledger, LEDGER_NAME, nullptr);
    auto t2 = millis();
    CHECK(r);
    NAMED_SCOPE_GUARD(releaseLedgerGuard, {
        ledger_release(ledger, nullptr);
    });
    // Open a stream
    ledger_stream* stream = nullptr;
    auto t3 = millis();
    r = ledger_open(&stream, ledger, LEDGER_STREAM_MODE_READ, nullptr);
    auto t4 = millis();
    CHECK(r);
    NAMED_SCOPE_GUARD(closeStreamGuard, {
        int r = ledger_close(stream, 0, nullptr);
        if (r < 0) {
            LOG(ERROR, "ledger_close() failed: %d", r);
        }
    });
    if (onStreamOpen) {
        CHECK(onStreamOpen());
    }
    // Read the data
    auto t5 = millis();
    r = ledger_read(stream, data, size, nullptr);
    auto t6 = millis();
    CHECK(r);
    if ((size_t)r != size) {
        LOG(ERROR, "Unexpected size of ledger data");
        return Error::BAD_DATA;
    }
    char c = 0;
    r = ledger_read(stream, &c, 1, nullptr);
    if (r != SYSTEM_ERROR_END_OF_STREAM) {
        LOG(ERROR, "Unexpected size of ledger data");
        return Error::BAD_DATA;
    }
    // Close the stream
    closeStreamGuard.dismiss();
    auto t7 = millis();
    r = ledger_close(stream, 0, nullptr);
    auto t8 = millis();
    CHECK(r);
    // Release the ledger
    releaseLedgerGuard.dismiss();
    auto t9 = millis();
    ledger_release(ledger, nullptr);
    auto t10 = millis();

    stats.getInstance = t2 - t1;
    stats.open = t4 - t3;
    stats.readOrWrite = t6 - t5;
    stats.close = t8 - t7;
    stats.release = t10 - t9;
    stats.openReadOrWriteClose = stats.open + stats.readOrWrite + stats.close;
    stats.total = stats.getInstance + stats.openReadOrWriteClose + stats.release;
    return 0;
}

int writeToLedger(const char* data, size_t size, Stats& stats) {
    int r = 0;
    // Get the ledger
    ledger_instance* ledger = nullptr;
    auto t1 = millis();
    r = ledger_get_instance(&ledger, LEDGER_NAME, nullptr);
    auto t2 = millis();
    CHECK(r);
    NAMED_SCOPE_GUARD(releaseLedgerGuard, {
        ledger_release(ledger, nullptr);
    });
    // Open a stream
    ledger_stream* stream = nullptr;
    auto t3 = millis();
    r = ledger_open(&stream, ledger, LEDGER_STREAM_MODE_WRITE, nullptr);
    auto t4 = millis();
    CHECK(r);
    NAMED_SCOPE_GUARD(closeStreamGuard, {
        int r = ledger_close(stream, LEDGER_STREAM_CLOSE_DISCARD, nullptr);
        if (r < 0) {
            LOG(ERROR, "ledger_close() failed: %d", r);
        }
    });
    // Write the data
    auto t5 = millis();
    r = ledger_write(stream, data, size, nullptr);
    auto t6 = millis();
    CHECK(r);
    if ((size_t)r != size) {
        LOG(ERROR, "Unexpected number of bytes written");
        return Error::IO;
    }
    // Close the stream
    closeStreamGuard.dismiss();
    auto t7 = millis();
    r = ledger_close(stream, 0, nullptr);
    auto t8 = millis();
    CHECK(r);
    // Release the ledger
    releaseLedgerGuard.dismiss();
    auto t9 = millis();
    ledger_release(ledger, nullptr);
    auto t10 = millis();

    stats.getInstance = t2 - t1;
    stats.open = t4 - t3;
    stats.readOrWrite = t6 - t5;
    stats.close = t8 - t7;
    stats.release = t10 - t9;
    stats.openReadOrWriteClose = stats.open + stats.readOrWrite + stats.close;
    stats.total = stats.getInstance + stats.openReadOrWriteClose + stats.release;
    return 0;
}

void updateStats(const Stats& stats, Stats& min, Stats& max, Stats& total) {
    min.open = std::min(min.open, stats.open);
    min.close = std::min(min.close, stats.close);
    min.readOrWrite = std::min(min.readOrWrite, stats.readOrWrite);
    min.getInstance = std::min(min.getInstance, stats.getInstance);
    min.release = std::min(min.release, stats.release);
    min.openReadOrWriteClose = std::min(min.openReadOrWriteClose, stats.openReadOrWriteClose);
    min.total = std::min(min.total, stats.total);

    max.open = std::max(max.open, stats.open);
    max.close = std::max(max.close, stats.close);
    max.readOrWrite = std::max(max.readOrWrite, stats.readOrWrite);
    max.getInstance = std::max(max.getInstance, stats.getInstance);
    max.release = std::max(max.release, stats.release);
    max.openReadOrWriteClose = std::max(max.openReadOrWriteClose, stats.openReadOrWriteClose);
    max.total = std::max(max.total, stats.total);

    total.open += stats.open;
    total.close += stats.close;
    total.readOrWrite += stats.readOrWrite;
    total.getInstance += stats.getInstance;
    total.release += stats.release;
    total.openReadOrWriteClose += stats.openReadOrWriteClose;
    total.total += stats.total;
}

void averageStats(Stats& total, int count) {
    total.open = std::round((double)total.open / count);
    total.close = std::round((double)total.close / count);
    total.readOrWrite = std::round((double)total.readOrWrite / count);
    total.getInstance = std::round((double)total.getInstance / count);
    total.release = std::round((double)total.release / count);
    total.openReadOrWriteClose = std::round((double)total.openReadOrWriteClose / count);
    total.total = std::round((double)total.total / count);
}

void printStatsRow(const char* desc, system_tick_t min, system_tick_t max, system_tick_t avg) {
    const auto w = MIN_MAX_AVG_COLUMN_WIDTH;
    LOG_PRINTF(INFO, "%-*s%-*u%-*u%-*u\r\n", DESCR_COLUMN_WIDTH, desc, w, (unsigned)avg, w, (unsigned)max, w, (unsigned)min);
}

void printStats(const Stats& min, const Stats& max, const Stats& avg, bool reading) {
    const auto w = MIN_MAX_AVG_COLUMN_WIDTH;
    LOG_PRINT(INFO, "\033[2m");
    LOG_PRINTF(INFO, "%-*s%-*s%-*s%-*s\r\n", DESCR_COLUMN_WIDTH, "", w, "avg", w, "max", w, "min");
    // ledger_get_instance
    printStatsRow("ledger_get_instance", min.getInstance, max.getInstance, avg.getInstance);
    // ledger_open
    printStatsRow("ledger_open", min.open, max.open, avg.open);
    // ledger_read/ledger_write
    auto readOrWriteFn = reading ? "ledger_read" : "ledger_write";
    printStatsRow(readOrWriteFn, min.readOrWrite, max.readOrWrite, avg.readOrWrite);
    // ledger_close
    printStatsRow("ledger_close", min.close, max.close, avg.close);
    // ledger_release
    printStatsRow("ledger_release", min.release, max.release, avg.release);
    LOG_PRINT(INFO, "\033[0m");
    // ledger_open + ledger_read/ledger_write + ledger_close
    // This is a typical scenario for applications that instantiate ledgers globally
    LOG_PRINT(INFO, "\033[1m");
    auto desc = String::format("ledger_open + %s + ledger_close", readOrWriteFn);
    printStatsRow(desc, min.openReadOrWriteClose, max.openReadOrWriteClose, avg.openReadOrWriteClose);
    // ledger_get_instance + ledger_open + ledger_read/ledger_write + ledger_close + ledger_release
    // This is a typical scenario for applications that instantiate ledgers on demand
    printStatsRow("ledger_get_instance + ledger_open + ... + ledger_release", min.total, max.total, avg.total);
    LOG_PRINT(INFO, "\033[0m");
}

int testWriteAndRead(size_t size, bool flushOnRead = false) {
    if (size > LARGE_DATA_SIZE) {
        return Error::INTERNAL;
    }

    CHECK(ledger_purge(LEDGER_NAME, nullptr));

    // Pre-initialize the ledger directory
    ledger_instance* ledger = nullptr;
    CHECK(ledger_get_instance(&ledger, LEDGER_NAME, nullptr));
    NAMED_SCOPE_GUARD(releaseLedgerGuard, {
        ledger_release(ledger, nullptr);
    });
    if (!flushOnRead) {
        releaseLedgerGuard.dismiss();
        ledger_release(ledger, nullptr);
    }

    Stats readMin = { .open = MAX_TICKS, .close = MAX_TICKS, .readOrWrite = MAX_TICKS, .getInstance = MAX_TICKS,
            .release = MAX_TICKS, .openReadOrWriteClose = MAX_TICKS, .total = MAX_TICKS };
    Stats readMax = {};
    Stats readAvg = {};

    Stats writeMin = { .open = MAX_TICKS, .close = MAX_TICKS, .readOrWrite = MAX_TICKS, .getInstance = MAX_TICKS,
            .release = MAX_TICKS, .openReadOrWriteClose = MAX_TICKS, .total = MAX_TICKS };
    Stats writeMax = {};
    Stats writeAvg = {};

    Random rand;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        delay(rand.gen<unsigned>() % 50);

        ledger_stream* stream = nullptr;
        NAMED_SCOPE_GUARD(closeStreamGuard, {
            int r = ledger_close(stream, 0, nullptr); // Can be called with a null stream
            if (r < 0) {
                LOG(ERROR, "ledger_close() failed: %d", r);
            }
        });
        if (flushOnRead) {
            // Start reading the ledger using a separate stream so that the data written below can't
            // be flushed immediately
            CHECK(ledger_open(&stream, ledger, LEDGER_STREAM_MODE_READ, nullptr));
        }

        // Write data
        rand.gen(outBuffer, size);
        Stats writeStats = {};
        CHECK(writeToLedger(outBuffer, size, writeStats));

        // Read the data back
        Stats readStats = {};
        CHECK(readFromLedger(inBuffer, size, readStats, [&]() {
            // Close the first stream so that the data can be flushed when reading is finished
            if (flushOnRead) {
                closeStreamGuard.dismiss();
                CHECK(ledger_close(stream, 0, nullptr));
            }
            return 0;
        }));
        if (std::memcmp(inBuffer, outBuffer, size) != 0) {
            LOG(ERROR, "Unexpected ledger data");
            return Error::BAD_DATA;
        }

        updateStats(writeStats, writeMin, writeMax, writeAvg);
        updateStats(readStats, readMin, readMax, readAvg);
    }

    averageStats(writeAvg, REPEAT_COUNT);
    averageStats(readAvg, REPEAT_COUNT);

    auto msg = flushOnRead ? "flushing when reading is finished" : "flushing immediately";
    LOG_PRINTF(INFO, "\r\n\033[4m%d writes of %d bytes; %s\033[0m:\r\n", REPEAT_COUNT, (int)size, msg);
    printStats(writeMin, writeMax, writeAvg, false);

    LOG_PRINTF(INFO, "\r\n\033[4m%d reads of %d bytes; %s\033[0m:\r\n", REPEAT_COUNT, (int)size, msg);
    printStats(readMin, readMax, readAvg, true /* reading */);

    return 0;
}

int runTests() {
    CHECK(ledger_purge_all(nullptr));

    const int sizeCount = 3;
    const size_t sizes[sizeCount] = { SMALL_DATA_SIZE, MEDIUM_DATA_SIZE, LARGE_DATA_SIZE };

    const int testCount = sizeCount * 2;
    int testIndex = 1;

    for (int i = 0; i < sizeCount; ++i) {
        LOG_PRINTF(INFO, "\r\nRunning test %d of %d...\r\n", testIndex++, testCount);
        CHECK(testWriteAndRead(sizes[i]));

        LOG_PRINTF(INFO, "\r\nRunning test %d of %d...\r\n", testIndex++, testCount);
        CHECK(testWriteAndRead(sizes[i], true /* flushOnRead */));
    }

    CHECK(ledger_purge_all(nullptr));

    LOG_PRINT(INFO, "\r\nDone.\r\n");

    return 0;
}

} // namespace

void setup() {
    waitUntil(Serial.isConnected);
    delay(1000);

    int r = runTests();
    if (r < 0) {
        LOG(ERROR, "runTests() failed: %d", r);
    }
}

void loop() {
}
