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

        // Get ADC GAIN parameter from efuse
        uint8_t efuseBuf[2] = {};
        for (uint8_t index = 0; index < 2; index++) {
            EFUSE_PMAP_READ8(0, EFUSE_ADC_GAIN + index, efuseBuf + index, L25EOUTVOLTAGE);
        }
        adcGain_ = efuseBuf[1] << 8 | efuseBuf[0];
        if (adcGain_ == 0xFFFF) {
            // This should not happen, right?
            adcGain_ = DEFAULT_GAIN;
        }
        for (uint8_t index = 0; index < 2; index++) {
            EFUSE_PMAP_READ8(0, EFUSE_ADC_OFFSET + index, efuseBuf + index, L25EOUTVOLTAGE);
        }
        adcOffset_ = (efuseBuf[1] << 8 | efuseBuf[0]);

        if (getCachedOffset(&adcCh8Offset_) != SYSTEM_ERROR_NONE) {
            calibration();
        }

        LOG_DEBUG(TRACE, "adcGain=%u adcOffset=%u adcCh8Offset=%u", adcGain_, adcOffset_, adcCh8Offset_);

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

        uint32_t adcValueRaw = (ADC_Read() & BIT_MASK_DAT_GLOBAL);
        LOG_DEBUG(TRACE, "raw adc=%u", adcValueRaw);
        int mv = rawAdcToMv(adcValueRaw, adcOffset_, adcGain_);
        LOG_DEBUG(TRACE, "efuse corrected mv=%d", mv);
        // Convert to 0 - 4095
        uint32_t adcValue = mvToAdcRange(mv);
        LOG_DEBUG(TRACE, "efuse corrected adc=%u", adcValue);
        if (adcValue <= mvToAdcRange(NON_LINEAR_SECTION_LOW_MV)) {
            LOG_DEBUG(TRACE, "<= 800mV");
            // <= 800mV
            // Calculate new gain using two points (0mV with CH8 offset, 800mV with efuse offset)
            uint32_t efuse800Raw = mvToRawAdc(NON_LINEAR_SECTION_LOW_MV, adcOffset_, adcGain_);
            uint32_t ch8GndRaw = mvToRawAdc(ZERO_MV, adcCh8Offset_ + CH8_ADJUSTMENT, adcGain_);
            // Calculate new gain based on (0, ch8GndRaw) and (800, efuse800Raw)
            uint32_t gain = ((efuse800Raw - ch8GndRaw) * 10000) / (NON_LINEAR_SECTION_LOW_MV - ZERO_MV);
            uint32_t newOffset = adcCh8Offset_ + CH8_ADJUSTMENT;
            LOG_DEBUG(TRACE, "efuse800Raw=%u ch8GndRaw=%u gain=%u newOffset=%u", efuse800Raw, ch8GndRaw, gain, newOffset);
            mv = rawAdcToMv(adcValueRaw, newOffset, gain);
            LOG_DEBUG(TRACE, "mv ch8 corrected = %d", mv);
            adcValue = mvToAdcRange(mv);
            LOG_DEBUG(TRACE, "ch8 corrected adc=%u", adcValue);
        } else if (adcValue >= mvToAdcRange(NON_LINEAR_SECTION_HIGH_MV)) {
            LOG_DEBUG(TRACE, ">= 2000mV");
            // >= 2000mV
            // Calculate new gain using two points (3300mV with CH8 offset, 2000mV with efuse offset)
            uint32_t efuse2000Raw = mvToRawAdc(NON_LINEAR_SECTION_HIGH_MV, adcOffset_, adcGain_);
            uint32_t ch8VccRaw = mvToRawAdc(VCC_MV, adcCh8Offset_ + CH8_ADJUSTMENT, adcGain_);
            // Calculate new gain based on (2000mV, efuse200Raw) and (3300mV, ch8VccRaw)
            uint32_t gain = ((ch8VccRaw - efuse2000Raw) * 10000) / (VCC_MV - NON_LINEAR_SECTION_HIGH_MV);
            uint32_t newOffset = offsetFromAdcAndMv(efuse2000Raw, NON_LINEAR_SECTION_HIGH_MV, gain);
            LOG_DEBUG(TRACE, "efuse2000Raw=%u ch8VccRaw=%u gain=%u newOffset=%u", efuse2000Raw, ch8VccRaw, gain, newOffset);
            mv = rawAdcToMv(adcValueRaw, newOffset, gain);
            LOG_DEBUG(TRACE, "mv ch8 corrected = %d", mv);
            adcValue = mvToAdcRange(mv);
            LOG_DEBUG(TRACE, "ch8 corrected adc=%u", adcValue);
        }

        return adcValue;
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
        adcCh8Offset_ = getInternalGndValue();
        return setCachedOffset(adcCh8Offset_);
    }

    static Adc& instance() {
        static Adc adc;
        return adc;
    }

