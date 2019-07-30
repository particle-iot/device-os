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

#include "nrfx.h"
#include "nrfx_saadc.h"
#include "adc_hal.h"
#include "pinmap_impl.h"


static volatile bool m_adc_initiated = false;

static const nrfx_saadc_config_t saadc_config =
{
    .resolution         = NRF_SAADC_RESOLUTION_12BIT,
    .oversample         = NRF_SAADC_OVERSAMPLE_DISABLED,
    .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY
};

nrf_saadc_reference_t VREF = NRF_SAADC_REFERENCE_VDD4;


static void analog_in_event_handler(nrfx_saadc_evt_t const *p_event)
{
    (void) p_event;
}

void HAL_ADC_Set_Sample_Time(uint8_t ADC_SampleTime)
{
    // deprecated
}

/*
 * @brief @brief Set the ADC reference to either VDD / 4 (VDD4) or the internal 0.6v (INTERNAL)
 */

void HAL_ADC_Set_VREF(vref_e v_e){
    switch (v_e) {
        case VDD4: VREF = NRF_SAADC_REFERENCE_VDD4; break;

        case INTERNAL: VREF = NRF_SAADC_REFERENCE_INTERNAL; break;

        default: VREF = NRF_SAADC_REFERENCE_VDD4; break;
    }

}


/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t HAL_ADC_Read(uint16_t pin)
{
    if (!m_adc_initiated)
    {
        m_adc_initiated = true;
        HAL_ADC_DMA_Init();
    }

    int16_t    adc_value = 0;
    ret_code_t ret_code;
    nrf_saadc_input_t nrf_adc_channel;
    NRF5x_Pin_Info *PIN_MAP = HAL_Pin_Map();

    switch (PIN_MAP[pin].adc_channel)
    {
        case 0: nrf_adc_channel = NRF_SAADC_INPUT_AIN0; break;
        case 1: nrf_adc_channel = NRF_SAADC_INPUT_AIN1; break;
        case 2: nrf_adc_channel = NRF_SAADC_INPUT_AIN2; break;
        case 3: nrf_adc_channel = NRF_SAADC_INPUT_AIN3; break;
        case 4: nrf_adc_channel = NRF_SAADC_INPUT_AIN4; break;
        case 5: nrf_adc_channel = NRF_SAADC_INPUT_AIN5; break;
        case 6: nrf_adc_channel = NRF_SAADC_INPUT_AIN6; break;
        case 7: nrf_adc_channel = NRF_SAADC_INPUT_AIN7; break;
        default:
            return 0;
    }

    if (PIN_MAP[pin].pin_func != PF_NONE && PIN_MAP[pin].pin_func != PF_DIO)
    {
        return 0;
    }

     //Single ended, negative input to ADC shorted to GND.
    nrf_saadc_channel_config_t channel_config = {
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,      \
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,      \
        .gain       = NRF_SAADC_GAIN1_4,                \
        .reference  = VREF,                             \
        .acq_time   = NRF_SAADC_ACQTIME_10US,           \
        .mode       = NRF_SAADC_MODE_SINGLE_ENDED,      \
        .burst      = NRF_SAADC_BURST_DISABLED,         \
        .pin_p      = (nrf_saadc_input_t)(nrf_adc_channel),       \
        .pin_n      = NRF_SAADC_INPUT_DISABLED          \
    };

    ret_code = nrfx_saadc_channel_init(PIN_MAP[pin].adc_channel, &channel_config);
    if (ret_code)
    {
        goto err_ret;
    }

    ret_code = nrfx_saadc_sample_convert(PIN_MAP[pin].adc_channel, &adc_value);
    if (ret_code)
    {
        goto err_ret;
    }

    if (adc_value < 0)
    {
        // Even in the single ended mode measured value can be negative value. Saturation for avoid casting to a big integer.
        goto err_ret;
    }
    else
    {
        nrfx_saadc_channel_uninit(PIN_MAP[pin].adc_channel);
        return (uint16_t) adc_value;
    }

err_ret:
    nrfx_saadc_channel_uninit(PIN_MAP[pin].adc_channel);
    return 0;
}



/*
 * @brief Initialize the ADC peripheral.
 */
void HAL_ADC_DMA_Init()
{
    uint32_t err_code = nrfx_saadc_init(&saadc_config, analog_in_event_handler);
    SPARK_ASSERT(err_code == NRF_SUCCESS);
}
