/**
 ******************************************************************************
 * @file    spark_wiring_hciserial.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring hci port module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

#if PLATFORM_ID == 88 // Duo

#include "spark_wiring_hciserial.h"
#include "spark_wiring_constants.h"
#include "spark_wiring.h"

HCIUsart::HCIUsart(HAL_HCI_USART_Serial serial, HCI_USART_Ring_Buffer *rx_buffer, HCI_USART_Ring_Buffer *tx_buffer)
{
    _serial = serial;
    HAL_HCI_USART_Init(_serial, rx_buffer, tx_buffer);
}

void HCIUsart::registerReceiveHandler(ReceiveHandler_t handler)
{
    HAL_HCI_USART_registerReceiveHandler(handler);
}

int HCIUsart::downloadFirmware(void)
{
    return HAL_HCI_USART_downloadFirmeare(_serial);
}

void HCIUsart::begin(unsigned long baudrate)
{
    // Reset BT_Controller.
    pinMode(BT_POWER, OUTPUT);
    digitalWrite(BT_POWER, LOW);
    delay(50);
    digitalWrite(BT_POWER, HIGH);

    pinMode(BT_RTS, OUTPUT);
    digitalWrite(BT_RTS, HIGH);
    pinMode(BT_CTS, INPUT_PULLUP);

    HAL_HCI_USART_Begin(_serial, baudrate);

    // Wait for CTS is LOW.
    digitalWrite(BT_RTS, LOW);
    delay(1);
    while(digitalRead(BT_CTS) == HIGH)
    {
        delay(5);
    }
}

void HCIUsart::end(void)
{
    pinMode(BT_POWER, OUTPUT);
    digitalWrite(BT_POWER, LOW);

    HAL_HCI_USART_End(_serial);
}

int HCIUsart::available(void)
{
  return HAL_HCI_USART_Available_Data(_serial);
}

int HCIUsart::read(void)
{
  return HAL_HCI_USART_Read_Data(_serial);
}

size_t HCIUsart::write(uint8_t dat)
{
  return HAL_HCI_USART_Write_Data(_serial, dat);
}

size_t HCIUsart::write(const uint8_t *buf, uint16_t len)
{
    size_t n = 0;
    while(len--)
    {
        unsigned int status = HAL_HCI_USART_Write_Data(_serial, *buf++);
        if(status > 0)
            n++;
        else
            break;
    }

    return n;
}

void HCIUsart::restartTransmit(void)
{
    HAL_HCI_USART_RestartSend(_serial);
}

void HCIUsart::setRTS(uint8_t value)
{
    digitalWrite(BT_RTS, value);
}

uint8_t HCIUsart::checkCTS(void)
{
    return digitalRead(BT_CTS);
}


#if Wiring_Serial6
static HCI_USART_Ring_Buffer serial6_rx_buffer;
static HCI_USART_Ring_Buffer serial6_tx_buffer;

HCIUsart BTUsart(HAL_HCI_USART_SERIAL6, &serial6_rx_buffer, &serial6_tx_buffer);
#endif

#endif

