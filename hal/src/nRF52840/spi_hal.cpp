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
#include "check.h"

#define TOTAL_SPI               2

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

    hal_spi_mode_t                      spi_mode;
    uint8_t                             data_mode;
    uint8_t                             bit_order;
    uint32_t                            clock;

    volatile uint8_t                    spi_ss_state;
    void                                *slave_tx_buf;
    void                                *slave_rx_buf;
    uint32_t                            slave_buf_length;

    hal_spi_dma_user_callback           dma_user_callback;
    hal_spi_select_user_callback        select_user_callback;

    volatile bool                       enabled;
    volatile bool                       transmitting;
    volatile uint16_t                   transfer_length;
    volatile bool                       suspended;

    os_mutex_recursive_t                mutex;
} nrf5x_spi_info_t;

static const nrfx_spim_t m_spim2 = NRFX_SPIM_INSTANCE(2);
static const nrfx_spim_t m_spim3 = NRFX_SPIM_INSTANCE(3);
static const nrfx_spis_t m_spis2 = NRFX_SPIS_INSTANCE(2);

static nrf5x_spi_info_t spiMap[TOTAL_SPI] = {
    {&m_spim3, nullptr,  APP_IRQ_PRIORITY_HIGH, PIN_INVALID, SCK, MOSI, MISO}, // TODO: SPI3 doesn't support SPI slave mode
#if PLATFORM_ID == PLATFORM_TRACKER
    {&m_spim2, &m_spis2, APP_IRQ_PRIORITY_HIGH, PIN_INVALID, SCK1, MOSI1, MISO1},
#else
    {&m_spim2, &m_spis2, APP_IRQ_PRIORITY_HIGH, PIN_INVALID, D2, D3, D4},  // TODO: Change pin number
#endif
};

static void spiMasterEventHandler(nrfx_spim_evt_t const * p_event, void * p_context) {
    if (p_event->type == NRFX_SPIM_EVENT_DONE) {
        // LOG_DEBUG(TRACE, ">> spi: rx: %d, tx: %d", p_event->xfer_desc.tx_length, p_event->xfer_desc.rx_length);
        int spi = (int)p_context;
        spiMap[spi].transmitting = false;

        if (spiMap[spi].dma_user_callback) {
            (*spiMap[spi].dma_user_callback)();
        }
    }
}

static void spiSlaveEventHandler(nrfx_spis_evt_t const * p_event, void * p_context) {
    int spi = (int) p_context;

    if (p_event->evt_type == NRFX_SPIS_XFER_DONE) {
        spiMap[spi].transfer_length = p_event->rx_amount;
        spiMap[spi].transmitting = false;

        if (p_event->rx_amount) {
            if (spiMap[spi].dma_user_callback) {
                (*spiMap[spi].dma_user_callback)();
            }
        }
        // LOG_DEBUG(TRACE, ">> spi: rx: %d, tx: %d", p_event->rx_amount, p_event->tx_amount);

        // Reset spi slave data buffer
        SPARK_ASSERT(nrfx_spis_buffers_set(spiMap[spi].slave,
                                        (uint8_t *)spiMap[spi].slave_tx_buf,
                                        spiMap[spi].slave_buf_length,
                                        (uint8_t *)spiMap[spi].slave_rx_buf,
                                        spiMap[spi].slave_buf_length) == NRF_SUCCESS);
    } else if (p_event->evt_type == NRFX_SPIS_BUFFERS_SET_DONE) {
        // LOG_DEBUG(TRACE, ">> NRFX_SPIS_BUFFERS_SET_DONE");
    }
}

static void spiOnSelectedHandler(void *data) {
    int spi = (int)data;
    uint8_t state = !HAL_GPIO_Read(spiMap[spi].ss_pin);
    spiMap[spi].spi_ss_state = state;
    if (spiMap[spi].select_user_callback) {
        spiMap[spi].select_user_callback(state);
    }
}

