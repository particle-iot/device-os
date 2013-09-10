/**
 ******************************************************************************
 * @file    hw_config.h
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Hardware Configuration & Setup
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
/* Includes ------------------------------------------------------------------*/

#include "platform_config.h"
#include "sst25vf_spi.h"
#include "cc3000_common.h"
#include "usb_type.h"

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
	LED1 = 0, LED2 = 1, LED3 = 2, LED4 = 3, LED3_LED4_LED2 = 231
} Led_TypeDef;

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

/* External Flash memory address where server public RSA key resides */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_ADDRESS	((uint32_t)0x01000)
/* Length in bytes of DER-encoded 2048-bit RSA public key */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH		(294)
/* External Flash memory address where core private RSA key resides */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_ADDRESS		((uint32_t)0x02000)
/* Length in bytes of DER-encoded 2048-bit RSA private key */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH		(1191)

#if defined (USE_SPARK_CORE_V02)
//Extended LED Types
#define LED_RED				LED3
#define LED_GREEN			LED4
#define LED_BLUE			LED2
#define LED_RGB				LED3_LED4_LED2
#define LED_USER			LED1

//RGB Basic Colors
#define RGB_COLOR_RED		0xFF0000
#define RGB_COLOR_GREEN		0x00FF00
#define RGB_COLOR_BLUE		0x0000FF
#define RGB_COLOR_YELLOW	0xFFFF00
#define RGB_COLOR_CYAN		0x00FFFF
#define RGB_COLOR_MAGENTA	0xFF00FF
#define RGB_COLOR_WHITE		0xFFFFFF
#endif

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

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void NVIC_Configuration(void);

void Delay(__IO uint32_t nTime);

#if defined (USE_SPARK_CORE_V02)
void RTC_Configuration(void);
void Enter_STANDBY_Mode(void);
#endif

void IWDG_Reset_Enable(uint32_t msTimeout);

void DIO_Init(DIO_TypeDef Dx);
DIO_Error_TypeDef DIO_SetState(DIO_TypeDef Dx, DIO_State_TypeDef State);

void UI_Timer_Configure(void);

#if defined (USE_SPARK_CORE_V02)
void LED_SetRGBColor(uint32_t RGB_Color);
#endif

void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void LED_Fade(Led_TypeDef Led);

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

/* Internal Flash Backup to sFlash and Restore from sFlash Helper routines */
void FLASH_Backup(uint32_t sFLASH_Address);
void FLASH_Restore(uint32_t sFLASH_Address);
/* External Flash Helper routines */
void FLASH_Begin(uint32_t sFLASH_Address);
void FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize);
void FLASH_End(void);
void FLASH_Read_ServerPublicKey(uint8_t *keyBuffer);
void FLASH_Read_CorePrivateKey(uint8_t *keyBuffer);

void Factory_Flash_Reset(void);
void OTA_Flash_Update(void);

void Reset_Device(void);

/* Hardware CRC32 calculation */
uint32_t Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize);

void Get_Unique_Device_ID(uint8_t *Device_ID);

/* External variables --------------------------------------------------------*/
extern int8_t OTA_UPDATE_MODE;
extern uint8_t DFU_DEVICE_MODE;
extern uint8_t FACTORY_RESET_MODE;
extern uint8_t USE_SYSTEM_FLAGS;

extern __IO uint32_t TimingDelay;
extern __IO uint32_t TimingLED;
extern __IO uint32_t TimingBUTTON;
extern __IO uint32_t TimingIWDGReload;

extern __IO uint8_t IWDG_SYSTEM_RESET;

extern uint16_t NetApp_Timeout_SysFlag;
extern uint16_t Smart_Config_SysFlag;
extern uint16_t Flash_Update_SysFlag;

extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

#endif  /*__HW_CONFIG_H*/
