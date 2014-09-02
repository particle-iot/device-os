/**
 ******************************************************************************
 * @file    spark_flasher_ymodem.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    28-July-2014
 * @brief   Header for spark_flasher_ymodem.cpp module
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
#ifndef __SPARK_FLASHER_YMODEM_H
#define __SPARK_FLASHER_YMODEM_H

#include "spark_wiring.h"

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

/* Compute the FLASH max image size */
#define FLASH_MAX_SIZE          (int32_t)(INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS)

#ifdef __cplusplus
extern "C" {
#endif

  int32_t Ymodem_Receive(Stream *serialObj, uint32_t sFlashAddress, uint8_t *buf, uint8_t* fileName);
  bool Serial_Flash_Update(Stream *serialObj, uint32_t sFlashAddress);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif  /* __SPARK_FLASHER_YMODEM_H */
