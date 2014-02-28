/**
 ******************************************************************************
 * @file     application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version  V1.0.0
 * @date     05-November-2013
 * @brief    Tinker application
 *
 * @extended RGB-LED control & RX/TX pin digitalRead/Write support
 * @author   ScruffR
 * @version  V1.0.1
 * @date     20-March-2014
 ******************************************************************************
 Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include "application.h"

/* Types required for RGB-LED control */
struct ARGB_COLOR {
	byte B;  // blue  channel
	byte G;  // green channel
	byte R;  // red   channel
	byte A;  // alpha channel to indicate RGB.control true (!= 0) / false (== 0)
};

union COLOR {
	ARGB_COLOR argb;
	unsigned int value;
} color;

/* Function prototypes -------------------------------------------------------*/
int extTinkerDigitalRead(String pin);
int extTinkerDigitalWrite(String command);
int extTinkerAnalogRead(String pin);
int extTinkerAnalogWrite(String command);

/* This function is called once at start up ----------------------------------*/

/* This function loops forever --------------------------------------------*/
void setup() {
	//Register all the Tinker functions
	Spark.function("digitalread", extTinkerDigitalRead);
	Spark.function("digitalwrite", extTinkerDigitalWrite);

	Spark.function("analogread", extTinkerAnalogRead);
	Spark.function("analogwrite", extTinkerAnalogWrite);

	//default to normal RGB.control(false) - normally means cyan breathing
	color.value = 0;
}

/* This function loops forever --------------------------------------------*/
void loop() {

}

/*******************************************************************************
 * Function Name  : extTinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin (A0..A7, D0..D7, RX, TX)
 * OUTPUT         : None.
 * Return         : Value of the pin (0 or 1) in INT type
 Returns -1 on fail
 *******************************************************************************/
int extTinkerDigitalRead(String command) {
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';

	//Sanity check
	if (!(                                                  // Error if not
	((command.startsWith("A") || command.startsWith("D")) // A or D pin that are
	&& 0 <= pinNumber && pinNumber <= 7)                    // numbered 0..7
	|| command.startsWith("RX") || command.startsWith("TX") // or RX or TX pin
	))
		return -1;

	// adapt pinNumber if required
	if (command.startsWith("RX") || command.startsWith("TX")) {
		//Serial.end();  // only required if Serial.begin() had been called previously
		pinNumber = (command.charAt(0) == 'R' ? RX : TX);
	} else if (command.charAt(0) == 'A')
		pinNumber += 10;

	// use proper pinNumber for digitalRead
	pinMode(pinNumber, INPUT_PULLDOWN);
	return digitalRead(pinNumber);
}

/*******************************************************************************
 * Function Name  : extTinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value (A0..A7, D0..D7, RX, TX)
 * Output         : None.
 * Return         : 0 for LOW and 1 for HIGH on success and -1 on fail
 *******************************************************************************/
int extTinkerDigitalWrite(String command) {
	bool value = 0;

	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';

	if (!(                                                  // Error if not
	((command.startsWith("A") || command.startsWith("D")) // A or D pin that are
	&& 0 <= pinNumber && pinNumber <= 7)                    // numbered 0..7
	|| command.startsWith("RX") || command.startsWith("TX") // or RX or TX pin
	))
		return -1;

	if (command.substring(3, 7) == "HIGH")
		value = 1;
	else if (command.substring(3, 6) == "LOW")
		value = 0;
	else
		return -1;

	// adapt pinNumber if required
	if (command.charAt(1) == 'X') {
		//Serial.end();  // only required if Serial.begin() had been called previously
		pinNumber = (command.charAt(0) == 'R' ? RX : TX);
	} else if (command.charAt(0) == 'A')
		pinNumber += 10;

	// use proper pinNumber for digitalRead
	pinMode(pinNumber, OUTPUT);
	digitalWrite(pinNumber, value);

	return value;
}

/*******************************************************************************
 * Function Name  : extTinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin (A0..A7)
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
 Returns -1 on fail
 *******************************************************************************/
int extTinkerAnalogRead(String command) {
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';

	if (!(                                                      // Error if not
	((command.startsWith("A")) // || command.startsWith("D"))   // A or D pin that are
	&& 0 <= pinNumber && pinNumber <= 7)                        // numbered 0..7
	// || command.startsWith("RX")  || command.startsWith("TX") // or RX or TX pin
	))
		return -1;

	if (command.startsWith("A"))
		pinNumber += 10;

	// use proper pinNumber for digitalRead
	pinMode(pinNumber, INPUT);
	return analogRead(pinNumber);
}

/*******************************************************************************
 * Function Name  : extTinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255 for I/O pins and ARGB value for LED)
 *                  A0,A1,A4,A5,A6,A7,D0,D1 -> 0..255
 *                  CL (=ColorLed) -> 0xAARRGGBB -> AA == 00 -> RGB.control(false)
 *                                                     != 00 -> RGB.control(true)
 * Output         : None.
 * Return         : received value success and -1/-2/-3 depending on fail reason
 *******************************************************************************/
int extTinkerAnalogWrite(String command) {
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	String sValue = command.substring(3);
	int value = sValue.toInt();  // sValue has to be less or equal 0x7FFFFFFF
								 // any sValue greater will result in 0x7FFFFFFF
	switch (command.charAt(0)) {
	case 'C':
		color.value = value;
		RGB.control(color.argb.A);
		RGB.color(color.argb.R, color.argb.G, color.argb.B);
		break;
	case 'A':
		switch (pinNumber) {
		case 7:
		case 6:
		case 5:
		case 4:
		//case 3: // not capable of analogWrite (PWM)
		//case 2: // not capable of analogWrite (PWM)
		case 1:
		case 0:
			pinNumber += 10;
			break;
		default:
			// invalid A pin
			return -3;
		}
		break;
	case 'D':
		switch (pinNumber) {
		case 1: // D pin capable of analogWrite (PWM)
		case 0: // D pin capable of analogWrite (PWM)
			// nothing to do - just get on with it
			break;
		default:
			// invalid D pin
			return -2;
		}
		break;
	default:
		// neither A nor D port
		return -1;
	}

	// still here, so everything must be fine
	pinMode(pinNumber, OUTPUT);
	analogWrite(pinNumber, value);

	return value;
}

