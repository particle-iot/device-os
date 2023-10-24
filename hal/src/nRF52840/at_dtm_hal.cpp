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

#if HAL_PLATFORM_AT_DTM

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
#define MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE ((5000 + 2 * UART_POLL_CYCLE) / UART_POLL_CYCLE)

namespace {

os_thread_t atDtmThread = nullptr;

hal_usart_interface_t bleDtmUart;

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
hal_usart_interface_t wifiDtmUart;
#endif

#if HAL_PLATFORM_CELLULAR
hal_usart_interface_t cellDtmUart;
#endif

hal_at_dtm_type_t currDtmType = HAL_AT_DTM_TYPE_MAX;


uint32_t dtm_cmd_put(uint16_t command) {
    dtm_cmd_t      command_code = (command >> 14) & 0x03;
    dtm_freq_t     freq         = (command >> 8) & 0x3F;
    uint32_t       length       = (command >> 2) & 0x3F;
    dtm_pkt_type_t payload      = command & 0x03;

    return dtm_cmd(command_code, freq, length, payload);
}

void bleDtmLoop() {
    static uint32_t msb_time          = 0;     // Time when MSB of the DTM command was read. Used to catch stray bytes from "misbehaving" testers.
    static bool     is_msb_read       = false; // True when MSB of the DTM command has been read and the application is waiting for LSB.
    static uint16_t dtm_cmd_from_uart = 0;     // Packed command containing command_code:freqency:length:payload in 2:6:6:2 bits.

    // Will return every UART pool timeout,
    uint32_t current_time = dtm_wait();

    if (hal_usart_available(bleDtmUart) == 0) {
        return;
    }
    uint8_t rx_byte = hal_usart_read(bleDtmUart);

    if (!is_msb_read) {
        // This is first byte of two-byte command.
        is_msb_read       = true;
        dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
        msb_time          = current_time;

        // Go back and wait for 2nd byte of command word.
        return;
    }

    // This is the second byte read; combine it with the first and process command
    if (current_time > (msb_time + MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE)) {
        // More than ~5mS after msb: Drop old byte, take the new byte as MSB.
        // The variable is_msb_read will remains true.
        // Go back and wait for 2nd byte of the command word.
        dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
        msb_time          = current_time;
        return;
    }

    // 2-byte UART command received.
    is_msb_read        = false;
    dtm_cmd_from_uart |= (dtm_cmd_t)rx_byte;

    if (dtm_cmd_put(dtm_cmd_from_uart) != DTM_SUCCESS) {
        LOG(ERROR, "dtm_cmd_put() failed");
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
        hal_usart_write(bleDtmUart, (result >> 8) & 0xFF);
        // Transmit LSB of the result.
        hal_usart_write(bleDtmUart, result & 0xFF);
        hal_usart_flush(bleDtmUart);
    }
}

void wifiCellularDtmLoop(hal_at_dtm_type_t type) {
    hal_usart_interface_t uartModem = HAL_USART_SERIAL1;
    hal_usart_interface_t uartUser = HAL_USART_SERIAL1;

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    if (type == HAL_AT_DTM_TYPE_WIFI) {
        uartModem = HAL_PLATFORM_WIFI_SERIAL;
        uartUser = wifiDtmUart;
    }
#endif
#if HAL_PLATFORM_CELLULAR
    if (type == HAL_AT_DTM_TYPE_WIFI) {
        uartModem = HAL_PLATFORM_CELLULAR_SERIAL;
        uartUser = cellDtmUart;
    }
#endif
    if (hal_usart_available(uartModem)) {
        uint8_t c = hal_usart_read(uartModem);
        hal_usart_write(uartUser, c);
    }
    if (hal_usart_available(uartUser)) {
        uint8_t c = hal_usart_read(uartUser);
        hal_usart_write(uartModem, c);
    }
}

os_thread_return_t atDtmLoop(void* param) {
    while (true) {
        if (currDtmType == HAL_AT_DTM_TYPE_BLE) {
            bleDtmLoop();
        } else if (currDtmType == HAL_AT_DTM_TYPE_WIFI || currDtmType == HAL_AT_DTM_TYPE_CELLULAR) {
            wifiCellularDtmLoop(currDtmType);
        }
    }
    os_thread_exit(atDtmThread);
}

} // anonymous namespace

