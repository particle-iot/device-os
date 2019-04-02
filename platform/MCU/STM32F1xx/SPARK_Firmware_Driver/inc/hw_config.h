/**
 ******************************************************************************
 * @file    hw_config.h
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
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
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
/* Includes ------------------------------------------------------------------*/

#include <limits.h>
#include "platform_config.h"
#include "config.h"
#include "sst25vf_spi.h"
#include "usb_type.h"
#include "rgbled.h"
#include "system_tick_hal.h"
#include "usb_hal.h"
#include "hw_ticks.h"
#include "button_hal.h"


#ifdef __cplusplus
extern "C" {
#endif

enum SpiBusOwner {
    BUS_OWNER_NONE = 0,
    BUS_OWNER_CC3000 = 1,
    BUS_OWNER_SFLASH = 2
};


/* Exported types ------------------------------------------------------------*/

typedef enum
{
	CC3000_DMA_TX = 0, CC3000_DMA_RX = 1
} CC3000_DMADirection_TypeDef;


/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

#ifndef INTERNAL_FLASH_SIZE
#   error "INTERNAL_FLASH_SIZE not defined"
#endif

// Firmware image size is usually same size as internal flash
#ifndef FIRMWARE_IMAGE_SIZE
#define FIRMWARE_IMAGE_SIZE INTERNAL_FLASH_SIZE
#endif

#if FIRMWARE_IMAGE_SIZE > INTERNAL_FLASH_SIZE
#   error "FIRMWARE_IMAGE_SIZE too large to fit into internal flash"
#endif

/* Internal Flash memory address where various firmwares are located */
#ifndef INTERNAL_FLASH_START
#define INTERNAL_FLASH_START                    ((uint32_t)0x08000000)
#endif

#define USB_DFU_ADDRESS			        INTERNAL_FLASH_START
#define CORE_FW_ADDRESS			        ((uint32_t)0x08005000)
#define APP_START_MASK                          ((uint32_t)0x2FFE0000)
/* Internal Flash memory address where the System Flags will be saved and loaded from  */
#define SYSTEM_FLAGS_ADDRESS		((uint32_t)0x08004C00)
/* Internal Flash end memory address */

#define INTERNAL_FLASH_END_ADDRESS	((uint32_t)INTERNAL_FLASH_START+INTERNAL_FLASH_SIZE)	//For 128KB Internal Flash
/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE	((uint16_t)0x400)

/* External Flash block size allocated for firmware storage */
#define EXTERNAL_FLASH_BLOCK_SIZE	((uint32_t)FIRMWARE_IMAGE_SIZE)
/* External Flash memory address where Factory programmed core firmware is located */
#define EXTERNAL_FLASH_FAC_ADDRESS	((uint32_t)EXTERNAL_FLASH_BLOCK_SIZE)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_FAC_ADDRESS))
/* External Flash memory address where OTA upgraded core firmware will be saved */
#define EXTERNAL_FLASH_OTA_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS))

/* External flash location where user can start writing */
#define EXTERNAL_FLASH_USER_ADDRESS     ((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_OTA_ADDRESS))

/* External Flash memory address where server domain/IP resides */
#define EXTERNAL_FLASH_SERVER_DOMAIN_ADDRESS      ((uint32_t)0x1180)
/* Length in bytes of server domain/IP data */
#define EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH       (128)
/* External Flash memory address where server public RSA key resides */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS	((uint32_t)0x01000)
/* Length in bytes of DER-encoded 2048-bit RSA public key */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH		(294)
/* External Flash memory address where core private RSA key resides */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_ADDRESS		((uint32_t)0x02000)
/* Length in bytes of DER-encoded 1024-bit RSA private key */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH		(612)

/* Bootloader Flash Pages that needs to be protected: 0x08000000 - 0x08003FFF */
#define BOOTLOADER_FLASH_PAGES		( FLASH_WRProt_Pages0to3	\
									| FLASH_WRProt_Pages4to7	\
									| FLASH_WRProt_Pages8to11	\
									| FLASH_WRProt_Pages12to15 )

#define SYSTEM_US_TICKS		(SystemCoreClock / 1000000)//cycles per microsecond

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void Reset_System(void);
void NVIC_Configuration(void);
void SysTick_Configuration(void);

