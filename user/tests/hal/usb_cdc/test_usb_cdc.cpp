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
 #include "adc_hal.h"
 #include "gpio_hal.h"
 #include "usb_hal.h"


SYSTEM_MODE(SEMI_AUTOMATIC);

static uint8_t m_rx_buf[128];
static uint8_t m_tx_buf[128];

/* executes once at startup */
void setup() {
    HAL_USB_USART_Config config = {
        .size = 0,
        .rx_buffer = m_rx_buf,
        .rx_buffer_size = 128,
        .tx_buffer = m_tx_buf,
        .tx_buffer_size = 128
    };

    HAL_USB_Init();
    HAL_USB_USART_Init(HAL_USB_USART_SERIAL, &config);
    HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, 115200, 0);
}

/* executes continuously after setup() runs */
void loop() {
    if (HAL_USB_USART_Available_Data(HAL_USB_USART_SERIAL))
    {
        uint8_t data = HAL_USB_USART_Receive_Data(HAL_USB_USART_SERIAL, false);
        HAL_USB_USART_Send_Data(HAL_USB_USART_SERIAL, data);
    }
}
