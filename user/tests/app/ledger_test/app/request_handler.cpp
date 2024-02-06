#include <algorithm>
#include <memory>

#include <spark_wiring_cloud.h>
#include <spark_wiring_network.h>
#include <spark_wiring_system.h>
#include <spark_wiring_json.h>
#include <spark_wiring_logging.h>
#include <spark_wiring_error.h>

#include <delay_hal.h>

#include <endian_util.h>
#include <scope_guard.h>
#include <check.h>

#include "request_handler.h"
#include "logger.h"
#include "config.h"

namespace particle::test {

namespace {

const size_t LEDGER_READ_BLOCK_SIZE = 1024;

enum Result {
    RESET_PENDING = 1
};

enum BinaryRequestType {
    READ = 1,
    WRITE = 2
};

struct LedgerAppData {
};

// Completion handler for system_ctrl_set_result()
void systemResetCompletionHandler(int result, void* data) {
    HAL_Delay_Milliseconds(1000);
    System.reset();
}

void destroyLedgerAppData(void* appData) {
    auto d = static_cast<LedgerAppData*>(appData);
    delete d;
}

int getLedgerInfo(ledger_info& info, ledger_instance* ledger) {
    int r = ledger_get_info(ledger, &info, nullptr);
    if (r < 0) {
        Log.error("ledger_get_info() failed: %d", r);
    }
    return r;
}

int getLedgerNames(char**& names, size_t& count) {
    int r = ledger_get_names(&names, &count, nullptr);
    if (r < 0) {
        Log.error("ledger_get_names() failed: %d", r);
    }
    return r;
}

int openLedgerStream(ledger_stream*& stream, ledger_instance* ledger, int mode) {
    int r = ledger_open(&stream, ledger, mode, nullptr);
    if (r < 0) {
        Log.error("ledger_open() failed: %d", r);
    }
    return r;
}

int closeLedgerStream(ledger_stream* stream, int flags = 0) {
    int r = ledger_close(stream, flags, nullptr);
    if (r < 0) {
        Log.error("ledger_close() failed: %d", r);
    }
    return r;
}

int readLedgerStream(ledger_stream* stream, char* data, size_t size) {
    int r = ledger_read(stream, data, size, nullptr);
    if (r < 0) {
        Log.error("ledger_read() failed: %d", r);
    }
    return r;
}

int writeLedgerStream(ledger_stream* stream, const char* data, size_t size) {
    int r = ledger_write(stream, data, size, nullptr);
    if (r < 0) {
        Log.error("ledger_write() failed: %d", r);
    }
    return r;
}

void ledgerSyncCallback(ledger_instance* ledger, void* appData) {
    ledger_info info = {};
    info.version = LEDGER_API_VERSION;
    int r = getLedgerInfo(info, ledger);
    if (r < 0) {
        return;
    }
    Log.info("Ledger synchronized: %s", info.name);
}

int getLedger(ledger_instance*& ledger, const char* name) {
    ledger_instance* lr = nullptr;
    int r = ledger_get_instance(&lr, name, nullptr);
    if (r < 0) {
        Log.error("ledger_get_instance() failed: %d", r);
        return r;
    }
    ledger_lock(lr, nullptr);
    SCOPE_GUARD({
        ledger_unlock(lr, nullptr);
    });
    auto appData = static_cast<LedgerAppData*>(ledger_get_app_data(lr, nullptr));
    if (!appData) {
        appData = new(std::nothrow) LedgerAppData();
        if (!appData) {
            return Error::NO_MEMORY;
        }
        ledger_callbacks callbacks = {};
        callbacks.version = LEDGER_API_VERSION;
        callbacks.sync = ledgerSyncCallback;
        ledger_set_callbacks(lr, &callbacks, nullptr);
        ledger_set_app_data(lr, appData, destroyLedgerAppData, nullptr);
        ledger_add_ref(lr, nullptr); // Keep the instance around
    }
    ledger = lr;
    return 0;
}

} // namespace

class RequestHandler::JsonRequest {
public:
    JsonRequest() :
            req_(nullptr) {
    }

    int init(ctrl_request* req) {
        auto d = JSONValue::parse(req->request_data, req->request_size);
        if (!d.isObject()) {
            return Error::BAD_DATA;
        }
        data_ = std::move(d);
        req_ = req;
        return 0;
    }

