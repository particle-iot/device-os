/**
 ******************************************************************************
 * @file    stm32_it.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and peripherals
 *          interrupt service routine.
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

/* Includes ------------------------------------------------------------------*/
#include "spark_macros.h"
#include "debug.h"
#include "stm32_it.h"
#include "main.h"
#include "usb_lib.h"
#include "usb_istr.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/
extern __IO uint16_t BUTTON_DEBOUNCED_TIME[];

/* Private function prototypes -----------------------------------------------*/
void Wiring_ADC1_2_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_USART1_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_USART2_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_I2C1_EV_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_I2C1_ER_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_SPI1_Interrupt_Handler(void) __attribute__ ((weak));
void Wiring_EXTI_Interrupt_Handler(uint8_t EXTI_Line_Number) __attribute__ ((weak));
void Wiring_RTC_Interrupt_Handler(void) __attribute__ ((weak));

void (*Wiring_TIM2_Interrupt_Handler)(void);
void (*Wiring_TIM3_Interrupt_Handler)(void);
void (*Wiring_TIM4_Interrupt_Handler)(void);

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/*******************************************************************************
 * Function Name  : NMI_Handler
 * Description    : This function handles NMI exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void NMI_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : HardFault_Handler
 * Description    : This function handles Hard Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
        PANIC(HardFault,"HardFault");
	while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : MemManage_Handler
 * Description    : This function handles Memory Manage exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
        PANIC(MemManage,"MemManage");
	while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : BusFault_Handler
 * Description    : This function handles Bus Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
        PANIC(BusFault,"BusFault");
        while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : UsageFault_Handler
 * Description    : This function handles Usage Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
        PANIC(UsageFault,"UsageFault");
	while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : SVC_Handler
 * Description    : This function handles SVCall exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SVC_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : DebugMon_Handler
 * Description    : This function handles Debug Monitor exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DebugMon_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : PendSV_Handler
 * Description    : This function handles PendSVC exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void PendSV_Handler(void)
{
}

/*******************************************************************************
 * Function Name  : SysTick_Handler
 * Description    : This function handles SysTick Handler.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Handler(void)
{
	System1MsTick();
	Timing_Decrement();
}

/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                       */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.S).                                                */
/******************************************************************************/

/*******************************************************************************
 * Function Name  : ADC1_2_IRQHandler
 * Description    : This function handles ADC1 and ADC2 global interrupts requests.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void ADC1_2_IRQHandler(void)
{
	if(NULL != Wiring_ADC1_2_Interrupt_Handler)
	{
		Wiring_ADC1_2_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : USART1_IRQHandler
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART1_IRQHandler(void)
{
	if(NULL != Wiring_USART1_Interrupt_Handler)
	{
		Wiring_USART1_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : USART2_IRQHandler
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART2_IRQHandler(void)
{
	if(NULL != Wiring_USART2_Interrupt_Handler)
	{
		Wiring_USART2_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : I2C1_EV_IRQHandler
 * Description    : This function handles I2C1 Event interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C1_EV_IRQHandler(void)
{
	if(NULL != Wiring_I2C1_EV_Interrupt_Handler)
	{
		Wiring_I2C1_EV_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : I2C1_ER_IRQHandler
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C1_ER_IRQHandler(void)
{
	if(NULL != Wiring_I2C1_ER_Interrupt_Handler)
	{
		Wiring_I2C1_ER_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : SPI1_IRQHandler
 * Description    : This function handles SPI1 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SPI1_IRQHandler(void)
{
	if(NULL != Wiring_SPI1_Interrupt_Handler)
	{
		Wiring_SPI1_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : EXTI0_IRQHandler
 * Description    : This function handles EXTI0 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line0);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(0);
		}
	}
}

/*******************************************************************************
 * Function Name  : EXTI1_IRQHandler
 * Description    : This function handles EXTI1 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line1) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line1);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(1);
		}
	}
}

/*******************************************************************************
 * Function Name  : EXTI2_IRQHandler
 * Description    : This function handles EXTI2 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI2_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line2) != RESET)//BUTTON1_EXTI_LINE
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line2);//BUTTON1_EXTI_LINE

		BUTTON_DEBOUNCED_TIME[BUTTON1] = 0x00;

		/* Disable BUTTON1 Interrupt */
		BUTTON_EXTI_Config(BUTTON1, DISABLE);

		/* Enable TIM1 CC4 Interrupt */
		TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);
	}
}

