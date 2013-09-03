/**
 ******************************************************************************
 * @file    main.c
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Main program body.
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_conf.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_prop.h"
#include "sst25vf_spi.h"
#include "spark_utilities.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t TimingMillis;

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
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0xC000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xC000);

	USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
    /* Disable the Serial Wire JTAG Debug Port SWJ-DP */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#endif

	Set_System();

#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);

#if defined (SPARK_RTC_ENABLE)
    RTC_Configuration();
#endif
#endif

#ifdef IWDG_RESET_ENABLE
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		IWDG_SYSTEM_RESET = 1;

		/* Clear reset flags */
		RCC_ClearFlag();
	}

	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();

	if(Flash_Update_SysFlag != 0xC000)
	{
		Flash_Update_SysFlag = 0xC000;
		Save_SystemFlags();
	}
#endif

#ifdef SPARK_WIRING_ENABLE
	if((IWDG_SYSTEM_RESET != 1) && (NULL != setup))
	{
		setup();
	}
#endif

#ifdef SPARK_WLAN_ENABLE
	SPARK_WLAN_Setup();
#endif

	/* Main loop */
	while (1)
	{
#ifdef SPARK_WLAN_ENABLE
		SPARK_WLAN_Loop();
#endif

#ifdef SPARK_WIRING_ENABLE
#ifdef SPARK_WLAN_ENABLE
		if(SPARK_DEVICE_ACKED && !IWDG_SYSTEM_RESET)
		{
#endif

			if(NULL != loop)
			{
				//Execute user application loop
				loop();
			}

			if(NULL != pHandleMessage)
			{
				pHandleMessage();
			}

			userFuncExecute();

			userVarReturn();

#ifdef SPARK_WLAN_ENABLE
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
	TimingMillis++;

    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }

    if (TimingLED != 0x00)
    {
        TimingLED--;
    }
    else if(SPARK_LED_FADE)
    {
#if defined (USE_SPARK_CORE_V02)
    	LED_Fade(LED_RGB);
    	TimingLED = 25;	//25ms
#endif
    }
    else if(!SPARK_DEVICE_ACKED)
    {
#if defined (USE_SPARK_CORE_V01)
    	LED_Toggle(LED1);
#elif defined (USE_SPARK_CORE_V02)
    	if(SPARK_LED_TOGGLE)
    		LED_Toggle(LED_RGB);
#endif
    	TimingLED = 100;	//100ms
    }
    else
    {
    	static __IO uint8_t SparkDeviceAckedLedOn = 0;
    	if(!SparkDeviceAckedLedOn)
    	{
#if defined (USE_SPARK_CORE_V01)
    		LED_On(LED1);//SPARK_DEVICE_ACKED
#elif defined (USE_SPARK_CORE_V02)
    		LED_SetRGBColor(RGB_COLOR_CYAN);
    		LED_On(LED_RGB);
    		SPARK_LED_FADE = 1;
#endif
    		SparkDeviceAckedLedOn = 1;
    	}
    }

	if(BUTTON_GetDebouncedTime(BUTTON1) >= 3000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);
    	//Enter Smart Config Process On Next System Reset
    	//Since socket connect() is currently blocking
#ifdef DFU_BUILD_ENABLE
		Smart_Config_SysFlag = 0xFFFF;
		Save_SystemFlags();
#else
		BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);
#endif

    	NVIC_SystemReset();
    }

#ifdef IWDG_RESET_ENABLE
	if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
	{
		TimingIWDGReload = 0;

	    /* Reload IWDG counter */
	    IWDG_ReloadCounter();
	}
	else
	{
		TimingIWDGReload++;
	}
#endif

#ifdef SPARK_WLAN_ENABLE
	SPARK_WLAN_Timing();
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
