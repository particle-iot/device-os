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

#include "periph_lock.h"
#include "rng_hal.h"
#include "logging.h"
extern "C" {
#include "rtl8721d.h"
}

static void app_gen_random_seed(void)
{
    const uint32_t APBPeriph_ADC_CLOCK = (SYS_CLK_CTRL1  << 30 | BIT_LSYS_ADC_CKE);
    const uint32_t APBPeriph_ADC = (SYS_FUNC_EN1  << 30 | BIT_LSYS_ADC_FEN);

    u16 value;
    u32 data;
    int i = 0, j = 0;
    u8 random[4], tmp;

    /*Write reg CT_ADC_REG1X_LPAD[7:6] with 2b'11*/
    u32 RegTemp, RegData;
    CAPTOUCH_TypeDef *CapTouch = CAPTOUCH_DEV;
    RegData = (u32)CapTouch->CT_ADC_REG1X_LPAD;
    RegTemp = RegData | (BIT(6) | BIT(7));
    CapTouch->CT_ADC_REG1X_LPAD = RegTemp;

    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, DISABLE);
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

    ADC_Cmd(DISABLE);

    ADC->ADC_INTR_CTRL = 0;
    ADC_INTClear();
    ADC_ClearFIFO();
    ADC->ADC_CLK_DIV = 0;
    ADC->ADC_CONF = 0;
    ADC->ADC_IN_TYPE = 0;
    ADC->ADC_CHSW_LIST[0] = 8;
    ADC->ADC_CHSW_LIST[1] = 0;
    ADC->ADC_IT_CHNO_CON = 0;
    ADC->ADC_FULL_LVL = 0;
    ADC->ADC_DMA_CON = 0x700;
    ADC->ADC_DELAY_CNT = 0x00000000;

    ADC_Cmd(ENABLE);

    for(i = 0; i < 4; i++) {
retry:
        tmp = 0;
        for(j = 0; j < 8; j++) {
            ADC_SWTrigCmd(ENABLE);
            while(ADC_Readable() == 0);
            ADC_SWTrigCmd(DISABLE);

            value = ADC_Read();
            data = value & BIT_MASK_DAT_GLOBAL;
            tmp |= ((data & BIT(0)) << j);
        }

        if(tmp == 0 || tmp == 0xFF)
            goto retry;

        random[i] = tmp;
    }

    ADC_Cmd(DISABLE);

    /*Restore reg CT_ADC_REG1X_LPAD[7:6] of initial value 2b'00*/
    CapTouch->CT_ADC_REG1X_LPAD = RegData;

    rand_first = 1;
    data = (random[3] << 24) | (random[2] << 16) | (random[1] << 8) | (random[0]);
    rand_seed[0] = data;
    rand_seed[1] = data;
    rand_seed[2] = data;
    rand_seed[3] = data;

    return;
}

void HAL_RNG_Configuration() {
    app_gen_random_seed();
}

uint32_t HAL_RNG_GetRandomNumber() {
    periph_lock();
    uint32_t random = Rand();
    periph_unlock();
    return random;
}
