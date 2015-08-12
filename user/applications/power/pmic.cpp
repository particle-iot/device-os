/**
 ******************************************************************************
 * @file    pmic.cpp
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    11-August-2015
 * @brief   driver for the power managment IC BQ24195
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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


#include "PMIC.h"

PMIC::PMIC()
{

}


/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
bool PMIC::begin()
{
	HAL_I2C3_Begin(0, 0x00); //0 equates to Master Mode, 0x00 is the default address
	return 1;
}


// Return the version number of the chip
// Value at reset: 00100011, 0x23
byte PMIC::getVersion() {

	byte DATA = 0;
	DATA = readRegister(PMIC_VERSION_REGISTER);
	return DATA;
}

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
byte PMIC::getSystemStatus() {

	byte DATA = 0;
	DATA = readRegister(SYSTEM_STATUS_REGISTER);
	return DATA;
}

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
byte PMIC::getFault() {

	byte DATA = 0;
	DATA = readRegister(FAULT_REGISTER);
	return DATA;

}

/*
//-----------------------------------------------------------------------------
// Input source control register
//-----------------------------------------------------------------------------

REG00
BIT
7 : 0:DISABLE 1:ENABLE
--- input volatge limit. this is used to determine if USB source is overloaded
6 : VINDPM[3] 640mV | offset is 3.88V, Range is 3.88 to 5.08
5 : VINDPM[2] 320mV	| enabling bits 3 to 6 adds the volatges to 3.88 base value
4 : VINDPM[1] 160mV	| Default is 4.36 (0110)
3 : VINDPM[0] 80mV	|
--- input current limit
2 : INLIM[2]  000: 100mA, 001: 150mA, 010: 500mA,	| Default: 100mA when OTG pin is LOW and
1 : INLIM[1]  011: 900mA, 100: 1.2A,   101: 1.5A 	| 500mA when OTG pin is HIGH 
0 : INLIM[0]  110: 2.0A,  111: 3.0A   				| when charging port detected, 1.5A

//-----------------------------------------------------------------------------
*/


/*******************************************************************************
 * Function Name  : setInputCurrentLimit
 * Description    : Sets the input current limit for the PMIC
 * Input          : 100,150,500,900,1200,1500,2000,3000 (mAmp)
 * Return         : 0 Error, 1 Success
 *******************************************************************************/

bool PMIC::setInputCurrentLimit(uint16_t current) {


	byte DATA = readRegister(INPUT_SOURCE_REGISTER);
	byte mask = DATA & 0b11111000; 

	switch (current) {

		case 100:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000000));
		break;

		case 150:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000001));
		break;

		case 500:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000010));
		break;

		case 900:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000011));
		break;

		case 1200:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000100));
		break;

		case 1500:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000101));
		break;

		case 2000:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000110));
		break;

		case 3000:
		writeRegister(INPUT_SOURCE_REGISTER, (mask | 0b00000111));
		break;

		default: 
		return 0; // return error since the value passed didn't match
	}

	return 1; // value was written successfully
}

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
byte PMIC::readRegister(byte startAddress) {

	byte DATA = 0;
	HAL_I2C3_Begin_Transmission(PMIC_ADDRESS);
	HAL_I2C3_Write_Data(startAddress);
	HAL_I2C3_End_Transmission(true);
	
	HAL_I2C3_Request_Data(PMIC_ADDRESS, 1, true);
	DATA = HAL_I2C3_Read_Data();
	return DATA;
}


/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
void PMIC::writeRegister(byte address, byte DATA) {

	HAL_I2C3_Begin_Transmission(PMIC_ADDRESS);
	HAL_I2C3_Write_Data(address);
	HAL_I2C3_Write_Data(DATA);
	HAL_I2C3_End_Transmission(true);
}
