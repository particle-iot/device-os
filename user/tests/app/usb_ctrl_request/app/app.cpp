#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC)

namespace {

struct Request {
    ctrl_request* req; // Request handle
    system_tick_t time; // Reply time
};

Vector<Request> g_reqs;

} // namespace

// Handler for application-specific control requests (runs in the application thread)
void ctrl_request_custom_handler(ctrl_request* req) {
    Request r;
    r.req = req;
    r.time = millis() + random(100, 500);
    g_reqs.append(r);
}

void setup() {
    randomSeed(HAL_RNG_GetRandomNumber()); // Just in case
}

void loop() {
    int i = 0;
    while (i < g_reqs.size()) {
        if (g_reqs.at(i).time <= millis()) {
            // Echo the request data back to the host
            const Request r = g_reqs.takeAt(i);
            if (system_ctrl_alloc_reply_data(r.req, r.req->request_size, nullptr) == 0) {
                memcpy(r.req->reply_data, r.req->request_data, r.req->request_size);
                system_ctrl_set_result(r.req, SYSTEM_ERROR_NONE, nullptr);
            } else {
                system_ctrl_set_result(r.req, SYSTEM_ERROR_NO_MEMORY, nullptr);
            }
        } else {
            ++i;
        }
    }
}
