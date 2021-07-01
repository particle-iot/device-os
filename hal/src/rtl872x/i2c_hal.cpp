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

typedef enum transfer_state_t {
    TRANSFER_STATE_IDLE,
    TRANSFER_STATE_BUSY,
    TRANSFER_STATE_ERROR_ADDRESS,
    TRANSFER_STATE_ERROR_DATA
} transfer_state_t;

int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config) {
    CHECK_TRUE(i2c < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Disable threading to create the I2C mutex
    os_thread_scheduling(false, nullptr);
    // if (i2cMap[i2c].mutex == nullptr) {
    //     os_mutex_recursive_create(&i2cMap[i2c].mutex);
    // } else {
    //     // Already initialized
    //     os_thread_scheduling(true, nullptr);
    //     return SYSTEM_ERROR_NONE;
    // }

    // Capture the mutex and re-enable threading
    I2cLock lk(i2c);
    os_thread_scheduling(true, nullptr);

    return SYSTEM_ERROR_NONE;
}

void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }

    I2cLock lk(i2c);
}

void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved) {
    // always enabled
}

void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }

    I2cLock lk(i2c);
}

void hal_i2c_end(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }

    I2cLock lk(i2c);
    if (hal_i2c_is_enabled(i2c, nullptr)) {

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
    if (i2c >= HAL_PLATFORM_I2C_NUM || !hal_i2c_is_enabled(i2c, nullptr)) {
        return 0;
    }

    I2cLock lk(i2c);

    return 0;
}

void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config) {
    if (i2c >= HAL_PLATFORM_I2C_NUM || !hal_i2c_is_enabled(i2c, nullptr)) {
        return;
    }

    I2cLock lk(i2c);
}

uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return 6;
    }
    if (!hal_i2c_is_enabled(i2c, nullptr)) {
        return 7;
    }

    I2cLock lk(i2c);

    return 0;
}

uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM || !hal_i2c_is_enabled(i2c, nullptr)) {
        return 0;
    }

    I2cLock lk(i2c);

    return 1;
}

int32_t hal_i2c_available(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return 0;
    }

    I2cLock lk(i2c);

    return 0;
}

int32_t hal_i2c_read(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return -1;
    }

    I2cLock lk(i2c);

    return 0;
}

int32_t hal_i2c_peek(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return -1;
    }

    I2cLock lk(i2c);

    return 0;
}

void hal_i2c_flush(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }
    I2cLock lk(i2c);
}

bool hal_i2c_is_enabled(hal_i2c_interface_t i2c,void* reserved) {
    return false;
}

void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int),void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }

    I2cLock lk(i2c);
}

void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void),void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return;
    }
    
    I2cLock lk(i2c);
}

void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable,void* reserved) {
    // use DMA to send data by default
}

uint8_t hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserved1) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return 1;
    }

    I2cLock lk(i2c);

    return !hal_i2c_is_enabled(i2c, nullptr);
}

int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return -1;
    }
    if (!hal_interrupt_is_isr()) {
        // os_mutex_recursive_t mutex = i2cMap[i2c].mutex;
        // if (mutex) {
        //     return os_mutex_recursive_lock(mutex);
        // }
    }
    return -1;
}

int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return -1;
    }
    if (!hal_interrupt_is_isr()) {
        // os_mutex_recursive_t mutex = i2cMap[i2c].mutex;
        // if (mutex) {
        //     return os_mutex_recursive_unlock(mutex);
        // }
    }
    return -1;
}

int hal_i2c_sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved) {
    if (i2c >= HAL_PLATFORM_I2C_NUM) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    I2cLock lk(i2c);
    if (sleep) {
        // Suspend I2C

    } else {
        // Restore I2C

    }

    return SYSTEM_ERROR_NONE;
}
