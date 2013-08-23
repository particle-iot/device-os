/**
  ******************************************************************************
  * @file    main.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Header for main.c module
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/

#include "hw_config.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/* Internal Flash memory address where the System Flags will be saved and loaded from  */
#define SYSTEM_FLAGS_ADDRESS		((uint32_t)0x08004C00)
/* Internal Flash end memory address */
#define INTERNAL_FLASH_END_ADDRESS	((uint32_t)0x08020000)	//For 128KB Internal Flash
/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE	((uint16_t)0x400)
/* External Flash memory address where Factory programmed core firmware is located */
#define EXTERNAL_FLASH_FACT_ADDRESS	((uint32_t)0x00001000)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP1_ADDRESS	((uint32_t)0x00010000)

/* Exported functions ------------------------------------------------------- */

void Timing_Decrement(void);
void Delay(__IO uint32_t nTime);

void Load_SystemFlags(void);
void Save_SystemFlags(void);

/* Internal and External Flash Operations */
void FLASH_Begin(void);
void FLASH_End(void);
void FLASH_Backup(uint32_t sFLASH_Address);
void FLASH_Restore(uint32_t sFLASH_Address);

void Factory_Reset(void);

#endif /* __MAIN_H */
