#include "application.h"
#include "string.h"
#include "handshake.h"

int toggle = 0;
int UserLedToggle(char *ledPin);

int interruptCount = 0;

double testReal = 99.99;

//String stringyone;

void ISR_A0 ()
{
	toggle ^= 1;
	//digitalWrite(A0, toggle);
	digitalWrite(D7, toggle);
}


void setup()
{
	// runs once

/*
	// Serial Test
	Serial.begin(9600);
*/

	attachInterrupt(A1, ISR_A0, CHANGE);
	//attachInterrupt(D1, ISR_A1, CHANGE);
	//attachInterrupt(D2, ISR_A2, CHANGE);
	//attachInterrupt(D3, ISR_A3, CHANGE);
	//attachInterrupt(D4, ISR_A4, CHANGE);
	//attachInterrupt(D5, ISR_A5, RISING);
	//attachInterrupt(A6, ISR_A6, CHANGE);
	//attachInterrupt(A7, ISR_A7, CHANGE);
	
	pinMode(A0, OUTPUT);

	pinMode(D7, OUTPUT);
	digitalWrite(D7, HIGH);
	digitalWrite(A0, HIGH);

	//Register UserLedToggle() function
	//Spark.function("UserLed", UserLedToggle);

	//Register testReal variable
	//Spark.variable("testReal", &testReal, DOUBLE);

	//String.compareto("mohit");


	
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
	// Serial print test
	Serial.print("Hello ");
	Serial.println("Spark");
	delay(500);
*/

	// Call this in the process_command() to schedule the "UserLedToggle" function to execute
	//userFuncSchedule("UserLed", 0xc3, "D7");

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
	 delay(500);
}

int UserLedToggle(char *ledPin)
{
	if(0 == strncmp("D7", ledPin, strlen(ledPin)))
	{
		toggle ^= 1;
		digitalWrite(D7, toggle);
		return 1;
	}
	return 0;
}