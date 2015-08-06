


#include "PMIC.h"

PMIC::PMIC()
{

}

boolean PMIC::begin()
{
	Wire.begin();
	return 1;
}


// Return the version number of the chip
// Value at reset: 00100011, 0x23
byte PMIC::getVersion() {

	byte DATA = 0;
	DATA = readRegister(PMIC_VERSION_REGISTER);
	return DATA;
}

byte PMIC::getSystemStatus() {

	byte DATA = 0;
	DATA = readRegister(SYSTEM_STATUS_REGISTER);
	return DATA;
}

byte PMIC::getFault() {

	byte DATA = 0;
	DATA = readRegister(FAULT_REGISTER);
	return DATA;

}


byte PMIC::readRegister(byte startAddress) {

	byte DATA = 0;
	Wire.beginTransmission(PMIC_ADDRESS);
	Wire.write(startAddress);
	Wire.endTransmission();
	
	Wire.requestFrom(PMIC_ADDRESS, 1);
	DATA = Wire.read();
	return DATA;
}

void PMIC::writeRegister(byte address, byte DATA) {

	Wire.beginTransmission(PMIC_ADDRESS);
	Wire.write(address);
	Wire.write(DATA);
	Wire.endTransmission();
}
