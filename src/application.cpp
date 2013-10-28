#include "application.h"
#include "string.h"
#include "handshake.h"

int toggle = 0;
int UserLedToggle(char *ledPin);

int interruptCount = 0;
int bytesent;

double testReal = 99.99;

uint8_t incomingByte;

/*
void ISR_A0 ()
{
  toggle ^= 1;
  //digitalWrite(A0, toggle);
  digitalWrite(D7, toggle);
}
*/

void setup()
{
	// runs once

/*
	//Wiring - SPI test using an External Flash IC
	SPI.begin();
	pinMode(D7, OUTPUT);
	uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
	//Select the FLASH: Chip Select low
	digitalWrite(SS, LOW);
	//Send "JEDEC ID Read" instruction
	SPI.transfer(0x9F);
	//Read a byte from the FLASH
	Temp0 = SPI.transfer(0xFF);
	//Read a byte from the FLASH
	Temp1 = SPI.transfer(0xFF);
	//Read a byte from the FLASH
	Temp2 = SPI.transfer(0xFF);
	//Deselect the FLASH: Chip Select high
	digitalWrite(SS, HIGH);
	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
	if(Temp == sFLASH_SST25VF016_ID)
		digitalWrite(D7, HIGH);
	else
		digitalWrite(D7, LOW);
*/

/*
	//Wiring - I2C test using a Pressure Sensor IC
	Wire.begin();
*/


	// Serial Test
	Serial.begin(9600);

	Serial1.begin(9600);
/*
	attachInterrupt(A1, ISR_A0, CHANGE);
	//attachInterrupt(D1, ISR_A1, CHANGE);
	//attachInterrupt(D2, ISR_A2, CHANGE);
	//attachInterrupt(D3, ISR_A3, CHANGE);
	//attachInterrupt(D4, ISR_A4, CHANGE);
	//attachInterrupt(D5, ISR_A5, RISING);
	//attachInterrupt(A6, ISR_A6, CHANGE);
	//attachInterrupt(A7, ISR_A7, CHANGE);

	pinMode(A0, OUTPUT);
*/
	pinMode(D7, OUTPUT);
	//digitalWrite(D7, HIGH);
	//digitalWrite(A0, HIGH);

	//Register UserLedToggle() function
	Spark.function("brew", UserLedToggle);

	//Register testReal variable
	//Spark.variable("testReal", &testReal, DOUBLE);

}

void loop()
{
	// runs repeatedly

	/*
	// Serial loopback test: what is typed on serial console
	// using Hyperterminal/Putty should echo back on the console
	if(Serial.available())
	{
		Serial.write(Serial.read());
	}
	*/

/*
	Wire.requestFrom(80, 2);    // request 2 bytes from Pressure Sensor

	while(Wire.available())    // slave may send less than requested
	{
		int c = Wire.read();    // receive a byte as character
	}
*/

	// Serial print test
	//Serial.print("Hello ");
	//Serial.println("Spark");
	//delay(500);

	
	if(Serial1.available() > 0)
	{
		incomingByte = Serial1.read();
		Serial.write(incomingByte);
		bytesent = Serial1.print(incomingByte);

		if (incomingByte == 'a')
		{
			userFuncSchedule("UserLed", "D7");
		}
	
	}

	//USART_SendData(USART2, incomingByte);
	

	// Call this in the process_command() to schedule the "UserLedToggle" function to execute
	//userFuncSchedule("UserLed", "D7");

	// Call this in the process_command() to schedule the return of "testReal" value
	//userVarSchedule("testReal", 0xa1);

	// if (interruptCount == 0)
	// {
	// 	noInterrupts();
	// }
	// delay(5000);
	// interruptCount = 1;
	// if (interruptCount == 1)
	// {
	// 	noInterrupts();
	// }
	// interruptCount = 2;
	// interrupts();

	delay(50);
}

int UserLedToggle(char *ledPin)
{
	if(0 == strncmp("D0", ledPin, strlen(ledPin)))
	{
		toggle ^= 1;
		digitalWrite(D0, toggle);
		return 1;
	}
	return 0;
}