    template<typename F>
    int response(F fn) {
        if (!req_) {
            return Error::INVALID_STATE;
        }
        JSONBufferWriter writer(nullptr, 0);
        fn(writer);
        size_t size = writer.dataSize();
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        writer = JSONBufferWriter(req_->reply_data, req_->reply_size);
        fn(writer);
        if (writer.dataSize() != size) {
            return Error::INTERNAL;
        }
        return 0;
    }

    JSONValue get(const char* name) const {
        JSONObjectIterator it(data_);
        while (it.next()) {
            if (it.name() == name) {
                return it.value();
            }
        }
        return JSONValue();
    }

    bool has(const char* name) const {
        return get(name).isValid();
    }

private:
    JSONValue data_;
    ctrl_request* req_;
};

class RequestHandler::BinaryRequest {
public:
    BinaryRequest() :
            req_(nullptr),
            data_(nullptr),
            size_(0),
            type_(0) {
    }

    int init(ctrl_request* req) {
        if (req->request_size < 4) {
            return Error::BAD_DATA;
        }
        uint32_t type = 0;
        std::memcpy(&type, req->request_data, 4);
        type_ = bigEndianToNative(type);
        data_ = req->request_data + 4;
        size_ = req->request_size - 4;
        req_ = req;
        return 0;
    }

    int allocResponse(char*& data, size_t size) {
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        data = req_->reply_data;
        return 0;
    }

    void responseSize(size_t size) {
        req_->reply_size = size;
    }

    const char* data() const {
        return data_;
    }

    size_t size() const {
        return size_;
    }

