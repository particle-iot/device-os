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
#include "core_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/
extern volatile uint8_t SPARK_WLAN_SETUP;
extern volatile uint8_t SPARK_CLOUD_CONNECT;
extern volatile uint8_t SPARK_LED_FADE;

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
#if !defined (RGB_NOTIFICATIONS_ON)     && defined (RGB_NOTIFICATIONS_OFF)
        LED_RGB_OVERRIDE = 1;
#endif

#if defined (SPARK_RTC_ENABLE)
        RTC_Configuration();
#endif

        /* Execute Stop mode if STOP mode flag is set via Spark.sleep(pin, mode) */
        HAL_Core_Execute_Stop_Mode();

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
