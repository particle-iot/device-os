#include <utility>
#include <cstring>
#include <iostream> // FIXME

#include <spark_wiring_logging.h>
#include <spark_wiring_error.h>

#include <check.h>

#include "protocol.h"
#include "frame_codec.h"

namespace particle::constrained {

namespace {

const unsigned MIN_LORAWAN_APP_PORT = 1;
const unsigned MAX_LORAWAN_APP_PORT = 223;

} // namespace

struct Protocol::InRequest {
};

struct Protocol::OutRequest: RefCount {
    Buffer data; // Payload data
    ProtocolBase::ResponseReceivedFn responseReceivedFn; // Callback to invoke when a response is received
    RequestOptions options; // Request options
    unsigned id; // Request ID
    unsigned type; // Request type
};

Protocol::Protocol() :
        state_(State::NEW),
        maxPayloadSize_(100), // TODO
        nextOutReqId_(0) {
}

Protocol::~Protocol() {
    if (state_ == State::CONNECTED) {
        Log.warn("Destroying message channel while it's connected");
    }
}

int Protocol::init(ProtocolConfig conf) {
    if (state_ != State::NEW) {
        return Error::INVALID_STATE;
    }
    if (!conf.sendMessage() || conf.port() < MIN_LORAWAN_APP_PORT || conf.port() > MAX_LORAWAN_APP_PORT) {
        return Error::INVALID_ARGUMENT;
    }
    conf_ = std::move(conf);
    state_ = State::DISCONNECTED;
    return 0;
}

int Protocol::connect() {
    if (state_ == State::NEW) {
        return Error::INVALID_STATE;
    }
    if (state_ == State::CONNECTED) {
        return 0;
    }
    state_ = State::CONNECTED;
    if (conf_.connected()) {
        conf_.connected()();
    }
    return 0;
}

void Protocol::disconnect() {
    if (state_ != State::CONNECTED) {
        return;
    }
    decltype(outReqs_) reqs;
    using std::swap;
    swap(reqs, outReqs_);
    state_ = State::DISCONNECTED;
    for (auto& [id, req]: reqs) {
        if (req->responseReceivedFn && !req->options.noResponse()) {
            req->responseReceivedFn(Error::CANCELLED, 0, Buffer());
        }
    }
}

int Protocol::receiveMessage(int port, Buffer data) {
    FrameHeader header;
    size_t headerSize = CHECK(decodeFrameHeader(data.data(), data.size(), header));

    if (header.frameType() != FrameType::RESPONSE) {
        return 0; // TODO
    }
    auto it = outReqs_.find(header.requestId());
    if (it == outReqs_.end()) {
        return 0;
    }
    auto req = std::move(it->second);
    outReqs_.erase(it);

    if (req->responseReceivedFn) {
        std::memmove(data.data(), data.data() + headerSize, data.size() - headerSize);
        CHECK(data.resize(data.size() - headerSize));
        req->responseReceivedFn(0 /* error */, header.requestTypeOrResultCode(), std::move(data));
    }

    return 0;
}

int Protocol::updateMaxPayloadSize(size_t size) {
    return Error::NOT_SUPPORTED; // TODO
}

int Protocol::run() {
    if (state_ != State::CONNECTED) {
        return 0;
    }
    // TODO
    return 0;
}

int Protocol::sendRequest(unsigned type, Buffer data, ResponseReceivedFn respFn, RequestOptions opts) {
    if (state_ != State::CONNECTED) {
        return Error::INVALID_STATE;
    }

    auto req = makeRefCountPtr<OutRequest>();
    if (!req) {
        return Error::NO_MEMORY;
    }
    req->id = nextOutReqId_;
    if (++nextOutReqId_ > MAX_REQUEST_ID) {
        nextOutReqId_ = 0;
    }
    req->type = type;
    req->data = std::move(data);
    req->responseReceivedFn = std::move(respFn);
    req->options = std::move(opts);

    FrameHeader header;
    header.frameType(FrameType::REQUEST);
    header.requestTypeOrResultCode(req->type);
    header.requestId(req->id);
    char headerData[MAX_FRAME_HEADER_SIZE] = {};
    size_t headerSize = CHECK(encodeFrameHeader(headerData, sizeof(headerData), header));

    Buffer buf;
    CHECK(buf.resize(headerSize + req->data.size()));
    std::memcpy(buf.data(), headerData, headerSize);
    std::memcpy(buf.data() + headerSize, req->data.data(), req->data.size());

    assert(conf_.sendMessage());
    CHECK(conf_.sendMessage()(buf, conf_.port(), AckReceivedFn()));

    if (!outReqs_.set(req->id, std::move(req))) {
        return Error::NO_MEMORY;
    }

    return 0;
}

} // namespace particle::constrained
