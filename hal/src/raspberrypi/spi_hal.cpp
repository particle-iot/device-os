/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Julien Vanier
 * @version V1.0.0
 * @date    28-Sept-2016
 * @brief
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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
#include "spi_hal.h"
#include <wiringPiSPI.h>
#include "service_debug.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// SPI must be enabled by adding dtparam=spi=on in /boot/config.txt
// See https://www.raspberrypi.org/documentation/hardware/raspberrypi/spi/README.md

#define NUM_SPI_CHANNELS 2

#define BOGUS_SYSTEM_CLOCK 64000000
#define DEFAULT_SPEED 4000000

int spiFd[NUM_SPI_CHANNELS] = { -1, -1 };
uint32_t spiSpeed[NUM_SPI_CHANNELS] = { DEFAULT_SPEED, DEFAULT_SPEED };
uint32_t lastLength = 0;

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    // Nothing to do
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    if (HAL_SPI_Is_Enabled(spi)) {
        HAL_SPI_End(spi);
    }

    spiFd[spi] = wiringPiSPISetup(spi, spiSpeed[spi]);
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved)
{
    // Raspberry Pi can only do SPI_MODE_MASTER
    HAL_SPI_Begin(spi, pin);
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    if (HAL_SPI_Is_Enabled(spi)) {
        close(spiFd[spi]);
        spiFd[spi] = -1;
    }
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
    // Not supported
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
    // Not supported
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    uint32_t divider = 2 << rate;
    DEBUG("SPI rate %d, divider %d", rate, divider);
    spiSpeed[spi] = BOGUS_SYSTEM_CLOCK / divider;

    // Reopen when changing speed
    if (HAL_SPI_Is_Enabled(spi)) {
        HAL_SPI_Begin(spi, 0);
    }
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    if (HAL_SPI_Is_Enabled(spi)) {
        uint8_t dataByte = data;
        wiringPiSPIDataRW(spi, &dataByte, 1);
        return dataByte;
    }

    return 0;
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return spiFd[spi] >= 0;
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
    if (HAL_SPI_Is_Enabled(spi)) {
        bool blank_tx = false;
        if (tx_buffer == NULL) {
            blank_tx = true;
            tx_buffer = malloc(length);
            memset(tx_buffer, 0xFF, length);
        }
        bool blank_rx = false;
        if (rx_buffer == NULL) {
            blank_rx = true;
            rx_buffer = malloc(length);
        }

        memcpy(tx_buffer, rx_buffer, length);
        wiringPiSPIDataRW(spi, (uint8_t *)rx_buffer, length);
        lastLength = length;

        if (blank_tx) {
            free(tx_buffer);
        }
        if (blank_rx) {
            free(tx_buffer);
        }

        if (userCallback) {
            userCallback();
        }
    }
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved)
{
    info->system_clock = BOGUS_SYSTEM_CLOCK;
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved)
{
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi)
{
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st)
{
    return lastLength;
}
