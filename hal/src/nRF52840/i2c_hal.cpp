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

#include <string.h>
#include "nrfx_twim.h"
#include "nrfx_twis.h"
#include "nrf_gpio.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "delay_hal.h"
#include "platforms.h"
#include "concurrent_hal.h"
#include "interrupts_hal.h"
#include "pinmap_impl.h"
#include "logging.h"
#include "system_error.h"
#include "system_tick_hal.h"
#include "timer_hal.h"
#include <memory>
#include "check.h"

#if PLATFORM_ID == PLATFORM_TRACKER
#include "usart_hal.h"
#define TOTAL_I2C                   3
#else
#define TOTAL_I2C                   2
#endif

#define I2C_IRQ_PRIORITY            APP_IRQ_PRIORITY_LOWEST

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

class I2cLock {
public:
    I2cLock() = delete;

    I2cLock(hal_i2c_interface_t i2c)
            : i2c_(i2c) {
        hal_i2c_lock(i2c, nullptr);
    }

    ~I2cLock() {
        hal_i2c_unlock(i2c_, nullptr);
    }

private:
    hal_i2c_interface_t i2c_;
};

/* TWI instance. */
static nrfx_twim_t m_twim0 = NRFX_TWIM_INSTANCE(0);
static nrfx_twim_t m_twim1 = NRFX_TWIM_INSTANCE(1);
static nrfx_twis_t m_twis0 = NRFX_TWIS_INSTANCE(0);
static nrfx_twis_t m_twis1 = NRFX_TWIS_INSTANCE(1);

typedef enum transfer_state_t {
    TRANSFER_STATE_IDLE,
    TRANSFER_STATE_BUSY,
    TRANSFER_STATE_ERROR_ADDRESS,
    TRANSFER_STATE_ERROR_DATA
} transfer_state_t;

typedef struct nrf5x_i2c_info_t {
    nrfx_twim_t                 *master;
    nrfx_twis_t                 *slave;
    nrfx_twis_event_handler_t   callback_twis;
    uint8_t                     scl_pin;
    uint8_t                     sda_pin;

    volatile hal_i2c_state_t    state;
    volatile transfer_state_t   transfer_state;
    hal_i2c_mode_t              mode;
    uint32_t                    speed;

    uint8_t                     address;    // 7bit data

    uint8_t*                    rx_buf;
    size_t                      rx_buf_size;
    size_t                      rx_index_head;
    size_t                      rx_index_tail;
    uint8_t*                    tx_buf;
    size_t                      tx_buf_size;
    size_t                      tx_index_head;
    size_t                      tx_index_tail;

    os_mutex_recursive_t        mutex;

    void (*callback_on_request)(void);
    void (*callback_on_receive)(int);

    hal_i2c_transmission_config_t transfer_config;
} nrf5x_i2c_info_t;

static void twis0Handler(nrfx_twis_evt_t const * p_event);
static void twis1Handler(nrfx_twis_evt_t const * p_event);

nrf5x_i2c_info_t i2cMap[TOTAL_I2C] = {
    {&m_twim0, &m_twis0, twis0Handler, SCL, SDA}
#if PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_TRACKER
   ,{&m_twim1, &m_twis1, twis1Handler, PMIC_SCL, PMIC_SDA}
#else
   ,{&m_twim1, &m_twis1, twis1Handler, D3, D2}
#endif
#if PLATFORM_ID == PLATFORM_TRACKER
   ,{&m_twim0, &m_twis0, twis0Handler, D8, D9}, // Shared with UART TX/RX
#endif
};

static void setConfigOrDefault(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config) {
    memset(&i2cMap[i2c].transfer_config, 0, sizeof(i2cMap[i2c].transfer_config));
    if (config) {
        memcpy(&i2cMap[i2c].transfer_config, config, std::min((size_t)config->size, sizeof(i2cMap[i2c].transfer_config)));
    } else {
        i2cMap[i2c].transfer_config = {
            .size = sizeof(hal_i2c_transmission_config_t),
            .version = 0,
            .address = 0xff,
            .reserved = {0},
            .quantity = 0,
            .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
            .flags = HAL_I2C_TRANSMISSION_FLAG_STOP
        };
    }
}

