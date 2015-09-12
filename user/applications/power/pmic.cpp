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
    Wire3.begin();
	return 1;
}


// Return the version number of the chip
// Value at reset: 00100011, 0x23
byte PMIC::getVersion() {

	byte DATA = 0;
	DATA = readRegister(PMIC_VERSION_REGISTER);
	return DATA;
}

/*
//-----------------------------------------------------------------------------
//System Status Register
//-----------------------------------------------------------------------------
//NOTE: This is a read-only register

REG08
BIT
--- VBUS status
7: VBUS_STAT[1]	| 00: Unknown (no input, or DPDM detection incomplete), 01: USB host
6: VBUS_STAT[0]	| 10: Adapter port, 11: OTG
--- Charging status
5: CHRG_STAT[1] | 00: Not Charging,  01: Pre-charge (<VBATLOWV)
4: CHRG_STAT[0] | 10: Fast Charging, 11: Charge termination done
3: DPM_STAT		0: Not DPM
				1: VINDPM or IINDPM
2: PG_STAT		0: Power NO Good :( 
				1: Power Good :)
1: THERM_STAT	0: Normal
				1: In Thermal Regulation (HOT)
0: VSYS_STAT	0: Not in VSYSMIN regulation (BAT > VSYSMIN)
				1: In VSYSMIN regulation (BAT < VSYSMIN)
*/

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
bool PMIC::isPowerGood(void) {

	byte DATA = 0;
	DATA = readRegister(SYSTEM_STATUS_REGISTER);
	if(DATA & 0b00000100) return 1;
	else return 0;
}

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
bool PMIC::isHot(void) {

	byte DATA = 0;
	DATA = readRegister(SYSTEM_STATUS_REGISTER);
	if(DATA & 0b00000010) return 1;
	else return 0;
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
7 : 0:Enable Buck regulator 1:disable buck regulator (powered only from a LiPo)
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
 * Function Name  : readInputSourceRegister
 * Description    : 
 * Input          : NONE
 * Return         :
 *******************************************************************************/

byte PMIC::readInputSourceRegister(void) {


	return readRegister(INPUT_SOURCE_REGISTER);
}

/*******************************************************************************
 * Function Name  : enableBuck
 * Description    : 
 * Input          : NONE
 * Return         :
 *******************************************************************************/

bool PMIC::enableBuck(void) {

	byte DATA = readRegister(INPUT_SOURCE_REGISTER);
	writeRegister(INPUT_SOURCE_REGISTER, (DATA & 0b01111111));
	return 1;
}

/*******************************************************************************
 * Function Name  : disableBuck
 * Description    : 
 * Input          : NONE
 * Return         :
 *******************************************************************************/

bool PMIC::disableBuck(void) {

	byte DATA = readRegister(INPUT_SOURCE_REGISTER);
	writeRegister(INPUT_SOURCE_REGISTER, (DATA | 0b10000000));
	return 1;
}

/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
byte PMIC::readRegister(byte startAddress) {

	byte DATA = 0;
	Wire3.beginTransmission(PMIC_ADDRESS);
	Wire3.write(startAddress);
	Wire3.endTransmission(true);
	
	Wire3.requestFrom(PMIC_ADDRESS, 1, true);
	DATA = Wire3.read();
	return DATA;
}


/*******************************************************************************
 * Function Name  : 
 * Description    : 
 * Input          : 
 * Return         : 
 *******************************************************************************/
void PMIC::writeRegister(byte address, byte DATA) {

    Wire3.beginTransmission(PMIC_ADDRESS);
    Wire3.write(address);
    Wire3.write(DATA);
    Wire3.endTransmission(true);
}