/*******************************************************************************
 * Function Name  : EXTI3_IRQHandler
 * Description    : This function handles EXTI3 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI3_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line3) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line3);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(3);
		}
	}
}

/*******************************************************************************
 * Function Name  : EXTI4_IRQHandler
 * Description    : This function handles EXTI4 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI4_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line4) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line4);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(4);
		}
	}
}

/*******************************************************************************
 * Function Name  : EXTI9_5_IRQHandler
 * Description    : This function handles EXTI9_5 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
	//EXTI_Line8 and EXTI_Line9 support is not required for CORE_V02

	if (EXTI_GetITStatus(EXTI_Line5) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line5);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(5);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line6) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line6);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(6);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line7) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line7);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(7);
		}
	}
}

/*******************************************************************************
 * Function Name  : EXTI15_10_IRQHandler
 * Description    : This function handles EXTI15_10 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI15_10_IRQHandler(void)
{
	//EXTI_Line10 and EXTI_Line12 support is not required for CORE_V02

	if (EXTI_GetITStatus(EXTI_Line13) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line13);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(13);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line14) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line14);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(14);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line15) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line15);

		if(NULL != Wiring_EXTI_Interrupt_Handler)
		{
			Wiring_EXTI_Interrupt_Handler(15);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line11) != RESET)//CC3000_WIFI_INT_EXTI_LINE
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line11);//CC3000_WIFI_INT_EXTI_LINE

		SPI_EXTI_IntHandler();
	}
}

/*******************************************************************************
 * Function Name  : TIM1_CC_IRQHandler
 * Description    : This function handles TIM1 Capture Compare interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void TIM1_CC_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_CC4) != RESET)
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_CC4);

		if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
		{
			BUTTON_DEBOUNCED_TIME[BUTTON1] += BUTTON_DEBOUNCE_INTERVAL;
		}
		else
		{
			/* Disable TIM1 CC4 Interrupt */
			TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);

			/* Enable BUTTON1 Interrupt */
			BUTTON_EXTI_Config(BUTTON1, ENABLE);
		}
	}
}

/*******************************************************************************
 * Function Name  : TIM2_IRQHandler
 * Description    : This function handles TIM2 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void TIM2_IRQHandler(void)
{
	if(NULL != Wiring_TIM2_Interrupt_Handler)
	{
		Wiring_TIM2_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : TIM3_IRQHandler
 * Description    : This function handles TIM3 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void TIM3_IRQHandler(void)
{
	if(NULL != Wiring_TIM3_Interrupt_Handler)
	{
		Wiring_TIM3_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : TIM4_IRQHandler
 * Description    : This function handles TIM4 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void TIM4_IRQHandler(void)
{
	if(NULL != Wiring_TIM4_Interrupt_Handler)
	{
		Wiring_TIM4_Interrupt_Handler();
	}
}

/*******************************************************************************
 * Function Name  : RTC_IRQHandler
 * Description    : This function handles RTC global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void RTC_IRQHandler(void)
{
	if(RTC_GetITStatus(RTC_IT_SEC) != RESET)
	{
		/* Clear the RTC Second Interrupt pending bit */
		RTC_ClearITPendingBit(RTC_IT_SEC);

		if(NULL != Wiring_RTC_Interrupt_Handler)
		{
			Wiring_RTC_Interrupt_Handler();
		}

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
}

/*******************************************************************************
* Function Name  : RTCAlarm_IRQHandler
* Description    : This function handles RTC Alarm interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTCAlarm_IRQHandler(void)
{
	if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
	{
	        /* Wake up from Spark.sleep mode(SLEEP_MODE_WLAN) */
		SPARK_WLAN_SLEEP = 0;

		/* Clear EXTI line17 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line17);

		/* Check if the Wake-Up flag is set */
		if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
		{
			/* Clear Wake Up flag */
			PWR_ClearFlag(PWR_FLAG_WU);
		}

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Clear RTC Alarm interrupt pending bit */
		RTC_ClearITPendingBit(RTC_IT_ALR);

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
}

/*******************************************************************************
 * Function Name  : DMA1_Channel5_IRQHandler
 * Description    : This function handles SPI2_TX_DMA interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA1_Channel5_IRQHandler(void)
{
	SPI_DMA_IntHandler();
}

/*******************************************************************************
* Function Name  : USB_LP_CAN1_RX0_IRQHandler
* Description    : This function handles USB Low Priority interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	USB_Istr();
}


/*******************************************************************************
 * Function Name  : PPP_IRQHandler
 * Description    : This function handles PPP interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
/*
 void PPP_IRQHandler(void) {
 }
 */
