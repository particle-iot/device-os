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

#include "xmodem_sender.h"

#include "timer_hal.h"

#include "stream.h"
#include "check.h"

#include <algorithm>

LOG_SOURCE_CATEGORY("xmodem");

namespace particle {

namespace {

// Control bytes
enum Ctrl: char {
    SOH = 0x01, // Start of header (128-byte packet)
    STX = 0x02, // Start of header (1024-byte packet)
    EOT = 0x04, // End of transmission
    ACK = 0x06, // Acknowledgement
    NAK = 0x15, // Negative acknowledgement
    CAN = 0x18, // Cancel transmission
    C = 0x43 // XMODEM-CRC/1K mode
};

struct __attribute__((packed)) PacketHeader {
    uint8_t start; // SOH or STX depending on the packet size
    uint8_t num; // Packet number (1-based)
    uint8_t numComp; // 255 - `num`
};

struct __attribute__((packed)) PacketCrc {
    uint8_t msb; // Most significant byte of the packet's CRC-16
    uint8_t lsb; // Least significant byte of the packet's CRC-16
};

// Size of the packet buffer
const size_t BUFFER_SIZE = 1024 + sizeof(PacketHeader) + sizeof(PacketCrc);

// Timeout settings
const unsigned NCG_TIMEOUT = 30000;
const unsigned ACK_TIMEOUT = 10000;
const unsigned SEND_TIMEOUT = 10000;

// Maximum number of retries before aborting the transfer
const unsigned MAX_PACKET_RETRY_COUNT = 2;
const unsigned MAX_EOT_RETRY_COUNT = 2;

// Number of CAN bytes that need to be received in order to cancel the transfer
const unsigned RECV_CAN_COUNT = 2;

// Calculates a 16-bit checksum using the CRC-CCITT (XMODEM) algorithm
uint16_t calcCrc16(const char* data, size_t size) {
    uint16_t crc = 0;
    const auto end = data + size;
    while (data < end) {
        const uint8_t c = *data++;
        crc ^= (uint16_t)c << 8;
        for (unsigned i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

} // particle::

XmodemSender::XmodemSender() :
        state_(State::NEW) {
}

XmodemSender::~XmodemSender() {
    destroy();
}

int XmodemSender::init(Stream* dest, InputStream* src, size_t size) {
    buf_.reset(new(std::nothrow) char[BUFFER_SIZE]);
    CHECK_TRUE(buf_, SYSTEM_ERROR_NO_MEMORY);
    srcStrm_ = src;
    destStrm_ = dest;
    fileSize_ = size;
    fileOffs_ = 0;
    packetSize_ = 0;
    packetOffs_ = 0;
    retryCount_ = 0;
    canCount_ = 0;
    packetNum_ = 1;
    setState(State::RECV_NCG);
    LOG_DEBUG(TRACE, "Waiting for NCGbyte (0x%02x)", (unsigned char)Ctrl::C);
    return 0;
}

void XmodemSender::destroy() {
    buf_.reset();
    state_ = State::NEW;
}

int XmodemSender::run() {
    int ret = 0;
    switch (state_) {
    case State::RECV_NCG: {
        ret = recvNcg();
        break;
    }
    case State::SEND_PACKET: {
        ret = sendPacket();
        break;
    }
    case State::RECV_PACKET_ACK: {
        ret = recvPacketAck();
        break;
    }
    case State::SEND_EOT: {
        ret = sendEot();
        break;
    }
    case State::RECV_EOT_ACK: {
        ret = recvEotAck();
        break;
    }
    default:
        ret = SYSTEM_ERROR_INVALID_STATE;
        break;
    }
    if (ret == Status::DONE || ret < 0) {
        destroy();
    }
    return ret;
}

int XmodemSender::recvNcg() {
    CHECK(checkTimeout(NCG_TIMEOUT));
    char c = 0;
    const size_t n = CHECK(readCtrl(&c));
    if (n > 0) {
        if (c != Ctrl::C) {
            LOG(ERROR, "Unexpected NCGbyte: 0x%02x", (unsigned char)c);
            return SYSTEM_ERROR_PROTOCOL;
        }
        LOG_DEBUG(TRACE, "Received NCGbyte");
        setState((fileSize_ > 0) ? State::SEND_PACKET : State::SEND_EOT);
    }
    return Status::RUNNING;    
}

int XmodemSender::sendPacket() {
    CHECK(checkTimeout(SEND_TIMEOUT));
    CHECK(readCtrl()); // Process CAN control bytes
    if (packetSize_ == 0) {
        PacketHeader h = {};
        size_t chunkSize = fileSize_ - fileOffs_;
        // Avoid sending more than 128 padding bytes in a 1K packet
        if (chunkSize > 896) {
            packetSize_ = 1024;
            h.start = Ctrl::STX;
        } else {
            packetSize_ = 128;
            h.start = Ctrl::SOH;
        }
        if (chunkSize > packetSize_) {
            chunkSize = packetSize_;
        }
        h.num = packetNum_ & 0xff;
        h.numComp = ~h.num;
        // Packet header
        memcpy(buf_.get(), &h, sizeof(PacketHeader));
        // TODO: Non-blocking reading of the source stream and graceful termination of the transfer
        // in case of source stream errors are not supported
        CHECK(srcStrm_->readAll(buf_.get() + sizeof(PacketHeader), chunkSize)); // Packet data
        // Padding bytes
        memset(buf_.get() + sizeof(PacketHeader) + chunkSize, 0, packetSize_ - chunkSize);
        // Packet checksum
        const uint16_t crc = calcCrc16(buf_.get() + sizeof(PacketHeader), packetSize_);
        PacketCrc c = {};
        c.msb = crc >> 8;
        c.lsb = crc & 0xff;
        memcpy(buf_.get() + sizeof(PacketHeader) + packetSize_, &c, sizeof(PacketCrc));
        packetSize_ += sizeof(PacketHeader) + sizeof(PacketCrc);
        LOG_DEBUG(TRACE, "Sending packet; number: %u, size: %u", packetNum_, (unsigned)packetSize_);
    }
    packetOffs_ += CHECK(destStrm_->write(buf_.get() + packetOffs_, packetSize_ - packetOffs_));
    if (packetOffs_ == packetSize_) {
        CHECK(destStrm_->flush());
        LOG_DEBUG(TRACE, "Waiting for ACK");
        setState(State::RECV_PACKET_ACK);
    }
    return Status::RUNNING;
}

int XmodemSender::recvPacketAck() {
    char c = 0;
    const size_t n = CHECK(readCtrl(&c));
    if (c == Ctrl::NAK || checkTimeout(ACK_TIMEOUT) != 0) {
        LOG_DEBUG(TRACE, "%s", (c == Ctrl::NAK) ? "Received NAK" : "ACK timeout");
        if (++retryCount_ > MAX_PACKET_RETRY_COUNT) {
            LOG(ERROR, "Maximum number of retransmissions exceeded");
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
        LOG_DEBUG(TRACE, "Resending packet");
        packetOffs_ = 0;
        setState(State::SEND_PACKET);
    } else if (n > 0) {
        if (c != Ctrl::ACK) {
            LOG(ERROR, "Unexpected control byte: 0x%02x", (unsigned char)c);
            return SYSTEM_ERROR_PROTOCOL;
        }
        LOG_DEBUG(TRACE, "Received ACK");
        retryCount_ = 0;
        fileOffs_ += std::min(fileSize_ - fileOffs_, packetSize_ - sizeof(PacketHeader) - sizeof(PacketCrc));
        if (fileOffs_ < fileSize_) {
            // Send next packet
            packetSize_ = 0;
            packetOffs_ = 0;
            ++packetNum_;
            setState(State::SEND_PACKET);
        } else {
            // Send "end of transmission" sequence
            setState(State::SEND_EOT);
        }
    }
    return Status::RUNNING;
}

int XmodemSender::sendEot() {
    CHECK(checkTimeout(SEND_TIMEOUT));
    CHECK(readCtrl()); // Process CAN control bytes
    const char c = Ctrl::EOT;
    const size_t n = CHECK(destStrm_->write(&c, 1));
    if (n > 0) {
        CHECK(destStrm_->flush());
        LOG_DEBUG(TRACE, "Sent EOT");
        setState(State::RECV_EOT_ACK);
    }
    return Status::RUNNING;
}

int XmodemSender::recvEotAck() {
    char c = 0;
    const size_t n = CHECK(readCtrl(&c));
    if (c == Ctrl::NAK || checkTimeout(ACK_TIMEOUT) != 0) {
        LOG_DEBUG(TRACE, "%s", (c == Ctrl::NAK) ? "Received NAK" : "ACK timeout");
        if (++retryCount_ > MAX_EOT_RETRY_COUNT) {
            LOG(ERROR, "Maximum number of retransmissions exceeded");
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
        LOG_DEBUG(TRACE, "Resending EOT");
        setState(State::SEND_EOT);
    } else if (n > 0) {
        if (c != Ctrl::ACK) {
            LOG(ERROR, "Unexpected control byte: 0x%02x", (unsigned char)c);
            return SYSTEM_ERROR_PROTOCOL;
        }
        LOG_DEBUG(TRACE, "Received ACK");
        return Status::DONE;
    }
    return Status::RUNNING;
}

int XmodemSender::readCtrl(char* c) {
    char cc = 0;
    size_t n = CHECK(destStrm_->read(&cc, 1));
    if (n > 0) {
        if (cc == Ctrl::CAN) {
            if (++canCount_ == RECV_CAN_COUNT) {
                LOG(WARN, "Receiver has cancelled the transfer");
                return SYSTEM_ERROR_CANCELLED;
            }
            n = 0;
        } else {
            canCount_ = 0;
            if (cc == Ctrl::C && state_ != State::RECV_NCG && packetNum_ == 1) {
                // Ignore superfluous NCGbyte's received while we're sending the first packet
                n = 0;
            } else if (c) {
                *c = cc;
            }
        }
    }
    return n;
}

int XmodemSender::checkTimeout(unsigned timeout) {
    if (HAL_Timer_Get_Milli_Seconds() - stateTime_ >= timeout) {
        LOG_DEBUG(TRACE, "Timeout; state: %d", (int)state_);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return 0;
}

void XmodemSender::setState(State state) {
    state_ = state;
    stateTime_ = HAL_Timer_Get_Milli_Seconds();
}

} // particle
