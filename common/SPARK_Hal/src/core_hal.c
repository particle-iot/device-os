/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
#include <stdint.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "syshealth_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t IWDG_SYSTEM_RESET;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
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
#if !defined (RGB_NOTIFICATIONS_ON) && defined (RGB_NOTIFICATIONS_OFF)
  LED_RGB_OVERRIDE = 1;
#endif

#if defined (SPARK_RTC_ENABLE)
  RTC_Configuration();
#endif

  /* Execute Stop mode if STOP mode flag is set via Spark.sleep(pin, mode) */
  HAL_Core_Execute_Stop_Mode();

  LED_SetRGBColor(RGB_COLOR_WHITE);
  LED_On(LED_RGB);

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
}

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    bool pressedState = false;

    if(BUTTON_GetDebouncedTime(BUTTON1) >= pressedMillisDuration)
    {
        pressedState = true;
    }

    return pressedState;
}

void HAL_Core_Mode_Button_Reset(void)
{
    BUTTON_ResetDebouncedState(BUTTON1);
}

void HAL_Core_System_Reset(void)
{
  NVIC_SystemReset();
}

void HAL_Core_Factory_Reset(void)
{
  Factory_Reset_SysFlag = 0xAAAA;
  Save_SystemFlags();
  HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(void)
{
  BKP_WriteBackupRegister(BKP_DR10, 0xFFFF);
  FLASH_OTA_Update_SysFlag = 0xFFFF;
  Save_SystemFlags();
  HAL_Core_System_Reset();
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
  if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
  {
    uint16_t BKP_DR9_Data = wakeUpPin;//set wakeup pin mumber
    BKP_DR9_Data |= (edgeTriggerMode << 8);//set edge trigger mode
    BKP_DR9_Data |= (0xA << 12);//set stop mode flag

    /*************************************************/
    //BKP_DR9_Data: 0xAXXX
    //                ||||
    //                ||----- octet wakeUpPin number
    //                |------ nibble edgeTriggerMode
    //                ------- nibble stop mode flag
    /*************************************************/

    /* Execute Stop mode on next system reset */
    BKP_WriteBackupRegister(BKP_DR9, BKP_DR9_Data);

    /* Reset System */
    NVIC_SystemReset();
  }
}

void HAL_Core_Execute_Stop_Mode(void)
{
  if((BKP_ReadBackupRegister(BKP_DR9) >> 12) == 0xA)
  {
    uint16_t wakeUpPin = BKP_ReadBackupRegister(BKP_DR9) & 0xFF;
    InterruptMode edgeTriggerMode = (InterruptMode)((BKP_ReadBackupRegister(BKP_DR9) >> 8) & 0x0F);

    /* Clear Stop mode system flag */
    BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

    if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
    {
      PinMode wakeUpPinMode = INPUT;

      /* Set required pinMode based on edgeTriggerMode */
      switch(edgeTriggerMode)
      {
        case CHANGE:
          wakeUpPinMode = INPUT;
          break;

        case RISING:
          wakeUpPinMode = INPUT_PULLDOWN;
          break;

        case FALLING:
          wakeUpPinMode = INPUT_PULLUP;
          break;
      }
      HAL_Pin_Mode(wakeUpPin, wakeUpPinMode);

      /* Configure EXTI Interrupt : wake-up from stop mode using pin interrupt */
      HAL_Interrupts_Attach(wakeUpPin, NULL, edgeTriggerMode);

      /* Request to enter STOP mode with regulator in low power mode */
      PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

      /* At this stage the system has resumed from STOP mode */
      /* Enable HSE, PLL and select PLL as system clock source after wake-up from STOP */

      /* Enable HSE */
      RCC_HSEConfig(RCC_HSE_ON);

      /* Wait till HSE is ready */
      if(RCC_WaitForHSEStartUp() != SUCCESS)
      {
        /* If HSE startup fails try to recover by system reset */
        NVIC_SystemReset();
      }

      /* Enable PLL */
      RCC_PLLCmd(ENABLE);

      /* Wait till PLL is ready */
      while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

      /* Select PLL as system clock source */
      RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

      /* Wait till PLL is used as system clock source */
      while(RCC_GetSYSCLKSource() != 0x08);
    }
  }
}

void HAL_Core_Enter_Standby_Mode(void)
{
  /* Execute Standby mode on next system reset */
  BKP_WriteBackupRegister(BKP_DR9, 0xA5A5);

  /* Reset System */
  NVIC_SystemReset();
}

void HAL_Core_Execute_Standby_Mode(void)
{
  /* Should we execute System Standby mode */
  if(BKP_ReadBackupRegister(BKP_DR9) == 0xA5A5)
  {
    /* Clear Standby mode system flag */
    BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

    /* Request to enter STANDBY mode */
    PWR_EnterSTANDBYMode();

    /* Following code will not be reached */
    while(1);
  }
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize)
{
    /* Hardware CRC32 calculation */
    uint32_t i, j;
    uint32_t Data;

    CRC_ResetDR();

    i = bufferSize >> 2;

    while (i--)
    {
        Data = *((uint32_t *)pBuffer);
        pBuffer += 4;

        Data = __RBIT(Data);//reverse the bit order of input Data
        CRC->DR = Data;
    }

    Data = CRC->DR;
    Data = __RBIT(Data);//reverse the bit order of output Data

    i = bufferSize & 3;

    while (i--)
    {
        Data ^= (uint32_t)*pBuffer++;

        for (j = 0 ; j < 8 ; j++)
        {
            if (Data & 1)
                Data = (Data >> 1) ^ 0xEDB88320;
            else
                Data >>= 1;
        }
    }

    Data ^= 0xFFFFFFFF;

    return Data;
}

// todo find a technique that allows accessor functions to be inlined while still keeping
// hardware independence.
bool HAL_watchdog_reset_flagged() 
{
    return IWDG_SYSTEM_RESET;
}

void HAL_Notify_WDT()
{
    KICK_WDT();
}
