/**
 ******************************************************************************
 * @file    stm32_it.h
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   This file contains the headers of the interrupt handlers.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_IT_H
#define __STM32_IT_H

extern "C" {

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "cc3000_spi.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
#if defined (USE_SPARK_CORE_V02)
void RTC_IRQHandler(void);
void RTCAlarm_IRQHandler(void);
#endif
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);

}

#endif /* __STM32_IT_H */

