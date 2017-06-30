#include "application.h"
#include "unit-test/unit-test.h"

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

extern String serverHost;
extern unsigned serverPort;

namespace {

// Maximum number of simultaneously active connections supported by the system
const unsigned MAX_CONNECTIONS = 10;

// Default timeout values for TestClient operations
const unsigned DEFAULT_CONNECT_TIMEOUT = 1000;
const unsigned DEFAULT_READ_TIMEOUT = 10000;

void fillRandomBytes(char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = random(256);
    }
}

class TestClient: public TCPClient {
public:
    bool connect(unsigned timeout = DEFAULT_CONNECT_TIMEOUT) {
        bool ok = false;
        const unsigned t = millis();
        for (;;) {
            // WICED/LwIP may need some time to release resources associated with previously closed sockets
            delay(100);
            if (TCPClient::connect(serverHost, serverPort)) {
                ok = true;
                break;
            }
            if (timeout == 0 || millis() - t >= timeout) {
                Log.warn("TCPClient::connect() failed");
                break;
            }
        }
        return ok;
    }

    bool write(const char* data, size_t size) {
        const size_t n = TCPClient::write((const uint8_t*)data, size);
        if (n != size) {
            Log.warn("TCPClient::write() failed");
            return false;
        }
        return true;
    }

    bool read(char* data, size_t size, unsigned timeout = DEFAULT_READ_TIMEOUT) {
        bool ok = false;
        const unsigned t = millis();
        for (;;) {
            if (size > 0) {
                int avail = TCPClient::available();
                if (avail < 0) {
                    Log.warn("TCPClient::available() failed");
                    break;
                }
                if (avail > 0) {
                    if ((size_t)avail > size) {
                        avail = size;
                    }
                    const int n = TCPClient::read((uint8_t*)data, avail);
                    if (n != avail) {
                        Log.warn("TCPClient::read() failed");
                        break;
                    }
                    data += n;
                    size -= n;
                }
            }
            if (size == 0) {
                ok = true;
                break;
            }
            if (timeout != 0 && millis() - t >= timeout) {
                Log.warn("TestClient::read() timeout");
                break;
            }
            Particle.process();
        }
        return ok;
    }

    // Sends random data to the echo server
    bool echo(size_t size = 0, unsigned timeout = DEFAULT_READ_TIMEOUT) {
        if (size == 0) {
            size = random(16, 257); // 16 to 256 bytes
        }
        char sendBuf[size];
        fillRandomBytes(sendBuf, size);
        if (!write(sendBuf, size)) {
            return false;
        }
        char recvBuf[size];
        if (!read(recvBuf, size, timeout)) {
            return false;
        }
        if (memcmp(sendBuf, recvBuf, size) != 0) {
            Log.warn("TestClient: Received unexpected data");
            return false;
        }
        return true;
    }
};

class TestClientRunner {
public:
    explicit TestClientRunner(const char* name) :
            minPacketSize_(16),
            maxPacketSize_(256),
            minPacketDelay_(50),
            maxPacketDelay_(250),
            bytesPerClient_(0), // unlimited
            packetsPerClient_(0), // unlimited
            concurConns_(MAX_CONNECTIONS),
            totalConns_(0), // unlimited
            clientConnTimeout_(3000),
            clientReadTimeout_(16000),
            errorsAllowed_(3),
            runDuration_(30000), // 30 seconds
            name_(name) {
    }

    TestClientRunner& minPacketSize(unsigned size) {
        minPacketSize_ = size;
        return *this;
    }

    TestClientRunner& maxPacketSize(unsigned size) {
        maxPacketSize_ = size;
        return *this;
    }

    TestClientRunner& packetSize(unsigned size) {
        minPacketSize_ = size;
        maxPacketSize_ = size;
        return *this;
    }

    TestClientRunner& minPacketDelay(unsigned msec) {
        minPacketDelay_ = msec;
        return *this;
    }

    TestClientRunner& maxPacketDelay(unsigned msec) {
        maxPacketDelay_ = msec;
        return *this;
    }

    TestClientRunner& packetDelay(unsigned msec) {
        minPacketDelay_ = msec;
        maxPacketDelay_ = msec;
        return *this;
    }

