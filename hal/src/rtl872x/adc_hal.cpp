/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

extern "C" {
#include "rtl8721d.h"
}
#include "adc_hal.h"
#include "system_tick_hal.h"
#include "pinmap_impl.h"
#include "timer_hal.h"
#include "check.h"
#include "delay_hal.h"
#include "service_debug.h"
#include "logging.h"
#include "system_cache.h"
#include <algorithm>

#define APBPeriph_ADC_CLOCK         (SYS_CLK_CTRL1  << 30 | BIT_LSYS_ADC_CKE)
#define APBPeriph_ADC               (SYS_FUNC_EN1  << 30 | BIT_LSYS_ADC_FEN)
#define DEFAULT_ADC_RESOLUTION_BITS (12)

#define WAIT_TIMED(timeout_ms, what) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    bool res = true;                                                            \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})

namespace {

using namespace particle::services;

class Adc {
public:
    int init() {
        RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

        ADC_InitTypeDef adcInitStruct = {};
        ADC_StructInit(&adcInitStruct);
        adcInitStruct.ADC_CvlistLen = 0;
        ADC_Init(&adcInitStruct);
        ADC_Cmd(ENABLE);

        if (getCachedOffset(&adcOffset_) != SYSTEM_ERROR_NONE) {
            calibration();
        }

        // Get ADC GAIN parameter from efuse
        uint8_t efuseBuf[2] = {};
        for (uint8_t index = 0; index < 2; index++) {
            EFUSE_PMAP_READ8(0, adcGainEfuseAddr + index, efuseBuf + index, L25EOUTVOLTAGE);
        }
        adcGain_ = efuseBuf[1] << 8 | efuseBuf[0];
        if (adcGain_ == 0xFFFF) {
            adcGain_ = 0x2F12;
        }

        adcState_ = HAL_ADC_STATE_ENABLED;
        return SYSTEM_ERROR_NONE;
    }

    int deinit() {
        ADC_INTClear();
        ADC_Cmd(DISABLE);
        RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, DISABLE);
        adcState_ = HAL_ADC_STATE_DISABLED;
        return SYSTEM_ERROR_NONE;
    }

    int read(uint16_t pin) {
        CHECK_TRUE(hal_pin_is_valid(pin), 0);
        hal_pin_info_t* pinInfo = hal_pin_map() + pin;
        if (pinInfo->adc_channel == ADC_CHANNEL_NONE) {
            return 0;
        }
        if (pinInfo->pin_func != PF_NONE && pinInfo->pin_func != PF_DIO) {
            return 0;
        }

        if (adcState_ != HAL_ADC_STATE_ENABLED) {
            init();
        }

        /* Set channel index into channel switch list*/
        ADC->ADC_CHSW_LIST[0] = pinInfo->adc_channel;
        ADC_ClearFIFO();

        ADC_SWTrigCmd(ENABLE);
        if (!WAIT_TIMED(10, ADC_Readable() == 0)) {
            ADC_SWTrigCmd(DISABLE);
            return 0;
        }
        ADC_SWTrigCmd(DISABLE);

        // (not used anymore) adcOffset - eFuse programmed ADC offset (raw * 10)
        // adcGain - eFuse programmed ADC transfer function slope ((y1-y0)/(yideal1 - yideal0) * 10000)
        // adcAtGndValue - raw ADC value acquired from internal channel connected to ground reference, filtered/averaged
        uint32_t adcValue = (ADC_Read() & BIT_MASK_DAT_GLOBAL);
        uint32_t adcOffsetClamped = std::min(adcValue, (uint_least32_t)adcOffset_); // Clamps to zero to avoid negative values
        uint32_t adcValueGainOffsetCorrectedAndZeroCalibrated = ((adcValue - adcOffsetClamped) * adcGain_) / 10000; // 0-4095

        LOG(TRACE, "ch8_gnd: %d, raw_value: %d, calib_value: %d", adcOffset_, adcValue, adcValueGainOffsetCorrectedAndZeroCalibrated);

        return (uint16_t)adcValueGainOffsetCorrectedAndZeroCalibrated;
    }

