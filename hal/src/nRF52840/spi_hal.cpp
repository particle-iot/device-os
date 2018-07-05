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
#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "spi_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "logging.h"
#include "interrupts_hal.h"

#define DEFAULT_SPI_MODE        SPI_MODE_MASTER
#define DEFAULT_DATA_MODE       SPI_MODE3
#define DEFAULT_BIT_ORDER       MSBFIRST
#define DEFAULT_SPI_CLOCK       SPI_CLOCK_DIV256

typedef struct {
    const nrfx_spim_t                   *instance;
    app_irq_priority_t                  priority;
    uint8_t                             ss_pin;
    uint8_t                             sck_pin;
    uint8_t                             mosi_pin;
    uint8_t                             miso_pin;

    SPI_Mode                            spi_mode; 
    uint8_t                             data_mode;
    uint8_t                             bit_order;
    uint32_t                            clock;

    volatile HAL_SPI_DMA_UserCallback   user_callback;

    bool                                enabled;
    volatile bool                       transmitting;
    uint16_t                            transfer_length;
} nrf5x_spi_info_t;

static const nrfx_spim_t m_spi2 = NRFX_SPIM_INSTANCE(2);  
static const nrfx_spim_t m_spi3 = NRFX_SPIM_INSTANCE(3);  
static nrf5x_spi_info_t m_spi_map[TOTAL_SPI] = {
    {&m_spi3, APP_IRQ_PRIORITY_LOWEST, NRFX_SPIM_PIN_NOT_USED, SCK, MOSI, MISO},
    {&m_spi2, APP_IRQ_PRIORITY_LOWEST, NRFX_SPIM_PIN_NOT_USED, D2, D3, D4},
};

static void spi_event_handler(nrfx_spim_evt_t const * p_event,
                              void *                  p_context)
{
    if (p_event->type == NRFX_SPIM_EVENT_DONE)
    {
        int spi = (int)p_context;
        m_spi_map[spi].transmitting = false;

        if (m_spi_map[spi].user_callback)
        {
            (*m_spi_map[spi].user_callback)();
        }
    }
}

static inline nrf_spim_frequency_t get_nrf_spi_frequency(HAL_SPI_Interface spi, uint8_t clock_div)
{
    switch (clock_div)
    {
        case SPI_CLOCK_DIV2: 
            if (spi == HAL_SPI_INTERFACE1)
            {
                return NRF_SPIM_FREQ_32M;
            }
        case SPI_CLOCK_DIV4:
            if (spi == HAL_SPI_INTERFACE1)
            {
                return NRF_SPIM_FREQ_16M;
            }
        case SPI_CLOCK_DIV8: 
            return NRF_SPIM_FREQ_8M;
        case SPI_CLOCK_DIV16: 
            return NRF_SPIM_FREQ_4M;
        case SPI_CLOCK_DIV32:
            return NRF_SPIM_FREQ_2M;
        case SPI_CLOCK_DIV64: 
            return NRF_SPIM_FREQ_1M;
        case SPI_CLOCK_DIV128: 
            return NRF_SPIM_FREQ_500K;
        case SPI_CLOCK_DIV256:
            return NRF_SPIM_FREQ_250K;
    }

    return NRF_SPIM_FREQ_8M;
}

static inline uint8_t get_nrf_pin_num(uint8_t pin)
{
    if (pin >= TOTAL_PINS)
    {
        return NRFX_SPIM_PIN_NOT_USED;
    }

    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    return NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
}

