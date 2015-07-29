#include "application.h"

#include "Serial3/Serial3.h"

SYSTEM_MODE(MANUAL);

#define now() millis()
uint32_t lastFlash = now();
#define PASS_GREEN() RGB.color(0,255,0)
#define FAIL_RED() RGB.color(255,0,0)
#define FAIL_BLUE() RGB.color(0,0,255)
#define RGB_OFF() RGB.color(0,0,0)

int8_t ensureUbloxIsReset();
void   clearUbloxBuffer();
int8_t sendATcommand(const char* ATcommand, const char* expected_answer1, system_tick_t timeout);
int8_t ensureATReturnsOK();
void   sendSMS(const char * msg);

void setup()
{
    RGB.control(true);
    // TEST RGB LED
    // FAIL_RED();
    // delay(500);
    // PASS_GREEN();
    // delay(500);
    // FAIL_BLUE();
    // delay(500);
    // RGB_OFF();
    // delay(500);

	pinMode(D7, OUTPUT);
	pinMode(PWR_UC, OUTPUT);
	pinMode(RESET_UC, OUTPUT);
	digitalWrite(PWR_UC, HIGH);
	digitalWrite(RESET_UC, HIGH);
    pinMode(RTS_UC, OUTPUT);
    digitalWrite(RTS_UC, LOW); // VERY IMPORTANT FOR CORRECT OPERATION!!

	Serial.begin(9600);
	Serial3.begin(9600);
	Serial.println("Hi, I'm Serial USB!");

    // TEST RGB LED
    FAIL_RED();
    delay(500);
    PASS_GREEN();
    delay(500);
    FAIL_BLUE();
    delay(500);
    RGB_OFF();
    delay(500);

    // TEST UBLOX
    if ( ensureUbloxIsReset() )
    {
        ( ensureATReturnsOK() == 1 ) ? PASS_GREEN() : FAIL_RED();
    }
    else {
        FAIL_BLUE();
    }
}

void loop()
{
    // TEST USER LED
	if ( now() - lastFlash > 100UL ) {
		lastFlash = now();
		digitalWrite(D7, !digitalRead(D7));
	}

	if ( Serial.available() ) {
		char c = Serial.read();

		if (c == 'a') {
            sendATcommand("AT", "OK", 500);
        }
		else if (c == 'p') {
			digitalWrite(PWR_UC, !digitalRead(PWR_UC));
			Serial.print("Power is: ");
			Serial.println(digitalRead(PWR_UC));
		}
		else if (c == 'r') {
			digitalWrite(RESET_UC, !digitalRead(RESET_UC));
			Serial.print("Reset is: ");
			Serial.println(digitalRead(RESET_UC));
		}
        else if (c == 'i') {
            Serial.println("What's the IMEI number?");
            sendATcommand("AT+CGSN", "OK", 500);
        }
        else if (c == 'o') {
            // Power to the 3V8 input measures 34uA after this is called
            Serial.println("Power Off...");
            sendATcommand("AT+CPWROFF", "OK", 500);
        }
        else if (c == 'f') {
            Serial.println("What's the FUN level?");
            sendATcommand("AT+CFUN?", "OK", 500);
        }
        else if (c == 'd') {
            // Power to the 3V8 input measures 2.8mA constantly after this is called
            Serial.println("Minimum Fun Level...");
            sendATcommand("AT+CFUN=0,0", "OK", 500);
        }
        else if (c == 'D') {
            // Power to the 3V8 input measures 2.8mA for >10s, then 56mA for 4s, repeating
            // after this is called.
            Serial.println("Standard Fun Level...");
            sendATcommand("AT+CFUN=1,0", "OK", 500);
        }
        else if (c == 's') {
            // Power to the 3V8 input measures 2.8mA for >10s, then 56mA for 4s, repeating
            // after this is called.
            Serial.println("STM32 Deep Sleep for 10 seconds...");
            System.sleep(SLEEP_MODE_DEEP, 10);
            // Does not currently wake up after 10 seconds, press reset or HIGH on WKP.
        }
        else if (c == 'm') {
            // Send a test SMS
            sendSMS("Hello from Particle!");
        }
        else if (c == '1') {
            // Check the CPIN
            Serial.println("Check the CPIN...");
            sendATcommand("AT+CPIN?", "OK", 500);
        }
        else if (c == '2') {
            // Check is SIM is 3G
            Serial.println("Is SIM 3G?");
            sendATcommand("AT+UUICC?", "OK", 500);
        }
	}

	// if (Serial.available()) {
	// 	char c = Serial.read();
	// 	Serial3.write(c);
	// }

	if (Serial3.available()) {
		char c = Serial3.read();
		Serial.write(c);
	}
}

void clearUbloxBuffer() {
  while (Serial3.available()) {
    Serial3.read();
  }
}

int8_t ensureUbloxIsReset() {
    clearUbloxBuffer();

    Serial3.println("AT");
    delay(100);

    if (Serial3.available())
    {
        // Ublox is on, just clear the buffer
        Serial.println("Ublox is on already.");
        clearUbloxBuffer();
        return 1;
    }
    else
    {
        // Ublox is off, so power it on
        Serial.println("Ublox is off, turning it on.");
        digitalWrite(PWR_UC,LOW);
        delay(500);
        digitalWrite(PWR_UC,HIGH);
        delay(2000);

        Serial3.println("AT");
        delay(100);

        if (Serial3.available())
        {
            // Ublox is on, just clear the buffer
            Serial.println("Ublox was off, now it is on.");
            clearUbloxBuffer();
            return 1;
        }
        else
        {
            // Ublox is off, so power it on
            Serial.println("Ublox remains off, error!");
            return 0;
        }
    }
}

int8_t sendATcommand(const char* ATcommand, const char* expected_answer1, system_tick_t timeout) {
    uint8_t x = 0, rv = 0;
    char response[100];
    system_tick_t previous;

    memset(response, '\0', 100);
    delay(100);

    clearUbloxBuffer();

    Serial3.println(ATcommand);

    previous = now();
    do {
        if (Serial3.available() != 0) {
            response[x] = Serial3.read();
            x++;

            // check whether we've received the expected answer
            if (strstr(response, expected_answer1) != NULL) {
                rv = 1;
            }
        }
    } while ((rv == 0) && ((now() - previous) < timeout));

  Serial.println(response);
  return rv;
}

int8_t ensureATReturnsOK() {
    int8_t rv = sendATcommand("AT", "OK", 500);
    return rv;
}

void sendSMS(const char * msg)
{
    Serial.print("Sending SMS: ");
    Serial.println(msg);

    Serial3.print("AT+CMGF=1\r");
    delay(500);
    Serial3.print("AT+CMGS=\"+16305555555\"\r");
    delay(1500);
    Serial3.print(msg);
    Serial3.print((char)26); // End AT command with a ^Z, ASCII code 26
    Serial3.print("\r");
}