static void twisHandler(hal_i2c_interface_t i2c, nrfx_twis_evt_t const * p_event) {
    switch (p_event->type) {
        case NRFX_TWIS_EVT_READ_REQ: {
            i2cMap[i2c].callback_on_request();
            if (p_event->data.buf_req) {
                nrfx_twis_tx_prepare(i2cMap[i2c].slave, (void *)i2cMap[i2c].tx_buf, i2cMap[i2c].tx_index_tail);
            }
            break;
        }
        case NRFX_TWIS_EVT_READ_DONE: {
            i2cMap[i2c].tx_index_tail = i2cMap[i2c].tx_index_head = 0;
            break;
        }
        case NRFX_TWIS_EVT_WRITE_REQ: {
            if (p_event->data.buf_req) {
                nrfx_twis_rx_prepare(i2cMap[i2c].slave, (void *)i2cMap[i2c].rx_buf, i2cMap[i2c].rx_buf_size);
            }
            break;
        }
        case NRFX_TWIS_EVT_WRITE_DONE: {
            i2cMap[i2c].rx_index_head = 0;
            i2cMap[i2c].rx_index_tail = p_event->data.rx_amount;
            i2cMap[i2c].callback_on_receive(p_event->data.rx_amount);
            break;
        }
        case NRFX_TWIS_EVT_READ_ERROR:
        case NRFX_TWIS_EVT_WRITE_ERROR:
        case NRFX_TWIS_EVT_GENERAL_ERROR: {
            LOG_DEBUG(TRACE, "TWIS ERROR");
            break;
        }
        default: {
            break;
        }
    }
}

static void twis0Handler(nrfx_twis_evt_t const * p_event) {
    twisHandler(HAL_I2C_INTERFACE1, p_event);
}

static void twis1Handler(nrfx_twis_evt_t const * p_event) {
    twisHandler(HAL_I2C_INTERFACE2, p_event);
}

static void twimHandler(nrfx_twim_evt_t const * p_event, void * p_context) {
    uint32_t interface = (uint32_t)p_context;
    switch (p_event->type) {
        case NRFX_TWIM_EVT_DONE: {
            // LOG_DEBUG(TRACE, "NRFX_TWIM_EVT_DONE");
            i2cMap[interface].transfer_state = TRANSFER_STATE_IDLE;
            break;
        }
        case NRFX_TWIM_EVT_ADDRESS_NACK: {
            LOG_DEBUG(TRACE, "NRFX_TWIM_EVT_ADDRESS_NACK");
            i2cMap[interface].transfer_state = TRANSFER_STATE_ERROR_ADDRESS;
            break;
        }
        case NRFX_TWIM_EVT_DATA_NACK: {
            LOG_DEBUG(TRACE, "NRFX_TWIM_EVT_DATA_NACK");
            i2cMap[interface].transfer_state = TRANSFER_STATE_ERROR_DATA;
            break;
        }
        default: {
            break;
        }
    }
}

