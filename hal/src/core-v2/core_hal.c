/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
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

#include <stdint.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "syshealth_hal.h"
#if (USE_WICED_SDK == 1)
#include "wiced.h"
#endif

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t IWDG_SYSTEM_RESET;

/* Private function prototypes -----------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Extern function prototypes ------------------------------------------------*/
extern void linkme(void);

void HAL_Core_Init(void)
{
#if (USE_WICED_SDK == 1)
    wiced_init();
#endif
}


/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
    // this ensures the stm32_it.c functions aren't dropped by the linker, thinking
    // they are unused. Without this none of the interrupts handlers are linked.
    linkme();

    DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
    /* Set the Vector Table(VT) base location at 0x20000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);

    USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
    /* Disable the Serial Wire JTAG Debug Port SWJ-DP */
    //To Do
#endif

    Set_System();

    SysTick_Configuration();

    /* Enable CRC clock */
    //To Do

    RTC_Configuration();

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
    //To Do
}

void HAL_Core_Enter_Bootloader(void)
{
    //To Do
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
    //To Do
}

void HAL_Core_Execute_Stop_Mode(void)
{
    //To Do
}

void HAL_Core_Enter_Standby_Mode(void)
{
    //To Do
}

void HAL_Core_Execute_Standby_Mode(void)
{
    //To Do
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize)
{
    //To Do
    return 0;
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
#if (USE_WICED_SDK == 1)
    wiced_watchdog_kick();
#endif
}

#if (USE_WICED_SDK == 1)
/**
 * The entrypoint from FreeRTOS to our application.
 * 
 */
void application_start() {
    
    // while this is linked as a single image, assume c'tors are handled by the
    // WICED startup scripts.    
    HAL_Core_Config();
    
    app_setup_and_loop();
}
#else
int main() {
    app_setup_and_loop();
    return 0;
}
#endif
