/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <cstdlib>
#include "hal_platform.h"
#include "at_dtm_hal.h"
#include "check.h"
#include "ble_dtm.h"
#include <nrf_sdh.h>
#include "concurrent_hal.h"
#include "usart_hal.h"
#include "service_debug.h"
#include "system_network.h"
#include "system_defs.h"
#include "delay_hal.h"
#include "ncp_client.h"
#include "network/ncp_client/quectel/quectel_ncp_client.h"
#include "network/ncp_client/sara/sara_ncp_client.h"
#include "network/ncp_client/esp32/esp32_ncp_client.h"
#include "spark_wiring_vector.h"

#if HAL_PLATFORM_AT_DTM

using spark::Vector;

namespace {
const char* dtmTypeToStr[] = {
    "BLE",
    "Cellular",
    "Wi-Fi",
    "Ethernet",
    "GNSS"
};
}

class AtDtmBaseClass {
public:
    os_thread_t thread_;
    os_semaphore_t semaphore_;
    bool started_;
    bool initialized_;
    hal_at_dtm_type_t type_;
    hal_at_dtm_interface_t interface_;
    uint8_t ifIndex_;
    hal_usart_interface_t userUart_;

    AtDtmBaseClass()
            : thread_(nullptr), semaphore_(nullptr), started_(false), initialized_(false) {
    }

    ~AtDtmBaseClass() = default;

    int start(hal_at_dtm_type_t type, const hal_at_dtm_interface_config_t* config) {
        CHECK_TRUE(this->initialized_, SYSTEM_ERROR_NONE);
        CHECK_FALSE(this->started_, SYSTEM_ERROR_NONE);
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        auto instance = interfaceInUse(config->interface, config->index);
        if (instance) {
            LOG(TRACE, "Stop the %s DTM that is using the same user interface", dtmTypeToStr[instance->type()]);
            instance->stop();
        }
        switch (config->interface) {
            case HAL_AT_DTM_INTERFACE_UART: {
                auto uart = (hal_usart_interface_t)(config->index);
                if (!hal_usart_is_enabled(uart)) {
                    LOG(TRACE, "Initializing user serial %d", uart);
                    const size_t bufferSize = SERIAL_BUFFER_SIZE;
                    hal_usart_buffer_config_t conf = {
                        .size = sizeof(hal_usart_buffer_config_t),
                        .rx_buffer = (uint8_t*)malloc(bufferSize),
                        .rx_buffer_size = bufferSize,
                        .tx_buffer = (uint8_t*)malloc(bufferSize),
                        .tx_buffer_size = bufferSize
                    };
                    CHECK(hal_usart_init_ex(uart, &conf, nullptr));
                }
                if (type == HAL_AT_DTM_TYPE_BLE) {
                    hal_usart_begin(uart, 19200); // TODO: The baudrate affects the timing in ble_dtm.c. We need to change it in ble_dtm.h
                } else {
                    hal_usart_begin(uart, config->params.baudrate);
                }
                LOG(TRACE, "User serial %d started", uart);
                break;
            }
            default: return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        this->type_ = type;
        this->interface_ = config->interface;
        this->ifIndex_ = config->index;
        this->userUart_ = (hal_usart_interface_t)(config->index);
        registerInstance(this);
        this->started_ = true;
        os_semaphore_give(this->semaphore_, false);
        LOG(TRACE, "%s DTM started", dtmTypeToStr[type]);
        return SYSTEM_ERROR_NONE;
    }

    int stop() {
        CHECK_TRUE(this->initialized_, SYSTEM_ERROR_NONE);
        CHECK_TRUE(this->started_, SYSTEM_ERROR_NONE);
        this->started_ = false;
        LOG(TRACE, "%s DTM stopped", dtmTypeToStr[this->type()]);
        return SYSTEM_ERROR_NONE;
    }

    hal_at_dtm_type_t type() const {
        return this->type_;
    }

    AtDtmBaseClass* interfaceInUse(hal_at_dtm_interface_t interface, uint8_t index) {
        for (auto instance : instances_) {
            if (interface == instance->interface_ && index == instance->ifIndex_) {
                return instance;
            }
        }
        return nullptr;
    }

    static void registerInstance(AtDtmBaseClass* instance) {
        instances_.append(instance);
    }

    virtual int init() = 0;
    virtual void loop() = 0;

private:
    static Vector<AtDtmBaseClass*> instances_;
};

Vector<AtDtmBaseClass*> AtDtmBaseClass::instances_;

#if HAL_PLATFORM_BLE
class BleAtDtmClass : public AtDtmBaseClass {
public:
    static BleAtDtmClass* instance() {
        static BleAtDtmClass obj;
        return &obj;
    }