static int twiUninit(hal_i2c_interface_t i2c) {
    if (i2cMap[i2c].state != HAL_I2C_STATE_ENABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (i2cMap[i2c].mode == I2C_MODE_MASTER) {
        nrfx_twim_uninit(i2cMap[i2c].master);
    } else {
        nrfx_twis_uninit(i2cMap[i2c].slave);
    }

    // Reset pin function
    HAL_Pin_Mode(i2cMap[i2c].scl_pin, PIN_MODE_NONE);
    HAL_Pin_Mode(i2cMap[i2c].sda_pin, PIN_MODE_NONE);
    HAL_Set_Pin_Function(i2cMap[i2c].scl_pin, PF_NONE);
    HAL_Set_Pin_Function(i2cMap[i2c].sda_pin, PF_NONE);

    return SYSTEM_ERROR_NONE;
}

static int twiInit(hal_i2c_interface_t i2c) {
    ret_code_t ret;
    nrf_twim_frequency_t nrfFrequency = (i2cMap[i2c].speed == CLOCK_SPEED_400KHZ) ? NRF_TWIM_FREQ_400K : NRF_TWIM_FREQ_100K;

    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (i2cMap[i2c].mode == I2C_MODE_MASTER) {
        const nrfx_twim_config_t twi_config = {
        .scl                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[i2cMap[i2c].scl_pin].gpio_port, PIN_MAP[i2cMap[i2c].scl_pin].gpio_pin),
        .sda                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[i2cMap[i2c].sda_pin].gpio_port, PIN_MAP[i2cMap[i2c].sda_pin].gpio_pin),
        .frequency          = nrfFrequency,
        .interrupt_priority = I2C_IRQ_PRIORITY,
        .hold_bus_uninit    = false
        };

        void *p_context = (void *)i2c;
        ret = nrfx_twim_init(i2cMap[i2c].master, &twi_config, twimHandler, p_context);
        SPARK_ASSERT(ret == NRF_SUCCESS);

        nrfx_twim_enable(i2cMap[i2c].master);
    } else {
        const nrfx_twis_config_t twi_config = {
        .addr               = {i2cMap[i2c].address, 0},
        .scl                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[i2cMap[i2c].scl_pin].gpio_port, PIN_MAP[i2cMap[i2c].scl_pin].gpio_pin),
        .sda                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[i2cMap[i2c].sda_pin].gpio_port, PIN_MAP[i2cMap[i2c].sda_pin].gpio_pin),
        .scl_pull           = NRF_GPIO_PIN_NOPULL,
        .sda_pull           = NRF_GPIO_PIN_NOPULL,
        .interrupt_priority = I2C_IRQ_PRIORITY
        };

        ret = nrfx_twis_init(i2cMap[i2c].slave, &twi_config, i2cMap[i2c].callback_twis);
        SPARK_ASSERT(ret == NRF_SUCCESS);

        nrfx_twis_enable(i2cMap[i2c].slave);
    }

    // Set pin function
    HAL_Set_Pin_Function(i2cMap[i2c].scl_pin, PF_I2C);
    HAL_Set_Pin_Function(i2cMap[i2c].sda_pin, PF_I2C);

    return SYSTEM_ERROR_NONE;
}

static bool isConfigValid(const hal_i2c_config_t* config) {
    if ((config == nullptr) ||
            (config->rx_buffer == nullptr ||
             config->rx_buffer_size == 0 ||
             config->tx_buffer == nullptr ||
             config->tx_buffer_size == 0)) {
        return false;
    }
    return true;
}

int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config) {
    CHECK_TRUE(i2c < TOTAL_I2C, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Disable threading to create the I2C mutex
    os_thread_scheduling(false, nullptr);
    if (i2cMap[i2c].mutex == nullptr) {
        os_mutex_recursive_create(&i2cMap[i2c].mutex);
    } else {
        // Already initialized
        os_thread_scheduling(true, nullptr);
        return SYSTEM_ERROR_NONE;
    }

    // Capture the mutex and re-enable threading
    I2cLock lk(i2c);
    os_thread_scheduling(true, nullptr);

    // Initialize internal data structure
    if (isConfigValid(config)) {
        i2cMap[i2c].rx_buf = config->rx_buffer;
        i2cMap[i2c].rx_buf_size = config->rx_buffer_size;
        i2cMap[i2c].tx_buf = config->tx_buffer;
        i2cMap[i2c].tx_buf_size = config->tx_buffer_size;
    } else {
        // Allocate default buffers
        i2cMap[i2c].rx_buf = new (std::nothrow) uint8_t[I2C_BUFFER_LENGTH];
        i2cMap[i2c].rx_buf_size = I2C_BUFFER_LENGTH;
        i2cMap[i2c].tx_buf = new (std::nothrow) uint8_t[I2C_BUFFER_LENGTH];
        i2cMap[i2c].tx_buf_size = I2C_BUFFER_LENGTH;

        SPARK_ASSERT(i2cMap[i2c].rx_buf && i2cMap[i2c].tx_buf);
    }

    // Uninitialize I2C no matter in which state
    twiUninit(i2c);

    // Initialize I2C state
    i2cMap[i2c].state = HAL_I2C_STATE_DISABLED;
    i2cMap[i2c].mode = I2C_MODE_MASTER;
    i2cMap[i2c].transfer_state = TRANSFER_STATE_IDLE;
    i2cMap[i2c].speed = CLOCK_SPEED_100KHZ;
    i2cMap[i2c].rx_index_head = 0;
    i2cMap[i2c].rx_index_tail = 0;
    i2cMap[i2c].tx_index_head = 0;
    i2cMap[i2c].tx_index_tail = 0;
    memset((void *)i2cMap[i2c].rx_buf, 0, i2cMap[i2c].rx_buf_size);
    memset((void *)i2cMap[i2c].tx_buf, 0, i2cMap[i2c].tx_buf_size);

    return SYSTEM_ERROR_NONE;
}

void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }

    I2cLock lk(i2c);
    i2cMap[i2c].speed = speed;
}

