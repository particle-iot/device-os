#include "usb_control_request_channel.h"
#include "active_object.h"

#include "mocks/alloc.h"
#include "tools/random.h"
#include "tools/catch.h"

#include "hippomocks.h"

#include <boost/optional.hpp>

#include <set>
#include <list>

ISRTaskQueue SystemISRTaskQueue;

namespace {

using namespace particle;
using namespace test;

// Minimum length of the data stage supported by high-speed USB devices
const size_t MIN_WLENGTH = 64;

inline bool processNextTask() {
    return SystemISRTaskQueue.process();
}

inline bool processAllTasks() {
    bool ok = false;
    while (processNextTask()) {
        ok = true;
    }
    return ok;
}

class Channel;

class ServiceRequest {
public:
    enum ServiceType {
        INIT = 1,
        CHECK = 2,
        SEND = 3,
        RECV = 4,
        RESET = 5
    };

    ServiceRequest& id(uint16_t id) {
        id_ = id;
        return *this;
    }

    ServiceRequest& type(uint16_t type) { // Sets application-specific request type
        type_ = type;
        return *this;
    }

    ServiceRequest& size(size_t size) {
        size_ = size;
        data_.clear();
        return *this;
    }

    ServiceRequest& data(std::string data) {
        data_ = std::move(data);
        size_ = data_.size();
        return *this;
    }

    ServiceType serviceType() const {
        return serviceType_;
    }

    bool send();

private:
    Channel* channel_;
    std::string data_;
    ServiceType serviceType_;
    uint16_t id_, type_, size_;

    // Use Channel::serviceRequest() to construct instances of this class
    ServiceRequest(ServiceType type, Channel* channel) :
            channel_(channel),
            serviceType_(type),
            id_(USB_REQUEST_INVALID_ID),
            type_(0),
            size_(0) {
    }

    friend class Channel;
};

class ServiceReply {
public:
    enum Status {
        OK = 0,
        ERROR = 1,
        PENDING = 2,
        BUSY = 3,
        NO_MEMORY = 4,
        NOT_FOUND = 5
    };

    ServiceReply() { // Constructs an invalid reply
    }

    explicit ServiceReply(std::string data) :
            data_(std::move(data)) {
    }

    Status status() const {
        REQUIRE(status_);
        return *status_;
    }

    bool hasStatus() const {
        return (bool)status_;
    }

    uint16_t id() const {
        REQUIRE(id_);
        return *id_;
    }

    bool hasId() const {
        return (bool)id_;
    }

    uint32_t size() const {
        REQUIRE(size_);
        return *size_;
    }

    bool hasSize() const {
        return (bool)size_;
    }

    int32_t result() const {
        REQUIRE(result_);
        return *result_;
    }

    bool hasResult() const {
        return (bool)result_;
    }

    const std::string& data() const {
        REQUIRE(data_);
        return *data_;
    }

    bool hasData() const {
        return (bool)data_;
    }

    explicit operator bool() const {
        return (status_ || size_ || result_ || id_ || data_);
    }

    static ServiceReply parse(const std::string& data) {
        ServiceReply rep;
        Buffer buf(data);
        // Field flags (4 bytes)
        auto flags = buf.readLe<uint32_t>();
        // Status code (2 bytes)
        REQUIRE((flags & FieldFlag::STATUS)); // Mandatory field
        const auto status = buf.readLe<uint16_t>();
        REQUIRE(status <= 5);
        rep.status_ = (Status)status;
        flags &= ~FieldFlag::STATUS;
        // Request ID (2 bytes, optional)
        if (flags & FieldFlag::ID) {
            rep.id_ = buf.readLe<uint16_t>();
            flags &= ~FieldFlag::ID;
        }
        // Payload size (4 bytes, optional)
        if (flags & FieldFlag::SIZE) {
            rep.size_ = buf.readLe<uint32_t>();
            flags &= ~FieldFlag::SIZE;
        }
        // Result code (4 bytes, optional)
        if (flags & FieldFlag::RESULT) {
            rep.result_ = buf.readLe<int32_t>();
            flags &= ~FieldFlag::RESULT;
        }
        REQUIRE(flags == 0);
        REQUIRE(buf.readPos() == buf.size());
        return rep;
    }

private:
    enum FieldFlag {
        STATUS = 0x01,
        ID = 0x02,
        SIZE = 0x04,
        RESULT = 0x08
    };

