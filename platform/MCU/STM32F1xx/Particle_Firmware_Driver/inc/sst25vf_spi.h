/**
  ******************************************************************************
  * @file    sst25vf_spi.h
  * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
  * @version V1.0.0
  * @date    15-May-2013
  * @brief   This file contains all the functions prototypes for the
  *          sst25vf_spi Flash firmware driver.
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
#ifndef __SST25VF_SPI_H
#define __SST25VF_SPI_H

#include <stdint.h>

#define sFLASH_PAGESIZE					0x1000		/* 4096 bytes */
#define sFLASH_PAGECOUNT                                512             /* 2MByte storage */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* High level functions. */
void sFLASH_Init(void);
void sFLASH_EraseSector(uint32_t SectorAddr);
void sFLASH_EraseBulk(void);
void sFLASH_WriteBuffer(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
void sFLASH_ReadBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
uint32_t sFLASH_ReadID(void);

/* Flash Self Test Routine */
int sFLASH_SelfTest(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SST25VF_SPI_H */
