/**
  ******************************************************************************
  * @file    hw_config.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Hardware Configuration & Setup
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
/* Includes ------------------------------------------------------------------*/

#include "platform_config.h"
#include "sst25vf_spi.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
	LED1 = 0, LED2 = 1
} Led_TypeDef;

typedef enum
{
	BUTTON1 = 0, BUTTON2 = 1
} Button_TypeDef;

typedef enum
{
	BUTTON_MODE_GPIO = 0, BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Flash memory address where various firmwares are located */
#define USB_DFU_ADDRESS	0x08000000
#define OTA_DFU_ADDRESS	0x08005000
#define MARVIN_ADDRESS	0x0800A000

/* Select sFLASH: Chip Select pin low */
#define sFLASH_CS_LOW()		GPIO_ResetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_PIN)
/* Deselect sFLASH: Chip Select pin high */
#define sFLASH_CS_HIGH()	GPIO_SetBits(sFLASH_MEM_CS_GPIO_PORT, sFLASH_MEM_CS_PIN)

/* Exported functions ------------------------------------------------------- */

void Set_System(void);
void NVIC_Configuration(void);

void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState);
uint8_t BUTTON_GetState(Button_TypeDef Button);
uint8_t BUTTON_GetDebouncedState(Button_TypeDef Button);

/* Serial Flash Hardware related methods */
void sFLASH_SPI_DeInit(void);
void sFLASH_SPI_Init(void);

/* USB hardware peripheral related methods */
void USB_Disconnect_Config(void);
void Set_USBClock(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config(FunctionalState NewState);
void Reset_Device(void);
void Get_SerialNum(void);

/* External variables --------------------------------------------------------*/

#endif  /*__HW_CONFIG_H*/
