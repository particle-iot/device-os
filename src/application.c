#include "application.h"

int toggle = 0;
void UserLedToggle(void);

void setup()
{
	// runs once

/*
	// Serial Test
	Serial.begin(9600);
*/

	pinMode(D7, OUTPUT);

	//Register "UserLedToggle" function
	Spark.function(UserLedToggle, "UserLedToggle");
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

	//Test code to invoke "UserLedToggle" function (This needs to be called via Spark API)
	Spark_User_Func_Invoke("UserLedToggle");
	delay(500);
}

void UserLedToggle(void)
{
	toggle ^= 1;
	digitalWrite(D7, toggle);
}
