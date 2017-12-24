#include "system_control.h"

int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
}

void system_ctrl_set_result(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data, void* reserved) {
}
