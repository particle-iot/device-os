/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "system_control.h"

#if SYSTEM_CONTROL_ENABLED && BLE_ENABLED

#include "control_request_handler.h"

#include "app_fifo.h"

#include <memory>

static_assert(BLE_MAX_PERIPH_CONN_COUNT == 1, "Concurrent peripheral connections are not supported");

namespace particle {

namespace system {

// Class implementing a BLE control request channel
class BleControlRequestChannel: public ControlRequestChannel {
public:
    explicit BleControlRequestChannel(ControlRequestHandler* handler);
    ~BleControlRequestChannel();

    int init();
    void destroy();

    // TODO: Use a separate thread for the BLE channel loop
    int run();

    // ControlRequestChannel
    virtual int allocReplyData(ctrl_request* ctrlReq, size_t size) override;
    virtual void freeRequestData(ctrl_request* ctrlReq) override;
    virtual void setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) override;

private:
    // Request data
    struct Request: ctrl_request {
        Request* next; // Next request
        int result; // Result code
        uint16_t id; // Request ID
    };

    Request* pendingReqs_; // Pending requests
    Request* readyReqs_; // Completed requests

    Request* sendReq_;
    std::unique_ptr<uint8_t[]> sendBuf_;
    size_t sendPos_;
    bool headerSent_;

    Request* recvReq_;
    std::unique_ptr<uint8_t[]> recvBuf_;
    app_fifo_t recvFifo_;
    size_t recvPos_;
    bool headerRecvd_;

    uint16_t sendCharHandle_;
    uint16_t recvCharHandle_;

    volatile uint16_t connHandle_;
    volatile uint16_t maxCharValSize_;
    volatile bool notifEnabled_;
    volatile bool writable_;

    int sendNext();
    int receiveNext();

    void connected(const ble_connected_event_data& event);
    void disconnected(const ble_disconnected_event_data& event);
    void connParamChanged(const ble_conn_param_changed_event_data& event);
    void charParamChanged(const ble_char_param_changed_event_data& event);
    void dataSent(const ble_data_sent_event_data& event);
    void dataReceived(const ble_data_received_event_data& event);

    int initProfile();

    static void processBleEvent(int event, const void* eventData, void* userData);

    static Request* allocRequest(size_t size = 0);
    static Request* freeRequest(Request* req);
};

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED && BLE_ENABLED
