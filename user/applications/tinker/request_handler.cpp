#include "request_handler.h"
#include "spark_wiring_logging.h"

#include "delay_hal.h"
#include "check.h"

#include "src/fqc_test.h"

namespace particle {

namespace {

using namespace spark; // For JSON classes

JSONValue getValue(const JSONValue& obj, const char* name) {
    JSONObjectIterator it(obj);
    while (it.next()) {
        if (it.name() == name) {
            return it.value();
        }
    }
    return JSONValue();
}

} // namespace

// Wrapper for a control request handle
class RequestHandler::Request {
public:
    Request() :
            req_(nullptr) {
    }

    ~Request() {
        destroy();
    }

    int init(ctrl_request* req) {
        if (req->request_size > 0) {
            // Parse request
            auto d = JSONValue::parse(req->request_data, req->request_size);
            CHECK_TRUE(d.isObject(), SYSTEM_ERROR_BAD_DATA);
            data_ = std::move(d);
        }
        req_ = req;
        return 0;
    }

    void destroy() {
        data_ = JSONValue();
        if (req_) {
            // Having a pending request at this point is an internal error
            system_ctrl_set_result(req_, SYSTEM_ERROR_INTERNAL, nullptr, nullptr, nullptr);
            req_ = nullptr;
        }
    }

    template<typename EncodeFn>
    int reply(EncodeFn fn) {
        CHECK_TRUE(req_, SYSTEM_ERROR_INVALID_STATE);
        // Calculate the size of the reply data
        JSONBufferWriter writer(nullptr, 0);
        fn(writer);
        const size_t size = writer.dataSize();
        CHECK_TRUE(size > 0, SYSTEM_ERROR_INTERNAL);
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        // Serialize the reply
        writer = JSONBufferWriter(req_->reply_data, req_->reply_size);
        fn(writer);
        CHECK_TRUE(writer.dataSize() == size, SYSTEM_ERROR_INTERNAL);
        return 0;
    }

    int reply(char * buffer, int buffer_size){
        CHECK(system_ctrl_alloc_reply_data(req_, buffer_size, nullptr));
        memcpy(req_->reply_data, buffer, req_->reply_size);
        return 0;
    }

    void done(int result, ctrl_completion_handler_fn fn = nullptr, void* data = nullptr) {
        if (req_) {
            system_ctrl_set_result(req_, result, fn, data, nullptr);
            req_ = nullptr;
            destroy();
        }
    }

    JSONValue get(const char* name) const {
        return getValue(data_, name);
    }

    bool has(const char* name) const {
        return getValue(data_, name).isValid();
    }

    const JSONValue& data() const {
        return data_;
    }

    bool isEmpty() const {
        return !data_.isValid();
    }

private:
    JSONValue data_;
    ctrl_request* req_;
};

RequestHandler::RequestHandler() :
        inited_(false){
}

RequestHandler::~RequestHandler() {
    destroy();
}

int RequestHandler::init() {
    inited_ = true;
    return 0;
}

void RequestHandler::destroy() {
    inited_ = false;
}

void RequestHandler::process(ctrl_request* ctrlReq) {
    const int r = request(ctrlReq);
    if (r < 0) {
        system_ctrl_set_result(ctrlReq, r, nullptr, nullptr, nullptr);
    }
}

int RequestHandler::request(ctrl_request* ctrlReq) {
    //CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    Request req;
    CHECK(req.init(ctrlReq));
    const int r = CHECK(request(&req));
    req.done(r);
    return 0;
}

int RequestHandler::request(Request* req) {
    auto FqcTester = FqcTest::instance();

    // Check for FQC commands
    if(FqcTester->process(req->data()))
    {
        req->reply(FqcTester->reply(), FqcTester->replySize());
        return SYSTEM_ERROR_NONE;
    }
    else if (req->isEmpty()) { // Ping request
        return SYSTEM_ERROR_NONE;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
}

RequestHandler* RequestHandler::instance() {
    static RequestHandler handler;
    return &handler;
}

} // namespace particle