    boost::optional<std::string> data_;
    boost::optional<Status> status_;
    boost::optional<uint32_t> size_;
    boost::optional<int32_t> result_;
    boost::optional<uint16_t> id_;
};

// Wrapper over UsbControlRequestChannel mocking necessary HAL and system functions
class Channel: public ControlRequestHandler {
public:
    typedef std::function<void(ctrl_request*, ControlRequestChannel*)> RequestHandlerFunc;

    Channel() :
            heapAlloc_(&mocks_),
            poolAlloc_(&mocks_),
            reqHandlerCalled_(false),
            halReqCallback_(nullptr),
            halStateCallback_(nullptr),
            halReqCallbackData_(nullptr),
            halStateCallbackData_(nullptr) {
        mocks_.OnCallFunc(HAL_USB_Set_Vendor_Request_Callback).Do([this](HAL_USB_Vendor_Request_Callback callback, void* data) {
            this->halReqCallback_ = callback;
            this->halReqCallbackData_ = data;
        });
        mocks_.OnCallFunc(HAL_USB_Set_Vendor_Request_State_Callback).Do([this](HAL_USB_Vendor_Request_State_Callback callback, void* data) {
            this->halStateCallback_ = callback;
            this->halStateCallbackData_ = data;
        });
        reset();
    }

    ~Channel() {
        processAllTasks();
        channel_.reset();
    }

    ServiceRequest serviceRequest(ServiceRequest::ServiceType type) {
        return ServiceRequest(type, this);
    }

    const ServiceReply& serviceReply() const {
        return serviceRep_;
    }

    Channel& requestHandler(RequestHandlerFunc handler) {
        reqHandler_ = std::move(handler);
        reqHandlerCalled_ = false;
        return *this;
    }

    bool requestHandlerCalled() const {
        return reqHandlerCalled_;
    }

    HeapAllocator& heapAllocator() {
        return heapAlloc_;
    }

    PoolAllocator& poolAllocator() {
        return poolAlloc_;
    }

    void reset() {
        processAllTasks();
        channel_.reset();
        heapAlloc_.reset();
        poolAlloc_.reset();
        serviceRep_ = ServiceReply();
        reqHandlerCalled_ = false;
        channel_.reset(new UsbControlRequestChannel(this));
    }

    void checkMemory() {
        heapAlloc_.check();
        poolAlloc_.check();
    }

    // ControlRequestHandler
    virtual void processRequest(ctrl_request* req, ControlRequestChannel* channel) override {
        if (reqHandler_) {
            reqHandler_(req, channel);
        }
        reqHandlerCalled_ = true;
    }

private:
    // Values of the `bmRequestType` field of the USB setup packet
    enum UsbRequestType {
        HOST_TO_DEVICE = 0x40, // 01000000b (direction: host-to-device; type: vendor; recipient: device)
        DEVICE_TO_HOST = 0xc0 // 11000000b (direction: device-to-host; type: vendor; recipient: device)
    };

    MockRepository mocks_;
    HeapAllocator heapAlloc_;
    PoolAllocator poolAlloc_;
    ServiceReply serviceRep_;
    RequestHandlerFunc reqHandler_;
    bool reqHandlerCalled_;

    HAL_USB_Vendor_Request_Callback halReqCallback_;
    HAL_USB_Vendor_Request_State_Callback halStateCallback_;
    void* halReqCallbackData_;
    void* halStateCallbackData_;

    std::unique_ptr<UsbControlRequestChannel> channel_;

