/*
Spark Core HTU21D Temperature / Humidity Sensor Library
By: Romain MP
Licence: GPL v3
*/

#include "HTU21D.h"

HTU21D::HTU21D(){
}

bool HTU21D::begin(void)
{
	// Only join the I2C bus as master if needed
	if(! Wire.isEnabled()) {
		Wire.begin();
	}

	// Reset the sensor
	reset();

	// Read User register after reset to confirm sensor is OK
	return(read_user_register() == 0x2); // 0x2 is the default value of the user register
}

float HTU21D::readHumidity(){
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(TRIGGER_HUMD_MEASURE_NOHOLD);
	Wire.endTransmission();

	// Wait for the sensor to measure
	delay(55); // matches arduino library delay

	if (Wire.requestFrom(HTDU21D_ADDRESS, 3) != 3)
		return HTU21D_I2C_TIMEOUT; // if all data not received

	//Wait for data to become available
	// int counter = 0;
	// while(!Wire.available()){
	// 	counter++;
	// 	delay(1);
	// 	if(counter > 100) return HTU21D_I2C_TIMEOUT; //after 100ms consider I2C timeout
	// }

	uint16_t h = Wire.read();
	h <<= 8;
	h |= Wire.read();

	// CRC check
	uint8_t crc = Wire.read();
	if(checkCRC(h, crc) != 0) return(HTU21D_BAD_CRC);

	h &= 0xFFFC; // zero the status bits
	float hum = h;
	hum *= 125;
	hum /= 65536;
	hum -= 6;

	return hum;
}

float HTU21D::readTemperature(){
//	uint8_t rv = 0;

//	Serial.println("Slave Address...");
	Wire.beginTransmission(HTDU21D_ADDRESS);
//	Serial.print("Write: ");
	// rv =
	Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
//	Serial.println(rv);
//	Serial.print("End Transmission: ");
	// rv =
	Wire.endTransmission(true);
//	Serial.println(rv);

	// Wait for the sensor to measure
	delay(55); // 50ms measure time for 14bit measures

//	Serial.print("Request From: ");
	// rv =
	if (Wire.requestFrom(HTDU21D_ADDRESS, 3) != 3)
		return HTU21D_I2C_TIMEOUT; // if all data not received
//	Serial.println(rv);

	//Wait for data to become available
// 	int counter = 0;
// 	while(!Wire.available()){
// 		counter++;
// 		delay(1);
// 		if(counter > 100) {
// //			Serial.println("HTU21D_I2C_TIMEOUT!");
// 			return HTU21D_I2C_TIMEOUT; //after 100ms consider I2C timeout
// 		}
// 	}

	uint16_t t = Wire.read();
	t <<= 8;
	t |= Wire.read();

	// CRC check
	uint8_t crc = Wire.read();
	if( checkCRC(t, crc) != 0) return(HTU21D_BAD_CRC);

	t &= 0xFFFC; // zero the status bits
	float temp = t;
	temp *= 175.72;
	temp /= 65536;
	temp -= 46.85;

	return temp;
}

void HTU21D::setResolution(byte resolution)
{
  byte userRegister = read_user_register(); //Go get the current register state
  userRegister &= 0b01111110; //Turn off the resolution bits
  resolution &= 0b10000001; //Turn off all other bits but resolution bits
  userRegister |= resolution; //Mask in the requested resolution bits

  //Request a write to user register
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(WRITE_USER_REG); //Write to the user register
  Wire.write(userRegister); //Write the new resolution bits
  Wire.endTransmission();
}

void HTU21D::reset(){
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(SOFT_RESET);
	Wire.endTransmission();
	delay(20);
}

byte HTU21D::read_user_register()
{
	//Request the user register
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(READ_USER_REG);
	Wire.endTransmission();

	//Read result
	if (Wire.requestFrom(HTDU21D_ADDRESS, 1) == 1)
		return (Wire.read());
	else
		return 0xFF;
}

byte HTU21D::checkCRC(uint16_t message, uint8_t crc){
	uint32_t reste = (uint32_t)message << 8;
	reste |= crc;

	uint32_t diviseur = (uint32_t)CRC_POLY;

	for(int i = 0; i<16; i++){
		if (reste & (uint32_t)1<<(23 - i) )
			reste ^= diviseur;

		diviseur >>= 1;
	}

	return (byte)reste;
}


