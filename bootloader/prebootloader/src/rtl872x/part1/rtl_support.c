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

#include "ameba_soc.h"
#include "rtl_support.h"
#include "rtl8721d_system.h"
#include "rtl8721d_bor.h"
#include "rtl8721d.h"
#include <stdbool.h>

extern CPU_PWR_SEQ HSPWR_OFF_SEQ[];

uint32_t tickless_debug = 0;
static volatile bool km4_powered_on = false;

/* The binary data of generated ram_retention.bin should copy into retention_ram_patch_array. 
   Then fill in the patch size at the second dword */
const uint32_t retention_ram_patch_array[2][RETENTION_RAM_SYS_OFFSET / 4] = {
    {
        0x000c0009,
        0x00000080,
        0x2210f240,
        0x0200f6c4,
        0x49106813,
        0xf240400b,
        0x430b2100,
        0x22906013,
        0x05d22380,
        0x045b6811,
        0xd1124219,
        0x6013430b,
        0x3300f240,
        0x0300f6c4,
        0x4907681a,
        0x601a400a,
        0x3318f240,
        0xf6c42101,
        0x681a0300,
        0x601a430a,
        0x4770bf30,
        0xfffff9ff,
        0xdfffffff,
    },
    {
        0x000c0009,
        0x00000080,
        0x2210f240,
        0x0200f6c4,
        0x49036813,
        0xf240400b,
        0x430b2100,
        0x47706013,
        0xfffff9ff,
    }
};

uint32_t SDM32K_Read(uint32_t adress);
void SOCPS_SNOOZE_Config(uint32_t bitmask, uint32_t status);

extern uintptr_t link_retention_ram_start;
extern CPU_PWR_SEQ SYSPLL_ON_SEQ[];


static void invalidate_flash_auto_write(void) {
    /* Auto write related bits in valid command register are all set to 0,
        just need to invalidate write single and write enable cmd in auto mode. */
    SPIC_TypeDef *spi_flash = SPIC;

    /* Disable SPI_FLASH User Mode */
    spi_flash->ssienr = 0;
    
    /* Invalidate write single and write enable cmd in auto mode */
    spi_flash->wr_single = 0x0;
    spi_flash->wr_enable = 0x0;
}

static void boot_ram_function_enable(void) {
    uint32_t temp = 0;

    /*Reset SIC function*/
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0);
    temp &= ~BIT_LSYS_SIC_FEN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0, temp);
    /*Reset SIC clock*/
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0);
    temp &= ~BIT_SIC_CLK_EN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0, temp);

    RCC_PeriphClockCmd(APBPeriph_GDMA0, APBPeriph_GDMA0_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_QDEC0, APBPeriph_QDEC0_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_SGPIO, APBPeriph_SGPIO_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_LOGUART, APBPeriph_LOGUART_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_IPC, APBPeriph_IPC_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_VENDOR_REG, APBPeriph_VENDOR_REG_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_RTC, APBPeriph_RTC_CLOCK, ENABLE);

    /* CLK SEL */
    RCC_PeriphClockSource_QDEC(0, 1);
    
    /* AON */
    RCC_PeriphClockCmd(APBPeriph_CTOUCH, APBPeriph_CTOUCH_CLOCK, ENABLE);

    /*SDM32K*/
    SDM32K_Enable(SDM32K_ALWAYS_CAL); /* 0.6ms */
}

static void fast_timer_init(void) {
    RTIM_TimeBaseInitTypeDef TIM_InitStruct;

    RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
    RTIM_TimeBaseStructInit(&TIM_InitStruct);

    TIM_InitStruct.TIM_Idx = 5;
    TIM_InitStruct.TIM_Prescaler = 39; //40M/40 =1M->1us
    TIM_InitStruct.TIM_Period = 0xFFFF;

    TIM_InitStruct.TIM_UpdateEvent = ENABLE; /* UEV enable */
    TIM_InitStruct.TIM_UpdateSource = TIM_UpdateSource_Overflow;
    TIM_InitStruct.TIM_ARRProtection = ENABLE;

    RTIM_TimeBaseInit(TIMM05, &TIM_InitStruct, 0, (IRQ_FUN) NULL, (uint32_t)NULL);
    RTIM_Cmd(TIMM05, ENABLE);
}

