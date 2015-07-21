/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stdatomic.h>
#include "core_hal_stm32f2xx.h"
#include "core_hal.h"
#include "stm32f2xx.h"
#include <string.h>
#include "hw_config.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/
/**
 * Start of interrupt vector table.
 */
extern char link_interrupt_vectors_location;

extern char link_ram_interrupt_vectors_location;
extern char link_ram_interrupt_vectors_location_end;

const unsigned HardFaultIndex = 3;
const unsigned UsageFaultIndex = 6;
const unsigned SysTickIndex = 15;
const unsigned USART1Index = 53;
const unsigned USART2Index = 54;
const unsigned USART3Index = 55;
// const unsigned ButtonExtiIndex = BUTTON1_EXTI_IRQ_INDEX;
// const unsigned TIM7Index = 71;

/* Extern variables ---------------------------------------------------------*/
/**
 * Updated by HAL_1Ms_Tick()
 */
extern volatile uint32_t TimingDelay;

/* Private function prototypes ----------------------------------------------*/

void HAL_Core_Config_systick_configuration(void) {
    SysTick_Configuration();
}

/**
 * Called by HAL_Core_Config() to allow the HAL implementation to override
 * the interrupt table if required.
 */
void HAL_Core_Setup_override_interrupts(void)
{
    memcpy(&link_ram_interrupt_vectors_location, &link_interrupt_vectors_location, &link_ram_interrupt_vectors_location_end-&link_ram_interrupt_vectors_location);
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[HardFaultIndex] = (uint32_t)HardFault_Handler;
    isrs[UsageFaultIndex] = (uint32_t)UsageFault_Handler;
    isrs[SysTickIndex] = (uint32_t)SysTickOverride;
    isrs[USART1Index] = (uint32_t)HAL_USART1_Handler;
    isrs[USART2Index] = (uint32_t)HAL_USART2_Handler;
    isrs[USART3Index] = (uint32_t)HAL_USART3_Handler;
    isrs[ButtonExtiIndex] = (uint32_t)Handle_Mode_Button_EXTI_irq;
    //isrs[TIM7Index] = (uint32_t)TIM7_override;  // WICED uses this for a JTAG watchdog handler
    SCB->VTOR = (unsigned long)isrs;
}

/**
 * Called at the beginning of app_setup_and_loop() from main.cpp to
 * pre-initialize any low level hardware before the main loop runs.
 */
void HAL_Core_Init(void)
{
}

void HAL_1Ms_Tick()
{
    if (TimingDelay != 0x00)
    {
        __sync_sub_and_fetch(&TimingDelay, 1);
    }
}

/**
 * Called by HAL_Core_Setup() to perform any post-setup config after the
 * watchdog has been disabled.
 */
void HAL_Core_Setup_finalize(void)
{
}