static inline nrf_spim_frequency_t getNrfSpiFrequency(hal_spi_interface_t spi, uint8_t clock_div) {
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

static inline uint8_t getNrfPinNum(uint8_t pin) {
    if (pin >= TOTAL_PINS) {
        return NRFX_SPIM_PIN_NOT_USED;
    }

    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
    return NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
}

static void spiInit(hal_spi_interface_t spi, hal_spi_mode_t mode) {
    uint32_t err_code;

    if (mode == SPI_MODE_MASTER) {
        static const nrf_spim_mode_t nrf_spim_mode[4] = {NRF_SPIM_MODE_0, NRF_SPIM_MODE_1, NRF_SPIM_MODE_2, NRF_SPIM_MODE_3};
        nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG;
        spim_config.sck_pin      = getNrfPinNum(spiMap[spi].sck_pin);
        spim_config.mosi_pin     = getNrfPinNum(spiMap[spi].mosi_pin);
        spim_config.miso_pin     = getNrfPinNum(spiMap[spi].miso_pin);
        spim_config.ss_pin       = NRFX_SPIM_PIN_NOT_USED;
        spim_config.irq_priority = spiMap[spi].priority;
        spim_config.orc          = 0xFF;
        spim_config.frequency    = getNrfSpiFrequency(spi, spiMap[spi].clock);
        spim_config.mode         = nrf_spim_mode[spiMap[spi].data_mode];
        spim_config.bit_order    = (spiMap[spi].bit_order == MSBFIRST) ? NRF_SPIM_BIT_ORDER_MSB_FIRST : NRF_SPIM_BIT_ORDER_LSB_FIRST;

        err_code = nrfx_spim_init(spiMap[spi].master, &spim_config, spiMasterEventHandler, (void *)((int)spi));
        SPARK_ASSERT(err_code == NRF_SUCCESS);

        if (HAL_Pin_Is_Valid(spiMap[spi].ss_pin)) {
            hal_gpio_config_t conf = {
                .size = sizeof(conf),
                .version = 0,
                .mode = OUTPUT,
                .set_value = true,
                .value = 1
            };
            HAL_Pin_Configure(spiMap[spi].ss_pin, &conf);
        }
    } else {
        static const nrf_spis_mode_t nrf_spis_mode[4] = {NRF_SPIS_MODE_0, NRF_SPIS_MODE_1, NRF_SPIS_MODE_2, NRF_SPIS_MODE_3};
        nrfx_spis_config_t spis_config;
        spis_config.sck_pin      = getNrfPinNum(spiMap[spi].sck_pin);
        spis_config.mosi_pin     = getNrfPinNum(spiMap[spi].mosi_pin);
        spis_config.miso_pin     = getNrfPinNum(spiMap[spi].miso_pin);
        spis_config.csn_pin      = getNrfPinNum(spiMap[spi].ss_pin);
        spis_config.csn_pullup   = NRF_GPIO_PIN_PULLUP;
        spis_config.miso_drive   = NRF_GPIO_PIN_S0S1;
        spis_config.irq_priority = spiMap[spi].priority;
        spis_config.orc          = 0xFF;
        spis_config.def          = 0xFF;
        spis_config.mode         = nrf_spis_mode[spiMap[spi].data_mode];
        spis_config.bit_order    = (spiMap[spi].bit_order == MSBFIRST) ? NRF_SPIS_BIT_ORDER_MSB_FIRST : NRF_SPIS_BIT_ORDER_LSB_FIRST;

        err_code = nrfx_spis_init(spiMap[spi].slave, &spis_config, spiSlaveEventHandler, (void *)((int) spi));
        SPARK_ASSERT(err_code == NRF_SUCCESS);

        HAL_Pin_Mode(spiMap[spi].ss_pin, INPUT_PULLUP);
        HAL_Interrupts_Attach(spiMap[spi].ss_pin, &spiOnSelectedHandler, (void*)(spi), CHANGE, nullptr);
    }

    // Set pin function
    HAL_Set_Pin_Function(spiMap[spi].sck_pin, PF_SPI);
    HAL_Set_Pin_Function(spiMap[spi].mosi_pin, PF_SPI);
    HAL_Set_Pin_Function(spiMap[spi].miso_pin, PF_SPI);
}

static void spiUninit(hal_spi_interface_t spi) {
    if (spiMap[spi].spi_mode == SPI_MODE_MASTER) {
        nrfx_spim_uninit(spiMap[spi].master);
    } else {
        nrfx_spis_uninit(spiMap[spi].slave);
        HAL_Interrupts_Detach(spiMap[spi].ss_pin);
    }

    HAL_Set_Pin_Function(spiMap[spi].sck_pin, PF_NONE);
    HAL_Set_Pin_Function(spiMap[spi].mosi_pin, PF_NONE);
    HAL_Set_Pin_Function(spiMap[spi].miso_pin, PF_NONE);
}

static uint32_t spiTransfer(hal_spi_interface_t spi, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t size) {
    // LOG_DEBUG(TRACE, "spi send, size: %d", size);

    uint32_t err_code;
    spiMap[spi].transmitting = true;
    spiMap[spi].transfer_length = size;

    nrfx_spim_xfer_desc_t const spim_xfer_desc = {
        .p_tx_buffer = tx_buf,
        .tx_length   = tx_buf ? size : 0,
        .p_rx_buffer = rx_buf,
        .rx_length   = rx_buf ? size : 0,
    };
    err_code = nrfx_spim_xfer(spiMap[spi].master, &spim_xfer_desc, 0);

    if (err_code) {
        spiMap[spi].transmitting = false;
    }

    return err_code ? 0 : size;
}

static void spiTransferCancel(hal_spi_interface_t spi) {
    if (spiMap[spi].spi_mode == SPI_MODE_MASTER) {
        nrfx_spim_abort(spiMap[spi].master);
    } else {
        // Not supported by SPI Slave
    }
}

void hal_spi_init(hal_spi_interface_t spi) {
    os_thread_scheduling(false, nullptr);
    if (spiMap[spi].mutex == nullptr) {
        os_mutex_recursive_create(&spiMap[spi].mutex);
    }
    os_thread_scheduling(true, nullptr);

    hal_spi_acquire(spi, nullptr);

    if (spiMap[spi].enabled) {
        spiUninit(spi);
    }

    // Default: SPI_MODE_MASTER, SPI_MODE3, MSBFIRST, 16MHZ
    spiMap[spi].enabled = false;
    spiMap[spi].suspended = false;
    spiMap[spi].transmitting = false;
    spiMap[spi].spi_mode = DEFAULT_SPI_MODE;
    spiMap[spi].bit_order = DEFAULT_BIT_ORDER;
    spiMap[spi].data_mode = DEFAULT_DATA_MODE;
    spiMap[spi].clock = DEFAULT_SPI_CLOCK;
    spiMap[spi].spi_ss_state = 0;
    spiMap[spi].dma_user_callback = nullptr;
    spiMap[spi].select_user_callback = nullptr;
    spiMap[spi].transfer_length = 0;

    hal_spi_release(spi, nullptr);
}

void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin) {
    // Default to Master mode
    hal_spi_begin_ext(spi, SPI_MODE_MASTER, pin, nullptr);
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, void* reserved) {
    if (spi >= TOTAL_SPI) {
        return;
    }

    if (spi == HAL_SPI_INTERFACE1 && mode == SPI_MODE_SLAVE) {
        // HAL_SPI_INTERFACE1 does not support slave mode
        return;
    }

    spiMap[spi].suspended = false;

    if (spiMap[spi].enabled) {
        // Make sure we reset the enabled state
        spiUninit(spi);
        spiMap[spi].enabled = false;
    }

    if (pin == SPI_DEFAULT_SS) {
        if (spi == HAL_SPI_INTERFACE1) {
            spiMap[spi].ss_pin = SS;
        } else if (spi == HAL_SPI_INTERFACE2) {
            spiMap[spi].ss_pin = D5;
        } else {
            spiMap[spi].ss_pin = PIN_INVALID;
        }
    } else {
        spiMap[spi].ss_pin = pin;
    }

    if (mode == SPI_MODE_SLAVE && !HAL_Pin_Is_Valid(spiMap[spi].ss_pin)) {
        // Slave mode requires a valid pin
        return;
    }

    spiMap[spi].spi_mode = mode;
    spiInit(spi, mode);
    spiMap[spi].enabled = true;
}

