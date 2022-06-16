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

constexpr uint32_t normalChCalOffsetAddr = 0x1D0;
constexpr uint32_t normalChCalGainDivAddr = 0x1D2;

volatile hal_adc_state_t adcState = HAL_ADC_STATE_DISABLED;

/*
 * OFFSET:   10 times of sample data at 0.000v, 10*value(0.000v)
 * GAIN_DIV: 10 times of value(1.000v)-value(0.000v) or value(2.000v)-value(1.000v) or value(3.000v)-value(2.000v)
 */
/* Normal channel*/
uint16_t adcOffset = 0xFFFF;
uint16_t adcGainDiv = 0xFFFF;

int adcInit() {
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

    ADC_InitTypeDef adcInitStruct = {}; 
    ADC_StructInit(&adcInitStruct);
    adcInitStruct.ADC_CvlistLen = 0;
    ADC_Init(&adcInitStruct);
    ADC_Cmd(ENABLE);

    uint8_t efuseBuf[2];
	for (uint8_t index = 0; index < 2; index++) {
		EFUSE_PMAP_READ8(0, normalChCalOffsetAddr + index, efuseBuf + index, L25EOUTVOLTAGE);
	}
    adcOffset = efuseBuf[1] << 8 | efuseBuf[0];
	for (uint8_t index = 0; index < 2; index++) {
		EFUSE_PMAP_READ8(0, normalChCalGainDivAddr + index, efuseBuf + index, L25EOUTVOLTAGE);
	}
	adcGainDiv = efuseBuf[1] << 8 | efuseBuf[0];
	if (adcOffset == 0xFFFF) {
		adcOffset = 0x9B0;
	}
	if (adcGainDiv == 0xFFFF) {
		adcGainDiv = 0x2F12;
	}

    adcState = HAL_ADC_STATE_ENABLED;
    return SYSTEM_ERROR_NONE;
}

int adcDeinit() {
    ADC_INTClear();
    ADC_Cmd(DISABLE);
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, DISABLE);
    adcState = HAL_ADC_STATE_DISABLED;
    return SYSTEM_ERROR_NONE;
}

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
    CHECK_TRUE(hal_pin_is_valid(pin), 0);
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    if (pinInfo->adc_channel == ADC_CHANNEL_NONE) {
        return 0;
    }
    if (pinInfo->pin_func != PF_NONE && pinInfo->pin_func != PF_DIO) {
        return 0;
    }

    if (adcState != HAL_ADC_STATE_ENABLED) {
        adcInit();
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

    uint16_t adcValue = (ADC_Read() & BIT_MASK_DAT_GLOBAL);

    uint32_t mv = 0;
    if (adcValue < 0xfa) {
        mv = 0; // Ignore persistent low voltage measurement error
    } else {
        mv = ((10 * adcValue - adcOffset) * 1000 / adcGainDiv); // Convert measured ADC value to millivolts
    }
    adcValue = (mv / 3300.0) * (1 << DEFAULT_ADC_RESOLUTION_BITS);
    if (adcValue > 4096) { // The measured voltage might greater than 3300mV
        adcValue = 4096;
    }

    return adcValue;
}

int hal_adc_sleep(bool sleep, void* reserved) {
    if (sleep) {
        // Suspend ADC
        CHECK_TRUE(adcState == HAL_ADC_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);
        adcDeinit();
        adcState = HAL_ADC_STATE_SUSPENDED;
    } else {
        // Restore ADC
        CHECK_TRUE(adcState == HAL_ADC_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
        adcInit();
    }
    return SYSTEM_ERROR_NONE;
}

