/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "core_hal.h"
#include "core_hal_stm32f2xx.h"
#include "wiced.h"
#include "wlan_internal.h"
#include "module_info.h"
#include "flash_mal.h"
#include "delay_hal.h"
#include <stdint.h>

/**
 * Start of interrupt vector table.
 */
extern char link_interrupt_vectors_location;

extern char link_ram_interrupt_vectors_location;
extern char link_ram_interrupt_vectors_location_end;

const unsigned HardFaultIndex = 3;
const unsigned MemManageIndex = 4;
const unsigned BusFaultIndex = 5;
const unsigned UsageFaultIndex = 6;
const unsigned SysTickIndex = 15;
const unsigned USART1Index = 53;
const unsigned USART2Index = 54;
const unsigned ButtonExtiIndex = BUTTON1_EXTI_IRQ_INDEX;
const unsigned TIM7Index = 71;
const unsigned DMA2Stream2Index = 74;
const unsigned CAN2_TX_IRQHandler_Idx               = 79;
const unsigned CAN2_RX0_IRQHandler_Idx              = 80;
const unsigned CAN2_RX1_IRQHandler_Idx              = 81;
const unsigned CAN2_SCE_IRQHandler_Idx              = 82;
/**
 * Updated by HAL_1Ms_Tick()
 */
volatile uint32_t TimingDelay;

void HAL_Core_Config_systick_configuration(void) {
    //SysTick_Configuration(); This causes the Photon to sometimes hang on startup. See FIRM-123.
}

void HAL_Core_Setup_override_interrupts(void) {

    memcpy(&link_ram_interrupt_vectors_location, &link_interrupt_vectors_location, &link_ram_interrupt_vectors_location_end-&link_ram_interrupt_vectors_location);
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[HardFaultIndex] = (uint32_t)HardFault_Handler;
    isrs[MemManageIndex] = (uint32_t)MemManage_Handler;
    isrs[BusFaultIndex] = (uint32_t)BusFault_Handler;
    isrs[UsageFaultIndex] = (uint32_t)UsageFault_Handler;
    isrs[SysTickIndex] = (uint32_t)SysTickOverride;
    isrs[USART1Index] = (uint32_t)HAL_USART1_Handler;
    isrs[USART2Index] = (uint32_t)HAL_USART2_Handler;
    isrs[ButtonExtiIndex] = (uint32_t)Mode_Button_EXTI_irq;
    isrs[TIM7Index] = (uint32_t)TIM7_override;  // WICED uses this for a JTAG watchdog handler
    isrs[DMA2Stream2Index] = (uint32_t)DMA2_Stream2_irq_override;
    isrs[CAN2_TX_IRQHandler_Idx]            = (uint32_t)CAN2_TX_irq;
    isrs[CAN2_RX0_IRQHandler_Idx]           = (uint32_t)CAN2_RX0_irq;
    isrs[CAN2_RX1_IRQHandler_Idx]           = (uint32_t)CAN2_RX1_irq;
    isrs[CAN2_SCE_IRQHandler_Idx]           = (uint32_t)CAN2_SCE_irq;
    SCB->VTOR = (unsigned long)isrs;
}


/* Extern function prototypes ------------------------------------------------*/
void HAL_Core_Init_finalize(void)
{
    wiced_core_init();
    wlan_initialize_dct();
}

void SysTickChain()
{
    void (*chain)(void) = (void (*)(void))((uint32_t*)&link_interrupt_vectors_location)[SysTickIndex];

    chain();

    SysTickOverride();
}

void HAL_1Ms_Tick()
{
    if (TimingDelay != 0x00)
    {
        __sync_sub_and_fetch(&TimingDelay, 1);
    }
}

void HAL_Core_Setup_finalize(void)
{
    // Now that main() has been executed, we can plug in the original WICED ISR in the
    // ISR chain.)
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[SysTickIndex] = (uint32_t)SysTickChain;

#ifdef MODULAR_FIRMWARE
    const uint32_t app_backup = 0x800C000;
    // when unpacking from the combined image, we have the default application image stored
    // in the eeprom region.
    const module_info_t* app_info = FLASH_ModuleInfo(FLASH_INTERNAL, app_backup);
    if (app_info->module_start_address==(void*)0x80A0000) {
    		LED_SetRGBColor(RGB_COLOR_GREEN);
    		uint32_t length = app_info->module_end_address-app_info->module_start_address+4;
    		if (length < 80*1024 && FLASH_CopyMemory(FLASH_INTERNAL, app_backup, FLASH_INTERNAL, 0x80E0000, length, MODULE_FUNCTION_USER_PART, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION)) {
        		FLASH_CopyMemory(FLASH_INTERNAL, app_backup, FLASH_INTERNAL, 0x80A0000, length, MODULE_FUNCTION_USER_PART, MODULE_VERIFY_CRC|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_FUNCTION);
    			FLASH_EraseMemory(FLASH_INTERNAL, app_backup, length);
    		}
    		LED_SetRGBColor(RGB_COLOR_WHITE);
    	}
#endif
}

/**
 * @brief  This function handles EXTI2_IRQ or EXTI_9_5_IRQ Handler.
 * @param  None
 * @retval None
 */
void Mode_Button_EXTI_irq(void)
{
    void (*chain)(void) = (void (*)(void))((uint32_t*)&link_interrupt_vectors_location)[ButtonExtiIndex];

    Handle_Mode_Button_EXTI_irq();

    chain();
}
