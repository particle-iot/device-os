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

#pragma once

#include "spi_hal.h"
#include "gpio_hal.h"
// FIXME: using the same events here just for simplicity
#include "usart_hal_private.h"
#include <atomic>
#include "spi_lock.h"
#include "concurrent_hal.h"
#include "sdspi_host.h"

namespace particle {

class Esp32Sdio {
public:
    Esp32Sdio(hal_spi_interface_t spi, const hal_spi_info_t* conf, hal_pin_t cs, hal_pin_t intr);
    ~Esp32Sdio();

    int init();
    void destroy();

    static Esp32Sdio* instance() {
        return instance_;
    }

    void chipSelect(bool state);
    int spiTransmit(const void* tx, void* rx, size_t len);
    void lock();
    void unlock();
    int waitEvent(uint32_t events, system_tick_t timeout);

    int read(uint8_t* buf, size_t len);
    int write(const uint8_t* buf, size_t len);

    void txInterruptSupported(bool state);

    EventGroupHandle_t eventGroup();

    size_t txBufferSize() const;

    int on(bool on);

private:
    int rxData();
    int txSpace();
    int processInterrupts();

private:
    SpiConfigurationLock lock_;
    hal_pin_t csPin_;
    hal_pin_t intrPin_;
    os_semaphore_t sem_;

    /* FIXME: SDSPI port functions do not have any kind of state arguments :( */
    static Esp32Sdio* instance_;

    EventGroupHandle_t evGroup_;

    volatile bool txInterruptSupported_;
    spi_context_t context_;
};

} // particle