void IWDG_Reset_Enable(uint32_t msTimeout);

void UI_Timer_Configure(void);

void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState);
uint8_t BUTTON_GetState(Button_TypeDef Button);
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button);
void BUTTON_ResetDebouncedState(Button_TypeDef Button);

/* CC3000 Hardware related methods */
void CC3000_WIFI_Init(void);
void CC3000_SPI_Init(void);
void CC3000_DMA_Config(CC3000_DMADirection_TypeDef Direction, uint8_t* buffer, uint16_t NumData);
void CC3000_SPI_DMA_Init(void);
void CC3000_SPI_DMA_Channels(FunctionalState NewState);
void CC3000_CS_LOW(void);
void CC3000_CS_HIGH(void);

/* CC3000 Hardware related callbacks passed to wlan_init */
long CC3000_Read_Interrupt_Pin(void);
void CC3000_Interrupt_Enable(void);
void CC3000_Interrupt_Disable(void);
void CC3000_Write_Enable_Pin(unsigned char val);

/* Serial Flash Hardware related methods */
void sFLASH_SPI_DeInit(void);
void sFLASH_SPI_Init(void);
void sFLASH_CS_LOW(void);
void sFLASH_CS_HIGH(void);

/* USB hardware peripheral related methods */
void USB_Disconnect_Config(void);
void Set_USBClock(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config(FunctionalState NewState);

#define SYSTEM_FLAG(x) (x)
void Load_SystemFlags(void);
void Save_SystemFlags(void);

/* Internal Flash Clear Flags : used before calling Flash Erase/Program operations */
void FLASH_ClearFlags(void);
/* Internal Flash Write Protection routines */
FLASH_Status FLASH_WriteProtection_Enable(uint32_t FLASH_Pages);
FLASH_Status FLASH_WriteProtection_Disable(uint32_t FLASH_Pages);
/* Internal Flash Backup to sFlash and Restore from sFlash Helper routines */
void FLASH_Erase(void);
void FLASH_Backup(uint32_t sFLASH_Address);
void FLASH_Restore(uint32_t sFLASH_Address);
/* External Flash Helper routines */
uint32_t FLASH_PagesMask(uint32_t fileSize);
void FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize);
int FLASH_Update(const uint8_t *pBuffer, uint32_t sFLASH_Address, uint32_t bufferSize);
void FLASH_End(void);

/**
 * @param server_addr   The buffer to hold the data. Must be at least
 * EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH bytes.
 */
void FLASH_Read_ServerAddress_Data(void *server_addr);
void FLASH_Write_ServerAddress_Data(const uint8_t *buf);
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer);
void FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer);
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer);

bool FACTORY_Flash_Reset(void);
void BACKUP_Flash_Reset(void);
void OTA_Flash_Reset(void);

bool OTA_Flashed_GetStatus(void);
void OTA_Flashed_ResetStatus(void);

void Finish_Update(void);

uint16_t Bootloader_Get_Version(void);
void Bootloader_Update_Version(uint16_t bootloaderVersion);

/* External variables --------------------------------------------------------*/
extern uint8_t USE_SYSTEM_FLAGS;

extern volatile uint32_t TimingDelay;

extern uint16_t Bootloader_Version_SysFlag;
extern uint16_t NVMEM_SPARK_Reset_SysFlag;
extern uint16_t FLASH_OTA_Update_SysFlag;
extern uint16_t OTA_FLASHED_Status_SysFlag;
extern uint16_t Factory_Reset_SysFlag;
extern uint8_t dfu_on_no_firmware;
extern uint8_t Factory_Reset_Done_SysFlag;
extern uint8_t StartupMode_SysFlag;
extern uint8_t FeaturesEnabled_SysFlag;
extern uint32_t RCC_CSR_SysFlag;

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

#define KICK_WDT() IWDG_ReloadCounter()

void Save_Reset_Syndrome();

/**
 * Assume factory reset image present on the core.
 */
inline bool FLASH_IsFactoryResetAvailable() { return true; }

#ifdef __cplusplus
}
#endif


#endif  /*__HW_CONFIG_H*/
