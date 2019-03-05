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

#if SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE

#include "control_request_handler.h"
#include "simple_pool_allocator.h"

#include "intrusive_queue.h"
#include "linked_buffer.h"

#include "ble_hal.h"

#include "spark_wiring_thread.h"

#include <memory>
#include <atomic>

// Set this macro to 0 to disable the channel security
#ifndef BLE_CHANNEL_SECURITY_ENABLED
#define BLE_CHANNEL_SECURITY_ENABLED 1
#endif

// Set this macro to 1 to enable additional logging
#ifndef BLE_CHANNEL_DEBUG_ENABLED
#define BLE_CHANNEL_DEBUG_ENABLED 0
#endif

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

    void run();

    // Reimplemented from `ControlRequestChannel`
    virtual int allocReplyData(ctrl_request* ctrlReq, size_t size) override;
    virtual void freeRequestData(ctrl_request* ctrlReq) override;
    virtual void setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) override;

private:
    class HandshakeHandler;
    class JpakeHandler;
    class AesCcmCipher;

    struct Buffer: LinkedBuffer<> {
        char* data;
        size_t size;
    };

    // Request data
    struct Request: ctrl_request {
        Request* next; // Next request
        char* reqBuf; // Request buffer (assembled from multiple input buffers)
        Buffer* repBuf; // Reply buffer (allocated as a single output buffer)
        ctrl_completion_handler_fn handler; // Completion handler
        void* handlerData; // Completion handler data
        int result; // Result code
        unsigned connId; // Connection ID
        uint16_t id; // Request ID
    };

#if BLE_CHANNEL_DEBUG_ENABLED
    std::atomic<unsigned> allocReqCount_;
    std::atomic<unsigned> heapBufCount_;
    std::atomic<unsigned> poolBufCount_;
#endif
    IntrusiveQueue<Request> readyReqs_; // Completed requests
    IntrusiveQueue<Request> pendingReps_; // Pending completion handlers
    Mutex readyReqsLock_;

    AtomicIntrusiveQueue<Buffer> inBufs_; // Packets received from the client (input buffers)
    IntrusiveQueue<Buffer> outBufs_; // Packets to be sent to the client (output buffers)
    IntrusiveQueue<Buffer> readInBufs_; // "Consumed" input buffers (see read() function)
    size_t inBufSize_; // Total size of all consumed input buffers

    Request* curReq_; // A request being received
    size_t reqBufSize_; // Size of the request buffer
    size_t reqBufOffs_; // Offset in the request buffer

    std::unique_ptr<char[]> packetBuf_; // Intermediate buffer for BLE packet data
    size_t packetSize_; // Size of the pending BLE packet
#if BLE_CHANNEL_SECURITY_ENABLED
    std::unique_ptr<AesCcmCipher> aesCcm_; // AES cipher
    std::unique_ptr<JpakeHandler> jpake_; // J-PAKE handshake handler
#endif
    AtomicAllocedPool pool_; // Pool allocator

    uint16_t connHandle_; // Connection handle used by the processing thread
    volatile uint16_t curConnHandle_; // Current connection handle

    unsigned connId_; // Last connection ID known to the processing thread
    std::atomic<unsigned> curConnId_; // Current connection ID

    std::atomic<unsigned> packetCount_; // Number of pending notification packets
    volatile uint16_t maxPacketSize_; // Maximum number of bytes that can be sent in a single notification packet
    volatile bool subscribed_; // Set to `true` if the client is subscribed to the notifications
    volatile bool writable_; // Set to `true` if the TX characteristic is writable

    uint16_t sendCharHandle_; // TX characteristic handle
    uint16_t recvCharHandle_; // RX characteristic handle

    int initChannel();
    void resetChannel();

    int receiveRequest();
    int sendReply();
    int sendPacket();

    bool readAll(char* data, size_t size);
    size_t readSome(char* data, size_t size);
    void sendBuffer(Buffer* buf);

    int connected(const ble_connected_event_data& event);
    int disconnected(const ble_disconnected_event_data& event);
    int connParamChanged(const ble_conn_param_changed_event_data& event);
    int charParamChanged(const ble_char_param_changed_event_data& event);
    int dataSent(const ble_data_sent_event_data& event);
    int dataReceived(const ble_data_received_event_data& event);

    int initProfile();
#if BLE_CHANNEL_SECURITY_ENABLED
    int initAesCcm();
    int initJpake();
#endif
    int allocRequest(size_t size, Request** req);
    void freeRequest(Request* req);

    int reallocBuffer(size_t size, Buffer** buf);
    void freeBuffer(Buffer* buf);

    int allocPooledBuffer(size_t size, Buffer** buf);
    void freePooledBuffer(Buffer* buf);

    static void processBleEvent(int event, const void* eventData, void* userData);
};

inline void BleControlRequestChannel::sendBuffer(Buffer* buf) {
    outBufs_.pushBack(buf);
}

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE
