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
#include "logging.h"

#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrfx_spis.h"
#include "spi_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "delay_hal.h"



#define DEFAULT_SPI_MODE        SPI_MODE_MASTER
#define DEFAULT_DATA_MODE       SPI_MODE3
#define DEFAULT_BIT_ORDER       MSBFIRST
#define DEFAULT_SPI_CLOCK       SPI_CLOCK_DIV256

typedef struct {
    const nrfx_spim_t                   *master;
    const nrfx_spis_t                   *slave;
    app_irq_priority_t                  priority;
    uint8_t                             ss_pin;
    uint8_t                             sck_pin;
    uint8_t                             mosi_pin;
    uint8_t                             miso_pin;

    SPI_Mode                            spi_mode;
    uint8_t                             data_mode;
    uint8_t                             bit_order;
    uint32_t                            clock;

    volatile uint8_t                    spi_ss_state;
    void                                *slave_tx_buf;
    void                                *slave_rx_buf;
    uint32_t                            slave_buf_length;

    HAL_SPI_DMA_UserCallback            spi_dma_user_callback;
    HAL_SPI_Select_UserCallback         spi_select_user_callback;

    bool                                enabled;
    volatile bool                       transmitting;
    volatile uint16_t                   transfer_length;

    os_mutex_recursive_t                mutex;
} nrf5x_spi_info_t;

static const nrfx_spim_t m_spim2 = NRFX_SPIM_INSTANCE(2);
static const nrfx_spim_t m_spim3 = NRFX_SPIM_INSTANCE(3);
static const nrfx_spis_t m_spis2 = NRFX_SPIS_INSTANCE(2);

static nrf5x_spi_info_t m_spi_map[TOTAL_SPI] = {
    {&m_spim3, NULL, APP_IRQ_PRIORITY_HIGH, PIN_INVALID, SCK, MOSI, MISO}, // TODO: SPI3 doesn't support SPI slave mode
    {&m_spim2, &m_spis2, APP_IRQ_PRIORITY_HIGH, PIN_INVALID, D2, D3, D4},  // TODO: Change pin number
};

static void spi_master_event_handler(nrfx_spim_evt_t const * p_event, void * p_context) {
    if (p_event->type == NRFX_SPIM_EVENT_DONE) {
        // LOG_DEBUG(TRACE, ">> spi: rx: %d, tx: %d", p_event->xfer_desc.tx_length, p_event->xfer_desc.rx_length);
        int spi = (int)p_context;
        m_spi_map[spi].transmitting = false;

        if (m_spi_map[spi].spi_dma_user_callback) {
            (*m_spi_map[spi].spi_dma_user_callback)();
        }
    }
}

void spi_slave_event_handler(nrfx_spis_evt_t const * p_event, void * p_context) {
    int spi = (int) p_context;

    if (p_event->evt_type == NRFX_SPIS_XFER_DONE) {
        m_spi_map[spi].transfer_length = p_event->rx_amount;
        m_spi_map[spi].transmitting = false;

        if (p_event->rx_amount) {
            if (m_spi_map[spi].spi_dma_user_callback) {
                (*m_spi_map[spi].spi_dma_user_callback)();
            }
        }
        // LOG_DEBUG(TRACE, ">> spi: rx: %d, tx: %d", p_event->rx_amount, p_event->tx_amount);

        // Reset spi slave data buffer
        SPARK_ASSERT(nrfx_spis_buffers_set(m_spi_map[spi].slave, 
                                        (uint8_t *)m_spi_map[spi].slave_tx_buf, 
                                        m_spi_map[spi].slave_buf_length, 
                                        (uint8_t *)m_spi_map[spi].slave_rx_buf, 
                                        m_spi_map[spi].slave_buf_length) == NRF_SUCCESS);
    } else if (p_event->evt_type == NRFX_SPIS_BUFFERS_SET_DONE) {
        // LOG_DEBUG(TRACE, ">> NRFX_SPIS_BUFFERS_SET_DONE");
    }
}

static void HAL_SPI_SS_Handler(void *data) {
    int spi = (int)data;
    uint8_t state = !HAL_GPIO_Read(m_spi_map[spi].ss_pin);
    m_spi_map[spi].spi_ss_state = state;
    if (m_spi_map[spi].spi_select_user_callback) {
        m_spi_map[spi].spi_select_user_callback(state);
    }
}

static inline nrf_spim_frequency_t get_nrf_spi_frequency(HAL_SPI_Interface spi, uint8_t clock_div) {
    switch (clock_div) {
        case SPI_CLOCK_DIV2: 
            if (spi == HAL_SPI_INTERFACE1) {
                return NRF_SPIM_FREQ_32M;
            }
        case SPI_CLOCK_DIV4:
            if (spi == HAL_SPI_INTERFACE1) {
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

static inline uint8_t get_nrf_pin_num(uint8_t pin) {
    if (pin >= TOTAL_PINS) {
        return NRFX_SPIM_PIN_NOT_USED;
    }

    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    return NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
}

static void spi_init(HAL_SPI_Interface spi, SPI_Mode mode) {
    uint32_t err_code;

    if (mode == SPI_MODE_MASTER) {
        static const nrf_spim_mode_t nrf_spim_mode[4] = {NRF_SPIM_MODE_0, NRF_SPIM_MODE_1, NRF_SPIM_MODE_2, NRF_SPIM_MODE_3};
        nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG;
        spim_config.sck_pin      = get_nrf_pin_num(m_spi_map[spi].sck_pin);
        spim_config.mosi_pin     = get_nrf_pin_num(m_spi_map[spi].mosi_pin);
        spim_config.miso_pin     = get_nrf_pin_num(m_spi_map[spi].miso_pin);
        spim_config.ss_pin       = NRFX_SPIM_PIN_NOT_USED; 
        spim_config.irq_priority = m_spi_map[spi].priority;
        spim_config.orc          = 0xFF;
        spim_config.frequency    = get_nrf_spi_frequency(spi, m_spi_map[spi].clock);
        spim_config.mode         = nrf_spim_mode[m_spi_map[spi].data_mode];
        spim_config.bit_order    = (m_spi_map[spi].bit_order == MSBFIRST) ? NRF_SPIM_BIT_ORDER_MSB_FIRST : NRF_SPIM_BIT_ORDER_LSB_FIRST;

        err_code = nrfx_spim_init(m_spi_map[spi].master, &spim_config, spi_master_event_handler, (void *)((int)spi));
        SPARK_ASSERT(err_code == NRF_SUCCESS);

        HAL_GPIO_Write(m_spi_map[spi].ss_pin, 1);
        HAL_Pin_Mode(m_spi_map[spi].ss_pin, OUTPUT);
    } else {
        static const nrf_spis_mode_t nrf_spis_mode[4] = {NRF_SPIS_MODE_0, NRF_SPIS_MODE_1, NRF_SPIS_MODE_2, NRF_SPIS_MODE_3};
        nrfx_spis_config_t spis_config;
        spis_config.sck_pin      = get_nrf_pin_num(m_spi_map[spi].sck_pin);
        spis_config.mosi_pin     = get_nrf_pin_num(m_spi_map[spi].mosi_pin);
        spis_config.miso_pin     = get_nrf_pin_num(m_spi_map[spi].miso_pin);
        spis_config.csn_pin      = get_nrf_pin_num(m_spi_map[spi].ss_pin);
        spis_config.csn_pullup   = NRF_GPIO_PIN_PULLUP;
        spis_config.miso_drive   = NRF_GPIO_PIN_S0S1;
        spis_config.irq_priority = m_spi_map[spi].priority;
        spis_config.orc          = 0xFF;
        spis_config.def          = 0xFF;
        spis_config.mode         = nrf_spis_mode[m_spi_map[spi].data_mode];
        spis_config.bit_order    = (m_spi_map[spi].bit_order == MSBFIRST) ? NRF_SPIS_BIT_ORDER_MSB_FIRST : NRF_SPIS_BIT_ORDER_LSB_FIRST;

        err_code = nrfx_spis_init(m_spi_map[spi].slave, &spis_config, spi_slave_event_handler, (void *)((int) spi));
        SPARK_ASSERT(err_code == NRF_SUCCESS);

        HAL_Pin_Mode(m_spi_map[spi].ss_pin, INPUT_PULLUP);
        HAL_Interrupts_Attach(m_spi_map[spi].ss_pin, &HAL_SPI_SS_Handler, (void*)(spi), CHANGE, NULL);
    }

    // Set pin function
    HAL_Set_Pin_Function(m_spi_map[spi].sck_pin, PF_SPI);
    HAL_Set_Pin_Function(m_spi_map[spi].mosi_pin, PF_SPI);
    HAL_Set_Pin_Function(m_spi_map[spi].miso_pin, PF_SPI);
}

static void spi_uninit(HAL_SPI_Interface spi) {
    if (m_spi_map[spi].spi_mode == SPI_MODE_MASTER) {
        nrfx_spim_uninit(m_spi_map[spi].master);
    } else {
        nrfx_spis_uninit(m_spi_map[spi].slave);
        HAL_Interrupts_Detach(m_spi_map[spi].ss_pin);
    }

    HAL_Set_Pin_Function(m_spi_map[spi].sck_pin, PF_NONE);
    HAL_Set_Pin_Function(m_spi_map[spi].mosi_pin, PF_NONE);
    HAL_Set_Pin_Function(m_spi_map[spi].miso_pin, PF_NONE);
}

static uint32_t spi_tx_rx(HAL_SPI_Interface spi, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t size) {
    // LOG_DEBUG(TRACE, "spi send, size: %d", size);

    uint32_t err_code;
    m_spi_map[spi].transmitting = true;
    m_spi_map[spi].transfer_length = size;

    nrfx_spim_xfer_desc_t const spim_xfer_desc = {
        .p_tx_buffer = tx_buf,
        .tx_length   = tx_buf ? size : 0,
        .p_rx_buffer = rx_buf,
        .rx_length   = rx_buf ? size : 0,
    };
    err_code = nrfx_spim_xfer(m_spi_map[spi].master, &spim_xfer_desc, 0);

    if (err_code) {
        m_spi_map[spi].transmitting = false;
    }

    return err_code ? 0 : size;
}

static void spi_transfer_cancel(HAL_SPI_Interface spi) {
    if (m_spi_map[spi].spi_mode == SPI_MODE_MASTER) {
        nrfx_spim_abort(m_spi_map[spi].master);
    } else {
        // Not supported by SPI Slave
    }
}

void HAL_SPI_Init(HAL_SPI_Interface spi) {
    os_thread_scheduling(false, nullptr);
    if (m_spi_map[spi].mutex == nullptr) {
        os_mutex_recursive_create(&m_spi_map[spi].mutex);
    }
    os_thread_scheduling(true, NULL);

    HAL_SPI_Acquire(spi, nullptr);

    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
    }

    // Default: SPI_MODE_MASTER, SPI_MODE3, MSBFIRST, 16MHZ
    m_spi_map[spi].enabled = false;
    m_spi_map[spi].transmitting = false;
    m_spi_map[spi].spi_mode = DEFAULT_SPI_MODE;
    m_spi_map[spi].bit_order = DEFAULT_BIT_ORDER;
    m_spi_map[spi].data_mode = DEFAULT_DATA_MODE;
    m_spi_map[spi].clock = DEFAULT_SPI_CLOCK;
    m_spi_map[spi].spi_ss_state = 0;
    m_spi_map[spi].spi_dma_user_callback = NULL;
    m_spi_map[spi].spi_select_user_callback = NULL;
    m_spi_map[spi].transfer_length = 0;

    HAL_SPI_Release(spi, nullptr);
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin) {
    // Default to Master mode
    HAL_SPI_Begin_Ext(spi, SPI_MODE_MASTER, pin, NULL);
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved) {
    if (spi == HAL_SPI_INTERFACE1 && mode == SPI_MODE_SLAVE) {
        // HAL_SPI_INTERFACE1 does not support slave mode
        return;
    }

    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
    }

    if (pin == SPI_DEFAULT_SS) {
        if (spi == HAL_SPI_INTERFACE1) {
            m_spi_map[spi].ss_pin = SS;
        } else if (spi == HAL_SPI_INTERFACE2) {
            m_spi_map[spi].ss_pin = D5;
        } else {
            m_spi_map[spi].ss_pin = PIN_INVALID;
        }
    } else {
        m_spi_map[spi].ss_pin = pin;
    }

    m_spi_map[spi].spi_mode = mode;
    spi_init(spi, mode);
    m_spi_map[spi].enabled = true;
}

void HAL_SPI_End(HAL_SPI_Interface spi) {
    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
        m_spi_map[spi].enabled = false;
    }
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order) {
    m_spi_map[spi].bit_order = order;
    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
        spi_init(spi, m_spi_map[spi].spi_mode);
    }
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode) {
    m_spi_map[spi].data_mode = mode;
    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
        spi_init(spi, m_spi_map[spi].spi_mode);
    }
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate) {
    // actual speed is the system clock divided by some scalar
    m_spi_map[spi].clock = rate;
    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
        spi_init(spi, m_spi_map[spi].spi_mode);
    }
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data) {
    if (m_spi_map[spi].spi_mode == SPI_MODE_SLAVE) {
        return 0;
    }

    uint8_t tx_buffer __attribute__((__aligned__(4)));
    uint8_t rx_buffer __attribute__((__aligned__(4)));

    // Wait for SPI transfer finished
    while(m_spi_map[spi].transmitting) {
        ;
    }

    tx_buffer = data;

    m_spi_map[spi].spi_dma_user_callback = NULL;
    spi_tx_rx(spi, &tx_buffer, &rx_buffer, 1);

    // Wait for SPI transfer finished
    while(m_spi_map[spi].transmitting) {
        ;
    }

    return rx_buffer;
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi) {
    return m_spi_map[spi].enabled;
}

