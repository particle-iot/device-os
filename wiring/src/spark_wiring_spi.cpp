/**
 ******************************************************************************
 * @file    spark_wiring_spi.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring SPI module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>

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

#include "spark_wiring_spi.h"
#include "core_hal.h"
#include "spark_macros.h"
#include "check.h"

namespace
{
/**
 * The divisors. The index+1 is the power of 2 of the divisor.
 */
static const uint8_t clock_divisors[] = {SPI_CLOCK_DIV2,   SPI_CLOCK_DIV4,  SPI_CLOCK_DIV8,
                                         SPI_CLOCK_DIV16,  SPI_CLOCK_DIV32, SPI_CLOCK_DIV64,
                                         SPI_CLOCK_DIV128, SPI_CLOCK_DIV256};

/**
 * \brief Divisor Shift Scale
 *
 * Calculates how far a clock value should be right-shifted
 */
uint8_t divisorShiftScale(uint8_t divider)
{
    unsigned result = 0;
    for (; result < arraySize(clock_divisors); result++)
    {
        if (clock_divisors[result] == divider)
            break;
    }
    return result + 1;
}

/**
 * \brief Query SPI Info
 *
 * This method enables the inspection and caching of the SPI configuration,
 * which allows multiple consumers to issue SPI transactions on the same SPI
 * peripheral.
 *
 * \warning This method is NOT THREADSAFE and callers will need to utilize
 *          HAL synchronization primatives.
 */
static void querySpiInfo(HAL_SPI_Interface spi, hal_spi_info_t* info)
{
    memset(info, 0, sizeof(hal_spi_info_t));
    info->version = HAL_SPI_INFO_VERSION_1;
    HAL_SPI_Info(spi, info, nullptr);
}

/**
 * \brief Extract SPI Settings from SPI Info Structure
 */
static particle::__SPISettings spiSettingsFromSpiInfo(hal_spi_info_t* info)
{
    if (!info || !info->enabled || info->default_settings)
    {
        return particle::__SPISettings();
    }
    return particle::__SPISettings(info->clock, info->bit_order, info->data_mode);
}
} // namespace

SPIClass::SPIClass(HAL_SPI_Interface spi)
{
    _spi = spi;
    HAL_SPI_Init(_spi);
    _dividerReference = SPI_CLK_SYSTEM; // 0 indicates the system clock
}

void SPIClass::begin()
{
    // TODO: Fetch default pin from HAL
    if (!lock())
    {
        HAL_SPI_Begin(_spi, SPI_DEFAULT_SS);
        unlock();
    }
}

void SPIClass::begin(uint16_t ss_pin)
{
    if (!lock())
    {
        HAL_SPI_Begin(_spi, ss_pin);
        unlock();
    }
}

void SPIClass::begin(SPI_Mode mode, uint16_t ss_pin)
{
    if (!lock())
    {
        HAL_SPI_Begin_Ext(_spi, mode, ss_pin, NULL);
        unlock();
    }
}

void SPIClass::end()
{
    if (!lock())
    {
        HAL_SPI_End(_spi);
        unlock();
    }
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
    if (!lock())
    {
        HAL_SPI_Set_Bit_Order(_spi, bitOrder);
        unlock();
    }
}

void SPIClass::setDataMode(uint8_t mode)
{
    if (!lock())
    {
        HAL_SPI_Set_Data_Mode(_spi, mode);
        unlock();
    }
}

int32_t SPIClass::beginTransaction()
{
    return lock();
}

int32_t SPIClass::beginTransaction(const particle::__SPISettings& settings)
{
    // Lock peripheral
    CHECK(lock());

    // Collect existing SPI info
    hal_spi_info_t spi_info;
    querySpiInfo(_spi, &spi_info);

    // Transform SPI info to SPI settings
    particle::__SPISettings spi_settings = spiSettingsFromSpiInfo(&spi_info);

    // Reconfigure SPI peripheral (if necessary)
    if (settings != spi_settings)
    {
        if (settings.default_)
        {
            HAL_SPI_Set_Settings(_spi, settings.default_, 0, 0, 0, nullptr);
        }
        else
        {
            // Compute valid clock value and clock divider from supplied clock value
            uint8_t divisor = 0;
            unsigned int clock; // intentionally left uninitialized
            computeClockDivider((unsigned int)spi_info.system_clock, settings.clock_, divisor,
                                clock);

            // Ensure inequality aside from computed clock value
            if (!(spi_settings <= settings && clock == spi_settings.clock_))
            {
                HAL_SPI_Set_Settings(_spi, settings.default_, divisor, settings.bitOrder_,
                                     settings.dataMode_, nullptr);
            }
        }
    }

    return 0;
}

