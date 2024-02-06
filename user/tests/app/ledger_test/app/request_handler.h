#pragma once

#include <system_ledger.h>
#include <system_control.h>

namespace particle::test {

class RequestHandler {
public:
    RequestHandler() :
            ledgerStream_(nullptr),
            ledgerBytesLeft_(0) {
    }

    ~RequestHandler();

    void handleRequest(ctrl_request* req);

private:
    class JsonRequest;
    class BinaryRequest;

    ledger_stream* ledgerStream_;
    size_t ledgerBytesLeft_;

    int handleRequestImpl(ctrl_request* req);
    int handleJsonRequest(JsonRequest& req);
    int handleBinaryRequest(BinaryRequest& req);

    int get(JsonRequest& req);
    int set(JsonRequest& req);
    int touch(JsonRequest& req);
    int list(JsonRequest& req);
    int info(JsonRequest& req);
    int reset(JsonRequest& req);
    int remove(JsonRequest& req);
    int connect(JsonRequest& req);
    int disconnect(JsonRequest& req);
    int autoConnect(JsonRequest& req);
    int debug(JsonRequest& req);
    
    int read(BinaryRequest& req);
    int readImpl(BinaryRequest& req);
    int write(BinaryRequest& req);
    int writeImpl(BinaryRequest& req);
};

} // namespace particle::test
