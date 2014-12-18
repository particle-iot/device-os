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
#include "rtc_hal.h"
#include "rgbled.h"
#include "delay_hal.h"
#include "wiced.h"

/**
 * Start of interrupt vector table.
 */
extern char link_interrupt_vectors_location;

extern char link_ram_interrupt_vectors_location;
extern char link_ram_interrupt_vectors_location_end;

const unsigned SysTickIndex = 15;
const unsigned USART1Index = 53;

void SysTickOverride(void);
void HAL_USART1_Handler(void);

void override_interrupts(void) {
    
    memcpy(&link_ram_interrupt_vectors_location, &link_interrupt_vectors_location, &link_ram_interrupt_vectors_location_end-&link_ram_interrupt_vectors_location);
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[SysTickIndex] = (uint32_t)SysTickOverride;
    isrs[USART1Index] = (uint32_t)HAL_USART1_Handler;
    SCB->VTOR = (unsigned long)isrs;
}


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t IWDG_SYSTEM_RESET;

/* Private function prototypes -----------------------------------------------*/
void Mode_Button_EXTI_irq(void);

/* Extern variables ----------------------------------------------------------*/
extern __IO uint16_t BUTTON_DEBOUNCED_TIME[];

/* Extern function prototypes ------------------------------------------------*/
void HAL_Core_Init(void)
{
    wiced_core_init();
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
    DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
    //Currently this is done through WICED library API so commented.
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);
    USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
    /* Disable the Serial Wire JTAG Debug Port SWJ-DP */
    //To Do
#endif

    Set_System();

    override_interrupts();
    
    /* Register Mode Button Interrupt Handler (WICED hack for Mode Button usage) */
    HAL_EXTI_Register_Handler(BUTTON1_EXTI_LINE, Mode_Button_EXTI_irq);

    SysTick_Configuration();

    /* Enable CRC clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

    HAL_RTC_Configuration();
        
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
    //IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
    Load_SystemFlags();
#endif

#ifdef USE_SERIAL_FLASH
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
    system_flags.Factory_Reset_SysFlag = 0xAAAA;
    Save_SystemFlags();
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(void)
{
    RTC_WriteBackupRegister(RTC_BKP_DR10, 0xFFFF);
    system_flags.FLASH_OTA_Update_SysFlag = 0xFFFF;
    Save_SystemFlags();
    HAL_Core_System_Reset();
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
    wiced_watchdog_kick();
}

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

/**
 * The following tick hook will only get called if configUSE_TICK_HOOK
 * is set to 1 within FreeRTOSConfig.h
 */
void SysTickOverride(void)
{    
    void (*chain)(void) = (void (*)(void))((uint32_t*)&link_interrupt_vectors_location)[SysTickIndex];
    chain();    

    System1MsTick();

    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }

    if(HAL_SysTick_Handler)
    {
        HAL_SysTick_Handler();
    }
    
}

/**
 * @brief  This function handles EXTI2_IRQ Handler.
 * @param  None
 * @retval None
 */
void Mode_Button_EXTI_irq(void)
{
    /* Clear the EXTI line pending bit (cleared in WICED GPIO IRQ handler) */
    EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE);

    BUTTON_DEBOUNCED_TIME[BUTTON1] = 0x00;

    /* Disable BUTTON1 Interrupt */
    BUTTON_EXTI_Config(BUTTON1, DISABLE);

    /* Enable TIM2 CC1 Interrupt */
    TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
}

/**
 * @brief  This function handles TIM2_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM2_irq(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

        if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
        {
            BUTTON_DEBOUNCED_TIME[BUTTON1] += BUTTON_DEBOUNCE_INTERVAL;
        }
        else
        {
            /* Disable TIM2 CC1 Interrupt */
            TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);

            /* Enable BUTTON1 Interrupt */
            BUTTON_EXTI_Config(BUTTON1, ENABLE);
        }
    }
}