void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved) {
    // always enabled
}

void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }

    I2cLock lk(i2c);
#if PLATFORM_ID == PLATFORM_TRACKER
    /*
     * On Tracker both I2C_INTERFACE1 and I2C_INTERFACE3 use the same peripheral - m_twim0,
     * but on different pins. We cannot enable both of them at the same time.
     */
    if (i2c == HAL_I2C_INTERFACE1 || i2c == HAL_I2C_INTERFACE3) {
        hal_i2c_interface_t dependent = (i2c == HAL_I2C_INTERFACE1 ? HAL_I2C_INTERFACE3 : HAL_I2C_INTERFACE1);
        if (hal_i2c_is_enabled(dependent, nullptr)) {
            // Unfortunately we cannot return an error code here
            return;
        }
    }
    /*
     * On Tracker both I2C_INTERFACE3 and USART_SERIAL1 use the same pins - D8 and D9,
     * We cannot enable both of them at the same time.
     */
    if (i2c == HAL_I2C_INTERFACE3) {
        if (HAL_USART_Is_Enabled(HAL_USART_SERIAL1)) {
            // Unfortunately we cannot return an error code here
            return;
        }
    }
#endif

    if (twiUninit(i2c) == SYSTEM_ERROR_NONE) {
        i2cMap[i2c].state = HAL_I2C_STATE_DISABLED;
    }

    if (i2cMap[i2c].state == HAL_I2C_STATE_DISABLED) {
        i2cMap[i2c].rx_index_head = 0;
        i2cMap[i2c].rx_index_tail = 0;
        i2cMap[i2c].tx_index_head = 0;
        i2cMap[i2c].tx_index_tail = 0;
        memset((void *)i2cMap[i2c].rx_buf, 0, i2cMap[i2c].rx_buf_size);
        memset((void *)i2cMap[i2c].tx_buf, 0, i2cMap[i2c].tx_buf_size);
    }

    i2cMap[i2c].mode = mode;
    i2cMap[i2c].address = address;

    if (i2cMap[i2c].state != HAL_I2C_STATE_ENABLED && twiInit(i2c) == SYSTEM_ERROR_NONE) {
        i2cMap[i2c].state = HAL_I2C_STATE_ENABLED;
    }
}

void hal_i2c_end(hal_i2c_interface_t i2c,void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }

    I2cLock lk(i2c);
    if (hal_i2c_is_enabled(i2c, nullptr)) {
        if (twiUninit(i2c) == SYSTEM_ERROR_NONE) {
            i2cMap[i2c].state = HAL_I2C_STATE_DISABLED;
        }
    }
}

uint32_t hal_i2c_request(hal_i2c_interface_t i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved) {
    hal_i2c_transmission_config_t conf = {
        .size = sizeof(hal_i2c_transmission_config_t),
        .version = 0,
        .address = address,
        .reserved = {0},
        .quantity = quantity,
        .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
        .flags = (uint32_t)(stop ? HAL_I2C_TRANSMISSION_FLAG_STOP : 0)
    };
    return hal_i2c_request_ex(i2c, &conf, nullptr);
}

