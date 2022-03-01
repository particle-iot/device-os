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


 #include "application.h"


SYSTEM_MODE(SEMI_AUTOMATIC);

hal_usart_ring_buffer_t uart_tx_buffer;
hal_usart_ring_buffer_t uart_rx_buffer;


/* executes once at startup */
void setup() {
    // uint32_t config = SERIAL_STOP_BITS_1 | SERIAL_PARITY_EVEN | SERIAL_DATA_BITS_8 | SERIAL_FLOW_CONTROL_RTS_CTS;
    uint32_t config = 0;

    hal_usart_init(HAL_USART_SERIAL1, &uart_rx_buffer, &uart_tx_buffer);
    hal_usart_begin_config(HAL_USART_SERIAL1, 115200, config, NULL);
}

/* executes continuously after setup() runs */
void loop() {
    uint8_t data;

    if (hal_usart_available(HAL_USART_SERIAL1))
    {
        data = hal_usart_read(HAL_USART_SERIAL1);
        hal_usart_write(HAL_USART_SERIAL1, data);
    }
}