static void spi_init(HAL_SPI_Interface spi)
{
    uint32_t err_code;
    static const nrf_spim_mode_t nrf_spi_mode[4] = {NRF_SPIM_MODE_0, NRF_SPIM_MODE_1, NRF_SPIM_MODE_2, NRF_SPIM_MODE_3};
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    spi_config.sck_pin      = get_nrf_pin_num(m_spi_map[spi].sck_pin);
    spi_config.mosi_pin     = get_nrf_pin_num(m_spi_map[spi].mosi_pin);
    spi_config.miso_pin     = get_nrf_pin_num(m_spi_map[spi].miso_pin);
    spi_config.ss_pin       = get_nrf_pin_num(m_spi_map[spi].ss_pin);
    spi_config.irq_priority = m_spi_map[spi].priority;
    spi_config.orc          = 0xFF;
    spi_config.frequency    = get_nrf_spi_frequency(spi, m_spi_map[spi].clock);
    spi_config.mode         = nrf_spi_mode[m_spi_map[spi].data_mode];
    spi_config.bit_order    = (m_spi_map[spi].bit_order == MSBFIRST) ? NRF_SPIM_BIT_ORDER_MSB_FIRST : NRF_SPIM_BIT_ORDER_LSB_FIRST;

    err_code = nrfx_spim_init(m_spi_map[spi].instance, &spi_config, spi_event_handler, (void *)((int)spi));
    SPARK_ASSERT(err_code == NRF_SUCCESS);

    // Set pin function
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    PIN_MAP[m_spi_map[spi].ss_pin].pin_func = PF_SPI;
    PIN_MAP[m_spi_map[spi].sck_pin].pin_func = PF_SPI;
    PIN_MAP[m_spi_map[spi].mosi_pin].pin_func = PF_SPI;
    PIN_MAP[m_spi_map[spi].miso_pin].pin_func = PF_SPI;
}

static void spi_uninit(HAL_SPI_Interface spi)
{
    nrfx_spim_uninit(m_spi_map[spi].instance);

    // Set pin function
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    PIN_MAP[m_spi_map[spi].ss_pin].pin_func = PF_NONE;
    PIN_MAP[m_spi_map[spi].sck_pin].pin_func = PF_NONE;
    PIN_MAP[m_spi_map[spi].mosi_pin].pin_func = PF_NONE;
    PIN_MAP[m_spi_map[spi].miso_pin].pin_func = PF_NONE;
}

static void ss_pin_uninit(HAL_SPI_Interface spi)
{
    // Set pin function
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    PIN_MAP[m_spi_map[spi].ss_pin].pin_func = PF_NONE;
    nrf_gpio_cfg_default(get_nrf_pin_num(m_spi_map[spi].ss_pin));
}

static int spi_tx_rx(HAL_SPI_Interface spi, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t size)
{
    uint32_t err_code;
    m_spi_map[spi].transmitting = true;
    m_spi_map[spi].transfer_length = size;

    nrfx_spim_xfer_desc_t const spim_xfer_desc =
    {
        .p_tx_buffer = tx_buf,
        .tx_length   = size,
        .p_rx_buffer = rx_buf,
        .rx_length   = size,
    };
    err_code = nrfx_spim_xfer(m_spi_map[spi].instance, &spim_xfer_desc, 0);

    return err_code ? 0 : size;
}

