/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * 
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * 
 * @brief   Main program body.
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */
  
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "debug.h"
#include "spark_utilities.h"
extern "C" {
#include "usb_conf.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_prop.h"
#include "sst25vf_spi.h"
}

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint32_t TimingFlashUpdateTimeout;

uint8_t  USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;

uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
uint16_t USB_Rx_length = 0;
uint16_t USB_Rx_ptr = 0;

uint8_t  USB_Tx_State = 0;
uint8_t  USB_Rx_State = 0;

uint32_t USB_USART_BaudRate = 9600;

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

/* Extern variables ----------------------------------------------------------*/
extern LINE_CODING linecoding;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : SparkCoreConfig.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void SparkCoreConfig(void)
{
        DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0x5000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x5000);

	USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
	/* Disable the Serial Wire JTAG Debug Port SWJ-DP */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#endif

	Set_System();

	SysTick_Configuration();

	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	LED_RGB_OVERRIDE = 1;
#endif

#if defined (SPARK_RTC_ENABLE)
	RTC_Configuration();
#endif

        /* Execute Stop mode if STOP mode flag is set via Spark.sleep(pin, mode) */
        Enter_STOP_Mode();

        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(LED_RGB);
        SPARK_LED_FADE = 1;

#ifdef IWDG_RESET_ENABLE
	// ToDo this needs rework for new bootloader
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		IWDG_SYSTEM_RESET = 1;

		/* Clear reset flags */
		RCC_ClearFlag();
	}

	/* We are duplicating the IWDG call here for compatibility with old bootloader */
	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();
#endif

#ifdef SPARK_SFLASH_ENABLE
	sFLASH_Init();
#endif

#ifdef SPARK_WLAN_ENABLE
	/* Start Spark Wlan and connect to Wifi Router by default */
	SPARK_WLAN_SETUP = 1;

	/* Connect to Spark Cloud by default */
	SPARK_CLOUD_CONNECT = 1;
#endif
}

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
  // We have running firmware, otherwise we wouldn't have gotten here
  DECLARE_SYS_HEALTH(ENTERED_Main);
  DEBUG("Hello from Spark!");

#ifdef SPARK_WLAN_ENABLE
  if (SPARK_WLAN_SETUP)
  {
    SPARK_WLAN_Setup(Multicast_Presence_Announcement);
  }
#endif

  /* Main loop */
  while (1)
  {
#ifdef SPARK_WLAN_ENABLE
    if(SPARK_WLAN_SETUP)
    {
      DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
      SPARK_WLAN_Loop();
    }
#endif

#ifdef SPARK_WIRING_ENABLE
		static uint8_t SPARK_WIRING_APPLICATION = 0;
#ifdef SPARK_WLAN_ENABLE
		if(!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP || !SPARK_CLOUD_CONNECT || SPARK_CLOUD_CONNECTED || SPARK_WIRING_APPLICATION)
		{
			if(!SPARK_FLASH_UPDATE && !IWDG_SYSTEM_RESET)
			{
#endif
				if((SPARK_WIRING_APPLICATION != 1) && (NULL != setup))
				{
					//Execute user application setup only once
				        DECLARE_SYS_HEALTH(ENTERED_Setup);
					setup();
					SPARK_WIRING_APPLICATION = 1;
				}

				if(NULL != loop)
				{
					//Execute user application loop
			                DECLARE_SYS_HEALTH(ENTERED_Loop);
					loop();
                                        DECLARE_SYS_HEALTH(RAN_Loop);
				}
#ifdef SPARK_WLAN_ENABLE
			}
		}
#endif
#endif
	}
}

/*******************************************************************************
 * Function Name  : Timing_Decrement
 * Description    : Decrements the various Timing variables related to SysTick.
 * Input          : None
 * Output         : Timing
 * Return         : None
 *******************************************************************************/