private:
    int getCachedOffset(uint16_t* adcOffset) {
        uint16_t offset = 0;
        CHECK(SystemCache::instance().get(SystemCacheKey::ADC_CALIBRATION_OFFSET, &offset, sizeof(offset)));

        // Check the validity of cached OFFSET by comparing it with the efuse OFFSET * ±20%
        if (std::abs((int)offset - (int)adcOffset_) > (int)(adcOffset_ * 2) / 10) {
            LOG_DEBUG(WARN, "Invalid ADC offset detected, cached %u, efuse: %u", (unsigned)offset, (unsigned)adcOffset_);
            return SYSTEM_ERROR_BAD_DATA;
        }

        if (adcOffset) {
            *adcOffset = offset;
        }
        return SYSTEM_ERROR_NONE;
    }

    int setCachedOffset(uint16_t adcOffset) {
        int r = SystemCache::instance().set(SystemCacheKey::ADC_CALIBRATION_OFFSET, &adcOffset, sizeof(adcOffset));
        LOG_DEBUG(INFO, "set cached offset %u, res= %d", adcOffset, r);
        return r;
    }

    uint16_t getInternalGndValue() {
        uint32_t sumGndValue = 0;
        for (unsigned i = 0; i < CH8_SAMPLE_COUNT; i++) {
            // The internal reference of CH8 and CH9 is GND, choose either of them for self-calibration
            ADC->ADC_CHSW_LIST[0] = ADC_CH8;
            ADC_ClearFIFO();

            ADC_SWTrigCmd(ENABLE);
            if (!WAIT_TIMED(10, ADC_Readable() == 0)) {
                ADC_SWTrigCmd(DISABLE);
                return 0;
            }
            ADC_SWTrigCmd(DISABLE);

            uint32_t gndValue = (ADC_Read() & BIT_MASK_DAT_GLOBAL);
            sumGndValue += gndValue;
        }

        sumGndValue /= CH8_SAMPLE_COUNT / 10; // multiplied by 10 same as efuse offset

        LOG_DEBUG(TRACE, "avg ch8: %u", sumGndValue);
        return sumGndValue;
    }

    static constexpr uint32_t mvToAdcRange(int mv) {
        mv = std::max(mv, 0);
        return std::min<uint32_t>((mv * ((1 << DEFAULT_ADC_RESOLUTION_BITS) - 1)) / 3300, ((1 << DEFAULT_ADC_RESOLUTION_BITS) - 1));
    }

    static constexpr int rawAdcToMv(uint32_t adc, uint32_t offset, uint32_t gain) {
        return ((10 * (int)adc - (int)offset) * 1000) / (int)gain;
    }

    static constexpr uint32_t mvToRawAdc(int mv, uint32_t offset, uint32_t gain) {
        return ((int)(mv * gain) / 1000 + offset) / 10;
    }

    static constexpr uint32_t offsetFromAdcAndMv(uint32_t adc, uint32_t mv, uint32_t gain) {
        return 10 * adc - mv * gain / 1000;
    }

    Adc() {
        init();
    }

private:
    uint16_t adcOffset_ = 0xFFFF;  // OFFSET: 10 times of sample data at 0.000v, 10*value(0.000v)
    uint16_t adcGain_ = 0xFFFF;    // GAIN: 10 times of value(1.000v)-value(0.000v) or value(2.000v)-value(1.000v) or value(3.000v)-value(2.000v)
    uint16_t adcCh8Offset_ = 0xFFFF; // CH8 (GND reference) raw ADC value
    volatile hal_adc_state_t adcState_ = HAL_ADC_STATE_DISABLED;

    static constexpr uint32_t EFUSE_ADC_OFFSET = 0x1D0;
    static constexpr uint32_t EFUSE_ADC_GAIN = 0x1D2;
    static constexpr int CH8_ADJUSTMENT = -50; // CH8 - GND on a pin difference, found empirically works for most devices
    static constexpr unsigned CH8_SAMPLE_COUNT = 100;
    static constexpr uint32_t NON_LINEAR_SECTION_LOW_MV = 800;
    static constexpr uint32_t NON_LINEAR_SECTION_HIGH_MV = 2000;
    static constexpr uint32_t ZERO_MV = 0;
    static constexpr uint32_t VCC_MV = 3300;
    static constexpr uint32_t DEFAULT_GAIN = 0x2F12;
};

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
    return Adc::instance().read(pin);
}

int hal_adc_sleep(bool sleep, void* reserved) {
    return Adc::instance().sleep(sleep);
}

int hal_adc_calibrate(uint32_t reserved, void* reserved1) {
    return Adc::instance().calibration();
}

int hal_adc_set_reference(uint32_t reference, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_adc_get_reference(void* reserved) {
    return (int)HAL_ADC_REFERENCE_INTERNAL;
}
