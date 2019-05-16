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
#include "nrf_power.h"
#include "nrf_delay.h"

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
#include "service_debug.h"
#include "usb_hal.h"

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

static void power_failure_handler(void) {
    // Simply reset
    NVIC_SystemReset();
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

    // Enable power failure comparator
    nrf_drv_power_pofwarn_config_t conf = {
        .handler = power_failure_handler,
        .thrvddh = NRF_POWER_POFTHRVDDH_V28,
        .thr = NRF_POWER_POFTHR_V28,
    };
    ret = nrf_drv_power_pof_init(&conf);
    SPARK_ASSERT(ret == NRF_SUCCESS);

    // Disable RAM retention for any sector other than the one that contains backup sections
    // This should reduce power consumption in sleep modes
    // Unfortunately the backup sections reside in a 32K sector
    // TODO: move backup sections to a different region so that it resides in 8K sector?
    nrf_power_rampower_mask_on(8, NRF_POWER_RAMPOWER_S5RETENTION_MASK);
    nrf_power_rampower_mask_off(0, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(1, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(2, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(3, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(4, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(5, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(6, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(7, NRF_POWER_RAMPOWER_S0RETENTION_MASK | NRF_POWER_RAMPOWER_S1RETENTION_MASK);
    nrf_power_rampower_mask_off(8, NRF_POWER_RAMPOWER_S0RETENTION_MASK |
            NRF_POWER_RAMPOWER_S1RETENTION_MASK |
            NRF_POWER_RAMPOWER_S2RETENTION_MASK |
            NRF_POWER_RAMPOWER_S3RETENTION_MASK |
            NRF_POWER_RAMPOWER_S4RETENTION_MASK);

    DWT_Init();

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    // FIXME: Have to initialize USB before softdevice enabled,
    // otherwise USB module won't recevie power event
    HAL_USB_Init();
#endif

    /* Configure internal flash and external flash */
    SPARK_ASSERT(!hal_flash_init());
    SPARK_ASSERT(!hal_exflash_init());

    /* Configure the LEDs and set the default states */
    int LEDx;
    for(LEDx = 1; LEDx < LEDn; ++LEDx)
    {
        // LED_USER initialization is skipped during system setup
        LED_Init(LEDx);
    }

    // GPIOTE initialization
    HAL_Interrupts_Init();

    /* Configure the Button */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);
}

void Reset_System(void) {
    __DSB();

    SysTick_Disable();

    BUTTON_Uninit();

    hal_exflash_uninit();

    RGB_LED_Uninit();

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

    NVIC_SetPriority(SysTick_IRQn, SYSTICK_IRQ_PRIORITY);
}

void SysTick_Disable() {
    SysTick->CTRL = SysTick->CTRL & ~SysTick_CTRL_ENABLE_Msk;
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



__attribute__((section(".retained_system_flags"))) platform_system_flags_t system_flags;

#define SYSTEM_FLAGS_MAGIC_NUMBER 0x1ADEACC0u

void Load_SystemFlags()
{
	// if the header does not match the expected magic value, then initialize
	if (system_flags.header!=SYSTEM_FLAGS_MAGIC_NUMBER) {
        memset(&system_flags, 0xff, sizeof(platform_system_flags_t));
        system_flags.header = SYSTEM_FLAGS_MAGIC_NUMBER;
	}
}

void Save_SystemFlags()
{
	// nothing to do here
}

bool FACTORY_Flash_Reset(void)
{
    bool success;

    // Restore the Factory firmware using flash_modules application dct info
    success = FLASH_RestoreFromFactoryResetModuleSlot();
    //FLASH_AddToFactoryResetModuleSlot() is now called in HAL_Core_Config() in core_hal.c,
    //So FLASH_Restore(INTERNAL_FLASH_FAC_ADDRESS) is not required and hence commented

    system_flags.Factory_Reset_SysFlag = 0xFFFF;
    if (success) {
        system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
        system_flags.dfu_on_no_firmware = 0;
        SYSTEM_FLAG(Factory_Reset_Done_SysFlag) = 0x5A;
        Finish_Update();
    }
    else {
        Save_SystemFlags();
    }
    return success;
}

void BACKUP_Flash_Reset(void)
{
    //Not supported since there is no Backup copy of the firmware in Internal Flash
}

void OTA_Flash_Reset(void)
{
    //FLASH_UpdateModules() does the job of copying the split firmware modules
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


static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize, uint32_t const *p_crc)
{
    uint32_t crc = (p_crc) ? *p_crc : 0;

    crc = crc ^ ~0U;

    while (bufferSize--)
        crc = crc32_tab[(crc ^ *pBuffer++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
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

        // FIXME: DON'T enable watchdog temporarily
        // HAL_Watchdog_Init(msTimeout);
    }
}