    TestClientRunner& bytesPerClient(unsigned bytes) {
        bytesPerClient_ = bytes;
        return *this;
    }

    TestClientRunner& packetsPerClient(unsigned count) {
        packetsPerClient_ = count;
        return *this;
    }

    TestClientRunner& concurrentConnections(unsigned count) {
        concurConns_ = count;
        return *this;
    }

    TestClientRunner& totalConnections(unsigned count) {
        totalConns_ = count;
        return *this;
    }

    TestClientRunner& clientConnectTimeout(unsigned msec) {
        clientConnTimeout_ = msec;
        return *this;
    }

    TestClientRunner& clientReadTimeout(unsigned msec) {
        clientReadTimeout_ = msec;
        return *this;
    }

    TestClientRunner& errorsAllowed(unsigned count) {
        errorsAllowed_ = count;
        return *this;
    }

    TestClientRunner& runDuration(unsigned msec) {
        runDuration_ = msec;
        return *this;
    }

    bool run() {
        Vector<Client> clients;
        std::unique_ptr<char[]> sendBuf(new char[maxPacketSize_]), recvBuf(new char[maxPacketSize_]);
        unsigned errorCount = 0, connCount = 0, totalBytes = 0;
        bool ok = false;
        Log.info("%s: started", name_);
        const unsigned timeStarted = millis();
        unsigned timeAdjusted = timeStarted;
        for (;;) {
            // Connect to server
            while (clients.size() < (int)concurConns_ && (totalConns_ == 0 || connCount < totalConns_)) {
                Client c;
                const unsigned t = millis();
                if (!c.client.connect(clientConnTimeout_)) {
                    timeAdjusted += millis() - t;
                    if (++errorCount > errorsAllowed_) {
                        goto done;
                    }
                    continue;
                }
                clients.append(std::move(c));
                ++connCount;
            }
            if (clients.isEmpty()) {
                ok = true;
                break;
            }
            // Send packets
            fillRandomBytes(sendBuf.get(), maxPacketSize_);
            int i = 0;
            while (i < clients.size()) {
                Client& c = clients.at(i);
                if (c.lastPacketSize == 0 && millis() >= c.nextPacketTime) {
                    c.lastPacketSize = random(minPacketSize_, maxPacketSize_ + 1);
                    if (bytesPerClient_ != 0 && bytesPerClient_ - c.totalBytes < c.lastPacketSize) {
                        c.lastPacketSize = bytesPerClient_ - c.totalBytes;
                    }
                    const unsigned t = millis();
                    if (!c.client.write(sendBuf.get(), c.lastPacketSize)) {
                        timeAdjusted += millis() - t;
                        if (++errorCount > errorsAllowed_) {
                            goto done;
                        }
                        clients.removeAt(i); // Close connection
                        continue;
                    }
                }
                ++i;
            }
            // Receive packets
            i = 0;
            while (i < clients.size()) {
                Client& c = clients.at(i);
                if (c.lastPacketSize != 0) {
                    const unsigned t = millis();
                    bool readOk = false;
                    if (!(readOk = c.client.read(recvBuf.get(), c.lastPacketSize, clientReadTimeout_)) ||
                            memcmp(sendBuf.get(), recvBuf.get(), c.lastPacketSize) != 0) {
                        timeAdjusted += millis() - t;
                        if (readOk) {
                            Log.warn("TestClientRunner: Received unexpected data");
                        }
                        if (++errorCount > errorsAllowed_) {
                            goto done;
                        }
                        clients.removeAt(i);
                        continue;
                    }
                    totalBytes += c.lastPacketSize;
                    c.totalBytes += c.lastPacketSize;
                    ++c.totalPackets;
                    if ((bytesPerClient_ != 0 && c.totalBytes >= bytesPerClient_) ||
                            (packetsPerClient_ != 0 && c.totalPackets >= packetsPerClient_)) {
                        clients.removeAt(i);
                        continue;
                    }
                    c.nextPacketTime = millis() + random(minPacketDelay_, maxPacketDelay_ + 1);
                    c.lastPacketSize = 0;
                }
                ++i;
            }
            if (runDuration_ != 0 && millis() - timeAdjusted >= runDuration_) {
                ok = true;
                break;
            }
            Particle.process();
        }
    done:
        const unsigned dur = millis() - timeStarted;
        Log.info("%s: %s: total connections: %u; bytes sent/received: %u; errors: %u; duration: %u", name_,
                (ok ? "passed" : "failed"), connCount, totalBytes, errorCount, dur);
        return ok;
    }

private:
    struct Client {
        TestClient client;
        unsigned lastPacketSize, nextPacketTime, totalBytes, totalPackets;

