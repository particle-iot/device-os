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

#include "nrfx.h"
#include "nrfx_saadc.h"
#include "adc_hal.h"
#include "pinmap_impl.h"
#include "check.h"
#include "deviceid_hal.h"

static volatile hal_adc_state_t adcState = HAL_ADC_STATE_DISABLED;

static const nrfx_saadc_config_t saadcConfig = {
    .resolution         = NRF_SAADC_RESOLUTION_12BIT,
    .oversample         = NRF_SAADC_OVERSAMPLE_DISABLED,
    .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY,
    .low_power_mode     = false
};

static hal_adc_reference_t adcReference = HAL_ADC_REFERENCE_VCC;
static constexpr uint32_t HAL_VERSION_BRN404X_V003 = 0x03;
static constexpr uint32_t HW_FEATURE_FLAG_USE_INTERNAL_ADC_REFERENCE_BIT = (1<<15);

static void analog_in_event_handler(nrfx_saadc_evt_t const *p_event) {
    (void) p_event;
}

static hal_adc_reference_t hal_adc_get_default_reference(void) {
    hal_adc_reference_t defaultSource = HAL_ADC_REFERENCE_VCC;

#if PLATFORM_ID == PLATFORM_BORON
    uint32_t hwVersion = 0xFF;
    auto ret = hal_get_device_hw_version(&hwVersion, nullptr);
    if (ret == SYSTEM_ERROR_NONE && hwVersion == HAL_VERSION_BRN404X_V003) {
        defaultSource = HAL_ADC_REFERENCE_INTERNAL;
    }
#endif // PLATFORM_ID == PLATFORM_BORON

    hal_device_hw_info hwInfo = {};
    hwInfo.size = sizeof(hwInfo);
    hal_get_device_hw_info(&hwInfo, nullptr);
    if ((hwInfo.features & HW_FEATURE_FLAG_USE_INTERNAL_ADC_REFERENCE_BIT) == 0) {
        defaultSource = HAL_ADC_REFERENCE_INTERNAL;
    }

    return defaultSource;
}

void hal_adc_set_sample_time(uint8_t sample_time) {
    // deprecated
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t hal_adc_read(uint16_t pin) {
    if (adcState != HAL_ADC_STATE_ENABLED) {
        hal_adc_dma_init();
    }

    int16_t adcValue = 0;
    ret_code_t ret;
    nrf_saadc_input_t channel;
    hal_pin_info_t *PIN_MAP = hal_pin_map();

    switch (PIN_MAP[pin].adc_channel) {
        case 0: channel = NRF_SAADC_INPUT_AIN0; break;
        case 1: channel = NRF_SAADC_INPUT_AIN1; break;
        case 2: channel = NRF_SAADC_INPUT_AIN2; break;
        case 3: channel = NRF_SAADC_INPUT_AIN3; break;
        case 4: channel = NRF_SAADC_INPUT_AIN4; break;
        case 5: channel = NRF_SAADC_INPUT_AIN5; break;
        case 6: channel = NRF_SAADC_INPUT_AIN6; break;
        case 7: channel = NRF_SAADC_INPUT_AIN7; break;
        default: return 0;
    }

    if (PIN_MAP[pin].pin_func != PF_NONE && PIN_MAP[pin].pin_func != PF_DIO) {
        return 0;
    }

     //Single ended, negative input to ADC shorted to GND.
    nrf_saadc_channel_config_t channelConfig = {                                                   
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,      \
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,      \
        .gain       = NRF_SAADC_GAIN1_4,                \
        .reference  = NRF_SAADC_REFERENCE_VDD4,         \
        .acq_time   = NRF_SAADC_ACQTIME_10US,           \
        .mode       = NRF_SAADC_MODE_SINGLE_ENDED,      \
        .burst      = NRF_SAADC_BURST_DISABLED,         \
        .pin_p      = (nrf_saadc_input_t)(channel),       \
        .pin_n      = NRF_SAADC_INPUT_DISABLED          \
    };

    if (adcReference == HAL_ADC_REFERENCE_INTERNAL) {
        channelConfig.gain = NRF_SAADC_GAIN1_6;
        channelConfig.reference = NRF_SAADC_REFERENCE_INTERNAL;
    }

    ret = nrfx_saadc_channel_init(PIN_MAP[pin].adc_channel, &channelConfig);
    if (ret) {
        goto err_ret;
    }

    ret = nrfx_saadc_sample_convert(PIN_MAP[pin].adc_channel, &adcValue);
    if (ret) {
        goto err_ret;
    }

    if (adcValue < 0) {
        // Even in the single ended mode measured value can be negative value. Saturation for avoid casting to a big integer.
        goto err_ret;
    } else {
        nrfx_saadc_channel_uninit(PIN_MAP[pin].adc_channel);
        if (adcReference == HAL_ADC_REFERENCE_INTERNAL) {
            // With the internal reference (0.6V) and GAIN1_6, the 12bit ADC range is 0~4095 -> 0V~3.6V, need a conversion
            adcValue = (int16_t)(adcValue * 3600 / 3300);
        }
        return (uint16_t) adcValue;
    }

err_ret:
    nrfx_saadc_channel_uninit(PIN_MAP[pin].adc_channel);
    return 0;
}

/*
 * @brief Initialize the ADC peripheral.
 */
void hal_adc_dma_init() {
    adcState = HAL_ADC_STATE_ENABLED;
    uint32_t err_code = nrfx_saadc_init(&saadcConfig, analog_in_event_handler);
    SPARK_ASSERT(err_code == NRF_SUCCESS);

    adcReference = hal_adc_get_default_reference();
}

/*
 * @brief Uninitialize the ADC peripheral.
 */
int hal_adc_dma_uninit(void* reserved) {
    adcState = HAL_ADC_STATE_DISABLED;
    nrfx_saadc_uninit();
    return SYSTEM_ERROR_NONE;
}

/*
 * @brief ADC peripheral enters sleep mode
 */
int hal_adc_sleep(bool sleep, void* reserved) {
    if (sleep) {
        // Suspend ADC
        CHECK_TRUE(adcState == HAL_ADC_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);
        hal_adc_dma_uninit(nullptr);
        adcState = HAL_ADC_STATE_SUSPENDED;
    } else {
        // Restore ADC
        CHECK_TRUE(adcState == HAL_ADC_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
        hal_adc_dma_init();
    }
    return SYSTEM_ERROR_NONE;
}

int hal_adc_calibrate(uint32_t reserved, void* reserved1) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_adc_set_reference(uint32_t reference, void* reserved) {

    switch (reference) {
        case HAL_ADC_REFERENCE_VCC:
        case HAL_ADC_REFERENCE_INTERNAL:
            adcReference = (hal_adc_reference_t)reference;
            break;    
        case HAL_ADC_REFERENCE_DEFAULT:
            adcReference = hal_adc_get_default_reference();
            break;
        default: 
            return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return SYSTEM_ERROR_NONE;
}

int hal_adc_get_reference(void* reserved) {
    return (int)adcReference;
}
