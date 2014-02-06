/**
 ******************************************************************************
 * @file    spark_wiring_spi.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring SPI module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
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

SPIClass SPI;

SPI_InitTypeDef SPIClass::SPI_InitStructure;
bool SPIClass::SPI_Bit_Order_Set = false;
bool SPIClass::SPI_Data_Mode_Set = false;
bool SPIClass::SPI_Clock_Divider_Set = false;
bool SPIClass::SPI_Enabled = false;

void SPIClass::begin() {
	begin(SS);
}

void SPIClass::begin(uint16_t ss_pin) {
	if (ss_pin >= TOTAL_PINS )
	{
		return;
	}

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	pinMode(SCK, AF_OUTPUT_PUSHPULL);
	pinMode(MOSI, AF_OUTPUT_PUSHPULL);
	pinMode(MISO, INPUT);

	pinMode(ss_pin, OUTPUT);
	digitalWrite(ss_pin, HIGH);

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	if(SPI_Data_Mode_Set != true)
	{
		//Default: SPI_MODE3
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	}
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	if(SPI_Clock_Divider_Set != true)
	{
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	}
	if(SPI_Bit_Order_Set != true)
	{
		//Default: MSBFIRST
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	}
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);

	SPI_Enabled = true;
}

void SPIClass::end() {
	if(SPI_Enabled != false)
	{
		SPI_Cmd(SPI1, DISABLE);

		SPI_Enabled = false;
	}
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
	if(bitOrder == LSBFIRST)
	{
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
	}
	else
	{
		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	}

	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Bit_Order_Set = true;
}

void SPIClass::setDataMode(uint8_t mode)
{
	if(SPI_Enabled != false)
	{
		SPI_Cmd(SPI1, DISABLE);
	}

	switch(mode)
	{
	case SPI_MODE0:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
		break;

	case SPI_MODE1:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
		break;

	case SPI_MODE2:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
		break;

	case SPI_MODE3:
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
		break;
	}

	SPI_Init(SPI1, &SPI_InitStructure);

	if(SPI_Enabled != false)
	{
		SPI_Cmd(SPI1, ENABLE);
	}

	SPI_Data_Mode_Set = true;
}

void SPIClass::setClockDivider(uint8_t rate)
{
	SPI_InitStructure.SPI_BaudRatePrescaler = rate;

	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Clock_Divider_Set = true;
}

byte SPIClass::transfer(byte _data) {
	/* Wait for SPI1 Tx buffer empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	/* Send SPI1 data */
	SPI_I2S_SendData(SPI1, _data);

	/* Wait for SPI1 data reception */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	/* Read and return SPI1 received data */
	return SPI_I2S_ReceiveData(SPI1);
}

void SPIClass::attachInterrupt() {
	//To Do
}

void SPIClass::detachInterrupt() {
	//To Do
}

bool SPIClass::isEnabled() {
	return SPI_Enabled;
}

/*******************************************************************************
* Function Name  : Wiring_SPI1_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles SPI1 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Wiring_SPI1_Interrupt_Handler(void)
{
	//To Do
}
