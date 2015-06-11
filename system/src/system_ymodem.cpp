/**
 ******************************************************************************
 * @file    system_ymodem.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    28-July-2014
 * @brief   Serial port using YMODEM protocol for flashing user firmware.
 *          Use a Terminal program(eg. TeraTerm) that supports ymodem protocol
 *          to update the firmware. Adapted from ST app note AN2557.
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_YMODEM_H
#define __LIB_YMODEM_H

#include "spark_wiring.h"
#include "system_task.h"
#include "system_update.h"
#include "system_ymodem.h"
#include "ota_flash_hal.h"
#include "rgbled.h"
#include "file_transfer.h"

/**
 * @brief  Test to see if a key has been pressed on the HyperTerminal
 * @param  key: The key pressed
 * @retval 1: Correct
 *         0: Error
 */
static uint32_t Serial_KeyPressed(Stream *serialObj, uint8_t *key)
{
    if (serialObj->available())
    {
        *key = serialObj->read();
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief  Print a string on the HyperTerminal
 * @param  s: The string to be printed
 * @retval None
 */
static void Serial_PrintCharArray(Stream *serialObj, char *s)
{
    serialObj->print(s);
}

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

    const uint32_t NAK_TIMEOUT = (0x10000000);

    enum protocol_msg_t
    {
        SOH = (0x01), /* start of 128-byte data packet */
        STX = (0x02), /* start of 1024-byte data packet */
        EOT = (0x04), /* end of transmission */
        ACK = (0x06), /* acknowledge */
        NAK = (0x15), /* negative acknowledge */
        CA = (0x18), /* two of these in succession aborts transfer */
        CRC16 = (0x43), /* 'C' == 0x43, request 16-bit CRC */

        ABORT1 = (0x41), /* 'A' == 0x41, abort by user */
        ABORT2 = (0x61) /* 'a' == 0x61, abort by user */
    };

    struct file_desc_t
    {
        char file_name[FILE_NAME_LENGTH];
        char file_size[FILE_SIZE_LENGTH];
    };

    YModem(Stream& stream_) : stream(stream_) { }


    int32_t receive_file(FileTransfer::Descriptor& tx, file_desc_t& file_info);


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
    int32_t receive_byte(uint8_t& c, uint32_t timeout)
    {
        while (timeout-- > 0)
        {
            if (Serial_KeyPressed(&stream, &c) == 1)
            {
                return 0;
            }
        }
        return -1;
    }

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

    /* Constants used by Serial Command Line Mode */
    //#define CMD_STRING_SIZE         128

    int32_t receive_packet(uint8_t* data, int32_t& length, uint32_t timeout);
    int32_t handle_packet(uint8_t* packet_data, int32_t packet_length, FileTransfer::Descriptor& tx, file_desc_t& desc);
    void parse_file_packet(FileTransfer::Descriptor& tx, file_desc_t& desc, uint8_t* packet_data);

};

/**
 * @brief  Receive a packet from sender
 * @param  data
 * @param  length
 * @param  timeout
 *     0: end of transmission
 *    -1: abort by sender
 *    >0: packet length
 * @retval 0: normally return
 *        -1: timeout or packet error
 *         1: abort by user
 */
int32_t YModem::receive_packet(uint8_t *data, int32_t& length, uint32_t timeout)
{
    uint16_t i, packet_size;
    uint8_t c;
    length = 0;
    if (receive_byte(c, timeout) != 0)
    {
        return -1;
    }
    switch (c)
    {
    case SOH:
        packet_size = PACKET_SIZE;
        break;
    case STX:
        packet_size = PACKET_1K_SIZE;
        break;
    case EOT:
        return 0;
    case CA:
        if ((receive_byte(c, timeout) == 0) && (c == CA))
        {
            length = -1;
            return 0;
        }
        else
        {
            return -1;
        }
    case ABORT1:
    case ABORT2:
        return 1;
    default:
        return -1;
    }
    *data = c;
    for (i = 1; i < (packet_size + PACKET_OVERHEAD); i++)
    {
        if (receive_byte(data[i], timeout) != 0)
        {
            return -1;
        }
    }
    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
    {
        return -1;
    }
    length = packet_size;
    return 0;
}

void YModem::parse_file_packet(FileTransfer::Descriptor& tx, YModem::file_desc_t& desc, uint8_t* packet_data)
{
    /* Filename packet has valid data */
    const uint8_t* file_ptr;
    char* fileName = desc.file_name;
    char* file_size = desc.file_size;
    int i;
    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
    {
        fileName[i++] = *file_ptr++;
    }
    fileName[i++] = '\0';
    for (i = 0, file_ptr++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
    {
        file_size[i++] = *file_ptr++;
    }
    file_size[i++] = '\0';
    tx.file_length = strtoul((const char *) file_size, NULL, 10);
    tx.chunk_size = 1024;
}

int32_t YModem::handle_packet(uint8_t* packet_data, int32_t packet_length,
                              FileTransfer::Descriptor& tx, YModem::file_desc_t& desc)
{
    switch (packet_length)
    {
        /* Abort by sender */
    case -1:
        send_byte(ACK);
        return 0;

        /* End of transmission */
    case 0:
        send_byte(ACK);
        file_done = 1;
        return 1;
    }

    if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
    {
        send_byte(NAK);
    }
    else
    {
        if (packets_received == 0)
        {
            /* Filename packet */
            if (packet_data[PACKET_HEADER] != 0)
            {
                parse_file_packet(tx, desc, packet_data);
                if (Spark_Prepare_For_Firmware_Update(tx, 0, NULL))
                {
                    /* End session */
                    send_byte(CA);
                    send_byte(CA);
                    return -1;
                }
                tx.chunk_address = tx.file_address;
                send_byte(ACK);
                send_byte(CRC16);
            } /* Filename packet is empty, end session */
            else
            {
                send_byte(ACK);
                file_done = 1;
                session_done = 1;
            }
        } /* Data packet */
        else
        {
            tx.chunk_size = packet_length;
            if (Spark_Save_Firmware_Chunk(tx, packet_data + PACKET_HEADER, NULL))
            {
                /* End session if Spark_Save_Firmware_Chunk() fails */
                send_byte(CA);
                send_byte(CA);
                return -2;
            }
            tx.chunk_address += tx.chunk_size;
            send_byte(ACK);
        }
        packets_received++;
        session_begin = 1;
    }

    return 1; // success
}

int32_t YModem::receive_file(FileTransfer::Descriptor& tx, YModem::file_desc_t& file_info)
{
    memset(&file_info, 0, sizeof (file_info));
    session_done = 0;
    errors = 0;
    session_begin = 0;

    for (;;)
    {
        for (packets_received = 0, file_done = 0;;)
        {
            int32_t result;
            int32_t packet_length;
            switch (receive_packet(packet_data, packet_length, NAK_TIMEOUT))
            {
            case 0:
                errors = 0;
                result = handle_packet(packet_data, packet_length, tx, file_info);
                if (result<=0)
                    return result;
                break;

            case 1:
                send_byte(CA);
                send_byte(CA);
                return -3;

            default:
                if (session_begin > 0)
                {
                    errors++;
                    send_byte(CRC16);
                }
                if (errors > MAX_ERRORS)
                {
                    send_byte(CA);
                    send_byte(CA);
                    return 0;
                }
                break;
            }
            if (file_done != 0)
            {
                break;
            }
        }
        if (session_done != 0)
        {
            break;
        }
    }
    return tx.file_length;
}

/**
 * @brief  Flash update via serial port using ymodem protocol
 * @param  serialObj (Possible values : &Serial, &Serial1 or &Serial2)
 * @retval true on success
 */
bool Ymodem_Serial_Flash_Update(Stream *serialObj, FileTransfer::Descriptor& file, void* reserved)
{
    YModem::file_desc_t desc;
    YModem* ymodem = new YModem(*serialObj);
    int32_t size = ymodem->receive_file(file, desc);
    delete ymodem;
    if (size > 0)
    {
        serialObj->println("\r\nDownloaded file successfully!");
        serialObj->print("Name: ");
        Serial_PrintCharArray(serialObj, desc.file_name);
        serialObj->println("");
        serialObj->print("Size: ");
        serialObj->print(size);
        serialObj->println(" bytes");
        serialObj->flush();
        delay(1000);
        Spark_Finish_Firmware_Update(file, size>0 ? 1 : 0, NULL);
        return true;
    }
    else if (size == -1)
    {
        serialObj->println("The file size is higher than the allowed space memory!");
    }
    else if (size == -2)
    {
        serialObj->println("Verification failed!");
    }
    else if (size == -3)
    {
        serialObj->println("Aborted by user.");
    }
    else
    {
        serialObj->println("Failed to receive the file!");
    }
    return false;
}

#endif  /* __LIB_YMODEM_H */