    int type() const {
        return type_;
    }

private:
    ctrl_request* req_;
    const char* data_;
    size_t size_;
    int type_;
};

RequestHandler::~RequestHandler() {
    if (ledgerStream_) {
        closeLedgerStream(ledgerStream_, LEDGER_STREAM_CLOSE_DISCARD);
    }
}

void RequestHandler::handleRequest(ctrl_request* req) {
    int r = handleRequestImpl(req);
    if (r < 0) {
        Log.error("Error while handling control request: %d", r);
        system_ctrl_alloc_reply_data(req, 0 /* size */, nullptr /* reserved */);
        system_ctrl_set_result(req, r, nullptr /* handler */, nullptr /* data */, nullptr /* reserved */);
    }
}

int RequestHandler::handleRequestImpl(ctrl_request* req) {
    Log.trace("Received request");
    auto size = req->request_size;
    if (!size) {
        return Error::NOT_ENOUGH_DATA;
    }
    int result = 0;
    auto data = req->request_data;
    if (data[0] == '{') {
        Log.write(LOG_LEVEL_TRACE, data, size);
        Log.print(LOG_LEVEL_TRACE, "\r\n");
        JsonRequest jsonReq;
        CHECK(jsonReq.init(req));
        result = CHECK(handleJsonRequest(jsonReq));
        Log.trace("Sending response");
        if (req->reply_size > 0) {
            Log.write(LOG_LEVEL_TRACE, req->reply_data, req->reply_size);
            Log.print(LOG_LEVEL_TRACE, "\r\n");
        }
    } else {
        if (size > 0) {
            Log.dump(LOG_LEVEL_TRACE, data, size);
            Log.printf(LOG_LEVEL_TRACE, " (%u bytes)\r\n", (unsigned)size);
        }
        BinaryRequest binReq;
        CHECK(binReq.init(req));
        result = CHECK(handleBinaryRequest(binReq));
        Log.trace("Sending response");
        if (req->reply_size > 0) {
            Log.dump(LOG_LEVEL_TRACE, req->reply_data, req->reply_size);
            Log.printf(LOG_LEVEL_TRACE, " (%u bytes)\r\n", (unsigned)req->reply_size);
        }
    }
    auto handler = (result == Result::RESET_PENDING) ? systemResetCompletionHandler : nullptr;
    system_ctrl_set_result(req, result, handler, nullptr /* data */, nullptr /* reserved */);
    return 0;
}

int RequestHandler::handleJsonRequest(JsonRequest& req) {
    auto cmd = req.get("cmd").toString();
    if (cmd == "get") {
        return get(req);
    } else if (cmd == "set") {
        return set(req);
    } else if (cmd == "touch") {
        return touch(req);
    } else if (cmd == "list") {
        return list(req);
    } else if (cmd == "info") {
        return info(req);
    } else if (cmd == "reset") {
        return reset(req);
    } else if (cmd == "remove") {
        return remove(req);
    } else if (cmd == "connect") {
        return connect(req);
    } else if (cmd == "disconnect") {
        return disconnect(req);
    } else if (cmd == "auto_connect") {
        return autoConnect(req);
    } else if (cmd == "debug") {
        return debug(req);
    } else {
        Log.error("Unknown command: \"%s\"", cmd.data());
        return Error::NOT_SUPPORTED;
    }
}

int RequestHandler::handleBinaryRequest(BinaryRequest& req) {
    switch (req.type()) {
    case BinaryRequestType::READ:
        return read(req);
    case BinaryRequestType::WRITE:
        return write(req);
    default:
        Log.error("Unknown command: %d", req.type());
        return Error::NOT_SUPPORTED;
    }
}

int RequestHandler::get(JsonRequest& req) {
    auto name = req.get("name").toString();
    Log.info("Getting ledger data: %s", name.data());
    if (ledgerStream_) {
        Log.warn("\"get\" or \"set\" command is already in progress");
        CHECK(closeLedgerStream(ledgerStream_, LEDGER_STREAM_CLOSE_DISCARD));
        ledgerStream_ = nullptr;
    }
    ledger_instance* ledger = nullptr;
    CHECK(getLedger(ledger, name.data()));
    ledger_lock(ledger, nullptr);
    SCOPE_GUARD({
        ledger_unlock(ledger, nullptr);
        ledger_release(ledger, nullptr);
    });
    ledger_info info = {};
    info.version = LEDGER_API_VERSION;
    CHECK(getLedgerInfo(info, ledger));
    ledger_stream* stream = nullptr;
    CHECK(openLedgerStream(stream, ledger, LEDGER_STREAM_MODE_READ));
    NAMED_SCOPE_GUARD(closeStreamGuard, {
        closeLedgerStream(stream, LEDGER_STREAM_CLOSE_DISCARD);
    });
    CHECK(req.response([&](auto& w) {
        w.beginObject();
        w.name("size").value(info.data_size);
        w.endObject();
    }));
    if (info.data_size > 0) {
        ledgerStream_ = stream;
        ledgerBytesLeft_ = info.data_size;
        closeStreamGuard.dismiss();
    }
    return 0;
}

int RequestHandler::set(JsonRequest& req) {
    auto name = req.get("name").toString();
    Log.info("Setting ledger data: %s", name.data());
    if (ledgerStream_) {
        Log.warn("\"get\" or \"set\" command is already in progress");
        CHECK(closeLedgerStream(ledgerStream_, LEDGER_STREAM_CLOSE_DISCARD));
        ledgerStream_ = nullptr;
    }
    auto size = req.get("size").toInt();
    if (size < 0) {
        Log.error("Invalid size of ledger data");
        return Error::BAD_DATA;
    }
    ledger_instance* ledger = nullptr;
    CHECK(getLedger(ledger, name.data()));
    SCOPE_GUARD({
        ledger_release(ledger, nullptr);
    });
    ledger_stream* stream = nullptr;
    CHECK(openLedgerStream(stream, ledger, LEDGER_STREAM_MODE_WRITE));
    if (size > 0) {
        ledgerStream_ = stream;
        ledgerBytesLeft_ = size;
    } else {
        CHECK(closeLedgerStream(stream));
    }
    return 0;
}

int RequestHandler::touch(JsonRequest& req) {
    auto name = req.get("name").toString();
    Log.info("Getting ledger instance: %s", name.data());
    ledger_instance* ledger = nullptr;
    CHECK(getLedger(ledger, name.data()));
    ledger_release(ledger, nullptr);
    return 0;
}

int RequestHandler::list(JsonRequest& req) {
    Log.info("Enumerating ledgers");
    char** names = nullptr;
    size_t count = 0;
    CHECK(getLedgerNames(names, count));
    SCOPE_GUARD({
        for (size_t i = 0; i < count; ++i) {
            std::free(names[i]);
        }
        std::free(names);
    });
    CHECK(req.response([&](auto& w) {
        w.beginArray();
        for (size_t i = 0; i < count; ++i) {
            w.value(names[i]);
        }
        w.endArray();
    }));
    return 0;
}

int RequestHandler::info(JsonRequest& req) {
    auto name = req.get("name").toString();
    Log.info("Getting ledger info: %s", name.data());
    ledger_instance* ledger = nullptr;
    CHECK(getLedger(ledger, name.data()));
    SCOPE_GUARD({
        ledger_release(ledger, nullptr);
    });
    ledger_info info = {};
    info.version = LEDGER_API_VERSION;
    CHECK(getLedgerInfo(info, ledger));
    CHECK(req.response([&](auto& w) {
        w.beginObject();
        w.name("last_updated").value(info.last_updated);
        w.name("last_synced").value(info.last_synced);
        w.name("data_size").value(info.data_size);
        w.name("scope").value(info.scope);
        w.name("sync_direction").value(info.sync_direction);
        w.endObject();
    }));
    return 0;
}

int RequestHandler::reset(JsonRequest& req) {
    Log.info("Resetting device");
    return Result::RESET_PENDING;
}

int RequestHandler::remove(JsonRequest& req) {
    auto& conf = Config::get();
    if (conf.removeLedger || conf.removeAllLedgers) {
        Log.warn("\"remove\" command is already in progress");
    }
    auto removeAll = req.get("all").toBool();
    if (removeAll) {
        conf.removeAllLedgers = true;
    } else {
        auto name = req.get("name").toString();
        if (name.isEmpty()) {
            Log.error("Ledger name is missing");
            return Error::BAD_DATA;
        }
        size_t n = strlcpy(conf.removeLedgerName, name.data(), sizeof(conf.removeLedgerName));
        if (n >= sizeof(conf.removeLedgerName)) {
            Log.error("Ledger name is too long");
            return Error::BAD_DATA;
        }
        conf.removeLedger = true;
    }
    conf.setRestoreConnectionFlag();
    return Result::RESET_PENDING;
}

int RequestHandler::connect(JsonRequest& req) {
    Particle.connect();
    return 0;
}

int RequestHandler::disconnect(JsonRequest& req) {
    Network.off();
    return 0;
}

int RequestHandler::autoConnect(JsonRequest& req) {
    auto enabled = req.get("enabled");
    auto& conf = Config::get();
    conf.autoConnect = enabled.isValid() ? enabled.toBool() : true;
    if (conf.autoConnect) {
        Log.info("Enabled auto-connect");
        Particle.connect();
    } else {
        Log.info("Disabled auto-connect");
    }
    return 0;
}

int RequestHandler::debug(JsonRequest& req) {
    auto enabled = req.get("enabled");
    auto& conf = Config::get();
    conf.debugEnabled = enabled.isValid() ? enabled.toBool() : true;
    if (conf.debugEnabled) {
        Log.info("Enabled debug");
    } else {
        Log.info("Disabled debug");
    }
    CHECK(initLogger());
    return 0;
}

int RequestHandler::read(BinaryRequest& req) {
    int r = readImpl(req);
    if (r < 0 && ledgerStream_) {
        closeLedgerStream(ledgerStream_, LEDGER_STREAM_CLOSE_DISCARD);
        ledgerStream_ = nullptr;
    }
    return r;
}

int RequestHandler::readImpl(BinaryRequest& req) {
    if (!ledgerStream_) {
        Log.error("Ledger is not open");
        return Error::INVALID_STATE;
    }
    char* data = nullptr;
    CHECK(req.allocResponse(data, LEDGER_READ_BLOCK_SIZE));
    size_t n = std::min(ledgerBytesLeft_, LEDGER_READ_BLOCK_SIZE);
    CHECK(readLedgerStream(ledgerStream_, data, n));
    req.responseSize(n);
    ledgerBytesLeft_ -= n;
    if (!ledgerBytesLeft_) {
        closeLedgerStream(ledgerStream_); // Don't use CHECK here
        ledgerStream_ = nullptr;
    }
    return 0;
}

int RequestHandler::write(BinaryRequest& req) {
    int r = writeImpl(req);
    if (r < 0 && ledgerStream_) {
        closeLedgerStream(ledgerStream_, LEDGER_STREAM_CLOSE_DISCARD);
        ledgerStream_ = nullptr;
    }
    return r;
}

int RequestHandler::writeImpl(BinaryRequest& req) {
    if (!ledgerStream_) {
        Log.error("Ledger is not open");
        return Error::INVALID_STATE;
    }
    if (req.size() > ledgerBytesLeft_) {
        Log.error("Unexpected size of ledger data");
        return Error::TOO_LARGE;
    }
    CHECK(writeLedgerStream(ledgerStream_, req.data(), req.size()));
    ledgerBytesLeft_ -= req.size();
    if (!ledgerBytesLeft_) {
        closeLedgerStream(ledgerStream_); // Don't use CHECK here
        ledgerStream_ = nullptr;
    }
    return 0;
}

} // namespace particle::test
