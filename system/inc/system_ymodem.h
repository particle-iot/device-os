/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef SYSTEM_YMODEM_H
#define SYSTEM_YMODEM_H

#include "file_transfer.h"
#include "spark_wiring_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

bool Ymodem_Serial_Flash_Update(Stream *serialObj, FileTransfer::Descriptor& desc, void*);

#ifdef __cplusplus
}
#endif

class YModem
{
    Stream& stream;

public:
    enum protocol_params_t
    {
        PACKET_SEQNO_INDEX = 1,
        PACKET_SEQNO_COMP_INDEX = 2,
        PACKET_HEADER = 3,
        PACKET_TRAILER = 2,
        PACKET_OVERHEAD = (PACKET_HEADER + PACKET_TRAILER),
        PACKET_SIZE = 128,
        PACKET_1K_SIZE = 1024,
        FILE_NAME_LENGTH = 256,
        FILE_SIZE_LENGTH = 16,
        MAX_ERRORS = (5)
    };

    const uint32_t NAK_TIMEOUT = (5000);

    enum protocol_msg_t
    {
        SOH = (0x01),   /* start of 128-byte data packet */
        STX = (0x02),   /* start of 1024-byte data packet */
        EOT = (0x04),   /* end of transmission */
        ACK = (0x06),   /* acknowledge */
        NAK = (0x15),   /* negative acknowledge */
        CA = (0x18),    /* two of these in succession aborts transfer */
        CRC16 = (0x43), /* 'C' == 0x43, request 16-bit CRC */

        ABORT1 = (0x41), /* 'A' == 0x41, abort by user */
        ABORT2 = (0x61)  /* 'a' == 0x61, abort by user */
    };

    struct file_desc_t
    {
        char file_name[FILE_NAME_LENGTH];
        char file_size[FILE_SIZE_LENGTH];
    };

    YModem(Stream& stream_) : stream(stream_)
    {
    }

    int32_t receive_file(FileTransfer::Descriptor& tx, file_desc_t& file_info);

    /**
     * @brief  Send a byte
     * @param  c: Character
     * @retval 0: Byte sent
     */
    uint32_t send_byte(uint8_t c)
    {
        stream.write(c);
        return 0;
    }


private:
    uint8_t packet_data[YModem::PACKET_1K_SIZE + YModem::PACKET_OVERHEAD];
    int32_t session_done, file_done, packets_received, errors, session_begin;

    /**
     * @brief  Receive byte from sender
     * @param  c: Character
     * @param  timeout: Timeout
     * @retval 0: Byte received
     *         -1: Timeout
     */
    int32_t receive_byte(uint8_t& c, uint32_t timeout);

    /* Constants used by Serial Command Line Mode */
    //#define CMD_STRING_SIZE         128

    int32_t receive_packet(uint8_t* data, int32_t& length, uint32_t timeout);
    int32_t handle_packet(uint8_t* packet_data, int32_t packet_length, FileTransfer::Descriptor& tx,
                          file_desc_t& desc);
    void parse_file_packet(FileTransfer::Descriptor& tx, file_desc_t& desc, uint8_t* packet_data);
};

#endif /* SYSTEM_YMODEM_H */
