#pragma once

#include "spark_wiring_tcpclient.h"
#include "spark_wiring_thread.h"

// Declares TestClientRunner instance
#define TEST_CLIENT_RUNNER(_name) \
        TestClientRunner _name(test_name())

// Starts memory monitor thread (debug-only)
#ifdef DEBUG_BUILD
#define START_MEMORY_MONITOR() \
        MemoryMonitor PP_CAT(memMon, __COUNTER__)(test_name())
#else
#define START_MEMORY_MONITOR()
#endif

namespace particle {

namespace util {

enum class ServerType {
    ECHO,
    DISCARD,
    CHARGEN
};

class TestClient: public TCPClient {
public:
    static const unsigned DEFAULT_CONNECT_TIMEOUT = 1000;
    static const unsigned DEFAULT_READ_TIMEOUT = 10000;

    explicit TestClient(ServerType type = ServerType::ECHO);

    bool connect(unsigned timeout = DEFAULT_CONNECT_TIMEOUT);
    bool write(const char* data, size_t size);
    bool read(char* data, size_t size, unsigned timeout = DEFAULT_READ_TIMEOUT);

    // Sends random data to the echo server and checks its reply
    bool echo(size_t size = 0, unsigned timeout = DEFAULT_READ_TIMEOUT);

private:
    unsigned port_;
};

class TestClientRunner {
public:
    explicit TestClientRunner(const char* name);

    TestClientRunner& serverType(ServerType type);
    TestClientRunner& minPacketSize(unsigned size);
    TestClientRunner& maxPacketSize(unsigned size);
    TestClientRunner& packetSize(unsigned size); // Sets fixed packet size
    TestClientRunner& minPacketDelay(unsigned msec);
    TestClientRunner& maxPacketDelay(unsigned msec);
    TestClientRunner& packetDelay(unsigned msec); // Sets fixed packet delay
    TestClientRunner& bytesPerClient(unsigned bytes);
    TestClientRunner& packetsPerClient(unsigned count);
    TestClientRunner& concurrentConnections(unsigned count);
    TestClientRunner& totalConnections(unsigned count);
    TestClientRunner& clientConnectTimeout(unsigned msec);
    TestClientRunner& clientReadTimeout(unsigned msec);
    TestClientRunner& errorsAllowed(unsigned count);
    TestClientRunner& runDuration(unsigned msec);

    bool run();

private:
    struct Client {
        TestClient client;
        unsigned packetSize, packetTime, totalBytes, totalPackets;

        explicit Client(ServerType type) :
                client(type),
                packetSize(0),
                packetTime(0),
                totalBytes(0),
                totalPackets(0) {
        }
    };

    unsigned minPacketSize_, maxPacketSize_, minPacketDelay_, maxPacketDelay_, bytesPerClient_, packetsPerClient_,
            concurConns_, totalConns_, clientConnTimeout_, clientReadTimeout_, errorsAllowed_, runDuration_;
    ServerType serverType_;
    const char* name_;
};

class MemoryMonitor {
public:
    explicit MemoryMonitor(const char* name);
    ~MemoryMonitor();

    unsigned freeMemoryBefore() const;
    unsigned freeMemoryAfter() const;
    unsigned maxUsedMemory() const;

    void stop();

private:
    unsigned memFreeBefore_, memFreeAfter_, memMaxUsed_;
    const char* name_;

    std::unique_ptr<Thread> thread_;
    volatile bool running_;

    static void run(void* data);
};

void setServerHost(const char* host);
const char* serverHost();

void setServerPort(ServerType, unsigned port);
unsigned serverPort(ServerType);

void fillRandomBytes(char* data, size_t size);

} // namespace particle::util

} // namespace particle

inline particle::util::TestClientRunner& particle::util::TestClientRunner::serverType(ServerType type) {
    serverType_ = type;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::minPacketSize(unsigned size) {
    minPacketSize_ = size;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::maxPacketSize(unsigned size) {
    maxPacketSize_ = size;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::packetSize(unsigned size) {
    minPacketSize_ = size;
    maxPacketSize_ = size;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::minPacketDelay(unsigned msec) {
    minPacketDelay_ = msec;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::maxPacketDelay(unsigned msec) {
    maxPacketDelay_ = msec;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::packetDelay(unsigned msec) {
    minPacketDelay_ = msec;
    maxPacketDelay_ = msec;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::bytesPerClient(unsigned bytes) {
    bytesPerClient_ = bytes;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::packetsPerClient(unsigned count) {
    packetsPerClient_ = count;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::concurrentConnections(unsigned count) {
    concurConns_ = count;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::totalConnections(unsigned count) {
    totalConns_ = count;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::clientConnectTimeout(unsigned msec) {
    clientConnTimeout_ = msec;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::clientReadTimeout(unsigned msec) {
    clientReadTimeout_ = msec;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::errorsAllowed(unsigned count) {
    errorsAllowed_ = count;
    return *this;
}

inline particle::util::TestClientRunner& particle::util::TestClientRunner::runDuration(unsigned msec) {
    runDuration_ = msec;
    return *this;
}

inline unsigned particle::util::MemoryMonitor::freeMemoryBefore() const {
    return memFreeBefore_;
}

inline unsigned particle::util::MemoryMonitor::freeMemoryAfter() const {
    return memFreeAfter_;
}

inline unsigned particle::util::MemoryMonitor::maxUsedMemory() const {
    return memMaxUsed_;
}
