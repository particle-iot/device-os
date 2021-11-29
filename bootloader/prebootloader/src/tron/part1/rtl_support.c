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

extern CPU_PWR_SEQ SYSPLL_ON_SEQ[];

void BOOT_FLASH_Invalidate_Auto_Write(void)
{
    /* Auto write related bits in valid command register are all set to 0,
        just need to invalidate write single and write enable cmd in auto mode. */
    SPIC_TypeDef *spi_flash = SPIC;

    /* Disable SPI_FLASH User Mode */
    spi_flash->ssienr = 0;
    
    /* Invalidate write single and write enable cmd in auto mode */
    spi_flash->wr_single = 0x0;
    spi_flash->wr_enable = 0x0;
}

void BOOT_RAM_FuncEnable(void)
{
    u32 Temp = 0;

    /*Reset SIC function*/
    Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0);
    Temp &= ~BIT_LSYS_SIC_FEN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN0, Temp);
    /*Reset SIC clock*/
    Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0);
    Temp &= ~BIT_SIC_CLK_EN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0, Temp);

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

void BOOT_FLASH_fasttimer_init(void)
{
    RTIM_TimeBaseInitTypeDef TIM_InitStruct;

    RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
    RTIM_TimeBaseStructInit(&TIM_InitStruct);

    TIM_InitStruct.TIM_Idx = 5;
    TIM_InitStruct.TIM_Prescaler = 39; //40M/40 =1M->1us
    TIM_InitStruct.TIM_Period = 0xFFFF;

    TIM_InitStruct.TIM_UpdateEvent = ENABLE; /* UEV enable */
    TIM_InitStruct.TIM_UpdateSource = TIM_UpdateSource_Overflow;
    TIM_InitStruct.TIM_ARRProtection = ENABLE;

    RTIM_TimeBaseInit(TIMM05, &TIM_InitStruct, 0, (IRQ_FUN) NULL, (u32)NULL);
    RTIM_Cmd(TIMM05, ENABLE);
}

void BOOT_FLASH_DSLP_FlashInit(void)
{
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
u32 BOOT_FLASH_Reason_Set(void)
{
    u32 tmp_reason = 0;
    u32 reason_aon = 0;
    u32 temp_bootcfg = 0;

    /* get aon boot reason */
    reason_aon = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1);

    /* get syson boot reason */
    tmp_reason = BACKUP_REG->DWORD[0] & BIT_MASK_BOOT_REASON;
    tmp_reason = tmp_reason << BIT_BOOT_REASON_SHIFT;

    //DBG_8195A("BOOT_FLASH_Reason_Set: %x %x \n", reason_aon, tmp_reason);
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

    if ((tmp_reason & BIT_BOOT_KM4SYS_RESET_HAPPEN) && (tmp_reason & BIT_BOOT_KM4WDG_RESET_HAPPEN)) {
        tmp_reason &= ~BIT_BOOT_KM4WDG_RESET_HAPPEN;
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

// Copy-paste from BOOT_FLASH_Image1()
void rtlLowLevelInit() {
    BOOT_RAM_FuncEnable();
    /* temp for test */
    BOOT_FLASH_fasttimer_init();

    /* invalidate spic auto write */
    BOOT_FLASH_Invalidate_Auto_Write();

    /* set dslp boot reason */
    if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_BOOT_REASON1) & BIT_AON_BOOT_EXIT_DSLP) {
        /* open PLL */
        BOOT_ROM_CM4PON((u32)SYSPLL_ON_SEQ);
        BOOT_FLASH_DSLP_FlashInit();
    }

    BOOT_FLASH_Reason_Set();

    /* for simulation set ASIC */
    if (ROM_SIM_ENABLE != 0x12345678) {
        BKUP_Set(BKUP_REG0, BIT_SW_SIM_RSVD);
    }

    /* Disable RSIP if it is enabled. Not needed after KM0 boot */
    if((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) != 0 ) {
        uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control & (~BIT_LSYS_PLFM_FLASH_SCE)));
    }

    OSC4M_Init();
    OSC2M_Calibration(OSC2M_CAL_CYC_128, 30000); /* PPM=30000=3% *//* 0.5 */
}

// app_pmu_init()
void rtlPmuInit()
{
    u32 Temp;

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
    Temp=HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWR_PSW_CTRL);
    Temp &= 0x00FF0000;
#ifdef CONFIG_VERY_LOW_POWER
    Temp |= 0x3F007532;//8:1.05v 7:1.0v 6:0.95v 5:0.9v 4 stage waiting time 500us
    //Temp |= 0x7F007532;//8:1.05v 7:1.0v 6:0.95v 5:0.9v 4 stage waiting time 2ms
#else
    Temp |= 0x3F007654;//>0.81V is safe for MP
    //Temp |= 0x7F007654;//>0.81V is safe for MP
#endif
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWR_PSW_CTRL,Temp);

    /* Enable PFM to PWM delay to fix voltage peak issue when PFM=>PWM */
    Temp=HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_SWR_CTRL1);
    Temp |= BIT_SWR_ENFPWMDELAY_H;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_SWR_CTRL1,Temp);

    /* Set SWR ZCD & Voltage */
    Temp=HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG1);
    if (wifi_config.wifi_ultra_low_power) {
        Temp &= ~(0x0f);//SWR @ 1.05V
        Temp |= (0x07);
        Temp &= ~BIT_MASK_SWR_REG_ZCDC_H; /* reg_zcdc_H: EFUSE[5]BIT[6:5] 00 0.1u@PFM */ /* 4uA @ sleep mode */
    } else {
        /* 2mA higher in active mode */
        Temp &= ~BIT_MASK_SWR_REG_ZCDC_H; /* reg_zcdc_H: EFUSE[5]BIT[6:5] 00 0.1u@PFM */ /* 4uA @ sleep mode */
    }
    /*Mask OCP setting, or some chip won't wake up after sleep*/
    //Temp &= ~BIT_MASK_SWR_OCP_L1;
    //Temp |= (0x03 << BIT_SHIFT_SWR_OCP_L1); /* PWM:600 PFM: 250, default OCP: BIT[10:8] 100 */
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG1,Temp);

    /* LDO & SWR switch time when DSLP, default is 0x200=5ms (unit is 1cycle of 100K=10us) */
    Temp=HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_PMC_CTRL);
    Temp &= ~(BIT_AON_MASK_PMC_PSW_STABLE << BIT_AON_SHIFT_PMC_PSW_STABLE);
    Temp |= (0x60 << BIT_AON_SHIFT_PMC_PSW_STABLE);//set to 960us
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_PMC_CTRL,Temp);

    /* shutdown internal test pad GPIOF9 to fix wowlan power leakage issue */
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_GPIO_F9_PAD_CTRL, 0x0000DC00);
}

void rtlPowerOnBigCore() {
    InterruptRegister((IRQ_FUN)IPC_INTHandler, IPC_IRQ_LP, (u32)IPCM4_DEV, 2);
    InterruptEn(IPC_IRQ_LP, 2);
    InterruptDis(UART_LOG_IRQ_LP);

    km4_flash_highspeed_init();

    // FIXME: This might not be working correctly?
    // if (!SOCPS_DsleepWakeStatusGet()) {
        /* backup flash_init_para address for KM4 */
        BKUP_Write(BKUP_REG7, (u32)&flash_init_para);
    // }

    km4_boot_on();
}