        Client() :
                lastPacketSize(0),
                nextPacketTime(0),
                totalBytes(0),
                totalPackets(0) {
        }
    };

    unsigned minPacketSize_, maxPacketSize_, minPacketDelay_, maxPacketDelay_, bytesPerClient_, packetsPerClient_,
            concurConns_, totalConns_, clientConnTimeout_, clientReadTimeout_, errorsAllowed_, runDuration_;
    const char* name_;
};

class MemoryMonitor {
public:
    explicit MemoryMonitor(const char* name) :
            memFreeBefore_(0),
            memFreeAfter_(0),
            memMaxUsed_(0),
            name_(name),
            running_(false) {
        delay(250);
        memFreeBefore_ = System.freeMemory();
        thread_.reset(new Thread("memmon", run, this, OS_THREAD_PRIORITY_CRITICAL, 1024 /* stack_size */));
        while (!running_) {
            Particle.process();
        }
    }

    ~MemoryMonitor() {
        stop();
    }

    unsigned freeMemoryBefore() const {
        return memFreeBefore_;
    }

    unsigned freeMemoryAfter() const {
        return memFreeAfter_;
    }

    unsigned maxUsedMemory() const {
        return memMaxUsed_;
    }

    void stop() {
        if (thread_) {
            running_ = false;
            thread_->join();
            thread_.reset();
            delay(250);
            memFreeAfter_ = System.freeMemory();
            Log.info("%s: free memory before: %u; after: %u; max. used: %u", name_, memFreeBefore_,
                    memFreeAfter_, memMaxUsed_);
        }
    }

private:
    unsigned memFreeBefore_, memFreeAfter_, memMaxUsed_;
    const char* name_;

    std::unique_ptr<Thread> thread_;
    volatile bool running_;

    static void run(void* data) {
        MemoryMonitor* m = static_cast<MemoryMonitor*>(data);
        const unsigned freeStart = System.freeMemory();
        unsigned freeMin = freeStart;
        m->running_ = true;
        do {
            HAL_Delay_Milliseconds(20);
            const unsigned freeMem = System.freeMemory();
            if (freeMem < freeMin) {
                freeMin = freeMem;
            }
        } while (m->running_);
        m->memMaxUsed_ = freeStart - freeMin;
    }
};

} // namespace

test(tcp_client_01_basic_echo) {
    TestClient client;
    assertTrue(client.connect());
    assertTrue(client.echo());
    client.stop();
}

test(tcp_client_02_max_connections) {
    // Establish maximum number of connections
    Vector<TestClient> clients(MAX_CONNECTIONS);
    for (TestClient& client: clients) {
        assertTrue(client.connect());
    }
    // Try to establish one more connection
    TestClient client;
    assertFalse(client.connect());
    Log.info("^^ that's ok");
    // Close one of the connections
    clients.takeLast();
    // Try once again
    assertTrue(client.connect());
}

test(tcp_client_03_long_connections_small_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.minPacketSize(64);
    r.maxPacketSize(512);
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_04_long_connections_large_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.minPacketSize(2048); // Larger than MSS
    r.maxPacketSize(4096);
    // Limit to 3 connections to reduce heap usage
    r.concurrentConnections(std::min<unsigned>(3, MAX_CONNECTIONS));
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_05_short_connections_small_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.minPacketSize(64);
    r.maxPacketSize(512);
    r.packetsPerClient(3);
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_06_short_connections_large_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.minPacketSize(2048); // Larger than MSS
    r.maxPacketSize(4096);
    r.packetsPerClient(2);
    // Limit to 3 connections to reduce heap usage
    r.concurrentConnections(std::min<unsigned>(3, MAX_CONNECTIONS));
    r.runDuration(30000);
    assertTrue(r.run());
}