    bool sendServiceRequest(const ServiceRequest& req) {
        HAL_USB_SetupRequest halReq;
        memset(&halReq, 0, sizeof(halReq));
        halReq.bRequest = req.serviceType_;
        switch (req.serviceType_) {
        case ServiceRequest::INIT:
            halReq.bmRequestType = UsbRequestType::DEVICE_TO_HOST;
            halReq.wIndex = req.type_;
            halReq.wValue = req.size_;
            halReq.wLength = MIN_WLENGTH;
            break;
        case ServiceRequest::CHECK:
            halReq.bmRequestType = UsbRequestType::DEVICE_TO_HOST;
            halReq.wIndex = req.id_;
            halReq.wLength = MIN_WLENGTH;
            break;
        case ServiceRequest::SEND:
            // SEND is the only host-to-device service request defined by the protocol
            halReq.bmRequestType = UsbRequestType::HOST_TO_DEVICE;
            halReq.wIndex = req.id_;
            halReq.wLength = req.size_;
            break;
        case ServiceRequest::RECV:
            halReq.bmRequestType = UsbRequestType::DEVICE_TO_HOST;
            halReq.wIndex = req.id_;
            halReq.wLength = req.size_;
            break;
        case ServiceRequest::RESET:
            halReq.bmRequestType = UsbRequestType::DEVICE_TO_HOST;
            halReq.wIndex = req.id_;
            halReq.wLength = MIN_WLENGTH;
            break;
        }
        Buffer buf;
        if (halReq.wLength > MIN_WLENGTH) {
            // Request channel to allocate necessary buffer
            if (!invokeHalRequestCallback(&halReq)) {
                return false;
            }
            REQUIRE(halReq.data);
        } else if (halReq.wLength != 0) {
            // Provide internal buffer to the channel
            buf = Buffer(MIN_WLENGTH);
            halReq.data = (uint8_t*)buf.data();
        }
        if (halReq.bmRequestType == UsbRequestType::HOST_TO_DEVICE) {
            assert(req.data_.size() == halReq.wLength);
            memcpy(halReq.data, req.data_.data(), req.data_.size()); // Application data
        }
        // Process service request
        if (!invokeHalRequestCallback(&halReq)) {
            return false;
        }
        if (halReq.bmRequestType == UsbRequestType::DEVICE_TO_HOST) {
            const std::string data((const char*)halReq.data, halReq.wLength);
            if (req.serviceType_ == ServiceRequest::RECV) {
                serviceRep_ = ServiceReply(data); // Application data
            } else {
                serviceRep_ = ServiceReply::parse(data); // Protocol data
            }
            // Notify completion of the data transfer
            if (!invokeHalStateCallback(HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED)) {
                return false;
            }
        }
        if (!buf.isEmpty()) {
            REQUIRE(buf.isPaddingValid());
        }
        return true;
    }

    bool invokeHalRequestCallback(HAL_USB_SetupRequest* halReq) {
        REQUIRE(halReqCallback_);
        const auto ret = halReqCallback_(halReq, halReqCallbackData_);
        return (ret == 0);
    }

    bool invokeHalStateCallback(HAL_USB_VendorRequestState state) {
        REQUIRE(halStateCallback_);
        const auto ret = halStateCallback_(state, halStateCallbackData_);
        return (ret == 0);
    }

    friend class ServiceRequest;
};

inline bool ServiceRequest::send() {
    return channel_->sendServiceRequest(*this);
}

} // namespace

