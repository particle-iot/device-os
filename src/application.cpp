#include "application.h"
#include "string.h"
#include "handshake.h"

__IO uint32_t LED_Signaling_Timing;
uint32_t VIBGYOR_Colors[] = {0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000};
uint32_t i, n = sizeof(VIBGYOR_Colors) / sizeof(uint32_t);

/*
int toggle = 0;
int UserLedToggle(char *ledPin);

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

void MyISR(void)
{
	
}

void setup()
{
	// runs once

	String test = "hello";
	String meh = "123";

	test.toLowerCase();

	attachInterrupt(A0,MyISR,CHANGE);


/*
	memset(ciphertext, 0, 256);
	memcpy(id, (void *)0x01fff7e8, 12);
	FLASH_Read_ServerPublicKey(pubkey);

	pinMode(D7, OUTPUT);
	digitalWrite(D7, LOW);
	delay(500);
	digitalWrite(D7, HIGH);
	delay(500);
	digitalWrite(D7, LOW);
*/

/*
	// Serial Test
	Serial.begin(9600);
*/

/*
	//Register UserLedToggle() function
	Spark.function("UserLed", UserLedToggle);

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

/*
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
*/

/* This should be a "NON-BlOCKING" function
 * It will be executed every 1ms if LED_Signaling_Start() is called
 * and stopped as soon as LED_Signaling_Stop() is called */
void LED_Signaling_Override(void)
{
    if (LED_Signaling_Timing != 0)
    {
    	LED_Signaling_Timing--;
    }
    else
	{
		LED_SetSignalingColor(VIBGYOR_Colors[i]);
		LED_On(LED_RGB);

		LED_Signaling_Timing = 100;	//100ms

		++i;
		if(i >= n)
		{
			i = 0;
		}
	}
}

char handleMessage(char *user_arg)
{
	if(0 == strncmp(user_arg, "led_signaling_start", strlen("led_signaling_start")))
	{
		LED_Signaling_Start();
	}
	else if(0 == strncmp(user_arg, "led_signaling_stop", strlen("led_signaling_stop")))
	{
		LED_Signaling_Stop();
	}

	return 0;
}
