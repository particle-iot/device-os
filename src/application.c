#include "application.h"
#include "string.h"

int toggle = 0;
int UserLedToggle(char *ledPin);

void setup()
{
	// runs once

/*
	// Serial Test
	Serial.begin(9600);
*/

	pinMode(D7, OUTPUT);

	//Register UserLedToggle() function
	Spark.function("UserLed", UserLedToggle);
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
	userFuncSchedule("UserLed", 0xc3, "D7");

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
