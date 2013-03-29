/**
  ******************************************************************************
  * @file    sst25vf_spi.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    29-March-2013
  * @brief   This file contains all the functions prototypes for the 
  *          sst25vf_spi Flash firmware driver.
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SST25VF_SPI_H
#define __SST25VF_SPI_H

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"

 /* SST25 SPI Flash supported commands */
 #define sFLASH_CMD_RDSR					0x05	/* Read Status Register */
 #define sFLASH_CMD_WRSR					0x01	/* Write Status Register */
 #define sFLASH_CMD_EWSR					0x50	/* Write Enable Status */
 #define sFLASH_CMD_WRDI					0x04	/* Write Disable */
 #define sFLASH_CMD_WREN					0x06	/* Write Enable */
 #define sFLASH_CMD_READ					0x03	/* Read Data Bytes */
 #define sFLASH_CMD_FAST_READ				0x0B	/* Higher Speed Read */
 #define sFLASH_CMD_BP 						0x02	/* Byte Program */
 #define sFLASH_CMD_AAIP                 	0xAD	/* Auto Address Increment */
 #define sFLASH_CMD_ERASE_4K            	0x20	/* 4Kb Sector Erase */
 #define sFLASH_CMD_ERASE_32K           	0x52	/* 32Kbit Block Erase */
 #define sFLASH_CMD_ERASE_64K           	0xD8	/* 64Kbit Block Erase */
 #define sFLASH_CMD_ERASE_Full           	0xC7	/* Chip Erase */
 #define sFLASH_CMD_JEDEC_ID            	0x9F	/* JEDEC ID Read */
 #define sFLASH_CMD_EBSY                 	0x70	/* Enable SO RY/BY# Status */
 #define sFLASH_CMD_DBSY                 	0x80	/* Disable SO RY/BY# Status */

 #define sFLASH_SST25VF040_ID				0x8D25BF	/* JEDEC Read-ID Data */

 /* High layer functions */
 extern void sFLASH_Init(void);
 extern uint32_t sFLASH_Read(uint32_t offset,uint8_t * buffer,uint32_t size);
 extern uint32_t sFLASH_WritePage(uint32_t page,const uint8_t * buffer,uint32_t size);

#endif /* __SST25VF_SPI_H */