static void spi_transfer_cancel(HAL_SPI_Interface spi)
{
    nrfx_spim_abort(m_spi_map[spi].instance);
}

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
    }

    // Default: SPI_MODE_MASTER, SPI_MODE3, MSBFIRST, 16MHZ
    m_spi_map[spi].enabled = false;
    m_spi_map[spi].transmitting = false;   
    m_spi_map[spi].spi_mode = DEFAULT_SPI_MODE;
    m_spi_map[spi].bit_order = DEFAULT_BIT_ORDER;
    m_spi_map[spi].data_mode = DEFAULT_DATA_MODE; 
    m_spi_map[spi].clock = DEFAULT_SPI_CLOCK;
    m_spi_map[spi].user_callback = NULL;
    m_spi_map[spi].transfer_length = 0;
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    // Default to Master mode
    HAL_SPI_Begin_Ext(spi, SPI_MODE_MASTER, pin, NULL);
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved)
{
    if (m_spi_map[spi].ss_pin != NRFX_SPIM_PIN_NOT_USED)
    {
        ss_pin_uninit(spi);
    }

    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
    }

    m_spi_map[spi].ss_pin = pin;

    // TODO: need to support spi slave
    m_spi_map[spi].spi_mode = mode;
    spi_init(spi);

    m_spi_map[spi].enabled = true;
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    spi_uninit(spi);
    m_spi_map[spi].enabled = false;
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
    m_spi_map[spi].bit_order = order;
    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
        spi_init(spi);
    }
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
    m_spi_map[spi].data_mode = mode;
    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
        spi_init(spi);
    }
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    // actual speed is the system clock divided by some scalar
    m_spi_map[spi].clock = rate;
    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
        spi_init(spi);
    }
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    uint8_t tx_buffer __attribute__((__aligned__(4)));
    uint8_t rx_buffer __attribute__((__aligned__(4)));

    // Wait for SPI transfer finished
    while(m_spi_map[spi].transmitting);

    tx_buffer = data;

    m_spi_map[spi].user_callback = NULL;
    spi_tx_rx(spi, &tx_buffer, &rx_buffer, 1);

    // Wait for SPI transfer finished
    while(m_spi_map[spi].transmitting);

    return rx_buffer;
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return m_spi_map[spi].enabled;
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
    while(m_spi_map[spi].transmitting);

    m_spi_map[spi].user_callback = userCallback;
    spi_tx_rx(spi, (uint8_t *)tx_buffer, (uint8_t *)rx_buffer, length);
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved)
{
    info->system_clock = 64000000;
    if (info->version >= HAL_SPI_INFO_VERSION_1)
    {
        int32_t state = HAL_disable_irq();
        if (m_spi_map[spi].enabled)
        {
            switch (m_spi_map[spi].clock)
            {
                case SPI_CLOCK_DIV2: info->clock = info->system_clock / 2; break;
                case SPI_CLOCK_DIV4: info->clock = info->system_clock / 4; break;
                case SPI_CLOCK_DIV8: info->clock = info->system_clock / 8; break;
                case SPI_CLOCK_DIV16: info->clock = info->system_clock / 16; break;
                case SPI_CLOCK_DIV32: info->clock = info->system_clock / 32; break;
                case SPI_CLOCK_DIV64: info->clock = info->system_clock / 64; break;
                case SPI_CLOCK_DIV128: info->clock = info->system_clock / 128; break;
                case SPI_CLOCK_DIV256: info->clock = info->system_clock / 256; break;
                default: info->clock = 0;
            }
        }
        else
        {
            info->clock = 0;
        }
        info->default_settings = ((m_spi_map[spi].spi_mode == DEFAULT_SPI_MODE) ||
                                  (m_spi_map[spi].bit_order == DEFAULT_BIT_ORDER) ||
                                  (m_spi_map[spi].data_mode == DEFAULT_DATA_MODE) ||
                                  (m_spi_map[spi].clock == DEFAULT_SPI_CLOCK));
        info->enabled = m_spi_map[spi].enabled;
        info->mode = m_spi_map[spi].spi_mode;
        info->bit_order = m_spi_map[spi].bit_order;
        info->data_mode = m_spi_map[spi].data_mode;
        HAL_enable_irq(state);
    }
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved)
{
    // TODO: slave mode
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi)
{
    spi_transfer_cancel(spi);
    m_spi_map[spi].transmitting = false;
    m_spi_map[spi].user_callback = NULL; 
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st)
{
    int32_t transfer_length = 0;

    if (m_spi_map[spi].transmitting)
    {
        transfer_length = 0;
    }
    else
    {
        transfer_length = m_spi_map[spi].transfer_length;
    }

    if (st != NULL)
    {
        st->configured_transfer_length = m_spi_map[spi].transfer_length;
        st->transfer_length = (uint32_t)transfer_length;
        st->transfer_ongoing = m_spi_map[spi].transmitting;
        // TODO: slave mode
        // st->ss_state = spiState[spi].SPI_SS_State;
    }

    return transfer_length;
}

int32_t HAL_SPI_Set_Settings(HAL_SPI_Interface spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved)
{
    if (set_default)
    {
        m_spi_map[spi].data_mode = DEFAULT_DATA_MODE;
        m_spi_map[spi].bit_order = DEFAULT_BIT_ORDER;
        m_spi_map[spi].clock = DEFAULT_SPI_CLOCK;        
    }
    else
    {
        m_spi_map[spi].data_mode = mode;
        m_spi_map[spi].bit_order = order;
        m_spi_map[spi].clock = clockdiv;   
    }
    
    if (m_spi_map[spi].enabled)
    {
        spi_uninit(spi);
        spi_init(spi);
    }

    return 0;
}