void hal_spi_end(hal_spi_interface_t spi) {
    spiMap[spi].suspended = false;
    if (spiMap[spi].enabled) {
        spiUninit(spi);
        spiMap[spi].enabled = false;
    }
}

void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order) {
    spiMap[spi].bit_order = order;
    if (spiMap[spi].enabled) {
        spiUninit(spi);
        spiInit(spi, spiMap[spi].spi_mode);
    }
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {
    spiMap[spi].data_mode = mode;
    if (spiMap[spi].enabled) {
        spiUninit(spi);
        spiInit(spi, spiMap[spi].spi_mode);
    }
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {
    // actual speed is the system clock divided by some scalar
    spiMap[spi].clock = rate;
    if (spiMap[spi].enabled) {
        spiUninit(spi);
        spiInit(spi, spiMap[spi].spi_mode);
    }
}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data) {
    if (spiMap[spi].spi_mode == SPI_MODE_SLAVE) {
        return 0;
    }

    uint8_t tx_buffer __attribute__((__aligned__(4)));
    uint8_t rx_buffer __attribute__((__aligned__(4)));

    // Wait for SPI transfer finished
    while(spiMap[spi].transmitting) {
        ;
    }

    tx_buffer = data;

    spiMap[spi].dma_user_callback = nullptr;
    spiTransfer(spi, &tx_buffer, &rx_buffer, 1);

    // Wait for SPI transfer finished
    while(spiMap[spi].transmitting) {
        ;
    }

    return rx_buffer;
}

bool hal_spi_is_enabled(hal_spi_interface_t spi) {
    return spiMap[spi].enabled;
}

bool hal_spi_is_enabled_deprecated(void) {
    return false;
}

void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved) {
    info->system_clock = 64000000;
    if (info->version >= HAL_SPI_INFO_VERSION_1) {
        int32_t state = HAL_disable_irq();
        if (spiMap[spi].enabled) {
            switch (spiMap[spi].clock) {
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
        info->default_settings = ((spiMap[spi].spi_mode  == DEFAULT_SPI_MODE) &&
                                  (spiMap[spi].bit_order == DEFAULT_BIT_ORDER) &&
                                  (spiMap[spi].data_mode == DEFAULT_DATA_MODE) &&
                                  (spiMap[spi].clock     == DEFAULT_SPI_CLOCK));
        info->enabled = spiMap[spi].enabled;
        info->mode = spiMap[spi].spi_mode;
        info->bit_order = spiMap[spi].bit_order;
        info->data_mode = spiMap[spi].data_mode;
        if (info->version >= HAL_SPI_INFO_VERSION_2) {
            info->ss_pin = spiMap[spi].ss_pin;
        }
        HAL_enable_irq(state);
    }
}

void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved) {
    spiMap[spi].select_user_callback = cb;
}

void hal_spi_transfer_dma(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback) {
    if (length == 0) {
        return;
    }

    while(spiMap[spi].transmitting) {
        ;
    }

    spiMap[spi].dma_user_callback = userCallback;
    if (spiMap[spi].spi_mode == SPI_MODE_MASTER) {
        SPARK_ASSERT(spiTransfer(spi, (uint8_t *)tx_buffer, (uint8_t *)rx_buffer, length) == length);
    } else {
        // reset transfer length
        spiMap[spi].transfer_length = 0;
        spiMap[spi].slave_buf_length = length;
        spiMap[spi].slave_tx_buf = tx_buffer;
        spiMap[spi].slave_rx_buf = rx_buffer;
        uint32_t err_code = nrfx_spis_buffers_set(spiMap[spi].slave,
                                            (uint8_t *)spiMap[spi].slave_tx_buf,
                                            spiMap[spi].slave_buf_length,
                                            (uint8_t *)spiMap[spi].slave_rx_buf,
                                            spiMap[spi].slave_buf_length);
        if (err_code == NRF_ERROR_INVALID_STATE) {
            // LOG_DEBUG(WARN, "nrfx_spis_buffers_set, invalid state");
        } else {
            SPARK_ASSERT(err_code == NRF_SUCCESS);
            spiMap[spi].transmitting = true;
        }
    }
}

void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi) {
    if (spiMap[spi].spi_mode == SPI_MODE_MASTER) {
        spiTransferCancel(spi);
        spiMap[spi].transmitting = false;
        spiMap[spi].dma_user_callback = nullptr;
    } else {
        // Not supported by SPI Slave
    }
}