void SPIClass::endTransaction()
{
    // Release peripheral
    unlock();
}

void SPIClass::setClockDividerReference(unsigned value, unsigned scale)
{
    if (!lock())
    {
        _dividerReference = (value * scale);
        // set the clock to 1/4 of the reference by default.
        // We assume this is called before externally calling setClockDivider.
        setClockDivider(SPI_CLOCK_DIV4);
        unlock();
    }
}

void SPIClass::setClockDivider(uint8_t rate)
{
    if (!lock())
    {
        if (_dividerReference)
        {
            // determine the clock speed
            uint8_t scale = divisorShiftScale(rate);
            unsigned targetSpeed = _dividerReference >> scale;
            setClockSpeed(targetSpeed);
        }
        else
        {
            HAL_SPI_Set_Clock_Divider(_spi, rate);
        }
        unlock();
    }
}

void SPIClass::computeClockDivider(unsigned reference, unsigned targetSpeed, uint8_t& divider,
                                   unsigned& clock)
{
    clock = reference;
    uint8_t scale = 0;
    clock >>= 1; // div2 is the first
    while (clock > targetSpeed && scale < 7)
    {
        clock >>= 1;
        scale++;
    }
    divider = clock_divisors[scale];
}

unsigned SPIClass::setClockSpeed(unsigned value, unsigned value_scale)
{
    unsigned clock = 0;

    // actual speed is the system clock divided by some scalar
    unsigned targetSpeed = value * value_scale;

    if (!lock())
    {
        // Query SPI info
        hal_spi_info_t info;
        querySpiInfo(_spi, &info);

        // Calculate clock divider
        uint8_t divisor = 0;
        computeClockDivider(info.system_clock, targetSpeed, divisor, clock);

        // Update SPI peripheral
        HAL_SPI_Set_Clock_Divider(_spi, divisor);
        unlock();
    }

    return clock;
}

byte SPIClass::transfer(byte _data)
{
    uint16_t result = 0;
    if (!lock())
    {
        result = HAL_SPI_Send_Receive_Data(_spi, _data);
        unlock();
    }
    return static_cast<byte>(result);
}

void SPIClass::transfer(void* tx_buffer, void* rx_buffer, size_t length,
                        wiring_spi_dma_transfercomplete_callback_t user_callback)
{
    if (!lock())
    {
        HAL_SPI_DMA_Transfer(_spi, tx_buffer, rx_buffer, length, user_callback);
        if (user_callback == NULL)
        {
            HAL_SPI_TransferStatus st;
            do
            {
                HAL_SPI_DMA_Transfer_Status(_spi, &st);
            } while (st.transfer_ongoing);
        }
        unlock();
    }
}

void SPIClass::transferCancel()
{
    if (!lock())
    {
        HAL_SPI_DMA_Transfer_Cancel(_spi);
        unlock();
    }
}

int32_t SPIClass::available()
{
    int32_t result = 0;
    if (!lock())
    {
        result = HAL_SPI_DMA_Transfer_Status(_spi, NULL);
        unlock();
    }
    return result;
}

void SPIClass::attachInterrupt()
{
    // TODO: Implement
}

void SPIClass::detachInterrupt()
{
    // TODO: Implement
}

bool SPIClass::isEnabled()
{
    // XXX: pinAvailable() will call this method potentially even from
    // interrupt context. `enabled` flag in HAL is usually just a volatile
    // variable, so it's fine not to acquire the lock here.
    return HAL_SPI_Is_Enabled(_spi);
}

void SPIClass::onSelect(wiring_spi_select_callback_t user_callback)
{
    if (!lock())
    {
        HAL_SPI_Set_Callback_On_Select(_spi, user_callback, NULL);
        unlock();
    }
}