    int sleep(bool sleep) {
        if (sleep) {
            // Suspend ADC
            CHECK_TRUE(adcState_ == HAL_ADC_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);
            deinit();
            adcState_ = HAL_ADC_STATE_SUSPENDED;
        } else {
            // Restore ADC
            CHECK_TRUE(adcState_ == HAL_ADC_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
            init();
        }
        return SYSTEM_ERROR_NONE;
    }

    int calibration() {
        // Read the analog value of a CH8(Channel 8), CH8 is connected to the internal GND
        adcOffset_ = getInternalGndValue();
        return setCachedOffset(adcOffset_);
    }

private:
    int getCachedOffset(uint16_t* adcOffset) {
        uint16_t offset = 0;
        int ret = SystemCache::instance().get(SystemCacheKey::ADC_CALIBRATION_OFFSET, &offset, sizeof(offset));
        if (ret != sizeof(offset)) {
            return SYSTEM_ERROR_NOT_FOUND;
        }

        // Check the validity of cached OFFSET by comparing it with the efuse OFFSET * ±20%
        uint8_t efuseBuf[2] = {};
        for (uint8_t index = 0; index < 2; index++) {
            EFUSE_PMAP_READ8(0, adcOffsetEfuseAddr + index, efuseBuf + index, L25EOUTVOLTAGE);
        }
        uint16_t efuseOffset = (efuseBuf[1] << 8 | efuseBuf[0]) / 10;
        if (std::abs((int)offset - (int)efuseOffset) > (int)(efuseOffset * 0.2)) {
            LOG(WARN, "Invalid ADC offset detected, cached %d, efuse: %d", offset, efuseOffset);
            return SYSTEM_ERROR_BAD_DATA;
        }

        if (adcOffset) {
            *adcOffset = offset;
        }
        return SYSTEM_ERROR_NONE;
    }

    int setCachedOffset(uint16_t adcOffset) {
        return SystemCache::instance().set(SystemCacheKey::ADC_CALIBRATION_OFFSET, &adcOffset, sizeof(adcOffset));
    }

    uint16_t getInternalGndValue() {
        uint32_t gndValue = 0;
        uint32_t sumGndValue = 0;
        LOG_PRINTF(TRACE, "raw ch8: ");
        for (int i = 0; i < 100; i++) {
            // The internal reference of CH8 and CH9 is GND, choose either of them for self-calibration
            ADC->ADC_CHSW_LIST[0] = ADC_CH8;
            ADC_ClearFIFO();

            ADC_SWTrigCmd(ENABLE);
            if (!WAIT_TIMED(10, ADC_Readable() == 0)) {
                ADC_SWTrigCmd(DISABLE);
                return 0;
            }
            ADC_SWTrigCmd(DISABLE);

            gndValue = (ADC_Read() & BIT_MASK_DAT_GLOBAL);
            sumGndValue += gndValue;
            LOG_PRINTF(TRACE, "%d ", gndValue);
        }
        LOG_PRINTF(TRACE, "\r\n");

        LOG_PRINTF(TRACE, "avg ch8: %d\r\n", sumGndValue / 100);
        return (uint16_t)(sumGndValue / 100);
    }

private:
    uint16_t adcOffset_ = 0xFFFF;  // OFFSET: 10 times of sample data at 0.000v, 10*value(0.000v)
    uint16_t adcGain_ = 0xFFFF;    // GAIN: 10 times of value(1.000v)-value(0.000v) or value(2.000v)-value(1.000v) or value(3.000v)-value(2.000v)
    volatile hal_adc_state_t adcState_ = HAL_ADC_STATE_DISABLED;

    static constexpr uint32_t adcOffsetEfuseAddr = 0x1D0;
    static constexpr uint32_t adcGainEfuseAddr = 0x1D2;
};

Adc adc;

} // anonymous


void hal_adc_set_sample_time(uint8_t sample_time) {
    // deprecated
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t hal_adc_read(uint16_t pin) {
    return adc.read(pin);
}

int hal_adc_sleep(bool sleep, void* reserved) {
    return adc.sleep(sleep);
}

int hal_adc_calibrate(uint32_t reserved, void* reserved1) {
    return adc.calibration();
}
