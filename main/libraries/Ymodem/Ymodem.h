/**
 ******************************************************************************
 * @file    Ymodem.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    28-July-2014
 * @brief   Serial port using YMODEM protocol for flashing user firmware.
 *          Use a Terminal program(eg. TeraTerm) that supports ymodem protocol
 *          to update the firmware. Adapted from ST app note AN2557.
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
#include "spark_wlan.h"
#include "ota_flash_hal.h"

#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x100000)
#define MAX_ERRORS              (5)

/* Constants used by Serial Command Line Mode */
#define CMD_STRING_SIZE         128

using namespace spark;

/**
 * @brief  Test to see if a key has been pressed on the HyperTerminal
 * @param  key: The key pressed
 * @retval 1: Correct
 *         0: Error
 */
static uint32_t Serial_KeyPressed(Stream *serialObj, uint8_t *key)
{
  if(serialObj->available())
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
static void Serial_PrintCharArray(Stream *serialObj, uint8_t *s)
{
  while (*s != '\0')
  {
    serialObj->write(*s);
    s++;
  }
}

/**
 * @brief  Receive byte from sender
 * @param  c: Character
 * @param  timeout: Timeout
 * @retval 0: Byte received
 *         -1: Timeout
 */
static int32_t Receive_Byte(Stream *serialObj, uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (Serial_KeyPressed(serialObj, c) == 1)
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
static uint32_t Send_Byte(Stream *serialObj, uint8_t c)
{
  serialObj->write(c);
  return 0;
}

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
static int32_t Receive_Packet(Stream *serialObj, uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(serialObj, &c, timeout) != 0)
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
      if ((Receive_Byte(serialObj, &c, timeout) == 0) && (c == CA))
      {
        *length = -1;
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
  for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
  {
    if (Receive_Byte(serialObj, data + i, timeout) != 0)
    {
      return -1;
    }
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  *length = packet_size;
  return 0;
}

/**
 * @brief  Receive a file using the ymodem protocol
 * @param  buf: Address of the first byte
 * @retval The size of the file
 */
static int32_t Ymodem_Receive(Stream *serialObj, uint32_t sFlashAddress, uint8_t *buf, uint8_t* fileName)
{
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin;
  uint32_t size = 0;
  uint16_t current_index = 0, saved_index = 0;

  for (session_done = 0, errors = 0, session_begin = 0; ;)
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
      switch (Receive_Packet(serialObj, packet_data, &packet_length, NAK_TIMEOUT))
      {
        case 0:
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case -1:
              Send_Byte(serialObj, ACK);
              return 0;
              /* End of transmission */
            case 0:
              Send_Byte(serialObj, ACK);
              file_done = 1;
              break;
              /* Normal packet */
            default:
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(serialObj, NAK);
              }
              else
              {
                if (packets_received == 0)
                {
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      fileName[i++] = *file_ptr++;
                    }
                    fileName[i++] = '\0';
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    size = strtoul((const char *)file_size, NULL, 10);

                    /* Test the size of the image to be sent */
                    /* Image size is greater than Flash max size */
                    if (size > HAL_OTA_FlashLength())
                    {
                      /* End session */
                      Send_Byte(serialObj, CA);
                      Send_Byte(serialObj, CA);
                      return -1;
                    }

                    RGB.control(true);
                    RGB.color(RGB_COLOR_MAGENTA);
                    SPARK_FLASH_UPDATE = 1;
                    TimingFlashUpdateTimeout = 0;
                    HAL_FLASH_Begin(sFlashAddress, size);

                    Send_Byte(serialObj, ACK);
                    Send_Byte(serialObj, CRC16);
                  }
                  /* Filename packet is empty, end session */
                  else
                  {
                    Send_Byte(serialObj, ACK);
                    file_done = 1;
                    session_done = 1;
                    break;
                  }
                }
                /* Data packet */
                else
                {
                  memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
                  TimingFlashUpdateTimeout = 0;
                  saved_index = HAL_FLASH_Update(buf, packet_length);
                  LED_Toggle(LED_RGB);
                  if(saved_index > current_index)
                  {
                    current_index = saved_index;
                    Send_Byte(serialObj, ACK);
                  }
                  else
                  {
                    /* End session if Spark_Save_Firmware_Chunk() fails */
                    Send_Byte(serialObj, CA);
                    Send_Byte(serialObj, CA);
                    return -2;
                  }
                }
                packets_received++;
                session_begin = 1;
              }
          }
          break;
            case 1:
              Send_Byte(serialObj, CA);
              Send_Byte(serialObj, CA);
              return -3;
            default:
              if (session_begin > 0)
              {
                errors++;
              }
              if (errors > MAX_ERRORS)
              {
                Send_Byte(serialObj, CA);
                Send_Byte(serialObj, CA);
                return 0;
              }
              Send_Byte(serialObj, CRC16);
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
  return (int32_t)size;
}

/**
 * @brief  Flash update via serial port using ymodem protocol
 * @param  serialObj (Possible values : &Serial, &Serial1 or &Serial2)
 * @retval true on success
 */
bool Ymodem_Serial_Flash_Update(Stream *serialObj, uint32_t sFlashAddress)
{
  int32_t Size = 0;
  uint8_t fileName[FILE_NAME_LENGTH] = {0};
  uint8_t buffer[1024] = {0};

  serialObj->println("Waiting for the binary file to be sent ... (press 'a' to abort)");
  Size = Ymodem_Receive(serialObj, sFlashAddress, &buffer[0], fileName);
  RGB.control(false);;
  if (Size > 0)
  {
    serialObj->println("\r\nDownloaded file successfully!");
    serialObj->print("Name: ");
    Serial_PrintCharArray(serialObj, fileName);
    serialObj->println("");
    serialObj->print("Size: ");
    serialObj->print(Size);
    serialObj->println(" bytes");
    return true;
  }
  else if (Size == -1)
  {
    serialObj->println("The file size is higher than the allowed space memory!");
  }
  else if (Size == -2)
  {
    serialObj->println("Verification failed!");
  }
  else if (Size == -3)
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
