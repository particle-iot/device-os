/**
 ******************************************************************************
 * @file    stm32_it.c
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and peripherals
 *          interrupt service routine.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
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
void USER_EXTI_Line_Handler(uint8_t EXTI_Line_Number) __attribute__ ((weak));

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
	Timing_Decrement();
}

/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.S).                                            */
/******************************************************************************/

#if defined (USE_SPARK_CORE_V02)
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
//		/* If counter is equal to 86339: one day was elapsed */
//		if((RTC_GetCounter() / 3600 == 23)
//				&& (((RTC_GetCounter() % 3600) / 60) == 59)
//				&& (((RTC_GetCounter() % 3600) % 60) == 59)) /* 23*3600 + 59*60 + 59 = 86339 */
//		{
//			/* Wait until last write operation on RTC registers has finished */
//			RTC_WaitForLastTask();
//
//			/* Reset counter value */
//			RTC_SetCounter(0x0);
//
//			/* Wait until last write operation on RTC registers has finished */
//			RTC_WaitForLastTask();
//
//			/* Increment no_of_days_elapsed variable here */
//		}

		/* Clear the RTC Second Interrupt pending bit */
		RTC_ClearITPendingBit(RTC_IT_SEC);

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
#endif

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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(0);
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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(1);
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
#if defined (USE_SPARK_CORE_V02)
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
#endif
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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(3);
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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(4);
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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(5);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line6) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line6);

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(6);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line7) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line7);

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(7);
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

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(13);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line14) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line14);

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(14);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line15) != RESET)
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line15);

		if(NULL != USER_EXTI_Line_Handler)
		{
			USER_EXTI_Line_Handler(15);
		}
	}

	if (EXTI_GetITStatus(EXTI_Line11) != RESET)//CC3000_WIFI_INT_EXTI_LINE
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line11);//CC3000_WIFI_INT_EXTI_LINE

		SPI_EXTI_IntHandler();
	}

#if defined (USE_SPARK_CORE_V01)
	if (EXTI_GetITStatus(EXTI_Line10) != RESET)//BUTTON1_EXTI_LINE
	{
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(EXTI_Line10);//BUTTON1_EXTI_LINE

		BUTTON_DEBOUNCED_TIME[BUTTON1] = 0x00;

		/* Disable BUTTON1 Interrupt */
		BUTTON_EXTI_Config(BUTTON1, DISABLE);

		/* Enable TIM1 CC4 Interrupt */
		TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);
	}
#endif
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

//Implement an USART RX ISR here
void USART2_IRQHandler(void)
  {
    // check if the USART1 receive interrupt flag was set
	if( USART_GetITStatus(USART1, USART_IT_RXNE) )
	{
      //unsigned char c = USART_ReceiveData(USART1);
      //store_char(c, &rx_buffer);
   } 
	else 
	{
      //unsigned char c = USART_ReceiveData(USART1);
   }
  }