int32_t hal_i2c_request_ex(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config, void* reserved) {
    if (i2c >= TOTAL_I2C || !hal_i2c_is_enabled(i2c, nullptr)) {
        return 0;
    }

    I2cLock lk(i2c);
    uint32_t ret;
    size_t quantity = 0;

    if (!config) {
        goto ret;
    }

    quantity = config->quantity;

    // clamp to buffer length
    if (quantity > i2cMap[i2c].rx_buf_size) {
        quantity = i2cMap[i2c].rx_buf_size;
    }

    i2cMap[i2c].transfer_state = TRANSFER_STATE_BUSY;
    ret = nrfx_twim_rx(i2cMap[i2c].master, config->address, (uint8_t *)i2cMap[i2c].rx_buf, quantity);
    if (ret) {
        // FIXME: There is a bug in nrfx_twim driver, if we call nrfx_twim_rx repeatedly and quickly,
        // p_cb->busy will be set and never cleared, in this case nrfx_twim_rx always returns busy error
        LOG_DEBUG(TRACE, "BUSY ERROR, restore twi.");
        hal_i2c_reset(i2c, 0, nullptr);
        quantity = 0;
        goto ret;
    }

    if (!WAIT_TIMED(config->timeout_ms, i2cMap[i2c].transfer_state == TRANSFER_STATE_BUSY)) {
        hal_i2c_reset(i2c, 0, nullptr);
        quantity = 0;
        goto ret;
    }

    if (i2cMap[i2c].transfer_state != TRANSFER_STATE_IDLE) {
        // Get into error state
        hal_i2c_reset(i2c, 0, nullptr);
        quantity = 0;
        goto ret;
    }

ret:
    i2cMap[i2c].transfer_state = TRANSFER_STATE_IDLE;
    i2cMap[i2c].rx_index_head = 0;
    i2cMap[i2c].rx_index_tail = quantity;
    return quantity;
}

void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config) {
    if (i2c >= TOTAL_I2C || !hal_i2c_is_enabled(i2c, nullptr)) {
        return;
    }

    I2cLock lk(i2c);
    i2cMap[i2c].address = address;
    i2cMap[i2c].tx_index_head = 0;
    i2cMap[i2c].tx_index_tail = 0;
    setConfigOrDefault(i2c, config);
}

uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return 6;
    }
    if (!hal_i2c_is_enabled(i2c, nullptr)) {
        return 7;
    }

    I2cLock lk(i2c);

    uint32_t ret;
    uint8_t ret_code = 0;

    if (i2cMap[i2c].transfer_config.address != 0xff) {
        stop = i2cMap[i2c].transfer_config.flags & HAL_I2C_TRANSMISSION_FLAG_STOP;
    }

    i2cMap[i2c].transfer_state = TRANSFER_STATE_BUSY;
    ret = nrfx_twim_tx(i2cMap[i2c].master, i2cMap[i2c].address, (uint8_t *)i2cMap[i2c].tx_buf,
                                    i2cMap[i2c].tx_index_tail, !stop);
    if (ret) {
        hal_i2c_reset(i2c, 0, nullptr);
        ret_code = 1;
        goto ret;
    }

    if (!WAIT_TIMED(i2cMap[i2c].transfer_config.timeout_ms, i2cMap[i2c].transfer_state == TRANSFER_STATE_BUSY)) {
        hal_i2c_reset(i2c, 0, nullptr);
        ret_code = 2;
        goto ret;
    }

    if (i2cMap[i2c].transfer_state != TRANSFER_STATE_IDLE) {
        hal_i2c_reset(i2c, 0, nullptr);
        ret_code = 3;
        goto ret;
    }

ret:
    i2cMap[i2c].transfer_state = TRANSFER_STATE_IDLE;
    i2cMap[i2c].tx_index_head = 0;
    i2cMap[i2c].tx_index_tail = 0;
    return ret_code;
}

uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data, void* reserved) {
    if (i2c >= TOTAL_I2C || !hal_i2c_is_enabled(i2c, nullptr)) {
        return 0;
    }

    I2cLock lk(i2c);
    // in master/slave transmitter mode
    // don't bother if buffer is full
    if (i2cMap[i2c].tx_index_tail >= i2cMap[i2c].tx_buf_size) {
        return 0;
    }
    // put byte in tx buffer
    i2cMap[i2c].tx_buf[i2cMap[i2c].tx_index_head++] = data;
    // update amount in buffer
    i2cMap[i2c].tx_index_tail = i2cMap[i2c].tx_index_head;

    return 1;
}