int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st) {
    int32_t transfer_length = 0;

    if (spiMap[spi].transmitting) {
        transfer_length = 0;
    } else {
        transfer_length = spiMap[spi].transfer_length;
    }

    if (st != nullptr) {
        st->configured_transfer_length = spiMap[spi].transfer_length;
        st->transfer_length = (uint32_t)transfer_length;
        st->transfer_ongoing = spiMap[spi].transmitting;
        st->ss_state = spiMap[spi].spi_ss_state;
    }

    return transfer_length;
}

int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    if (set_default) {
        spiMap[spi].data_mode = DEFAULT_DATA_MODE;
        spiMap[spi].bit_order = DEFAULT_BIT_ORDER;
        spiMap[spi].clock = DEFAULT_SPI_CLOCK;
    } else {
        spiMap[spi].data_mode = mode;
        spiMap[spi].bit_order = order;
        spiMap[spi].clock = clockdiv;
    }

    if (spiMap[spi].enabled) {
        spiUninit(spi);
        spiInit(spi, spiMap[spi].spi_mode);
    }

    return 0;
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    if (sleep) {
        CHECK_TRUE(hal_spi_is_enabled(spi), SYSTEM_ERROR_NONE);
        CHECK_FALSE(spiMap[spi].suspended, SYSTEM_ERROR_NONE);
        hal_spi_transfer_dma_cancel(spi);
        while (spiMap[spi].transmitting);
        hal_spi_end(spi); // It doesn't clear spi settings, so we can reuse the previous settings on woken up.
        // TODO: configure SPI pins to appropriate mode
        spiMap[spi].suspended = true;
    } else {
        CHECK_TRUE(spiMap[spi].suspended, SYSTEM_ERROR_NONE);
        hal_spi_begin_ext(spi, spiMap[spi].spi_mode, spiMap[spi].ss_pin, nullptr);
        spiMap[spi].suspended = false;
    }
    return SYSTEM_ERROR_NONE;
}

int32_t hal_spi_acquire(hal_spi_interface_t spi, const hal_spi_acquire_config_t* conf) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = spiMap[spi].mutex;
        if (mutex) {
            // FIXME: os_mutex_recursive_lock doesn't take any arguments, using trylock for now
            if (!conf || conf->timeout != 0) {
                return os_mutex_recursive_lock(mutex);
            } else {
                return os_mutex_recursive_trylock(mutex);
            }
        }
    }
    return -1;
}

int32_t hal_spi_release(hal_spi_interface_t spi, void* reserved) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = spiMap[spi].mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}
