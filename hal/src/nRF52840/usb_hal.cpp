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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "usb_hal.h"
#include "usb_hal_private.h"
#include "usb_hal_cdc.h"
#include "usb_settings.h"
#include <mutex>
#include <nrf52840.h>
#include <nrf_nvic.h>

#ifdef USB_CDC_ENABLE

void HAL_USB_Init(void) {
    usb_hal_init();
}

void HAL_USB_Attach() {
    usb_hal_attach();
}

void HAL_USB_Detach() {
    usb_hal_detach();
}

void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config) {
    /*
     * Note: This function will call into SD API, which requires the SVC interrupt to be enabled,
     * otherwise, device runs into hardfault. Without the workaround, When LOG_FROM_ISR is defined,
     * device will run into hardfault due to the following code in spark_wiring_logging.cpp:
     *
     * if (stream_ == &Serial && Network.listening()) {
     *     return; // Do not mix logging and serial console output
     * }
     */
    int st = __get_BASEPRI();
    __set_BASEPRI(5 << (8 - __NVIC_PRIO_BITS)); // The SVC interrupt priority is 4

    if ((config == NULL) ||
        (config && (config->rx_buffer == NULL   ||
                    config->rx_buffer_size == 0 ||
                    config->tx_buffer == NULL   ||
                    config->tx_buffer_size == 0)))
    {
		static uint8_t *p_tx_buffer = NULL;
		static uint8_t *p_rx_buffer = NULL;

		if (p_tx_buffer == NULL) {
			p_tx_buffer = (uint8_t *)malloc(USB_TX_BUFFER_SIZE);
		}
		if (p_rx_buffer == NULL) {
			p_rx_buffer = (uint8_t *)malloc(USB_RX_BUFFER_SIZE);
		}
        usb_uart_init(p_rx_buffer, USB_RX_BUFFER_SIZE, p_tx_buffer, USB_TX_BUFFER_SIZE, true);
    } else {
        usb_uart_init(config->rx_buffer, config->rx_buffer_size, config->tx_buffer, config->tx_buffer_size, false);
    }

    __set_BASEPRI(st);
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved) {
    HAL_USB_Attach();
    usb_uart_set_baudrate(baud);
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial) {
    HAL_USB_Detach();
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial) {
    return usb_uart_get_baudrate();
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial) {
    return usb_uart_available_rx_data();
}

int32_t HAL_USB_USART_Available_Data_protected(HAL_USB_USART_Serial serial) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Available_Data(serial);
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial) {
    return usb_uart_available_tx_data();
}

int32_t HAL_USB_USART_Available_Data_For_Write_protected(HAL_USB_USART_Serial serial) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Available_Data_For_Write(serial);
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek) {
    if (usb_uart_available_rx_data() == 0) {
        return -1;
    }

    if (peek) {
        return usb_uart_peek_rx_data(0);
    } else {
        return usb_uart_get_rx_data();
    }
}

int32_t HAL_USB_USART_Receive_Data_protected(HAL_USB_USART_Serial serial, uint8_t peek) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Receive_Data(serial, peek);
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data) {
    return usb_uart_send(&data, 1);
}

int32_t HAL_USB_USART_Send_Data_protected(HAL_USB_USART_Serial serial, uint8_t data) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Send_Data(serial, data);
}

int32_t HAL_USB_USART_Send_Buffer(HAL_USB_USART_Serial serial, const void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return hal_usb_cdc_pvt_send_data((const char*)data, size);
}

int32_t HAL_USB_USART_Receive_Buffer(HAL_USB_USART_Serial serial, void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return hal_usb_cdc_pvt_recv_data((char*)data, size);
}

int32_t HAL_USB_USART_Peek_Buffer(HAL_USB_USART_Serial serial, void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return 0;
    }
    return usb_uart_peek_rx_buffer((uint8_t*)data, size);
}

int HAL_USB_USART_Wait_Event(HAL_USB_USART_Serial serial, uint32_t events, system_tick_t timeout, void* reserved) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    (void)reserved;
    return hal_usb_cdc_pvt_wait_event(events, timeout);
}

int32_t HAL_USB_USART_Send_Buffer_protected(HAL_USB_USART_Serial serial, const void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Send_Buffer(serial, data, size);
}

int32_t HAL_USB_USART_Receive_Buffer_protected(HAL_USB_USART_Serial serial, void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Receive_Buffer(serial, data, size);
}

int32_t HAL_USB_USART_Peek_Buffer_protected(HAL_USB_USART_Serial serial, void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Peek_Buffer(serial, data, size);
}

int HAL_USB_USART_Wait_Event_protected(HAL_USB_USART_Serial serial, uint32_t events, system_tick_t timeout, void* reserved) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Wait_Event(serial, events, timeout, reserved);
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial) {
    usb_uart_flush_tx_data();
}

void HAL_USB_USART_Flush_Data_protected(HAL_USB_USART_Serial serial) {
    if (security_mode_get(NULL) == MODULE_INFO_SECURITY_MODE_PROTECTED) {
        return;
    }
    HAL_USB_USART_Flush_Data(serial);
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial) {
    return usb_hal_is_enabled();
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial) {
    return usb_hal_is_connected();
}

#endif

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    // Old USB API, just for compatibility in main.cpp
    // Enable Serial by default
	HAL_USB_USART_LineCoding_BitRate_Handler(handler, NULL);
}

int32_t HAL_USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate), void* reserved) {
    // Enable Serial by default
    HAL_USB_USART_Init(HAL_USB_USART_SERIAL, nullptr);
    HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, 9600, NULL);
    usb_hal_set_bit_rate_changed_handler(handler);
    return 0;
}

int32_t USB_USART_Flush_Output(unsigned timeout, void* reserved)
{
    return 0;
}

HAL_USB_State HAL_USB_Get_State(void* reserved) {
    return usb_hal_get_state();
}

int HAL_USB_Set_State_Change_Callback(HAL_USB_State_Callback cb, void* context, void* reserved) {
    usb_hal_set_state_change_callback(cb, context, reserved);
    return 0;
}