int32_t hal_i2c_available(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return 0;
    }

    I2cLock lk(i2c);
    int32_t available = i2cMap[i2c].rx_index_tail - i2cMap[i2c].rx_index_head;
    return available;
}

int32_t hal_i2c_read(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return -1;
    }

    I2cLock lk(i2c);
    int value = -1;
    // get each successive byte on each call
    if (i2cMap[i2c].rx_index_head < i2cMap[i2c].rx_index_tail) {
        value = i2cMap[i2c].rx_buf[i2cMap[i2c].rx_index_head++];
    }
    return value;
}

int32_t hal_i2c_peek(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return -1;
    }

    I2cLock lk(i2c);
    int value = -1;
    if(i2cMap[i2c].rx_index_head < i2cMap[i2c].rx_index_tail) {
        value = i2cMap[i2c].rx_buf[i2cMap[i2c].rx_index_head];
    }
    return value;
}

void hal_i2c_flush(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }
    I2cLock lk(i2c);
    i2cMap[i2c].rx_index_head = 0;
    i2cMap[i2c].rx_index_tail = 0;
    i2cMap[i2c].tx_index_head = 0;
    i2cMap[i2c].tx_index_tail = 0;
}

bool hal_i2c_is_enabled(hal_i2c_interface_t i2c,void* reserved) {
    return i2cMap[i2c].state == HAL_I2C_STATE_ENABLED;
}

void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int),void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }

    I2cLock lk(i2c);
    i2cMap[i2c].callback_on_receive = function;
}

void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void),void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return;
    }
    
    I2cLock lk(i2c);
    i2cMap[i2c].callback_on_request = function;
}

void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable,void* reserved) {
    // use DMA to send data by default
}

uint8_t hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserved1) {
    if (i2c >= TOTAL_I2C) {
        return 1;
    }

    I2cLock lk(i2c);
    if (hal_i2c_is_enabled(i2c, nullptr)) {
        hal_i2c_end(i2c, nullptr);

        HAL_Pin_Mode(i2cMap[i2c].sda_pin, INPUT_PULLUP); //Turn SCA into high impedance input
        HAL_Pin_Mode(i2cMap[i2c].scl_pin, OUTPUT);       //Turn SCL into a normal GPO
        HAL_GPIO_Write(i2cMap[i2c].scl_pin, 1);     // Start idle HIGH

        //Generate 9 pulses on SCL to tell slave to release the bus
        for (int i = 0; i < 9; i++) {
            HAL_GPIO_Write(i2cMap[i2c].scl_pin, 0);
            HAL_Delay_Microseconds(100);
            HAL_GPIO_Write(i2cMap[i2c].scl_pin, 1);
            HAL_Delay_Microseconds(100);
        }

        //Change SCL to be an input
        HAL_Pin_Mode(i2cMap[i2c].scl_pin, INPUT_PULLUP);

        hal_i2c_begin(i2c, i2cMap[i2c].mode, i2cMap[i2c].address, nullptr);
        HAL_Delay_Milliseconds(50);
        return 0;
    }
    return 1;
}

int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return -1;
    }
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c].mutex;
        if (mutex) {
            return os_mutex_recursive_lock(mutex);
        }
    }
    return -1;
}

int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return -1;
    }
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c].mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}

int hal_i2c_sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved) {
    if (i2c >= TOTAL_I2C) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    I2cLock lk(i2c);
    if (sleep) {
        // Suspend I2C
        CHECK_TRUE(i2cMap[i2c].state == HAL_I2C_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);
        hal_i2c_end(i2c, nullptr);
        i2cMap[i2c].state = HAL_I2C_STATE_SUSPENDED;
    } else {
        // Restore I2C
        CHECK_TRUE(i2cMap[i2c].state == HAL_I2C_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
        hal_i2c_begin(i2c, i2cMap[i2c].mode, i2cMap[i2c].address, nullptr);
    }

    return SYSTEM_ERROR_NONE;
}
