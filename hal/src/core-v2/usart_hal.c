/**
 ******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    17-Dec-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include <string.h>
#include "wiced.h"

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
}


wiced_uart_config_t uart_config =
{
    .baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};
#define RX_BUFFER_SIZE 64
wiced_ring_buffer_t rx_buffer;
uint8_t             rx_data[RX_BUFFER_SIZE];


wiced_uart_t serial1 = WICED_UART_1;
void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    uart_config.baud_rate = baud;
    
     /* Initialise ring buffer */
    ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );

    /* Initialise UART. A ring buffer is used to hold received characters */
    wiced_uart_init( STDIO_UART, &uart_config, &rx_buffer );
           
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    wiced_uart_deinit(serial1);
}


uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    return wiced_uart_transmit_bytes(serial1, &data, 1) == 0 ? 1 : 0;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
	return (unsigned int)(RX_BUFFER_SIZE + rx_buffer.head - rx_buffer.tail) % RX_BUFFER_SIZE;
}


int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    uint8_t result = 0;
    if (wiced_uart_receive_bytes(serial1, &result, 1, 0)!=0) {
        result = -1;
    }
    return result;
}


int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    return -1;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
	return true; //usartMap[serial]->usart_enabled;
}