static void dslp_flash_init(void) {
    RRAM_TypeDef* RRAM = ((RRAM_TypeDef *) RRAM_BASE);
    FLASH_STRUCT_INIT_FUNC FLASH_StructInitTemp = (FLASH_STRUCT_INIT_FUNC)RRAM->FLASH_StructInit;

    if (FLASH_StructInitTemp != NULL) {
        FLASH_StructInitTemp(&flash_init_para);
    } else {
        FLASH_StructInit(&flash_init_para);
    }

    /* SPIC clock switch to PLL */
    FLASH_ClockDiv(RRAM->FLASH_ClockDiv);

    flash_init_para.FLASH_QuadEn_bit = RRAM->FLASH_QuadEn_bit;
    flash_init_para.FLASH_cmd_wr_status2 = RRAM->FLASH_cmd_wr_status2;
    flash_init_para.FLASH_addr_phase_len = RRAM->FLASH_addr_phase_len;
    flash_init_para.FLASH_cur_cmd = RRAM->FLASH_cur_cmd;
    flash_init_para.FALSH_quad_valid_cmd = RRAM->FALSH_quad_valid_cmd;
    flash_init_para.FALSH_dual_valid_cmd = RRAM->FALSH_dual_valid_cmd;
    flash_init_para.FLASH_pseudo_prm_en = RRAM->FLASH_pseudo_prm_en;
    flash_init_para.FLASH_rd_dummy_cyle[0] = RRAM->FLASH_rd_dummy_cyle0;
    flash_init_para.FLASH_rd_dummy_cyle[1] = RRAM->FLASH_rd_dummy_cyle1;
    flash_init_para.FLASH_rd_dummy_cyle[2] = RRAM->FLASH_rd_dummy_cyle2;

    flash_init_para.phase_shift_idx = RRAM->FLASH_phase_shift_idx;        
    flash_init_para.FLASH_rd_sample_phase_cal = RRAM->FLASH_rd_sample_phase_cal;
    flash_init_para.FLASH_rd_sample_phase = RRAM->FLASH_rd_sample_phase_cal;

    FLASH_CalibrationInit(30);

    /* we should open calibration new first, and then set phase index */
    FLASH_CalibrationNewCmd(ENABLE);
    FLASH_CalibrationPhaseIdx(flash_init_para.phase_shift_idx);
    
    /* this code is rom code, so it is safe */
    FLASH_Init(RRAM->FLASH_cur_bitmode);
}

/**
  * @brief  Get boot reason from AON & SYSON register, and back up to REG_LP_SYSTEM_CFG2
  * @param  NA
  * @retval 0.
  * @note this function is called once by bootloader when KM0 boot to flash, user can not use it.
  */
static uint32_t boot_reason_set(void) {
    uint32_t tmp_reason = 0;
    uint32_t reason_aon = 0;
    uint32_t temp_bootcfg = 0;

    /* get aon boot reason */
    reason_aon = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1);

    /* get syson boot reason */
    tmp_reason = BACKUP_REG->DWORD[0] & BIT_MASK_BOOT_REASON;
    tmp_reason = tmp_reason << BIT_BOOT_REASON_SHIFT;

    /* set dslp boot reason */
    if (reason_aon & BIT_AON_BOOT_EXIT_DSLP) {
        tmp_reason |= BIT_BOOT_DSLP_RESET_HAPPEN;
    } else {
        tmp_reason &= ~BIT_BOOT_DSLP_RESET_HAPPEN;
    }
    /* set BOD boot reason */
    if (reason_aon & BIT_AON_BOD_RESET_FLAG) {
        tmp_reason |= BIT_BOOT_BOD_RESET_HAPPEN;
    } else {
        tmp_reason &= ~BIT_BOOT_BOD_RESET_HAPPEN;
    }     

    if (tmp_reason == 0) {
        return 0;
    }

    /* reset reason register, because it can not be reset by HW */
    reason_aon &= ~(BIT_AON_BOOT_EXIT_DSLP | BIT_AON_BOD_RESET_FLAG);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1, reason_aon);
    BACKUP_REG->DWORD[0] &= ~BIT_MASK_BOOT_REASON;

    /* backup boot reason to REG_LP_SYSTEM_CFG2 */
    temp_bootcfg = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSTEM_CFG2);
    temp_bootcfg |= tmp_reason;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_SYSTEM_CFG2, tmp_reason);
    
    return 0;
}

