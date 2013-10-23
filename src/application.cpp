#include "application.h"
#include "string.h"
#include "handshake.h"

int toggle = 0;
int UserLedToggle(char *ledPin);

/*
double testReal = 99.99;
*/

/*
unsigned char ciphertext[256];
int err, encrypt = 1;

unsigned char nonce[40] =
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1
};
unsigned char id[12];
unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
*/

void setup()
{
	// runs once

/*
	memset(ciphertext, 0, 256);
	memcpy(id, (const void *)ID1, 12);
	FLASH_Read_ServerPublicKey(pubkey);

*/
	pinMode(D0, OUTPUT);
	digitalWrite(D0, LOW);
	delay(500);
	digitalWrite(D0, HIGH);
	delay(500);
	digitalWrite(D0, LOW);

/*
	// Serial Test
	Serial.begin(9600);
*/

	//Register UserLedToggle() function
	Spark.function("brew", UserLedToggle);

/*
	//Register testReal variable
	Spark.variable("testReal", &testReal, DOUBLE);
*/
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
	Serial.print("err: ");
	Serial.println(0 == err ? "zero" : "non-zero");
	Serial.print("ciphertext: ");
	Serial.println((char *)ciphertext);
	delay(2000);
*/

/*
	if(encrypt)
	{
		err = ciphertext_from_nonce_and_id(nonce, id, pubkey, ciphertext);
		if(err == 0)
		{
			//Success
			digitalWrite(D7, HIGH);
		}
		encrypt = 0;
	}
*/

/*
	// Call this in the process_command() to schedule the "UserLedToggle" function to execute
	userFuncSchedule("UserLed", 0xc3, "D7");

	// Call this in the process_command() to schedule the return of "testReal" value
	userVarSchedule("testReal", 0xa1);

	delay(1000);
*/
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
