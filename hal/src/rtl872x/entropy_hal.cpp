/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "entropy_hal.h"

#include "adc_hal.h"
#include "check.h"
#include "scope_guard.h"
#include "timer_hal.h"

#include <string.h>

extern "C" {
#include "rtl8721d.h"
#include "rtl8721d_adc.h"
}

#define APBPeriph_ADC_CLOCK         (SYS_CLK_CTRL1 << 30 | BIT_LSYS_ADC_CKE)
#define APBPeriph_ADC               (SYS_FUNC_EN1 << 30 | BIT_LSYS_ADC_FEN)

namespace {

using namespace particle;

const unsigned REPETITION_COUNT_CUTOFF = 81;
const unsigned ADAPTIVE_PROPORTION_WINDOW = 1024;
const unsigned ADAPTIVE_PROPORTION_CUTOFF = 914;

struct AdcState {
    bool functionEnabled;
    bool clockEnabled;
    uint32_t reg1xLpad;
    uint32_t intrCtrl;
    uint32_t clkDiv;
    uint32_t conf;
    uint32_t inType;
    uint32_t chswList[2];
    uint32_t itChnoCon;
    uint32_t fullLvl;
    uint32_t dmaCon;
    uint32_t delayCnt;
};

class AdcNoiseSource {
public:
    int read(uint8_t* data, size_t size, size_t sampleCount) {
        CHECK_TRUE(data && size && sampleCount >= size * 8, SYSTEM_ERROR_INVALID_ARGUMENT);

        AdcLock lk;
        setup();
        SCOPE_GUARD({
            restore();
        });

        memset(data, 0, size);
        for (size_t i = 0; i < sampleCount; ++i) {
            const int sample = CHECK(readSample());
            CHECK(healthTest(sample));
            if (i < size * 8) {
                data[i / 8] |= sample << (i % 8);
            }
        }
        return SYSTEM_ERROR_NONE;
    }

private:
    int healthTest(uint8_t sample) {
        if (sample == lastSample_) {
            CHECK_TRUE(++repetitionCount_ < REPETITION_COUNT_CUTOFF, SYSTEM_ERROR_BAD_DATA);
        } else {
            lastSample_ = sample;
            repetitionCount_ = 1;
        }

        if (proportionPosition_ == 0) {
            proportionSample_ = sample;
            proportionCount_ = 1;
            proportionPosition_ = 1;
            return SYSTEM_ERROR_NONE;
        }

        if (sample == proportionSample_) {
            CHECK_TRUE(++proportionCount_ < ADAPTIVE_PROPORTION_CUTOFF, SYSTEM_ERROR_BAD_DATA);
        }
        if (++proportionPosition_ == ADAPTIVE_PROPORTION_WINDOW) {
            proportionPosition_ = 0;
        }
        return SYSTEM_ERROR_NONE;
    }

    void setup() {
        state_.functionEnabled = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN1) & BIT_LSYS_ADC_FEN;
        state_.clockEnabled = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL1) & BIT_LSYS_ADC_CKE;
        state_.reg1xLpad = CAPTOUCH_DEV->CT_ADC_REG1X_LPAD;
        state_.intrCtrl = ADC->ADC_INTR_CTRL;
        state_.clkDiv = ADC->ADC_CLK_DIV;
        state_.conf = ADC->ADC_CONF;
        state_.inType = ADC->ADC_IN_TYPE;
        state_.chswList[0] = ADC->ADC_CHSW_LIST[0];
        state_.chswList[1] = ADC->ADC_CHSW_LIST[1];
        state_.itChnoCon = ADC->ADC_IT_CHNO_CON;
        state_.fullLvl = ADC->ADC_FULL_LVL;
        state_.dmaCon = ADC->ADC_DMA_CON;
        state_.delayCnt = ADC->ADC_DELAY_CNT;

        CAPTOUCH_DEV->CT_ADC_REG1X_LPAD = state_.reg1xLpad | BIT(6) | BIT(7);
        RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, DISABLE);
        RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);
        ADC_Cmd(DISABLE);
        ADC->ADC_INTR_CTRL = 0;
        ADC_INTClear();
        ADC_ClearFIFO();
        ADC->ADC_CLK_DIV = ADC_CLK_DIV_12;
        ADC->ADC_CONF = 0;
        ADC->ADC_IN_TYPE = 0;
        ADC->ADC_CHSW_LIST[0] = ADC_CH8;
        ADC->ADC_CHSW_LIST[1] = 0;
        ADC->ADC_IT_CHNO_CON = 0;
        ADC->ADC_FULL_LVL = 0;
        ADC->ADC_DMA_CON = 0x700;
        ADC->ADC_DELAY_CNT = 0;
        ADC_Cmd(ENABLE);
    }

    void restore() {
        ADC_Cmd(DISABLE);
        CAPTOUCH_DEV->CT_ADC_REG1X_LPAD = state_.reg1xLpad;
        ADC->ADC_INTR_CTRL = state_.intrCtrl;
        ADC->ADC_CLK_DIV = state_.clkDiv;
        ADC->ADC_CONF = state_.conf & ~BIT_ADC_ENABLE;
        ADC->ADC_IN_TYPE = state_.inType;
        ADC->ADC_CHSW_LIST[0] = state_.chswList[0];
        ADC->ADC_CHSW_LIST[1] = state_.chswList[1];
        ADC->ADC_IT_CHNO_CON = state_.itChnoCon;
        ADC->ADC_FULL_LVL = state_.fullLvl;
        ADC->ADC_DMA_CON = state_.dmaCon;
        ADC->ADC_DELAY_CNT = state_.delayCnt;
        if (state_.conf & BIT_ADC_ENABLE) {
            ADC_Cmd(ENABLE);
        }

        auto value = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN1);
        value = state_.functionEnabled ? value | BIT_LSYS_ADC_FEN : value & ~BIT_LSYS_ADC_FEN;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_FUNC_EN1, value);

        value = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL1);
        value = state_.clockEnabled ? value | BIT_LSYS_ADC_CKE : value & ~BIT_LSYS_ADC_CKE;
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL1, value);
    }

    int readSample() {
        ADC_SWTrigCmd(ENABLE);
        SCOPE_GUARD({
            ADC_SWTrigCmd(DISABLE);
        });
        const system_tick_t start = HAL_Timer_Get_Micro_Seconds();
        while (ADC_Readable() == 0) {
            CHECK_TRUE(HAL_Timer_Get_Micro_Seconds() - start < 1000, SYSTEM_ERROR_TIMEOUT);
        }
        return ADC_Read() & 1;
    }

    AdcState state_ = {};
    uint8_t lastSample_ = 0xff;
    unsigned repetitionCount_ = 0;
    uint8_t proportionSample_ = 0;
    unsigned proportionCount_ = 0;
    unsigned proportionPosition_ = 0;
};

} // anonymous namespace

int hal_entropy_read(uint8_t* data, size_t size, size_t sampleCount) {
    AdcNoiseSource source;
    return source.read(data, size, sampleCount);
}
