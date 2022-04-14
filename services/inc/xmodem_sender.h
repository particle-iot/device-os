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

#include <memory>
#include "system_tick_hal.h"

namespace particle {

class Stream;
class InputStream;

// Class implementing the XMODEM-1K protocol
class XmodemSender {
public:
    enum Status {
        DONE = 0,
        RUNNING
    };

    XmodemSender();
    ~XmodemSender();

    int init(Stream* dest, InputStream* src, size_t size);
    void destroy();

    // Returns one of the values defined by the `Status` enum or a negative value in case of an error.
    // This method needs to be called in a loop
    int run();

private:
    // Sender state
    enum class State {
        NEW, // Uninitialized
        RECV_NCG, // Waiting for the receiver to initiate the transfer
        SEND_PACKET, // Sending a packet
        RECV_PACKET_ACK, // Waiting for the packet acknowledgement
        SEND_EOT, // Sending the "end of transmission" sequence
        RECV_EOT_ACK // Waiting for the "end of transmission" acknowledgement
    };

    State state_; // Current sender state
    system_tick_t stateTime_; // Time when the sender state was last changed
    unsigned retryCount_; // Number of retries
    unsigned canCount_; // Number of received CAN bytes

    InputStream* srcStrm_; // Source stream
    Stream* destStrm_; // Destination stream
    size_t fileSize_; // File size
    size_t fileOffs_; // Current offset in the file

    size_t packetSize_; // Size of the current XMODEM packet
    size_t packetOffs_; // Number of transmitted bytes of the current packet
    unsigned packetNum_; // Packet number

    std::unique_ptr<char[]> buf_; // Packet buffer

    int recvNcg();
    int sendPacket();
    int recvPacketAck();
    int sendEot();
    int recvEotAck();

    int readCtrl(char* c = nullptr);
    int checkTimeout(unsigned timeout);
    void setState(State state);
};

} // particle
