#include "application.h"

SYSTEM_MODE(MANUAL)

#if USE_THREADING == 1
SYSTEM_THREAD(ENABLED)
#endif

// Assertion macro to use in Errorable classes
#define CHECK(_expr) \
        do { \
            if (!(_expr)) { \
                Errorable::setError("%s:%d: Assertion failed: %s", fileName(__FILE__), __LINE__, #_expr); \
            } \
        } while (false)

// Registers server factory
#define REGISTER_SERVER(_type) \
        STARTUP(ServerManager::instance()->registerServer(#_type, [](const char* id, unsigned port) { \
            return new _type(id, port); \
        }));

namespace {

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

const char* fileName(const char *path);

// Base class for everything that may fail
class Errorable {
public:
    explicit Errorable(const char* name = nullptr) :
            log(name ? name : "app"),
            error_(Error::NONE) {
    }

    virtual ~Errorable() = default;

    void setError(const char* fmt, ...) {
        char msg[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        setError(Error(msg));
    }

    void setError(Error error) {
        error_ = std::move(error);
        log.error(error_.message());
    }

    void resetError() {
        error_ = Error::NONE;
    }

    const Error& error() const {
        return error_;
    }

    bool hasError() const {
        return (error_ != Error::NONE);
    }

protected:
    Logger log;

private:
    Error error_;
};

// Base class for test servers
class Server: public Errorable {
public:
    Server(const char* id, unsigned port) :
            Errorable("app.Server"),
            id_(id),
            port_(port) {
        if (strlen(id) != 0 && id_.length() == 0) {
            setError(Error::NO_MEMORY);
        }
    }

    virtual ~Server() {
        stop();
    }

    // Starts server
    bool start() {
        if (!server_) {
            log.trace("%s: Starting server", (const char*)id_);
            server_.reset(new TCPServer(port_));
            if (!server_) {
                setError(Error::NO_MEMORY);
                return false;
            }
            if (!server_->begin()) {
                setError("Unable to start server");
                server_.reset();
                return false;
            }
        }
        return true;
    }

    // Stops server
    void stop() {
        if (server_) {
            log.trace("%s: Stopping server", (const char*)id_);
            server_->stop();
            server_.reset();
        }
    }

    // Runs next server loop iteration
    void run() {
        if (server_) {
            run(*server_);
        }
    }

    const char* id() const {
        return id_;
    }

    unsigned port() const {
        return port_;
    }

protected:
    virtual void run(TCPServer& server) {
        // Default implementation accepts and closes client connections immediately
        TCPClient client;
        while (client = server.available()) {
            client.stop();
        }
    }

private:
    const String id_;
    const unsigned port_;

    std::unique_ptr<TCPServer> server_;
};

class ServerManager: public Errorable {
public:
    typedef std::function<Server*(const char* id, unsigned port)> ServerFactoryFunction;

    bool startServer(const char* id, const char* type, unsigned port) {
        if (strlen(id) == 0) {
            setError("Invalid server ID");
            return false;
        }
        if (hasServer(id)) {
            setError("Server is already running: %s", id);
            return false;
        }
        log.info("Starting server: %s, port: %u (%s)", id, port, type);
        std::unique_ptr<Server> server(createServer(id, type, port));
        if (!server) {
            // Error has been set by createServer() already
            return false;
        }
        if (!server->start()) {
            setError(server->error());
            return false;
        }
        if (!servers_.append(std::move(server))) {
            setError(Error::NO_MEMORY);
            return false;
        }
        return true;
    }

    bool stopServer(const char* id) {
        for (int i = 0; i < servers_.size(); ++i) {
            if (strcmp(servers_.at(i)->id(), id) == 0) {
                log.info("Stopping server: %s", id);
                std::unique_ptr<Server> server(servers_.takeAt(i));
                server->stop();
                // Give WICED some time to complete any asynchronous cleanup, so that we could reuse the same
                // server address later
                delay(500);
                if (server->hasError()) {
                    setError(server->error()); // Server has finished with an error
                    return false;
                }
                return true;
            }
        }
        setError("Unknown server ID: %s", id);
        return false;
    }

