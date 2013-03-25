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
#include "cc3000_common.h"

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

typedef enum
{
	CC3000_DMA_TX = 0, CC3000_DMA_RX = 1
} CC3000_DMADirection_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/
/**
 * @brief  Select CC3000: ChipSelect pin low
 */
#define CC3000_CS_LOW()     GPIO_ResetBits(CC3000_SPI_CS_GPIO_PORT, CC3000_SPI_CS_PIN)
/**
 * @brief  Deselect CC3000: ChipSelect pin high
 */
#define CC3000_CS_HIGH()    GPIO_SetBits(CC3000_SPI_CS_GPIO_PORT, CC3000_SPI_CS_PIN)

/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void NVIC_Configuration(void);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);

void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
uint32_t BUTTON_GetState(Button_TypeDef Button);

/* CC3000 Hardware related methods */
void CC3000_SPI_DeInit(void);
void CC3000_SPI_Init(void);
void CC3000_DMA_Config(CC3000_DMADirection_TypeDef Direction, uint8_t* buffer, uint16_t NumData);
void CC3000_SPI_DMA_Init(void);
void CC3000_SPI_DMA_Channels(FunctionalState NewState);

/* CC3000 Hardware related callbacks passed to wlan_init */
long CC3000_Read_Interrupt_Pin(void);
void CC3000_Interrupt_Enable(void);
void CC3000_Interrupt_Disable(void);
void CC3000_Write_Enable_Pin(unsigned char val);

/* External variables --------------------------------------------------------*/
extern unsigned char wlan_rx_buffer[];
extern unsigned char wlan_tx_buffer[];

#endif  /*__HW_CONFIG_H*/
