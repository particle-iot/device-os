#include "system_control_mock.h"

#include "hippomocks.h"

struct test::SystemControlMock::Data {
    MockRepository mocks;
};

test::SystemControlMock::SystemControlMock() :
        d_(new Data) {
    // system_ctrl_alloc_reply_data()
    d_->mocks.OnCallFunc(system_ctrl_alloc_reply_data).Do([](ctrl_request* req, size_t size, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        return r->allocReplyData(size);
    });
    // system_ctrl_free_reply_data()
    d_->mocks.OnCallFunc(system_ctrl_free_reply_data).Do([](ctrl_request* req, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->freeReplyData();
    });
    // system_ctrl_free_request_data()
    d_->mocks.OnCallFunc(system_ctrl_free_request_data).Do([](ctrl_request* req, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->freeRequestData();
    });
    // system_ctrl_set_result()
    d_->mocks.OnCallFunc(system_ctrl_set_result).Do([](ctrl_request* req, int result, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->setResult(result);
    });
}

test::SystemControlMock::~SystemControlMock() {
}

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_free_reply_data(ctrl_request* req, void* reserved) {
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
}

void system_ctrl_set_result(ctrl_request* req, int result, void* reserved) {
}
