/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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
#include "hw_config.h"
#include "rtl8721d.h"
#include "rtl8721d_system.h"
#include "button_hal.h"
#include "interrupts_hal.h"
#include "service_debug.h"
#include "rgbled_hal.h"
#include "exflash_hal.h"
#include "km0_km4_ipc.h"
#include "core_hal.h"

// FIXME:
// static const uintptr_t RTL_DEFAULT_MSP_S = 0x1007FFF0;

uint8_t USE_SYSTEM_FLAGS;
uint16_t tempFlag;

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

extern uintptr_t link_ipc_data_start;
extern uintptr_t link_ipc_data_end;

static void hw_rtl_init_psram(void)
{
    u32 temp;
    PCTL_InitTypeDef  PCTL_InitStruct;

    /*set rwds pull down*/
    temp = HAL_READ32(PINMUX_REG_BASE, 0x104);
    temp &= ~(PAD_BIT_PULL_UP_RESISTOR_EN | PAD_BIT_PULL_DOWN_RESISTOR_EN);
    temp |= PAD_BIT_PULL_DOWN_RESISTOR_EN;
    HAL_WRITE32(PINMUX_REG_BASE, 0x104, temp);

    PSRAM_CTRL_StructInit(&PCTL_InitStruct);
    PSRAM_CTRL_Init(&PCTL_InitStruct);

    PSRAM_PHY_REG_Write(REG_PSRAM_CAL_PARA, 0x02030310);

    /*check psram valid*/
    HAL_WRITE32(PSRAM_BASE, 0, 0);
    assert_param(0 == HAL_READ32(PSRAM_BASE, 0));

    if(_FALSE == PSRAM_calibration())
        return;

    // if(FALSE == psram_dev_config.psram_dev_cal_enable) {
        temp = PSRAM_PHY_REG_Read(REG_PSRAM_CAL_CTRL);
        temp &= (~BIT_PSRAM_CFG_CAL_EN);
        PSRAM_PHY_REG_Write(REG_PSRAM_CAL_CTRL, temp);
    // }
}

void peripheralsClockEnable(void) {
	//RCC_PeriphClockCmd(APBPeriph_PSRAM, APBPeriph_PSRAM_CLOCK, ENABLE);//move into psram init, because 200ua leakage
	//RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_VENDOR_REG, APBPeriph_VENDOR_REG_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_USI_REG, APBPeriph_USI_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_IRDA_REG, APBPeriph_IRDA_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_IPC, APBPeriph_IPC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);

	/* close BT temp for simulation */
	//RCC_PeriphClockCmd(APBPeriph_BT, APBPeriph_BT_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_GDMA0, APBPeriph_GDMA0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LCDC, APBPeriph_LCDC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_I2S0, APBPeriph_I2S0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SECURITY_ENGINE, APBPeriph_SEC_ENG_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LXBUS, APBPeriph_LXBUS_CLOCK, ENABLE);

	//RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);
	//RCC_PeriphClockCmd(APBPeriph_OTG, APBPeriph_OTG_CLOCK, ENABLE);//move into otg init, because 150ua leakage
	//RCC_PeriphClockCmd(APBPeriph_SDIOH, APBPeriph_SDIOH_CLOCK, ENABLE);//move into sd init, because 600ua leakage
	RCC_PeriphClockCmd(APBPeriph_SDIOD, APBPeriph_SDIOD_CLOCK, ENABLE);
}

#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

