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

#include "interrupts_hal.h"
#include "esp32_sdio.h"
#include "check.h"
#include "scope_guard.h"
#include "port.h"
#include "sdspi_host.h"
#include <mutex>
#include "service_debug.h"
#include "delay_hal.h"

namespace {

const system_tick_t ESP32_SDIO_SPI_TIMEOUT_MS = 10000;
const system_tick_t ESP32_SDIO_TX_POLL_INTERVAL_MS = 100;

int espErrorToSystem(esp_err_t err) {
    switch (err) {
        case ESP_OK: {
            return SYSTEM_ERROR_NONE;
        }
        case ESP_ERR_NO_MEM: {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        case ESP_ERR_INVALID_ARG: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        case ESP_ERR_INVALID_STATE: {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        case ESP_ERR_INVALID_SIZE: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        case ESP_ERR_NOT_FOUND: {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        case ESP_ERR_NOT_SUPPORTED: {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        case ESP_ERR_TIMEOUT: {
            return SYSTEM_ERROR_TIMEOUT;
        }
        case ESP_ERR_INVALID_RESPONSE: {
            return SYSTEM_ERROR_BAD_DATA;
        }
        case ESP_FAIL:
        default: {
            return SYSTEM_ERROR_UNKNOWN;
        }
    }
}

#define CHECK_ESP(x) CHECK(espErrorToSystem(x))

} // anonymous

namespace particle {

Esp32Sdio* Esp32Sdio::instance_ = nullptr;

Esp32Sdio::Esp32Sdio(hal_spi_interface_t spi, const hal_spi_info_t* conf, hal_pin_t cs, hal_pin_t intr)
        : lock_(spi, *conf),
          csPin_(cs),
          intrPin_(intr),
          evGroup_(nullptr),
          txInterruptSupported_(false),
          context_{} {

    instance_ = this;
    SPARK_ASSERT(os_semaphore_create(&sem_, 1, 0) == 0);

    evGroup_ = xEventGroupCreate();
    SPARK_ASSERT(evGroup_);
}

Esp32Sdio::~Esp32Sdio() {
    destroy();
}

int Esp32Sdio::init() {
    hal_gpio_config_t conf = {
        .size = sizeof(hal_gpio_config_t),
        .version = HAL_GPIO_VERSION,
        .mode = OUTPUT,
        .set_value = 1,
        .value = 1,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    CHECK(hal_gpio_configure(csPin_, &conf, nullptr));
    hal_gpio_mode(intrPin_, INPUT_PULLUP);

    std::lock_guard<SpiConfigurationLock> spiGuarder(lock_);

    memset(&context_, 0, sizeof(context_));

    CHECK_ESP(at_sdspi_init(5000));

    hal_interrupt_attach(intrPin_, [](void* context) -> void {
        // NOTE: relying on the fact that ESP32 interrupts come through io expander
        // which is processed in a separate thread
        SPARK_ASSERT(!hal_interrupt_is_isr());
        auto self = static_cast<Esp32Sdio*>(context);
        xEventGroupSetBits(self->evGroup_, HAL_USART_PVT_EVENT_RESERVED1);
    }, this, FALLING, nullptr);

    //CHECK_ESP(at_sdspi_set_intr_ena(HOST_SLC0_TOHOST_BIT0_INT_ST | HOST_SLC0_RX_NEW_PACKET_INT_ST));

    // Clear current interrupts just in case
    CHECK(processInterrupts());

    return 0;
}

void Esp32Sdio::destroy() {
    on(false);

    if (sem_) {
        os_semaphore_destroy(sem_);
        sem_ = nullptr;
    }

    if (evGroup_) {
        vEventGroupDelete(evGroup_);
    }

    hal_gpio_mode(csPin_, INPUT);
    hal_gpio_mode(intrPin_, INPUT);
}

void Esp32Sdio::chipSelect(bool state) {
    hal_gpio_write(csPin_, !state);
}

int Esp32Sdio::spiTransmit(const void* tx, void* rx, size_t len) {
    os_semaphore_take(sem_, 0, true);

    hal_spi_transfer_dma(lock_.interface(), (void*)tx, rx, len, [](void) -> void {
        auto self = instance();
        os_semaphore_give(self->sem_, true);
    });
    auto r = os_semaphore_take(sem_, ESP32_SDIO_SPI_TIMEOUT_MS, true);
    if (r) {
        hal_spi_transfer_dma_cancel(lock_.interface());
        return SYSTEM_ERROR_TIMEOUT;
    }

    return 0;
}

void Esp32Sdio::lock() {
    lock_.lock();
}

void Esp32Sdio::unlock() {
    lock_.unlock();
}

EventGroupHandle_t Esp32Sdio::eventGroup() {
    return evGroup_;
}

size_t Esp32Sdio::txBufferSize() const {
    return SDSPI_MAX_DATA_LEN;
}

int Esp32Sdio::read(uint8_t* buf, size_t len) {
    auto canRead = CHECK(rxData());
    if (canRead <= 0) {
        return 0;
    }
    size_t willRead = std::min<size_t>(len, canRead);
    size_t didRead = 0;
    CHECK_ESP(at_sdspi_get_packet(&context_, buf, willRead, &didRead));
    // NOTE: HOST_SLC0_RX_NEW_PACKET_INT_ST is not cleared unless all data is read out
    CHECK(processInterrupts());
    return didRead;
}

int Esp32Sdio::write(const uint8_t* buf, size_t len) {
    size_t canWrite = CHECK(txSpace());
    size_t toWrite = std::min(len, canWrite);

    if (toWrite == 0) {
        return 0;
    }

    CHECK_ESP(at_sdspi_send_packet(&context_, buf, toWrite, 1));
    return toWrite;
}

int Esp32Sdio::waitEvent(uint32_t events, system_tick_t timeout) {
    if (events & HAL_USART_PVT_EVENT_READABLE) {
        if (rxData() > 0) {
            xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_READABLE);
        }
    }

    if (events & HAL_USART_PVT_EVENT_WRITABLE) {
        if (txSpace() > 0) {
            xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_WRITABLE);
        } else {
            // Adjust TX poll interval
            if (!txInterruptSupported_) {
                timeout = std::min(ESP32_SDIO_TX_POLL_INTERVAL_MS, timeout);
            }
        }
    }

    events |= HAL_USART_PVT_EVENT_RESERVED1;
    //timeout = std::min<system_tick_t>(timeout, 100);
    auto mask = xEventGroupWaitBits(evGroup_, events, pdTRUE, pdFALSE, timeout / portTICK_RATE_MS);

    if (mask & HAL_USART_PVT_EVENT_RESERVED1) {
        CHECK(processInterrupts());
        mask |= xEventGroupWaitBits(evGroup_, events, pdTRUE, pdFALSE, 0);
    }

    return mask;
}

int Esp32Sdio::processInterrupts() {
    std::lock_guard<SpiConfigurationLock> spiGuarder(lock_);
    uint32_t intr = 0;
    CHECK_ESP(at_sdspi_get_intr(&intr));
    CHECK_ESP(at_sdspi_clear_intr(intr));
    if (intr & HOST_SLC0_RX_NEW_PACKET_INT_ST) {
        xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_READABLE);
    }
    if (intr & HOST_SLC0_TOHOST_BIT0_INT_ST) {
        xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_WRITABLE);
    }
    return 0;
}

int Esp32Sdio::rxData() {
    uint32_t num = 0;
    CHECK_ESP(esp_slave_get_rx_data_size(&context_, &num));
    return num;
}

int Esp32Sdio::txSpace() {
    uint32_t num = 0;
    CHECK_ESP(esp_slave_get_tx_buffer_num(&context_, &num));
    return num * SDSPI_MAX_DATA_LEN;
}

void Esp32Sdio::txInterruptSupported(bool state) {
    txInterruptSupported_ = state;
}

int Esp32Sdio::on(bool on) {
    if (!on) {
        hal_interrupt_detach(intrPin_);
        txInterruptSupported(false);
    }
    return 0;
}

} // particle
