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

 #define SPI_DATA_LENGTH     257
char spi_tx_buf[SPI_DATA_LENGTH];
char spi_rx_buf[SPI_DATA_LENGTH];

void test_spi_data(void)
{
    HAL_SPI_Init(HAL_SPI_INTERFACE1);
    HAL_SPI_Set_Data_Mode(HAL_SPI_INTERFACE1, SPI_MODE3);
    HAL_SPI_Begin(HAL_SPI_INTERFACE1, D2);
    HAL_SPI_Send_Receive_Data(HAL_SPI_INTERFACE1, 0x55);
    HAL_SPI_Send_Receive_Data(HAL_SPI_INTERFACE1, 0xF0);
    HAL_SPI_DMA_Transfer(HAL_SPI_INTERFACE1, spi_tx_buf, spi_rx_buf, SPI_DATA_LENGTH, NULL);
    HAL_SPI_TransferStatus st;
    do {
        HAL_SPI_DMA_Transfer_Status(HAL_SPI_INTERFACE1, &st);
    }  while(st.transfer_ongoing);
    HAL_SPI_End(HAL_SPI_INTERFACE1);
}

/* executes once at startup */
void setup() {
    memset(spi_tx_buf, 0xAA, SPI_DATA_LENGTH);
    memset(spi_rx_buf, 0, SPI_DATA_LENGTH);
}

/* executes continuously after setup() runs */
void loop() {
    test_spi_data();
}