    bool stopServers() {
        bool ok = true;
        if (!servers_.isEmpty()) {
            log.info("Stopping all servers");
            for (auto& server: servers_) {
                server->stop();
                if (ok && server->hasError()) {
                    setError(server->error()); // Server has finished with an error
                    ok = false;
                }
            }
            servers_.clear();
            delay(500);
        }
        return ok;
    }

    bool hasServer(const char* id) const {
        for (const auto& server: servers_) {
            if (strcmp(server->id(), id) == 0) {
                return true;
            }
        }
        return false;
    }

    bool registerServer(const char* type, ServerFactoryFunction factory) {
        if (strlen(type) == 0) {
            setError("Invalid type name");
            return false;
        }
        ServerType t;
        t.name = type;
        t.factory = std::move(factory);
        if (t.name.length() == 0 || !serverTypes_.append(std::move(t))) {
            setError(Error::NO_MEMORY);
            return false;
        }
        return true;
    }

    void run() {
        for (auto& server: servers_) {
            server->run();
        }
    }

    static ServerManager* instance() {
        static ServerManager mgr;
        return &mgr;
    }

private:
    struct ServerType {
        String name;
        ServerFactoryFunction factory;
    };

    Vector<std::unique_ptr<Server>> servers_;
    Vector<ServerType> serverTypes_;

    ServerManager() :
            Errorable("app.ServerManager") {
    }

    Server* createServer(const char* id, const char* type, unsigned port) {
        for (const ServerType& t: serverTypes_) {
            if (t.name == type) {
                std::unique_ptr<Server> server(t.factory(id, port));
                if (!server) {
                    setError(Error::NO_MEMORY);
                    return nullptr;
                }
                if (server->hasError()) {
                    setError(server->error()); // Error while initializing server instance
                    return nullptr;
                }
                return server.release();
            }
        }
        setError("Unknown server type: %s", type);
        return nullptr;
    }
};

class NetworkManager: public Errorable {
public:
    bool connect() {
        if (isConnected()) {
            return true;
        }
        addr_ = IPAddress();
        WiFi.connect();
        if (!waitFor(WiFi.ready, 10000)) {
            WiFi.disconnect();
            setError("Unable to connect to network");
            return false;
        }
        // Network address doesn't seem to be available right after WiFi.connect() returns
        system_tick_t t = millis();
        for (;;) {
            addr_ = WiFi.localIP();
            if (addr_ || millis() - t >= 5000) {
                break;
            }
            Particle.process();
        }
        if (!addr_) {
            WiFi.disconnect();
            setError("Unable to get IP configuration");
            return false;
        }
        log.info("Connected to network, device address: %s", (const char*)addr_.toString());
        return true;
    }

    void disconnect() {
        const bool wasConnected = WiFi.ready();
        WiFi.disconnect();
        addr_ = IPAddress();
        if (wasConnected) {
            log.info("Disconnected from network");
        }
    }

    bool isConnected() const {
        return (WiFi.ready() && addr_);
    }

    const IPAddress& address() const {
        return addr_;
    }

    static NetworkManager* instance() {
        static NetworkManager mgr;
        return &mgr;
    }

private:
    IPAddress addr_;

    NetworkManager() :
            Errorable("app.NetworkManager") {
    }
};

// Server tracking client connections
class SimpleServer: public Server {
public:
    explicit SimpleServer(const char* id, unsigned port) :
            Server(id, port),
            nextConnId_(1) {
    }

protected:
    virtual void run(TCPServer& server) override {
        // Process disconnected clients
        int i = 0;
        while (i < conns_.size()) {
            if (!conns_.at(i).client.connected()) {
                Connection conn = conns_.takeAt(i);
                conn.client.stop();
                log.trace("%s: Client disconnected (%d)", this->id(), conn.id);
                disconnected(conn.id);
            } else {
                ++i;
            }
        }
        // Process readable clients
        for (Connection& conn: conns_) {
            if (conn.client.available() > 0) {
                readable(conn.client, conn.id);
            }
        }
        // Process next pending connection
        TCPClient client = server.available();
        if (client) {
            const int id = nextConnId_++;
            log.trace("%s: Client connected: %s (%d)", this->id(), (const char*)client.remoteIP().toString(), id);
            conns_.append(Connection(std::move(client), id));
            connected(conns_.last().client, id);
        }
    }

