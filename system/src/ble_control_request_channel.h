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

#include "ble_hal.h"

#if HAS_BLE_CONTROL_REQUEST_CHANNEL

#include "system_control.h"
#include "control_request_handler.h"

#include "nrf_ble_gatt.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble.h"

#include "app_fifo.h"

namespace particle {

namespace system {

// Class implementing a BLE control request channel
class BleControlRequestChannel: public ControlRequestChannel {
public:
    explicit BleControlRequestChannel(ControlRequestHandler* handler);
    ~BleControlRequestChannel();

    int init();
    void destroy();

    int startAdvert();
    void stopAdvert();

    int run();

    // ControlRequestChannel
    virtual int allocReplyData(ctrl_request* ctrlReq, size_t size) override;
    virtual void freeRequestData(ctrl_request* ctrlReq) override;
    virtual void setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) override;

    // FIXME
    bool advertActive() const;
    bool connected() const;

private:
    // Request data
    struct Req: ctrl_request {
        Req* next; // Next request
        int result; // Result code
        uint16_t id; // Request ID
    };

    Req* pendingReqs_; // Pending requests
    Req* readyReqs_; // Completed requests

    Req* recvReq_;
    app_fifo_t recvFifo_;
    uint8_t* recvBuf_;
    size_t recvDataPos_;

    bool recvHeader_;

    Req* sendReq_;
    uint8_t* sendBuf_;
    size_t sendDataPos_;
    bool sendHeader_;

    int recvNext();
    int sendNext();

    void onConnect(const ble_evt_t* event);
    void onDisconnect(const ble_evt_t* event);
    void onWrite(const ble_evt_t* event);
    void onTxComplete(const ble_evt_t* event);
    void bleEventHandler(const ble_evt_t* event);

    int initConnParam();
    int initAdvert();
    int initTxChar();
    int initRxChar();
    int initServices();
    int initGatt();
    int initGap();
    int initBle();

    static void bleEventHandler(const ble_evt_t* event, void* data);
    static void connParamEventHandler(ble_conn_params_evt_t* event);
    static void connParamErrorHandler(uint32_t error);
    static void advertEventHandler(ble_adv_evt_t event);
    static void gattEventHandler(nrf_ble_gatt_t* gatt, const nrf_ble_gatt_evt_t* event);
    static void qwrErrorHandler(uint32_t error);

    static Req* allocReq(size_t size = 0);
    static Req* freeReq(Req* req);
};

} // particle::system

} // particle

#endif // HAS_BLE_CONTROL_REQUEST_CHANNEL