    int init() override {
        CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
        if (os_semaphore_create(&semaphore_, 1, 0)) {
            semaphore_ = nullptr;
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        if (os_thread_create(&thread_, "BLE DTM Thread", OS_THREAD_PRIORITY_DEFAULT, bleDtmThread, this, 4096)) {
            thread_ = nullptr;
            if (semaphore_) {
                os_semaphore_destroy(semaphore_);
                semaphore_ = nullptr;
            }
            LOG(ERROR, "os_thread_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        // Disable SoftDevice
        nrf_sdh_disable_request();
        while (nrf_sdh_is_enabled()) {
            ;
        }
        if (dtm_init() != DTM_SUCCESS) {
            // If DTM cannot be correctly initialized, then we just return.
            return SYSTEM_ERROR_INTERNAL;
        }
        dtm_set_txpower(RADIO_TXPOWER_TXPOWER_Pos8dBm);
        dtmCmdPut(0x9303); // 2440MHZ 8dBm
        initialized_ = true;
        LOG(TRACE, "Initialized BLE DTM test mode");
        return SYSTEM_ERROR_NONE;
    }

private:
    /**@details Maximum iterations needed in the main loop between stop bit 1st byte and start bit 2nd
     * byte. DTM standard allows 5000us delay between stop bit 1st byte and start bit 2nd byte.
     * As the time is only known when a byte is received, then the time between between stop bit 1st
     * byte and stop bit 2nd byte becomes:
     *      5000us + transmission time of 2nd byte.
     *
     * Byte transmission time is (Baud rate of 19200):
     *      10bits * 1/19200 = approx. 520 us/byte (8 data bits + start & stop bit).
     *
     * Loop time on polling UART register for received byte is defined in ble_dtm.c as:
     *   UART_POLL_CYCLE = 260 us
     *
     * The max time between two bytes thus becomes (loop time: 260us / iteration):
     *      (5000us + 520us) / 260us / iteration = 21.2 iterations.
     *
     * This is rounded down to 21.
     *
     * @note If UART bit rate is changed, this value should be recalculated as well.
     */
    static constexpr uint32_t MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE = ((5000 + 2 * UART_POLL_CYCLE) / UART_POLL_CYCLE);

    BleAtDtmClass()
            : AtDtmBaseClass() {
    }

    ~BleAtDtmClass() = default;

    void loop() override {
        LOG(TRACE, "Enter the BLE DTM loop");
        while (started_) {
            static uint32_t msbTime = 0;            // Time when MSB of the DTM command was read. Used to catch stray bytes from "misbehaving" testers.
            static bool isMsbRead = false;          // True when MSB of the DTM command has been read and the application is waiting for LSB.
            static uint16_t dtmCmdFromUart = 0;     // Packed command containing command_code:freqency:length:payload in 2:6:6:2 bits.

            // Will return every UART pool timeout,
            uint32_t currentTime = dtm_wait();
            if (hal_usart_available(userUart_) == 0) {
                continue;
            }
            uint8_t rxByte = hal_usart_read(userUart_);
            // LOG(TRACE, "rxByte: %02X", rxByte);
            if (!isMsbRead) {
                // This is first byte of two-byte command.
                isMsbRead = true;
                dtmCmdFromUart = ((dtm_cmd_t)rxByte) << 8;
                msbTime = currentTime;
                // Go back and wait for 2nd byte of command word.
                continue;
            }
            // This is the second byte read; combine it with the first and process command
            if (currentTime > (msbTime + MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE)) {
                // More than ~5mS after msb: Drop old byte, take the new byte as MSB.
                // The variable isMsbRead will remains true.
                // Go back and wait for 2nd byte of the command word.
                dtmCmdFromUart = ((dtm_cmd_t)rxByte) << 8;
                msbTime = currentTime;
                continue;
            }
            // 2-byte UART command received.
            isMsbRead = false;
            dtmCmdFromUart |= (dtm_cmd_t)rxByte;
            if (dtmCmdPut(dtmCmdFromUart) != DTM_SUCCESS) {
                LOG(ERROR, "dtmCmdPut() failed");
                // Extended error handling may be put here.
                // Default behavior is to return the event on the UART (see below);
                // the event report will reflect any lack of success.
            }
            // Retrieve result of the operation. This implementation will busy-loop
            // for the duration of the byte transmissions on the UART.
            dtm_event_t result; // Result of a DTM operation.
            if (dtm_event_get(&result)) {
                // Report command status on the UART.
                // Transmit MSB of the result.
                hal_usart_write(userUart_, (result >> 8) & 0xFF);
                // Transmit LSB of the result.
                hal_usart_write(userUart_, result & 0xFF);
                hal_usart_flush(userUart_);
            }
        }
        LOG(TRACE, "Exit the BLE DTM loop");
    }

    uint32_t dtmCmdPut(uint16_t command) {
        dtm_cmd_t      command_code = (command >> 14) & 0x03;
        dtm_freq_t     freq         = (command >> 8) & 0x3F;
        uint32_t       length       = (command >> 2) & 0x3F;
        dtm_pkt_type_t payload      = command & 0x03;
        return dtm_cmd(command_code, freq, length, payload);
    }

    static os_thread_return_t bleDtmThread(void* param) {
        auto instance = (BleAtDtmClass*)param;
        while (true) {
            os_semaphore_take(instance->semaphore_, CONCURRENT_WAIT_FOREVER, false);
            instance->loop();
        }
        os_thread_exit(instance->thread_);
    }
};
#endif // HAL_PLATFORM_BLE

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
class WiFiAtDtmClass : public AtDtmBaseClass {
public:
    static WiFiAtDtmClass* instance() {
        static WiFiAtDtmClass obj;
        return &obj;
    }

    int init() override {
        CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
        LOG(TRACE, "Initialize Wi-Fi DTM test mode");
        if (os_semaphore_create(&semaphore_, 1, 0)) {
            semaphore_ = nullptr;
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        if (os_thread_create(&thread_, "Wi-Fi DTM Thread", OS_THREAD_PRIORITY_DEFAULT, wifiDtmThread, this, 4096)) {
            thread_ = nullptr;
            if (semaphore_) {
                os_semaphore_destroy(semaphore_);
                semaphore_ = nullptr;
            }
            LOG(ERROR, "os_thread_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        network_off(NETWORK_INTERFACE_WIFI_STA, 0, 0, nullptr);
        while (!network_is_off(NETWORK_INTERFACE_WIFI_STA, nullptr)) {
            ;
        }
        LOG(TRACE, "Turn on the Wi-Fi modem");
        auto client = new particle::Esp32NcpClient();
        auto conf = particle::NcpClientConfig();
        client->init(conf);
        client->on();
        initialized_ = true;
        return SYSTEM_ERROR_NONE;
    }

private:
    WiFiAtDtmClass()
            : AtDtmBaseClass() {
    }

    ~WiFiAtDtmClass() = default;

    void loop() override {
        LOG(TRACE, "Enter the Wi-Fi DTM loop");
        while (started_) {
            if (hal_usart_available(HAL_PLATFORM_WIFI_SERIAL)) {
                uint8_t c = hal_usart_read(HAL_PLATFORM_WIFI_SERIAL);
                hal_usart_write(userUart_, c);
            }
            if (hal_usart_available(userUart_)) {
                uint8_t c = hal_usart_read(userUart_);
                hal_usart_write(HAL_PLATFORM_WIFI_SERIAL, c);
            }
        }
        LOG(TRACE, "Exit the Wi-Fi DTM loop");
    }

    static os_thread_return_t wifiDtmThread(void* param) {
        auto instance = (WiFiAtDtmClass*)param;
        while (true) {
            os_semaphore_take(instance->semaphore_, CONCURRENT_WAIT_FOREVER, false);
            instance->loop();
        }
        os_thread_exit(instance->thread_);
    }
};
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
class CellularAtDtmClass : public AtDtmBaseClass {
public:
    static CellularAtDtmClass* instance() {
        static CellularAtDtmClass obj;
        return &obj;
    }

    int init() override {
        CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
        LOG(TRACE, "Initialize Cellular DTM test mode");
        if (os_semaphore_create(&semaphore_, 1, 0)) {
            semaphore_ = nullptr;
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        if (os_thread_create(&thread_, "Cellular DTM Thread", OS_THREAD_PRIORITY_DEFAULT, cellularDtmThread, this, 4096)) {
            thread_ = nullptr;
            if (semaphore_) {
                os_semaphore_destroy(semaphore_);
                semaphore_ = nullptr;
            }
            LOG(ERROR, "os_thread_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        network_off(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr);
        while (!network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
            ;
        }
#if PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER
        auto client = new particle::QuectelNcpClient();
#else
        auto client = new particle::SaraNcpClient();
#endif
        particle::CellularNcpClientConfig conf = {};
        conf.ncpIdentifier(platform_primary_ncp_identifier());
        client->init(conf);
        client->on();
        initialized_ = true;
        return SYSTEM_ERROR_NONE;
    }

private:
    CellularAtDtmClass()
            : AtDtmBaseClass() {
    }

    ~CellularAtDtmClass() = default;

    void loop() override {
        LOG(TRACE, "Enter the Cellular DTM loop");
        while (started_) {
            if (hal_usart_available(HAL_PLATFORM_CELLULAR_SERIAL)) {
                uint8_t c = hal_usart_read(HAL_PLATFORM_CELLULAR_SERIAL);
                hal_usart_write(userUart_, c);
            }
            if (hal_usart_available(userUart_)) {
                uint8_t c = hal_usart_read(userUart_);
                hal_usart_write(HAL_PLATFORM_CELLULAR_SERIAL, c);
            }
        }
        LOG(TRACE, "Exit the Cellular DTM loop");
    }

    static os_thread_return_t cellularDtmThread(void* param) {
        auto instance = (CellularAtDtmClass*)param;
        while (true) {
            os_semaphore_take(instance->semaphore_, CONCURRENT_WAIT_FOREVER, false);
            instance->loop();
        }
        os_thread_exit(instance->thread_);
    }
};
#endif // HAL_PLATFORM_CELLULAR


int hal_at_dtm_init(hal_at_dtm_type_t type, void* reserved) {
    if (type == HAL_AT_DTM_TYPE_BLE) {
#if HAL_PLATFORM_BLE
        return BleAtDtmClass::instance()->init();
#endif
    } else if (type == HAL_AT_DTM_TYPE_WIFI) {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
        return WiFiAtDtmClass::instance()->init();
#endif
    } else if (type == HAL_AT_DTM_TYPE_CELLULAR) {
#if HAL_PLATFORM_CELLULAR
        CellularAtDtmClass::instance()->init();
#endif
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_at_dtm_start(hal_at_dtm_type_t type, const hal_at_dtm_interface_config_t* config, void* reserved) {
    if (type == HAL_AT_DTM_TYPE_BLE) {
#if HAL_PLATFORM_BLE
        return BleAtDtmClass::instance()->start(type, config);
#endif
    } else if (type == HAL_AT_DTM_TYPE_WIFI) {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
        return WiFiAtDtmClass::instance()->start(type, config);
#endif
    } else if (type == HAL_AT_DTM_TYPE_CELLULAR) {
#if HAL_PLATFORM_CELLULAR
        CellularAtDtmClass::instance()->start(type, config);
#endif
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_at_dtm_stop(hal_at_dtm_type_t type, void* reserved) {
    if (type == HAL_AT_DTM_TYPE_BLE) {
#if HAL_PLATFORM_BLE
        return BleAtDtmClass::instance()->stop();
#endif
    } else if (type == HAL_AT_DTM_TYPE_WIFI) {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
        return WiFiAtDtmClass::instance()->stop();
#endif
    } else if (type == HAL_AT_DTM_TYPE_CELLULAR) {
#if HAL_PLATFORM_CELLULAR
        CellularAtDtmClass::instance()->stop();
#endif
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#endif // HAL_PLATFORM_AT_DTM
