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
extern u32 ConfigDebugClose;
extern void HardFault_Handler(void) __attribute__(( naked ));
extern void MemManage_Handler(void) __attribute__(( naked ));
extern void BusFault_Handler(void) __attribute__(( naked ));
extern void UsageFault_Handler(void) __attribute__(( naked ));


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

    // Enable CPU access to PSRAM
    if (PSRAM_DEV->CSR & BIT_PSRAM_MEM_IDLE) {
        PSRAM_DEV->CSR &= (~BIT_PSRAM_MEM_IDLE);
        while(PSRAM_DEV->CSR & BIT_PSRAM_MEM_IDLE);
    }

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

extern uintptr_t platform_backup_ram_all_start[];
extern uintptr_t platform_backup_ram_all_end;

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

    /* Mark "backup" (not really backup, just some pages in SRAM) RAM as no-cache */
    mpu_entry = mpu_entry_alloc();
    mpu_cfg.region_base = (uintptr_t)&platform_backup_ram_all_start;
    mpu_cfg.region_size = (uintptr_t)&platform_backup_ram_all_end - (uintptr_t)&platform_backup_ram_all_start;
    mpu_cfg.xn = MPU_EXEC_ALLOW;
    mpu_cfg.ap = MPU_UN_PRIV_RW;
    mpu_cfg.sh = MPU_NON_SHAREABLE;
    mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
    mpu_region_cfg(mpu_entry, &mpu_cfg);

    return 0;
}

/**
 * @brief  Configures Main system clocks & power.
 * @param  None
 * @retval None
 */

extern FLASH_InitTypeDef flash_init_para_km0;

void SOCPS_AudioLDO(u32 NewStatus) {
    u32 temp = 0;
    static u32 PadPower = 0;

    if (NewStatus == DISABLE) {
        // 0x4800_0344[8] = 0; 1: Enable Audio bandgap, 0: disable;
        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
        temp &= ~(BIT_LSYS_AC_MBIAS_POW | BIT_LSYS_AC_LDO_POW);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL, temp);
        // store pad power
        PadPower = (temp >> BIT_LSYS_SHIFT_AC_LDO_REG) &  BIT_LSYS_MASK_AC_LDO_REG;

        // 0x4800_0280[2:0] = 0, disable BG
        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSPLL_CTRL0);
        temp &= ~(BIT_LP_PLL_BG_EN | BIT_LP_PLL_BG_I_EN | BIT_LP_PLL_MBIAS_EN);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSPLL_CTRL0, temp);
    } else {
        // if (wifi_config.wifi_ultra_low_power) {
        //     return;
        // }
        // /* if audio not use, save power */
        // if (ps_config.km0_audio_pad_enable == FALSE) {
        //     return;
        // }

        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
        if ((temp & BIT_LSYS_AC_LDO_POW) != 0) {
            return;
        }

        // 0x4800_0280[2:0] = 0b111, enable BG
        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSPLL_CTRL0);
        temp |= (BIT_LP_PLL_BG_EN | BIT_LP_PLL_BG_I_EN | BIT_LP_PLL_MBIAS_EN);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSPLL_CTRL0, temp);

        // 0x4800_0344[8] = 1; 1: Enable Audio bandgap, 0: disable;
        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
        temp |= BIT_LSYS_AC_MBIAS_POW;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL, temp);

        // Delay 5us, 0x4800_0344[0] = 1'b1 (BIT_LSYS_AC_LDO_POW)
        DelayUs(5);

        // enable Audio LDO.
        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
        temp &= ~(BIT_LSYS_MASK_AC_LDO_REG << BIT_LSYS_SHIFT_AC_LDO_REG);
        // restore the pad power
        temp |= (PadPower) << BIT_LSYS_SHIFT_AC_LDO_REG;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL, temp);

        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
        temp |= BIT_LSYS_AC_LDO_POW;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL, temp);
    }
}

void app_audio_pad_enable(void) {
    u32 temp = 0;

    // 0x4800_0208[28] = 1; 1: enable Audio & GPIO shared PAD, 0: shutdown;
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0);
    temp |= BIT_SYS_AMACRO_EN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0, temp);

    // 0x4800_0344[9] = 1; 1: Enable Audio pad function, 0: disable;
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL);
    temp |= BIT_LSYS_AC_ANA_PORB;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AUDIO_SHARE_PAD_CTRL, temp);

    /* When Audio Pad(GPIOD_0~6) used as GPIO, output voltage can reach 3.064V(A-Cut)/3.3V(B-Cut), */
    /* which can be configured by 0x4800_0344[7:1] */
    SOCPS_AudioLDO(ENABLE);
}

void Set_System(void)
{
    // FIXME: don't mess with MSP and IRQ table
    // for now we are using whatever ROM has configured for us
    // irq_table_init(RTL_DEFAULT_MSP_S);
    // __set_MSP(RTL_DEFAULT_MSP_S);

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    __NVIC_SetVector(HardFault_IRQn, (u32)(void*)HardFault_Handler);
    __NVIC_SetVector(MemoryManagement_IRQn, (u32)(void*)MemManage_Handler);
    __NVIC_SetVector(BusFault_IRQn, (u32)(void*)BusFault_Handler);
    __NVIC_SetVector(UsageFault_IRQn, (u32)(void*)UsageFault_Handler);
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

    // Disable DiagPrintf
    ConfigDebugClose = 1;
    _memcpy((void *)&flash_init_para, &flash_init_para_km0, sizeof(FLASH_InitTypeDef));

    SystemCoreClockUpdate();

    // force SP align to 8 byte not 4 byte (initial SP is 4 byte align)
    // asm volatile(
	// 	"mov r0, sp\n"
	// 	"bic r0, r0, #7\n"
	// 	"mov sp, r0\n"
	// );

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    peripheralsClockEnable();
    app_audio_pad_enable();
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_RFAFE_IND_VIO1833);
    temp |= BIT_RFAFE_IND_VIO1833;
    HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_RFAFE_IND_VIO1833, temp);

    DWT_Init();

    // Enable MPU
    mpu_init();

    /* Configure flash */
    SPARK_ASSERT(!hal_exflash_init());

    /* Configure the LEDs and set the default states */
    for (int LEDx = 1; LEDx < LEDn; ++LEDx) {
        // PARTICLE_LED_USER initialization is skipped during system setup
        LED_Init(LEDx);
    }

    // GPIOTE initialization
    hal_interrupt_init();

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    /* Configure the Button */
    hal_button_init(HAL_BUTTON1, HAL_BUTTON_MODE_EXTI);

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
        // hal_watchdog_start(msTimeout);
    }
#endif // 0
}
