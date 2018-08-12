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
#include "usb_hal.h"
#include "usb_hal_cdc.h"

#define USB_RX_BUFFER_SIZE      256
#define USB_TX_BUFFER_SIZE      256

#ifdef USB_CDC_ENABLE

/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate) {
	static uint8_t inited = 0;

	if (!inited) {
		inited = 1;
        // For compatibility we allocate buffers, as application calling USB_USART_Init
        // assumes that the driver has its own buffers
        HAL_USB_USART_Config conf;
        memset(&conf, 0, sizeof(conf));
        HAL_USB_USART_Init(HAL_USB_USART_SERIAL, &conf);
    }

    if (HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL) != baudRate) {
        if (!baudRate && HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL) > 0) {
            HAL_USB_USART_End(HAL_USB_USART_SERIAL);
        } else if (!HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL)) {
           HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, baudRate, NULL);
        }
    }
}

unsigned int USB_USART_Baud_Rate(void) {
    return HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL);
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void) {
    int32_t available = HAL_USB_USART_Available_Data(HAL_USB_USART_SERIAL);
    if (available > 255) {
        return 255;
    } else if (available < 0) {
        return 0;
    }
    return available;
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(uint8_t peek) {
    return HAL_USB_USART_Receive_Data(HAL_USB_USART_SERIAL, peek);
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data_For_Write.
 * Description    : Return the length of available space in TX buffer
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data_For_Write(void) {
    return HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_SERIAL);
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data) {
    HAL_USB_USART_Send_Data(HAL_USB_USART_SERIAL, Data);
}

/*******************************************************************************
 * Function Name  : USB_USART_Flush_Data.
 * Description    : Flushes TX buffer
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Flush_Data(void) {
    HAL_USB_USART_Flush_Data(HAL_USB_USART_SERIAL);
}

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
        usb_uart_init(p_rx_buffer, USB_RX_BUFFER_SIZE, p_tx_buffer, USB_TX_BUFFER_SIZE);
    } else {
        usb_uart_init(config->rx_buffer, config->rx_buffer_size, config->tx_buffer, config->tx_buffer_size);
    }
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved) {
    usb_uart_set_baudrate(baud);
    HAL_USB_Attach();
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

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial) {
    return usb_uart_available_tx_data();
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek) {
    if (peek) {
        return usb_uart_peek_rx_data(0);
    } else {
        return usb_uart_get_rx_data();
    }
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data) {
    return usb_uart_send(&data, 1);
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial) {
    usb_uart_flush_tx_data();
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial) {
    return usb_hal_is_enabled();
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial) {
    return usb_hal_is_connected();
}

#endif

#ifdef USB_HID_ENABLE
/*******************************************************************************
 * Function Name : USB_HID_Send_Report.
 * Description   : Send HID Report Info to Host.
 * Input         : pHIDReport and reportSize.
 * Output        : None.
 * Return value  : None.
 *******************************************************************************/
void USB_HID_Send_Report(void *pHIDReport, uint16_t reportSize) {
    return;
}
#endif

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate)) {
    return;
}

int32_t USB_USART_Flush_Output(unsigned timeout, void* reserved) {
    return 0;
}
