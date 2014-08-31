/**
 ******************************************************************************
 * @file    hw_config.h
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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
#include "cc3000_common.h"
#include "usb_type.h"
#include "rgbled.h"

enum SpiBusOwner {
    BUS_OWNER_NONE = 0,
    BUS_OWNER_CC3000 = 1,
    BUS_OWNER_SFLASH = 2
};


/* Exported types ------------------------------------------------------------*/
typedef enum
{
	LOW = 0, HIGH = 1
} DIO_State_TypeDef;

typedef enum
{
	FAIL = -1, OK = 0
} DIO_Error_TypeDef;

typedef enum
{
	D0 = 0, D1 = 1, D2 = 2, D3 = 3, D4 = 4, D5 = 5, D6 = 6, D7 = 7
} DIO_TypeDef;

typedef enum
{
	BUTTON1 = 0, BUTTON2 = 1
} Button_TypeDef;

typedef enum
{
	BUTTON_MODE_GPIO = 0, BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

typedef enum
{
	CC3000_DMA_TX = 0, CC3000_DMA_RX = 1
} CC3000_DMADirection_TypeDef;

typedef enum
{
  IP_ADDRESS = 0, DOMAIN_NAME = 1, INVALID_INTERNET_ADDRESS = 0xff
} Internet_Address_TypeDef;

typedef struct ServerAddress {
  Internet_Address_TypeDef addr_type;
  union {
    char domain[127];
    uint32_t ip;
  };
} ServerAddress;

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Internal Flash memory address where various firmwares are located */
#define USB_DFU_ADDRESS				((uint32_t)0x08000000)
#define CORE_FW_ADDRESS				((uint32_t)0x08005000)
/* Internal Flash memory address where the System Flags will be saved and loaded from  */
#define SYSTEM_FLAGS_ADDRESS		((uint32_t)0x08004C00)
/* Internal Flash end memory address */
#define INTERNAL_FLASH_END_ADDRESS	((uint32_t)0x08020000)	//For 128KB Internal Flash
/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE	((uint16_t)0x400)

/* External Flash block size allocated for firmware storage */
#define EXTERNAL_FLASH_BLOCK_SIZE	((uint32_t)0x20000)	//128KB  (Maximum Internal Flash Size)
/* External Flash memory address where Factory programmed core firmware is located */
#define EXTERNAL_FLASH_FAC_ADDRESS	((uint32_t)EXTERNAL_FLASH_BLOCK_SIZE)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_FAC_ADDRESS))
/* External Flash memory address where OTA upgraded core firmware will be saved */
#define EXTERNAL_FLASH_OTA_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS))

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

/* USB Config : IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | \
                 CNTR_WKUPM | \
                 CNTR_SUSPM | \
                 CNTR_ERRM  | \
                 CNTR_SOFM  | \
                 CNTR_ESOFM | \
                 CNTR_RESETM  \
                )

#define TIMING_IWDG_RELOAD	1000 //1sec

#define SYSTEM_US_TICKS		(SystemCoreClock / 1000000)//cycles per microsecond

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void NVIC_Configuration(void);
void SysTick_Configuration(void);
void Delay(uint32_t nTime);
void Delay_Microsecond(uint32_t uSec);

typedef uint32_t system_tick_t;
void System1MsTick(void);
system_tick_t GetSystem1MsTick(void);

void RTC_Configuration(void);
void Enter_STANDBY_Mode(void);

void IWDG_Reset_Enable(uint32_t msTimeout);

void DIO_Init(DIO_TypeDef Dx);
DIO_Error_TypeDef DIO_SetState(DIO_TypeDef Dx, DIO_State_TypeDef State);

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

void Load_SystemFlags(void);
void Save_SystemFlags(void);

/* Internal Flash Write Protection routines */
void FLASH_WriteProtection_Enable(uint32_t FLASH_Pages);
void FLASH_WriteProtection_Disable(uint32_t FLASH_Pages);
/* Internal Flash Backup to sFlash and Restore from sFlash Helper routines */
void FLASH_Erase(void);
void FLASH_Backup(uint32_t sFLASH_Address);
void FLASH_Restore(uint32_t sFLASH_Address);
/* External Flash Helper routines */
void FLASH_Begin(uint32_t sFLASH_Address);
uint16_t FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize);
void FLASH_End(void);
void FLASH_Read_ServerAddress(ServerAddress *server_addr);
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer);
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer);

void FACTORY_Flash_Reset(void);
void BACKUP_Flash_Reset(void);
void OTA_Flash_Reset(void);

bool OTA_Flashed_GetStatus(void);
void OTA_Flashed_ResetStatus(void);

void Finish_Update(void);

/* Hardware CRC32 calculation */
uint32_t Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize);

void Get_Unique_Device_ID(uint8_t *Device_ID);

/* External variables --------------------------------------------------------*/
extern uint8_t USE_SYSTEM_FLAGS;

extern volatile uint32_t TimingDelay;
extern volatile uint32_t TimingLED;
extern volatile uint32_t TimingBUTTON;
extern volatile  uint32_t TimingIWDGReload;

extern __IO uint8_t IWDG_SYSTEM_RESET;

extern uint8_t LED_RGB_OVERRIDE;

extern uint16_t CORE_FW_Version_SysFlag;
extern uint16_t NVMEM_SPARK_Reset_SysFlag;
extern uint16_t FLASH_OTA_Update_SysFlag;
extern uint16_t Factory_Reset_SysFlag;

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

enum eSystemHealth {
  FIRST_RETRY = 1,
  SECOND_RETRY = 2,
  THIRD_RETRY = 3,
  ENTERED_SparkCoreConfig,
  ENTERED_Main,
  ENTERED_WLAN_Loop,
  ENTERED_Setup,
  ENTERED_Loop,
  RAN_Loop,
  PRESERVE_APP,
};

#define SET_SYS_HEALTH(health) BKP_WriteBackupRegister(BKP_DR1, (health))
#define GET_SYS_HEALTH() BKP_ReadBackupRegister(BKP_DR1)
extern uint16_t sys_health_cache;
#define DECLARE_SYS_HEALTH(health)  do { if ((health) > sys_health_cache) {SET_SYS_HEALTH(sys_health_cache=(health));}} while(0)
#define KICK_WDT() IWDG_ReloadCounter()
#endif  /*__HW_CONFIG_H*/