static void app_start_autoicg(void) {
    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
    temp |= BIT_LSYS_PLFM_AUTO_ICG_EN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, temp);
}

static void app_load_patch_to_retention(uint32_t version) {
    if (version <= 0x1){
        _memcpy(&link_retention_ram_start, retention_ram_patch_array[0], RETENTION_RAM_SYS_OFFSET);
    } else {
        _memcpy(&link_retention_ram_start, retention_ram_patch_array[1], RETENTION_RAM_SYS_OFFSET);
    }

    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1);
    temp |= BIT_DSLP_RETENTION_RAM_PATCH;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1, temp);
}

static void app_pmc_patch() {
    /* flash pin floating issue */
    //0x4800_00B0[31:0] = 0x0080_1A12
    //0x4800_00B8[31:0] = 0x0A00_301A
    //0x4800_00BC[31:0] = 0x0801_3802
    //0x4800_00C0[31:0] = 0x00C0_0123
    uint32_t version = SYSCFG_CUTVersion();

    if (version <= 0x1){
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, 0xB0, 0x00801A12);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, 0xB8, 0x0A00301A);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, 0xBC, 0x08013802);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, 0xC0, 0x00C00123);
    }

    /*PMC cpu clock gate flow patch*/
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP1_L, 0xD2004D84);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP1_H, 0x202);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP2_L, 0x1A0048B4);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP2_H, 0x08088849);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP3_L, 0x021A4D4C);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP4_L, 0x0080645B);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP5_L, 0x121A4C64);
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYSON_PMC_PATCH_GRP5_H, 0x0805C300);

    /* SPIC clock source switch patch */
    app_load_patch_to_retention(version);
}

static uint32_t app_mpu_nocache_init(void) {
	mpu_region_config mpu_cfg;
	uint32_t mpu_entry = 0;

	mpu_entry = mpu_entry_alloc();
	mpu_cfg.region_base = 0x00000000;
	mpu_cfg.region_size = 0x000C4000;
	mpu_cfg.xn = MPU_EXEC_ALLOW;
	mpu_cfg.ap = MPU_UN_PRIV_RW;
	mpu_cfg.sh = MPU_NON_SHAREABLE;
	mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
	mpu_region_cfg(mpu_entry, &mpu_cfg);

	return 0;
}

void bodIrqHandler() {
    // Brown-out - reset
    if (km4_powered_on) {
        BOOT_ROM_CM4PON((u32)HSPWR_OFF_SEQ);
    }
    NVIC_SystemReset();
}