bool HAL_SPI_Is_Enabled_Old(void) {
    return false;
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved) {
    info->system_clock = 64000000;
    if (info->version >= HAL_SPI_INFO_VERSION_1) {
        int32_t state = HAL_disable_irq();
        if (m_spi_map[spi].enabled) {
            switch (m_spi_map[spi].clock) {
                case SPI_CLOCK_DIV2:   info->clock = info->system_clock / 2;   break;
                case SPI_CLOCK_DIV4:   info->clock = info->system_clock / 4;   break;
                case SPI_CLOCK_DIV8:   info->clock = info->system_clock / 8;   break;
                case SPI_CLOCK_DIV16:  info->clock = info->system_clock / 16;  break;
                case SPI_CLOCK_DIV32:  info->clock = info->system_clock / 32;  break;
                case SPI_CLOCK_DIV64:  info->clock = info->system_clock / 64;  break;
                case SPI_CLOCK_DIV128: info->clock = info->system_clock / 128; break;
                case SPI_CLOCK_DIV256: info->clock = info->system_clock / 256; break;
                default: info->clock = 0;
            }
        } else {
            info->clock = 0;
        }
        info->default_settings = ((m_spi_map[spi].spi_mode  == DEFAULT_SPI_MODE) &&
                                  (m_spi_map[spi].bit_order == DEFAULT_BIT_ORDER) &&
                                  (m_spi_map[spi].data_mode == DEFAULT_DATA_MODE) &&
                                  (m_spi_map[spi].clock     == DEFAULT_SPI_CLOCK));
        info->enabled = m_spi_map[spi].enabled;
        info->mode = m_spi_map[spi].spi_mode;
        info->bit_order = m_spi_map[spi].bit_order;
        info->data_mode = m_spi_map[spi].data_mode;
        if (info->version >= HAL_SPI_INFO_VERSION_2) {
            info->ss_pin = m_spi_map[spi].ss_pin;
        }
        HAL_enable_irq(state);
    }
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved) {
    m_spi_map[spi].spi_select_user_callback = cb;
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback) {
    if (length == 0) {
        return;
    }

    while(m_spi_map[spi].transmitting) {
        ;
    }

    m_spi_map[spi].spi_dma_user_callback = userCallback;
    if (m_spi_map[spi].spi_mode == SPI_MODE_MASTER) {
        SPARK_ASSERT(spi_tx_rx(spi, (uint8_t *)tx_buffer, (uint8_t *)rx_buffer, length) == length);
    } else {
        // reset transfer length
        m_spi_map[spi].transfer_length = 0;
        m_spi_map[spi].slave_buf_length = length;
        m_spi_map[spi].slave_tx_buf = tx_buffer;
        m_spi_map[spi].slave_rx_buf = rx_buffer;
        uint32_t err_code = nrfx_spis_buffers_set(m_spi_map[spi].slave, 
                                            (uint8_t *)m_spi_map[spi].slave_tx_buf, 
                                            m_spi_map[spi].slave_buf_length, 
                                            (uint8_t *)m_spi_map[spi].slave_rx_buf, 
                                            m_spi_map[spi].slave_buf_length);
        if (err_code == NRF_ERROR_INVALID_STATE) {
            // LOG_DEBUG(WARN, "nrfx_spis_buffers_set, invalid state");
        } else {
            SPARK_ASSERT(err_code == NRF_SUCCESS);
            m_spi_map[spi].transmitting = true;
        }
    }
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi) {
    if (m_spi_map[spi].spi_mode == SPI_MODE_MASTER) {
        spi_transfer_cancel(spi);
        m_spi_map[spi].transmitting = false;
        m_spi_map[spi].spi_dma_user_callback = NULL;
    } else {
        // Not supported by SPI Slave
    }
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st) {
    int32_t transfer_length = 0;

    if (m_spi_map[spi].transmitting) {
        transfer_length = 0;
    } else {
        transfer_length = m_spi_map[spi].transfer_length;
    }

    if (st != NULL) {
        st->configured_transfer_length = m_spi_map[spi].transfer_length;
        st->transfer_length = (uint32_t)transfer_length;
        st->transfer_ongoing = m_spi_map[spi].transmitting;
        st->ss_state = m_spi_map[spi].spi_ss_state;
    }

    return transfer_length;
}

int32_t HAL_SPI_Set_Settings(HAL_SPI_Interface spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    if (set_default) {
        m_spi_map[spi].data_mode = DEFAULT_DATA_MODE;
        m_spi_map[spi].bit_order = DEFAULT_BIT_ORDER;
        m_spi_map[spi].clock = DEFAULT_SPI_CLOCK;
    } else {
        m_spi_map[spi].data_mode = mode;
        m_spi_map[spi].bit_order = order;
        m_spi_map[spi].clock = clockdiv;
    }

    if (m_spi_map[spi].enabled) {
        spi_uninit(spi);
        spi_init(spi, m_spi_map[spi].spi_mode);
    }

    return 0;
}

int32_t HAL_SPI_Acquire(HAL_SPI_Interface spi, void* reserved) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = m_spi_map[spi].mutex;
        if (mutex) {
            return os_mutex_recursive_lock(mutex);
        }
    }
    return -1;
}

int32_t HAL_SPI_Release(HAL_SPI_Interface spi, void* reserved) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = m_spi_map[spi].mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}