static void DWT_Init(void)
{
    // DBGMCU->CR |= DBGMCU_CR_SETTINGS;
    // DBGMCU->APB1FZ |= DBGMCU_APB1FZ_SETTINGS;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

u32 app_mpu_nocache_init(void) {
    mpu_region_config mpu_cfg;
    u32 mpu_entry = 0;

    /* close nocache section in the lib_wlan.a */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = (uint32_t)__ram_nocache_start__;
    mpu_cfg.region_size = __ram_nocache_end__ - __ram_nocache_start__;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
    if (mpu_cfg.region_size >= 32) {
        mpu_region_cfg(mpu_entry, &mpu_cfg);
    }

    /* close 216K irom_ns cache */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = 0x1010A000;
    mpu_cfg.region_size = 0x10140000 - 0x1010A000;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
    mpu_region_cfg(mpu_entry, &mpu_cfg);

    /* close 80K drom_ns cache */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = 0x101C0000;
    mpu_cfg.region_size = 0x101D4000 - 0x101C0000;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
    mpu_region_cfg(mpu_entry, &mpu_cfg);

    /* set 1KB retention ram no-cache */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = 0x000C0000;
    mpu_cfg.region_size = 0x400;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
    mpu_region_cfg(mpu_entry, &mpu_cfg);

    /* set No-Security PSRAM Memory Write-Back */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = 0x02000000;
    mpu_cfg.region_size = 0x400000;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_WB_T_RWA;
    mpu_region_cfg(mpu_entry, &mpu_cfg);

    return 0;
}

/**
 * @brief  Configures Main system clocks & power.
 * @param  None
 * @retval None
 */
volatile uint32_t rtlContinue = 0;
void Set_System(void)
{
    // FIXME: don't mess with MSP and IRQ table
    // for now we are using whatever ROM has configured for us
    // irq_table_init(RTL_DEFAULT_MSP_S);
    // __set_MSP(RTL_DEFAULT_MSP_S);

    _memcpy((void *)&flash_init_para, (const void *)BKUP_Read(BKUP_REG7), sizeof(FLASH_InitTypeDef));

    SystemCoreClockUpdate();

    // force SP align to 8 byte not 4 byte (initial SP is 4 byte align)
    // asm volatile( 
	// 	"mov r0, sp\n"
	// 	"bic r0, r0, #7\n" 
	// 	"mov sp, r0\n"
	// );

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    peripheralsClockEnable();
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

    // mpu_init();

    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_RFAFE_IND_VIO1833);
    temp |= BIT_RFAFE_IND_VIO1833;
    HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_RFAFE_IND_VIO1833, temp);

    DWT_Init();

    /* Configure flash */
    SPARK_ASSERT(!hal_exflash_init());

    /* Configure the LEDs and set the default states */
    for (int LEDx = 1; LEDx < LEDn; ++LEDx) {
        // PARTICLE_LED_USER initialization is skipped during system setup
        LED_Init(LEDx);
    }

    // GPIOTE initialization
    hal_interrupt_init();

    /* Configure the Button */
    hal_button_init(HAL_BUTTON1, HAL_BUTTON_MODE_EXTI);

    SYSTIMER_Init();

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    hw_rtl_init_psram();

    // set IDAU, the enabled regions are treated as Non-secure space
    // KM0 needs to read IPC message that shoud be allocated in KM4 NS SRAM
    IDAU_TypeDef* IDAU = ((IDAU_TypeDef *) KM4_IDAU_BASE);
    for (uint8_t i = 0; i < 7; i++) {
        if ((IDAU->IDAU_CTRL & BIT(i)) == 0) {
            IDAU->ENTRY[i].IDAU_BARx = (uint32_t)&link_ipc_data_start;
            IDAU->ENTRY[i].IDAU_LARx = (uint32_t)&link_ipc_data_end;
            IDAU->IDAU_CTRL |= BIT(i);
            break;
        }
    }
#else
    // Disable cache
    Cache_Enable(0);

    // Enable MPU and configure the nocache region
    mpu_init();
    app_mpu_nocache_init();

    // Enable cache
    Cache_Enable(1);
#endif
    ICache_Enable();

    InterruptRegister(IPC_INTHandler, IPC_IRQ, (u32)IPCM0_DEV, 5);
    InterruptEn(IPC_IRQ, 5);
    km0_km4_ipc_init(KM0_KM4_IPC_CHANNEL_GENERIC);
}

void Reset_System(void) {
    __DSB();

    SysTick_Disable();

    hal_button_uninit();

    hal_exflash_uninit();

    RGB_LED_Uninit();

    __DSB();
}

#if __Vendor_SysTickConfig
__STATIC_INLINE uint32_t SysTick_Config(uint32_t ticks)
{
    if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk) {
        return (1UL);                                                 /* Reload value impossible */
    }

    SysTick->LOAD  = (uint32_t)(ticks - 1UL);                         /* set reload register */
    NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
    SysTick->VAL   = 0UL;                                             /* Load the SysTick Counter Value */
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;                         /* Enable SysTick IRQ and SysTick Timer */
    return (0UL);                                                     /* Function successful */
}
#endif // __Vendor_SysTickConfig

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

    __NVIC_SetVector(SysTick_IRQn, (u32)(VOID*)SysTick_Handler);
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

    // FIXME: reset reason?
    HAL_Core_System_Reset_Ex(0, 0, NULL);
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
    return true;
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
#if 0
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
#endif // 0
}
