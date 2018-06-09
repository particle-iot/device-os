/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
 */

#include <stdint.h>

/* This is a legacy header */
#include "nrf_drv_clock.h"
/* This is a legacy header */
#include "nrf_drv_power.h"

#include "nrf_nvic.h"
#include "nrf_rtc.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#endif
#include "nrf52840.h"

#include "hw_config.h"
#include "service_debug.h"
#include "rgbled.h"
#include "rgbled_hal.h"
#include "button_hal.h"
#include "watchdog_hal.h"
#include "dct.h"
#include "flash_hal.h"
#include "exflash_hal.h"
#include "crc32.h"
#include "core_hal.h"

uint8_t USE_SYSTEM_FLAGS;
uint16_t tempFlag;


static void DWT_Init(void)
{
    // DBGMCU->CR |= DBGMCU_CR_SETTINGS;
    // DBGMCU->APB1FZ |= DBGMCU_APB1FZ_SETTINGS;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Configures Main system clocks & power.
 * @param  None
 * @retval None
 */
void Set_System(void)
{
    ret_code_t ret = nrf_drv_clock_init();
    SPARK_ASSERT(ret == NRF_SUCCESS || ret == NRF_ERROR_MODULE_ALREADY_INITIALIZED);

    nrf_drv_clock_hfclk_request(NULL);
    while (!nrf_drv_clock_hfclk_is_running())
    {
        /* Waiting */
    }

    nrf_drv_clock_lfclk_request(NULL);
    while (!nrf_drv_clock_lfclk_is_running())
    {
        /* Waiting */
    }

    ret = nrf_drv_power_init(NULL);
    SPARK_ASSERT(ret == NRF_SUCCESS || ret == NRF_ERROR_MODULE_ALREADY_INITIALIZED);

    DWT_Init();

    /* Configure RTC1 for BUTTON-DEBOUNCE usage */
    UI_Timer_Configure();

    /* Configure the LEDs and set the default states */
    int LEDx;
    for(LEDx = 0; LEDx < LEDn; ++LEDx)
    {
        LED_Init(LEDx);
    }

    /* Configure the Button */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);

    /* Configure internal flash and external flash */
    hal_flash_init();
    hal_exflash_init();
}

void Reset_System(void) {
    __DSB();

    SysTick_Disable();

    sd_nvic_DisableIRQ(RTC1_IRQn);

    const uint32_t evtMask = RTC_EVTEN_COMPARE3_Msk |
                             RTC_EVTEN_COMPARE2_Msk |
                             RTC_EVTEN_COMPARE1_Msk |
                             RTC_EVTEN_COMPARE0_Msk |
                             RTC_EVTEN_OVRFLW_Msk   |
                             RTC_EVTEN_TICK_Msk;

    const uint32_t intMask = NRF_RTC_INT_TICK_MASK     |
                             NRF_RTC_INT_OVERFLOW_MASK |
                             NRF_RTC_INT_COMPARE0_MASK |
                             NRF_RTC_INT_COMPARE1_MASK |
                             NRF_RTC_INT_COMPARE2_MASK |
                             NRF_RTC_INT_COMPARE3_MASK;

    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_STOP);

    /* NOTE: the second argument is a mask */
    nrf_rtc_int_disable(NRF_RTC1, intMask);
    nrf_rtc_event_disable(NRF_RTC1, evtMask);

    /* NOTE: the second argument is an event (nrf_rtc_event_t) */
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_TICK);
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_OVERFLOW);
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_COMPARE_0);
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_COMPARE_1);
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_COMPARE_2);
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_COMPARE_3);

    sd_nvic_ClearPendingIRQ(RTC1_IRQn);
    sd_nvic_SetPriority(RTC1_IRQn, 0);

    nrf_rtc_prescaler_set(NRF_RTC1, 0);

    __DSB();
}

void SysTick_Configuration(void) {
    /* Setup SysTick Timer for 1 msec interrupts */
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        /* Capture error */
        while (1)
        {
        }
    }

    /* Configure the SysTick Handler Priority: Preemption priority and subpriority */
    sd_nvic_SetPriority(SysTick_IRQn, SYSTICK_IRQ_PRIORITY);   //OLD: sd_nvic_EncodePriority(sd_nvic_GetPriorityGrouping(), 0x03, 0x00)
}

void Finish_Update() 
{
    //Set system flag to Enable IWDG in IWDG_Reset_Enable()
    //called in bootloader to recover from corrupt firmware
    system_flags.IWDG_Enable_SysFlag = 0xD001;

    system_flags.FLASH_OTA_Update_SysFlag = 0x5000;
    Save_SystemFlags();

    HAL_Core_Write_Backup_Register(BKP_DR_10, 0x5000);

    // USB_Cable_Config(DISABLE);

    sd_nvic_SystemReset();
}

void UI_Timer_Configure(void)
{
    nrf_rtc_prescaler_set(NRF_RTC1, RTC_FREQ_TO_PRESCALER(UI_TIMER_FREQUENCY));
    /* NOTE: the second argument is an event (nrf_rtc_event_t) */
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_TICK);
    /* NOTE: the second argument is a mask */
    nrf_rtc_event_enable(NRF_RTC1, RTC_EVTEN_TICK_Msk);
    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_START);
}

platform_system_flags_t system_flags;

void Load_SystemFlags()
{
    dct_read_app_data_copy(DCT_SYSTEM_FLAGS_OFFSET, &system_flags, sizeof(platform_system_flags_t));
}

void Save_SystemFlags()
{
    dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, sizeof(platform_system_flags_t));
}

bool OTA_Flashed_GetStatus(void)
{
    if(system_flags.OTA_FLASHED_Status_SysFlag == 0x0001)
        return true;
    else
        return false;
}

void OTA_Flashed_ResetStatus(void)
{
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();
}

uint16_t Bootloader_Get_Version(void)
{
    return system_flags.Bootloader_Version_SysFlag;
}

void Bootloader_Update_Version(uint16_t bootloaderVersion)
{
    system_flags.Bootloader_Version_SysFlag = bootloaderVersion;
    Save_SystemFlags();
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize, uint32_t const *p_crc)
{
    return crc32_compute((uint8_t*)pBuffer, bufferSize, p_crc);
}

void IWDG_Reset_Enable(uint32_t msTimeout)
{
    Load_SystemFlags();
    // Old versions of the bootloader were storing system flags in DCT
    const size_t dctFlagOffs = DCT_SYSTEM_FLAGS_OFFSET + offsetof(platform_system_flags_t, IWDG_Enable_SysFlag);
    uint16_t dctFlag = 0;
    if (dct_read_app_data_copy(dctFlagOffs, &dctFlag, sizeof(dctFlag)) == 0 && dctFlag == 0xD001)
    {
        dctFlag = 0xFFFF;
        dct_write_app_data(&dctFlag, dctFlagOffs, sizeof(dctFlag));
        SYSTEM_FLAG(IWDG_Enable_SysFlag) = 0xD001;
    }
    if(SYSTEM_FLAG(IWDG_Enable_SysFlag) == 0xD001)
    {
        if (msTimeout == 0)
        {
            system_flags.IWDG_Enable_SysFlag = 0xFFFF;
            Save_SystemFlags();

            NVIC_SystemReset();
        }

        HAL_Watchdog_Init(msTimeout);
    }
}