// Copy-paste from BOOT_FLASH_Image1()
void rtlLowLevelInit() {
    /*Get Chip Version*/
    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1);
    temp &= (~(BIT_MASK_AON_CHIP_VERSION_SW << BIT_SHIFT_AON_CHIP_VERSION_SW));
    if (0 != HAL_READ32(0, 0x1298)) {
        temp |= (SYSCFG_CUT_VERSION_A << BIT_SHIFT_AON_CHIP_VERSION_SW);
    } else {
        temp |= (SYSCFG_CUT_VERSION_B << BIT_SHIFT_AON_CHIP_VERSION_SW);
    }
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1, temp);

    // Use BOD interrupt as tresholds are much higher compared to reset mode
    BOR_ClearINT();
    InterruptRegister((IRQ_FUN)bodIrqHandler, BOR2_IRQ_LP, 0, 0);
    InterruptEn(BOR2_IRQ_LP, 0);
    BOR_ThresholdSet(BOR_TH_LOW7, BOR_TH_HIGH7);
    // BOR_ModeSet(BOR_INTR, ENABLE);
    // // XXX: Does this work? Enable BOD reset as well
    // BOR_ModeSet(BOR_RESET, ENABLE);
    BOR_ModeSet(BOR_INTR, DISABLE);
    BOR_ModeSet(BOR_RESET, DISABLE);

    boot_ram_function_enable();

    /* loguart use 40MHz */
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_GPIO_SHDN33_CTRL, 0xFFFF);

    /* temp for test */
    fast_timer_init();

    /* invalidate spic auto write */
    invalidate_flash_auto_write();

    /* set dslp boot reason */
    if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1) & BIT_AON_BOOT_EXIT_DSLP) {
        /* open PLL */
        BOOT_ROM_CM4PON((uint32_t)SYSPLL_ON_SEQ);
        dslp_flash_init();
    }

    boot_reason_set();

    /* for simulation set ASIC */
    if (ROM_SIM_ENABLE != 0x12345678) {
        BKUP_Set(BKUP_REG0, BIT_SW_SIM_RSVD);
    }

    BKUP_Write(BKUP_REG7, (uint32_t)&flash_init_para);

    // Copy-paste from app_start()

    SystemCoreClockUpdate();

    if(BIT_BOOT_DSLP_RESET_HAPPEN & BOOT_Reason()) {
        SOCPS_DsleepWakeStatusSet(TRUE);
    } else {
        SOCPS_DsleepWakeStatusSet(FALSE);
    }

    pinmap_init();

    if (SOCPS_DsleepWakeStatusGet() == FALSE) {
        OSC131K_Calibration(30000); /* PPM=30000=3% *//* 7.5ms */

        /* fix OTA upgarte fail after version 6.2, because 32k is not enabled*/
        temp = SDM32K_Read(REG_SDM_CTRL0);
        if (!(temp & BIT_AON_SDM_ALWAYS_CAL_EN)) {
            SDM32K_Enable(SDM32K_ALWAYS_CAL); /* 0.6ms */
        }
        
        SDM32K_RTCCalEnable(ps_config.km0_rtc_calibration); /* 0.3ms */

        temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1);
        temp &= ~BIT_DSLP_RETENTION_RAM_PATCH;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1, temp);
        // Retention Ram reset
        // Only clear sys stuff used by the SDK and part of RRAM_TypeDef again used by the SDK
        _memset((void*)RETENTION_RAM_BASE, 0, RETENTION_RAM_SYS_OFFSET + offsetof(RRAM_TypeDef, RRAM_USER_RSVD));
        assert_param(sizeof(RRAM_TypeDef) <= 0xB0);
    }

    OSC4M_Init();
    OSC2M_Calibration(OSC2M_CAL_CYC_128, 30000); /* PPM=30000=3% *//* 0.5 */

    SYSTIMER_Init(); /* 0.2ms */

    SOCPS_AONTimerCmd(DISABLE);

    SOCPS_SNOOZE_Config((BIT_XTAL_REQ_SNOOZE_MSK | BIT_CAPTOUCH_SNOOZE_MSK), ENABLE);
    
    app_start_autoicg();
    app_pmc_patch();

    mpu_init();
    app_mpu_nocache_init();
}

// app_pmu_init()
void rtlPmuInit() {
    uint32_t temp;

    if (BKUP_Read(BKUP_REG0) & BIT_SW_SIM_RSVD){
        return;
    }

    //pmu_set_sleep_type(SLEEP_CG);
    //pmu_acquire_wakelock(PMU_OS);
    //pmu_tickless_debug(ENABLE);

    //5: 0.9V
    //4: 0.85
    //3: 0.8V
    //2: 0.75V
    //1: 0.7V can not work normal
    //setting switch regulator PFM mode voltage
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWR_PSW_CTRL);
    temp &= 0x00FF0000;