    virtual void connected(TCPClient& client, int id) {
    }

    virtual void disconnected(int id) {
    }

    virtual void readable(TCPClient& client, int id) {
        while (client.available() > 0) {
            client.read();
        }
    }

private:
    struct Connection {
        TCPClient client;
        int id;

        Connection(TCPClient client, int id) :
                client(std::move(client)),
                id(id) {
        }
    };

    Vector<Connection> conns_;
    int nextConnId_;
};

REGISTER_SERVER(SimpleServer);

class EchoServer: public SimpleServer {
public:
    using SimpleServer::SimpleServer;

protected:
    virtual void readable(TCPClient& client, int id) override {
        int n = 0;
        while ((n = client.available()) > 0) {
            n = std::min(n, (int)sizeof(buf_));
            CHECK((int)client.read((uint8_t*)buf_, n) == n);
            CHECK((int)client.write((uint8_t*)buf_, n) == n);
        }
    }

private:
    char buf_[128];
};

REGISTER_SERVER(EchoServer);

// Test-specific servers (see client/tests.js)
class TcpClientClosesOnDestructionTestServer: public Server {
public:
    using Server::Server;

protected:
    virtual void run(TCPServer& server) override {
        // Accept next connection and give up returned TCPClient instance
        server.available();
    }
};

REGISTER_SERVER(TcpClientClosesOnDestructionTestServer);

const char* fileName(const char *path) {
    const char *s = strrchr(path, '/');
    if (s) {
        return s + 1;
    }
    return path;
}

} // namespace

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    // Parse JSON request
    JSONString cmd, id, type;
    unsigned port = 0;
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        const JSONString name = it.name();
        if (name == "cmd") {
            cmd = it.value().toString();
        } else if (name == "id") {
            id = it.value().toString();
        } else if (name == "type") {
            type = it.value().toString();
        } else if (name == "port") {
            port = it.value().toInt();
        }
    }
    // Process request
    ServerManager* serverManager = ServerManager::instance();
    NetworkManager* networkManager = NetworkManager::instance();
    Error error(Error::NONE);
    JSONBufferWriter json(buf, bufSize); // Reply JSON data
    json.beginObject();
    if (cmd == "startServer") {
        if (!serverManager->startServer((const char*)id, (const char*)type, port)) {
            error = serverManager->error();
        }
    } else if (cmd == "stopServer") {
        if (!serverManager->stopServer((const char*)id)) {
            error = serverManager->error();
        }
    } else if (cmd == "stopServers") { // Stop all servers
        if (!serverManager->stopServers()) {
            error = serverManager->error();
        }
    } else if (cmd == "connectToNetwork") {
        if (networkManager->connect()) {
            json.name("address").value(networkManager->address().toString());
        } else {
            error = networkManager->error();
        }
    } else if (cmd == "disconnectFromNetwork") {
        networkManager->disconnect();
    } else if (cmd == "getNetworkStatus") {
        json.name("connected").value(networkManager->isConnected());
        if (networkManager->isConnected()) {
            json.name("address").value(networkManager->address().toString());
        }
    } else if (cmd == "getFreeMemory") {
        json.name("bytes").value((unsigned)System.freeMemory());
    }
    if (error) {
        json.name("error").value(error.message());
    }
    json.endObject();
    *repSize = json.dataSize();
    return true;
}

void setup() {
    WiFi.on();
}

void loop() {
    ServerManager::instance()->run();
}