void Timing_Decrement(void)
{
	if (TimingDelay != 0x00)
	{
		TimingDelay--;
	}

#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	//Just needed in case LED_RGB_OVERRIDE is set to 0 by accident
	if (LED_RGB_OVERRIDE == 0)
	{
		LED_RGB_OVERRIDE = 1;
		LED_Off(LED_RGB);
	}
#endif

	if (LED_RGB_OVERRIDE != 0)
	{
		if ((LED_Spark_Signal != 0) && (NULL != LED_Signaling_Override))
		{
			LED_Signaling_Override();
		}
	}
	else if (TimingLED != 0x00)
	{
		TimingLED--;
	}
	else if(WLAN_SMART_CONFIG_START || SPARK_FLASH_UPDATE || Spark_Error_Count)
	{
		//Do nothing
	}
	else if(SPARK_LED_FADE)
	{
		LED_Fade(LED_RGB);
		TimingLED = 20;//Breathing frequency kept constant
	}
	else if(SPARK_WLAN_SETUP && SPARK_CLOUD_CONNECTED)
	{
#if defined (RGB_NOTIFICATIONS_CONNECTING_ONLY)
		LED_Off(LED_RGB);
#else
		LED_SetRGBColor(RGB_COLOR_CYAN);
		LED_On(LED_RGB);
		SPARK_LED_FADE = 1;
#endif
	}
	else
	{
		LED_Toggle(LED_RGB);
		if(SPARK_CLOUD_SOCKETED)
			TimingLED = 50;		//50ms
		else
			TimingLED = 100;	//100ms
	}

#ifdef SPARK_WLAN_ENABLE
	if(!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP)
	{
		//Do nothing
	}
	else if(SPARK_FLASH_UPDATE)
	{
		if (TimingFlashUpdateTimeout >= TIMING_FLASH_UPDATE_TIMEOUT)
		{
			//Reset is the only way now to recover from stuck OTA update
			NVIC_SystemReset();
		}
		else
		{
			TimingFlashUpdateTimeout++;
		}
	}
	else if(!WLAN_SMART_CONFIG_START && BUTTON_GetDebouncedTime(BUTTON1) >= 3000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);

		if(!SPARK_WLAN_SLEEP)
		{
			WiFi.listen();
		}
	}
	else if(BUTTON_GetDebouncedTime(BUTTON1) >= 7000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);

		WLAN_DELETE_PROFILES = 1;
	}
#endif

#ifdef IWDG_RESET_ENABLE
	if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
	{
		TimingIWDGReload = 0;

		/* Reload WDG counter */
		KICK_WDT();
		DECLARE_SYS_HEALTH(0xFFFF);
	}
	else
	{
		TimingIWDGReload++;
	}
#endif
}

/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
	linecoding.bitrate = baudRate;
	USB_Disconnect_Config();
	USB_Cable_Config(DISABLE);
	Delay_Microsecond(100000);
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			return (USB_Rx_length - USB_Rx_ptr);
		}
	}

	return 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			if((USB_Rx_length - USB_Rx_ptr) == 1)
			{
				USB_Rx_State = 0;

				/* Enable the receive of data on EP3 */
				SetEPRxValid(ENDP3);
			}

			return USB_Rx_Buffer[USB_Rx_ptr++];
		}
	}

	return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
	if(bDeviceState == CONFIGURED)
	{
		USART_Rx_Buffer[USART_Rx_ptr_in] = Data;

		USART_Rx_ptr_in++;

		/* To avoid buffer overflow */
		if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
		{
			USART_Rx_ptr_in = 0;
		}

		if(CC3000_Read_Interrupt_Pin())
		{
			//Delay 100us to avoid losing the data
			Delay_Microsecond(100);
		}
	}
}

/*******************************************************************************
 * Function Name  : Handle_USBAsynchXfer.
 * Description    : send data to USB.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Handle_USBAsynchXfer (void)
{

	uint16_t USB_Tx_ptr;
	uint16_t USB_Tx_length;

	if(USB_Tx_State != 1)
	{
		if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
		{
			USART_Rx_ptr_out = 0;
		}

		if(USART_Rx_ptr_out == USART_Rx_ptr_in)
		{
			USB_Tx_State = 0;
			return;
		}

		if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
		{
			USART_Rx_length = USART_RX_DATA_SIZE - USART_Rx_ptr_out;
		}
		else
		{
			USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
		}

		if (USART_Rx_length > VIRTUAL_COM_PORT_DATA_SIZE)
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;

			USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;
			USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;
		}
		else
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = USART_Rx_length;

			USART_Rx_ptr_out += USART_Rx_length;
			USART_Rx_length = 0;
		}
		USB_Tx_State = 1;
		UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
		SetEPTxCount(ENDP1, USB_Tx_length);
		SetEPTxValid(ENDP1);
	}

}

/*******************************************************************************
 * Function Name  : Get_SerialNum.
 * Description    : Create the serial number string descriptor.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Get_SerialNum(void)
{
	uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

	Device_Serial0 = *(uint32_t*)ID1;
	Device_Serial1 = *(uint32_t*)ID2;
	Device_Serial2 = *(uint32_t*)ID3;

	Device_Serial0 += Device_Serial2;

	if (Device_Serial0 != 0)
	{
		IntToUnicode (Device_Serial0, &Virtual_Com_Port_StringSerial[2] , 8);
		IntToUnicode (Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
	}
}

/*******************************************************************************
 * Function Name  : HexToChar.
 * Description    : Convert Hex 32Bits value into char.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
	uint8_t idx = 0;

	for( idx = 0 ; idx < len ; idx ++)
	{
		if( ((value >> 28)) < 0xA )
		{
			pbuf[ 2* idx] = (value >> 28) + '0';
		}
		else
		{
			pbuf[2* idx] = (value >> 28) + 'A' - 10;
		}

		value = value << 4;

		pbuf[ 2* idx + 1] = 0;
	}
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