#ifdef CONFIG_VERY_LOW_POWER
    temp |= 0x3F007532;//8:1.05v 7:1.0v 6:0.95v 5:0.9v 4 stage waiting time 500us
    //temp |= 0x7F007532;//8:1.05v 7:1.0v 6:0.95v 5:0.9v 4 stage waiting time 2ms
#else
    temp |= 0x3F007654;//>0.81V is safe for MP
    //temp |= 0x7F007654;//>0.81V is safe for MP
#endif
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWR_PSW_CTRL,temp);

    /* Enable PFM to PWM delay to fix voltage peak issue when PFM=>PWM */
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_SWR_CTRL1);
    temp |= BIT_SWR_ENFPWMDELAY_H;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_SWR_CTRL1,temp);

    /* Set SWR ZCD & Voltage */
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG1);
    if (wifi_config.wifi_ultra_low_power) {
        temp &= ~(0x0f);//SWR @ 1.05V
        temp |= (0x07);
        temp &= ~BIT_MASK_SWR_REG_ZCDC_H; /* reg_zcdc_H: EFUSE[5]BIT[6:5] 00 0.1u@PFM */ /* 4uA @ sleep mode */
    } else {
        /* 2mA higher in active mode */
        temp &= ~BIT_MASK_SWR_REG_ZCDC_H; /* reg_zcdc_H: EFUSE[5]BIT[6:5] 00 0.1u@PFM */ /* 4uA @ sleep mode */
    }
    /*Mask OCP setting, or some chip won't wake up after sleep*/
    //temp &= ~BIT_MASK_SWR_OCP_L1;
    //temp |= (0x03 << BIT_SHIFT_SWR_OCP_L1); /* PWM:600 PFM: 250, default OCP: BIT[10:8] 100 */
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG1,temp);

    /* LDO & SWR switch time when DSLP, default is 0x200=5ms (unit is 1cycle of 100K=10us) */
    temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_PMC_CTRL);
    temp &= ~(BIT_AON_MASK_PMC_PSW_STABLE << BIT_AON_SHIFT_PMC_PSW_STABLE);
    temp |= (0x60 << BIT_AON_SHIFT_PMC_PSW_STABLE);//set to 960us
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_PMC_CTRL,temp);

    /* shutdown internal test pad GPIOF9 to fix wowlan power leakage issue */
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_GPIO_F9_PAD_CTRL, 0x0000DC00);
}

void rtlPowerOnBigCore() {
    InterruptRegister((IRQ_FUN)IPC_INTHandler, IPC_IRQ_LP, (uint32_t)IPCM4_DEV, 2);
    InterruptEn(IPC_IRQ_LP, 2);
    InterruptDis(UART_LOG_IRQ_LP);

    if (SOCPS_DsleepWakeStatusGet() == FALSE) {
        km4_flash_highspeed_init();
    }

    /* Disable RSIP if it is enabled. Not needed after KM0 boot */
    // Note: it should be executed after km4_flash_highspeed_init(), otherwise, the flash
    // speed is lowered down if AES encryption is enabled.
    if ((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) != 0 ) {
        uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control & (~BIT_LSYS_PLFM_FLASH_SCE)));
    }

    km4_powered_on = true;
    km4_boot_on();
}

void pmu_acquire_wakelock(uint32_t nDeviceId) {
    // Stub
}

void pmu_release_wakelock(uint32_t nDeviceId) {
    // stub
}

void ipc_table_init() {
    // stub
}

void ipc_send_message(uint8_t channel, uint32_t message) {
    // stub
}

uint32_t ipc_get_message(uint8_t channel) {
    // stub
    return 0;
}

uint32_t pmu_exec_sleep_hook_funs(void) {
    // stub
    return PMU_MAX;
}

void pmu_exec_wakeup_hook_funs(uint32_t nDeviceIdMax) {
    // stub
}