int hal_at_dtm_init(hal_at_dtm_type_t type, const hal_at_dtm_interface_config_t* config, void* reserved) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (!atDtmThread) {
        if (os_thread_create(&atDtmThread, "AT DTM Thread", OS_THREAD_PRIORITY_DEFAULT, atDtmLoop, nullptr, 512)) {
            atDtmThread = nullptr;
            LOG(ERROR, "os_thread_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
    }

    switch (type) {
        case HAL_AT_DTM_TYPE_BLE: {
            LOG(TRACE, "hal_at_dtm_init(): HAL_AT_DTM_TYPE_BLE");
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
            dtm_cmd_put(0x9303); // 2440MHZ 8dBm
            bleDtmUart = (hal_usart_interface_t)(config->index);
            currDtmType = HAL_AT_DTM_TYPE_BLE;
            break;
        }
#if HAL_PLATFORM_CELLULAR
        case HAL_AT_DTM_TYPE_CELLULAR: {
            LOG(TRACE, "hal_at_dtm_init(): HAL_AT_DTM_TYPE_CELLULAR");
            network_off(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr);
            while (!network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
                ;
            }

#if PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_TRACKER
            particle::QuectelNcpClient client;
#else
            particle::SaraNcpClient client;
#endif
            particle::CellularNcpClientConfig conf = {};
            conf.ncpIdentifier(platform_primary_ncp_identifier());
            client.init(conf);
            client.on();

//             const auto QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE = 115200;
//             uint8_t flags = SERIAL_8N1;
// #if PLATFORM_ID != PLATFORM_B5SOM
//             flags |= SERIAL_FLOW_CONTROL_RTS_CTS;
// #endif
//             hal_usart_begin_config(HAL_PLATFORM_CELLULAR_SERIAL, QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE, flags, nullptr);

            cellDtmUart = (hal_usart_interface_t)(config->index);
            currDtmType = HAL_AT_DTM_TYPE_CELLULAR;
            break;
        }
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
        case HAL_AT_DTM_TYPE_WIFI: {
            LOG(TRACE, "hal_at_dtm_init(): HAL_AT_DTM_TYPE_WIFI");
            network_off(NETWORK_INTERFACE_WIFI_STA, 0, 0, nullptr);
            while (!network_is_off(NETWORK_INTERFACE_WIFI_STA, nullptr)) {
                ;
            }

            LOG(TRACE, "hal_at_dtm_init(): Turn on the modem");
            particle::Esp32NcpClient client;
            auto conf = particle::NcpClientConfig();
            client.init(conf);
            client.on();

            // const auto ESP32_NCP_DEFAULT_SERIAL_BAUDRATE = 921600;
            // hal_usart_begin_config(HAL_PLATFORM_WIFI_SERIAL, ESP32_NCP_DEFAULT_SERIAL_BAUDRATE, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS, nullptr);

            wifiDtmUart = (hal_usart_interface_t)(config->index);
            currDtmType = HAL_AT_DTM_TYPE_WIFI;
            break;
        }
#endif // HAL_PLATFORM_WIFI
        default: return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    switch (config->interface) {
        case HAL_AT_DTM_INTERFACE_UART: {
            LOG(TRACE, "hal_at_dtm_init(): HAL_AT_DTM_INTERFACE_UART %d", config->index);
            const size_t bufferSize = SERIAL_BUFFER_SIZE;
            auto uart = (hal_usart_interface_t)(config->index);
            hal_usart_buffer_config_t conf = {
                .size = sizeof(hal_usart_buffer_config_t),
                .rx_buffer = (uint8_t*)malloc(bufferSize),
                .rx_buffer_size = bufferSize,
                .tx_buffer = (uint8_t*)malloc(bufferSize),
                .tx_buffer_size = bufferSize
            };
            if (!hal_usart_is_enabled(uart)) {
                CHECK(hal_usart_init_ex(uart, &conf, nullptr));
            }
            if (type == HAL_AT_DTM_TYPE_BLE) {
                hal_usart_begin(uart, 19200); // TODO: The baudrate affects the timing in ble_dtm.c. We need to change it in ble_dtm.h
            } else {
                hal_usart_begin(uart, config->params.baudrate);
            }
            break;
        }
        default: return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    return SYSTEM_ERROR_NONE;
}

#endif // HAL_PLATFORM_AT_DTM
