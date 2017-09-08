#include "application.h"

SYSTEM_MODE(MANUAL)

// Handler for application-specific control requests
void ctrl_request_custom_handler(ctrl_request* req) {
    // Echo the same data back to the host
    if (system_ctrl_alloc_reply_data(req, req->request_size, nullptr) == 0) {
        memcpy(req->reply_data, req->request_data, req->request_size);
        system_ctrl_set_result(req, SYSTEM_ERROR_NONE, nullptr);
    } else {
        system_ctrl_set_result(req, SYSTEM_ERROR_NO_MEMORY, nullptr);
    }
}

void setup() {
}

void loop() {
}
