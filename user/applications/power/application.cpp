#include "application.h"


#include "Serial3/Serial3.h"
#include "pmic.h"
#include "fuelgauge.h"

SYSTEM_MODE(MANUAL);

PMIC PMIC;
FuelGauge fuel;

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
    FAIL_RED();
    delay(500);
    PASS_GREEN();
    delay(500);
    FAIL_BLUE();
    delay(500);
    RGB_OFF();
    delay(500);

    pinMode(LVLOE_UC, OUTPUT);
    digitalWrite(LVLOE_UC, HIGH);
    delay(500);
    digitalWrite(LVLOE_UC, LOW);
    delay(50);

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

    PMIC.begin(); 
    delay(500);
    
    fuel.quickStart();
    delay(100);

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
    byte pmicVersion = PMIC.getVersion();
    delay(50);
    Serial.print("Version Number: ");
    Serial.println(pmicVersion,HEX);

    Serial.print("System Status: ");
    Serial.println(PMIC.getSystemStatus(),BIN);
    delay(50);

    Serial.print("Fault Status: ");
    Serial.println(PMIC.getFault(),BIN);
    
    // Send the Voltage and SoC readings over serial
    Serial.println(fuel.getVCell());
    Serial.println(fuel.getSoC());

    delay(1000);

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
    Serial3.println("AT+CMGS=\"+16129997293\"");
    delay(1500);
    Serial3.println(msg);
    Serial3.println((char)26); // End AT command with a ^Z, ASCII code 26
}
