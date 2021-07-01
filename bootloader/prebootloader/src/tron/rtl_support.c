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

void pmu_acquire_wakelock(uint32_t nDeviceId) {
    // Stub
}

void pmu_release_wakelock(uint32_t nDeviceId) {
    // stub
}

void ipc_table_init() {
    // Stub
}

extern CPU_PWR_SEQ SYSPLL_ON_SEQ[];
extern void BOOT_FLASH_fasttimer_init();
extern void BOOT_FLASH_DSLP_FlashInit();
extern void BOOT_FLASH_Reason_Set();

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