TEST_CASE("UsbControlRequestChannel") {
    Channel channel;

    const uint16_t TEST_REQ = 1234; // Application-specific request type

    SECTION("INIT request") {
        SECTION("schedules an empty request for the processing immediately") {
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK); // Request processing is pending
            CHECK(rep.id() != USB_REQUEST_INVALID_ID);
            CHECK(processNextTask());
            CHECK(channel.requestHandlerCalled());
        }
        SECTION("fails with the NO_MEMORY status when a request object cannot be allocated") {
            channel.poolAllocator().allocLimit(0);
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::NO_MEMORY); // Memory allocation error
            CHECK(!processNextTask());
        }
        SECTION("allocates a small request buffer using the system pool") {
            size_t size = USB_REQUEST_MAX_POOLED_BUFFER_SIZE;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK); // Channel is ready to receive payload data
            CHECK(rep.id() != USB_REQUEST_INVALID_ID);
            CHECK(!processNextTask());
        }
        SECTION("allocates a large request buffer on the heap asynchronously") {
            size_t size = USB_REQUEST_MAX_POOLED_BUFFER_SIZE + 1;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::PENDING); // Buffer allocation is pending
            CHECK(rep.id() != USB_REQUEST_INVALID_ID);
            CHECK(processNextTask());
            CHECK(channel.heapAllocator().allocSize() == size);
        }
        SECTION("allocates a request buffer on the heap when there's no enough memory in the system pool") {
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            size_t reqSize = channel.poolAllocator().allocSize(); // Size of the request object
            channel.reset();
            channel.poolAllocator().allocLimit(reqSize);
            size_t size = 1;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::PENDING); // Buffer allocation is pending
            CHECK(rep.id() != USB_REQUEST_INVALID_ID);
            CHECK(processNextTask());
            CHECK(channel.heapAllocator().allocSize() == size);
        }
        SECTION("can initiate a limited number of concurrent requests") {
            for (unsigned i = 0; i < USB_REQUEST_MAX_ACTIVE_COUNT; ++i) {
                CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
                auto rep = channel.serviceReply();
                CHECK(rep.status() == ServiceReply::OK); // Request processing is pending
                CHECK(rep.id() != USB_REQUEST_INVALID_ID);
            }
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::BUSY); // Too many active requests
        }
    }

    SECTION("CHECK request") {
        SECTION("completes with the NOT_FOUND status when the request cannot be found") {
            uint16_t id = 1234;
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::NOT_FOUND);
        }
        SECTION("completes with the PENDING status when the request processing is pending") {
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::PENDING);
        }
        SECTION("completes with the PENDING status when the buffer allocation is pending") {
            size_t size = USB_REQUEST_MAX_POOLED_BUFFER_SIZE + 1;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::PENDING);
        }
        SECTION("completes with the NO_MEMORY status when the buffer cannot be allocated") {
            size_t size = USB_REQUEST_MAX_POOLED_BUFFER_SIZE + 1;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            uint16_t id = channel.serviceReply().id();
            channel.heapAllocator().allocLimit(0);
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::NO_MEMORY);
            // Request data should be cleared
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::NOT_FOUND);
        }
        SECTION("completes with the OK status when the buffer allocation succeeds") {
            size_t size = USB_REQUEST_MAX_POOLED_BUFFER_SIZE + 1;
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(size).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK);
        }
        SECTION("retrieves expected fields for a request without reply data") {
            system_error_t result = SYSTEM_ERROR_UNKNOWN;
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                ch->setResult(req, result);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK); // Status code
            CHECK(rep.result() == result); // Result code
            CHECK((!rep.hasSize() || rep.size() == 0)); // Size of the reply data
            // Request data should be cleared
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::NOT_FOUND);
        }
        SECTION("retrieves expected fields for a request with non-empty reply data") {
            size_t size = 1024;
            system_error_t result = SYSTEM_ERROR_UNKNOWN;
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                REQUIRE(ch->allocReplyData(req, size) == 0);
                ch->setResult(req, result);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            auto rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK); // Status code
            CHECK(rep.result() == result); // Result code
            CHECK(rep.size() == size); // Size of the reply data
            // Request data should remain valid until the RECV request
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            rep = channel.serviceReply();
            CHECK(rep.status() == ServiceReply::OK);
            CHECK(rep.result() == result);
            CHECK(rep.size() == size);
        }
        SECTION("causes the completion handler to be invoked for a request without reply data") {
            bool called = false;
            channel.requestHandler([&called](ctrl_request* req, ControlRequestChannel* ch) {
                ch->setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
                    *static_cast<bool*>(data) = true;
                }, &called);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            CHECK_FALSE(called); // Completion handler should be called asynchronously
            CHECK(processNextTask());
            CHECK(called);
        }
    }

    SECTION("SEND request") {
        SECTION("transfers request data to the channel using a buffer provided by the HAL") {
            std::string data = randomBytes(MIN_WLENGTH);
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                CHECK(std::string(req->request_data, req->request_size) == data);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(data.size()).send());
            uint16_t id = channel.serviceReply().id();
            if (data.size() > USB_REQUEST_MAX_POOLED_BUFFER_SIZE) {
                CHECK(processNextTask());
            }
            CHECK(channel.serviceRequest(ServiceRequest::SEND).id(id).data(data).send());
            CHECK(processNextTask());
            CHECK(channel.requestHandlerCalled());
            // Check status
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            CHECK(channel.serviceReply().status() == ServiceReply::PENDING);
        }
        SECTION("transfers request data to the channel using a buffer provided by the channel") {
            std::string data = randomBytes(MIN_WLENGTH + 1);
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                CHECK(std::string(req->request_data, req->request_size) == data);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(data.size()).send());
            uint16_t id = channel.serviceReply().id();
            if (data.size() > USB_REQUEST_MAX_POOLED_BUFFER_SIZE) {
                CHECK(processNextTask());
            }
            CHECK(channel.serviceRequest(ServiceRequest::SEND).id(id).data(data).send());
            CHECK(processNextTask());
            CHECK(channel.requestHandlerCalled());
            // Check status
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            CHECK(channel.serviceReply().status() == ServiceReply::PENDING);
        }
        SECTION("fails when the request cannot be not found") {
            uint16_t id = 1234;
            std::string data = "test";
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::SEND).id(id).data(data).send());
        }
        SECTION("fails when the request is in an incorrect state") {
            std::string data = "test";
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::SEND).id(id).data(data).send());
        }
        SECTION("fails when an unexpected size of the request data is specified") {
            std::string data = "test";
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(data.size() + 1).send());
            uint16_t id = channel.serviceReply().id();
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::SEND).id(id).data(data).send());
        }
    }

    SECTION("RECV request") {
        SECTION("transfers reply data to the client using a buffer provided by the HAL") {
            std::string data = randomBytes(MIN_WLENGTH);
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                REQUIRE(ch->allocReplyData(req, data.size()) == 0);
                memcpy(req->reply_data, data.data(), data.size());
                ch->setResult(req, SYSTEM_ERROR_NONE);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::RECV).id(id).size(data.size()).send());
            CHECK(channel.serviceReply().data() == data);
            // Check status
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            CHECK(channel.serviceReply().status() == ServiceReply::NOT_FOUND);
        }
        SECTION("transfers reply data to the client using a buffer provided by the channel") {
            std::string data = randomBytes(MIN_WLENGTH + 1);
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                REQUIRE(ch->allocReplyData(req, data.size()) == 0);
                memcpy(req->reply_data, data.data(), data.size());
                ch->setResult(req, SYSTEM_ERROR_NONE);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::RECV).id(id).size(data.size()).send());
            CHECK(channel.serviceReply().data() == data);
            // Check status
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            CHECK(channel.serviceReply().status() == ServiceReply::NOT_FOUND);
        }
        SECTION("causes the completion handler to be invoked for a request with non-empty reply data") {
            size_t size = 1024;
            bool called = false;
            channel.requestHandler([=, &called](ctrl_request* req, ControlRequestChannel* ch) {
                REQUIRE(ch->allocReplyData(req, size) == 0);
                ch->setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
                    *static_cast<bool*>(data) = true;
                }, &called);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK(channel.serviceRequest(ServiceRequest::CHECK).id(id).send());
            // The CHECK request should not cause the completion handler to be invoked for a request
            // with non-empty reply data
            CHECK_FALSE(processNextTask());
            CHECK_FALSE(called);
            CHECK(channel.serviceRequest(ServiceRequest::RECV).id(id).size(size).send());
            CHECK_FALSE(called); // Completion handler should be called asynchronously
            CHECK(processNextTask());
            CHECK(called);
        }
        SECTION("fails when the request cannot be not found") {
            uint16_t id = 1234;
            size_t size = 1;
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::RECV).id(id).size(size).send());
        }
        SECTION("fails when the request is in an incorrect state") {
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            size_t size = 4;
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::RECV).id(id).size(size).send());
        }
        SECTION("fails when an unexpected size of the reply data is specified") {
            std::string data = "test";
            channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
                REQUIRE(ch->allocReplyData(req, data.size()) == 0);
                memcpy(req->reply_data, data.data(), data.size());
                ch->setResult(req, SYSTEM_ERROR_NONE);
            });
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(processNextTask());
            CHECK_FALSE(channel.serviceRequest(ServiceRequest::RECV).id(id).size(data.size() + 1).send());
        }
    }

    SECTION("RESET request") {
        SECTION("can cancel an active request by its ID") {
            CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
            uint16_t id = channel.serviceReply().id();
            CHECK(channel.serviceRequest(ServiceRequest::RESET).id(id).send());
            for (unsigned i = 0; i < USB_REQUEST_MAX_ACTIVE_COUNT; ++i) {
                CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
                CHECK(channel.serviceReply().status() == ServiceReply::OK);
            }
        }
        SECTION("can cancel all active requests") {
            for (unsigned i = 0; i < USB_REQUEST_MAX_ACTIVE_COUNT; ++i) {
                CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
                CHECK(channel.serviceReply().status() == ServiceReply::OK);
            }
            CHECK(channel.serviceRequest(ServiceRequest::RESET).send());
            for (unsigned i = 0; i < USB_REQUEST_MAX_ACTIVE_COUNT; ++i) {
                CHECK(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).send());
                CHECK(channel.serviceReply().status() == ServiceReply::OK);
            }
        }
    }

    SECTION("requests can be processed asynchronously (stress test)") {
        const unsigned TOTAL_REQUESTS = 100; // Total number of requests to send
        const unsigned CANCEL_EVERY_N = 10; // Cancel every Nth request

        enum class State {
            WAIT_ALLOC, // Buffer allocation is pending
            SEND_DATA, // Channel is ready to receive payload data
            WAIT_REPLY // Request processing is pending
        };

        struct Request {
            std::string data; // Request data
            State state; // Request state
            uint16_t id; // Request ID
        };

        std::list<Request> reqs; // Active requests
        std::set<uint16_t> reqIds; // Request IDs
        unsigned reqCount = 0; // Total number of requests

        channel.requestHandler([=](ctrl_request* req, ControlRequestChannel* ch) {
            REQUIRE(req->type == TEST_REQ);
            if (req->request_size > 0) {
                // Echo request data back to the client
                REQUIRE(ch->allocReplyData(req, req->request_size) == 0);
                memcpy(req->reply_data, req->request_data, req->request_size);
            }
            ch->setResult(req, SYSTEM_ERROR_NONE);
        });

        for (;;) {
            // Send a bunch of requests
            while (reqs.size() < USB_REQUEST_MAX_ACTIVE_COUNT && reqCount < TOTAL_REQUESTS) {
                Request req;
                req.data = randomBytes(0, USB_REQUEST_MAX_POOLED_BUFFER_SIZE * 2);
                REQUIRE(channel.serviceRequest(ServiceRequest::INIT).type(TEST_REQ).size(req.data.size()).send());
                const auto rep = channel.serviceReply();
                REQUIRE((rep.status() == ServiceReply::OK || rep.status() == ServiceReply::PENDING));
                if (rep.status() == ServiceReply::PENDING) {
                    req.state = State::WAIT_ALLOC;
                } else if (!req.data.empty()) {
                    req.state = State::SEND_DATA;
                } else {
                    req.state = State::WAIT_REPLY;
                }
                req.id = rep.id();
                REQUIRE(req.id != USB_REQUEST_INVALID_ID);
                REQUIRE(reqIds.count(req.id) == 0); // ID of a new request should be unique
                reqIds.insert(req.id);
                reqs.push_back(req);
                ++reqCount;
            }
            if (reqs.empty()) {
                break; // Done
            }
            // Process pending requests
            auto req = reqs.begin();
            do {
                bool done = false;
                switch (req->state) {
                case State::WAIT_ALLOC: {
                    REQUIRE(channel.serviceRequest(ServiceRequest::CHECK).id(req->id).send());
                    const auto rep = channel.serviceReply();
                    REQUIRE((rep.status() == ServiceReply::OK || rep.status() == ServiceReply::PENDING));
                    if (rep.status() == ServiceReply::OK) {
                        req->state = State::SEND_DATA;
                    }
                    break;
                }
                case State::SEND_DATA: {
                    REQUIRE(channel.serviceRequest(ServiceRequest::SEND).id(req->id).data(req->data).send());
                    req->state = State::WAIT_REPLY;
                    break;
                }
                case State::WAIT_REPLY: {
                    REQUIRE(channel.serviceRequest(ServiceRequest::CHECK).id(req->id).send());
                    const auto rep = channel.serviceReply();
                    REQUIRE((rep.status() == ServiceReply::OK || rep.status() == ServiceReply::PENDING));
                    if (rep.status() == ServiceReply::OK) {
                        REQUIRE(rep.result() == SYSTEM_ERROR_NONE);
                        if (!req->data.empty()) {
                            REQUIRE(rep.size() == req->data.size());
                            REQUIRE(channel.serviceRequest(ServiceRequest::RECV).id(req->id).size(req->data.size()).send());
                            REQUIRE(channel.serviceReply().data() == req->data);
                        } else {
                            REQUIRE((!rep.hasSize() || rep.size() == 0));
                        }
                        done = true;
                    }
                    break;
                }
                default:
                    break;
                }
                const auto tmpReq = req;
                ++req;
                if (done) {
                    reqIds.erase(tmpReq->id);
                    reqs.erase(tmpReq);
                }
            } while (req != reqs.end());
            // Cancel some request
            if (!(reqCount % CANCEL_EVERY_N) && !reqs.empty()) {
                const auto req = reqs.begin();
                REQUIRE(channel.serviceRequest(ServiceRequest::RESET).id(req->id).send());
                REQUIRE(channel.serviceReply().status() == ServiceReply::OK);
                reqIds.erase(req->id);
                reqs.erase(req);
            }
            // Process a single asynchronous task in the queue
            processNextTask();
        }

        processAllTasks(); // Process remaining asynchronous tasks
        channel.checkMemory(); // Ensure there are no memory leaks
    }
}
