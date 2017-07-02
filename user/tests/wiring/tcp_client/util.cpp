#include "util.h"

#include "spark_wiring_cloud.h"
#include "spark_wiring_ticks.h"
#include "spark_wiring_random.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_logging.h"

namespace {

using spark::Vector;
using spark::Log;

String g_serverHost;
unsigned g_serverEchoPort = 0, g_serverDiscardPort = 0, g_serverChargenPort = 0;

} // namespace

particle::util::TestClient::TestClient(ServerType type) :
        port_(serverPort(type)) {
}

bool particle::util::TestClient::connect(unsigned timeout) {
    bool ok = false;
    const unsigned t = millis();
    for (;;) {
        // WICED/LwIP may need some time to release resources associated with previously closed sockets
        delay(100);
        if (TCPClient::connect(serverHost(), port_)) {
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

bool particle::util::TestClient::write(const char* data, size_t size) {
    const size_t n = TCPClient::write((const uint8_t*)data, size);
    if (n != size) {
        Log.warn("TCPClient::write() failed");
        return false;
    }
    return true;
}

bool particle::util::TestClient::read(char* data, size_t size, unsigned timeout) {
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

bool particle::util::TestClient::echo(size_t size, unsigned timeout) {
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

particle::util::TestClientRunner::TestClientRunner(const char* name) :
        minPacketSize_(16),
        maxPacketSize_(256),
        minPacketDelay_(50),
        maxPacketDelay_(250),
        bytesPerClient_(0), // unlimited
        packetsPerClient_(0), // unlimited
        concurConns_(1),
        totalConns_(0), // unlimited
        clientConnTimeout_(3000),
        clientReadTimeout_(15000),
        errorsAllowed_(3),
        runDuration_(30000), // 30 seconds
        serverType_(ServerType::ECHO),
        name_(name) {
}

bool particle::util::TestClientRunner::run() {
    std::unique_ptr<char[]> sendBuf, recvBuf;
    if (serverType_ == ServerType::ECHO || serverType_ == ServerType::DISCARD) {
        sendBuf.reset(new char[maxPacketSize_]);
    }
    if (serverType_ == ServerType::ECHO || serverType_ == ServerType::CHARGEN) {
        recvBuf.reset(new char[maxPacketSize_]);
    }
    Vector<Client> clients;
    unsigned errorCount = 0, connCount = 0, totalBytes = 0;
    bool ok = false;
    Log.info("%s: started", name_);
    const unsigned timeStarted = millis();
    unsigned timeAdjusted = timeStarted;
    for (;;) {
        // Connect to server
        while (clients.size() < (int)concurConns_ && (totalConns_ == 0 || connCount < totalConns_)) {
            Client c(serverType_);
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
        // Generate packet sizes
        for (Client& c: clients) {
            if (c.packetSize == 0 && millis() >= c.packetTime) {
                c.packetSize = random(minPacketSize_, maxPacketSize_ + 1);
                if (bytesPerClient_ != 0 && bytesPerClient_ - c.totalBytes < c.packetSize) {
                    c.packetSize = bytesPerClient_ - c.totalBytes;
                }
            }
        }
        // Send packets
        if (serverType_ == ServerType::ECHO || serverType_ == ServerType::DISCARD) {
            fillRandomBytes(sendBuf.get(), maxPacketSize_);
            int i = 0;
            while (i < clients.size()) {
                Client& c = clients.at(i);
                if (c.packetSize != 0) {
                    const unsigned t = millis();
                    if (!c.client.write(sendBuf.get(), c.packetSize)) {
                        if (++errorCount > errorsAllowed_) {
                            goto done;
                        }
                        timeAdjusted += millis() - t;
                        clients.removeAt(i); // Close connection
                        continue;
                    }
                }
                ++i;
            }
        }
        // Receive packets
        if (serverType_ == ServerType::ECHO || serverType_ == ServerType::CHARGEN) {
            int i = 0;
            while (i < clients.size()) {
                Client& c = clients.at(i);
                if (c.packetSize != 0) {
                    const unsigned t = millis();
                    bool ok = c.client.read(recvBuf.get(), c.packetSize, clientReadTimeout_);
                    if (ok && serverType_ == ServerType::ECHO) {
                        ok = (memcmp(sendBuf.get(), recvBuf.get(), c.packetSize) == 0);
                        if (!ok) {
                            Log.warn("TestClientRunner: Received unexpected data");
                        }
                    }
                    if (!ok) {
                        if (++errorCount > errorsAllowed_) {
                            goto done;
                        }
                        timeAdjusted += millis() - t;
                        clients.removeAt(i);
                        continue;
                    }
                }
                ++i;
            }
        }
        // Close connections
        int i = 0;
        while (i < clients.size()) {
            Client& c = clients.at(i);
            if (c.packetSize != 0) {
                totalBytes += c.packetSize;
                c.totalBytes += c.packetSize;
                ++c.totalPackets;
                if ((bytesPerClient_ != 0 && c.totalBytes >= bytesPerClient_) ||
                        (packetsPerClient_ != 0 && c.totalPackets >= packetsPerClient_)) {
                    clients.removeAt(i);
                    continue;
                }
                c.packetTime = millis() + random(minPacketDelay_, maxPacketDelay_ + 1);
                c.packetSize = 0;
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

particle::util::MemoryMonitor::MemoryMonitor(const char* name) :
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

particle::util::MemoryMonitor::~MemoryMonitor() {
    stop();
}

void particle::util::MemoryMonitor::stop() {
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

void particle::util::MemoryMonitor::run(void* data) {
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

void particle::util::setServerHost(const char* host) {
    g_serverHost = host;
}

const char* particle::util::serverHost() {
    return g_serverHost.c_str();
}

void particle::util::setServerPort(ServerType type, unsigned port) {
    switch (type) {
    case ServerType::ECHO:
        g_serverEchoPort = port;
        break;
    case ServerType::DISCARD:
        g_serverDiscardPort = port;
        break;
    case ServerType::CHARGEN:
        g_serverChargenPort = port;
        break;
    default:
        break;
    }
}

unsigned particle::util::serverPort(ServerType type) {
    switch (type) {
    case ServerType::ECHO:
        return g_serverEchoPort;
    case ServerType::DISCARD:
        return g_serverDiscardPort;
    case ServerType::CHARGEN:
        return g_serverChargenPort;
    default:
        return 0;
    }
}

void particle::util::fillRandomBytes(char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = random(256);
    }
}
