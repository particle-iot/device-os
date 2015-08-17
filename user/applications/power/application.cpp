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
void intialize_IO(void);
void runIOTEST(void);
void allHIGH(void);
void allLOW(void);

bool testSTATUS=1;

void setup()
{
    RGB.control(true);
    //RGB.color(0,255,0);

    //pinMode(TX, INPUT_PULLUP);
    //pinMode(RX, INPUT_PULLUP);

    intialize_IO();
    
	Serial.begin(9600);
	Serial3.begin(9600);
	Serial.println("Starting testing procedure: ");


    PMIC.begin(); 
    delay(500);

    //Serial.print("System Status: ");
    //Serial.println(PMIC.getSystemStatus(),BIN);

    //Serial.println("Enabling buck regulator");
    //PMIC.enableBuck();

    // Serial.println("Setting input current limit to 900mA");
    // PMIC.setInputCurrentLimit(900);
    // Serial.println(PMIC.readInputSourceRegister(),BIN);
    // delay(2000);
    
    fuel.quickStart();
    delay(100);

    Serial.println("System initialization complete");
    Serial.println("Press START button to begin the TEST");

    Serial.println("--");

}

void loop()
{
    

    // Begin the testing if the START button is pressed (active low)
    // The START button is connected between TX pin and GND
    if(1 == digitalRead(TX))
    {
        allLOW();

        runIOTEST();
        delay(1000);

        // TEST RGB LED
        FAIL_RED();
        delay(1000);
        PASS_GREEN();
        delay(1000);
        FAIL_BLUE();
        delay(1000);
        RGB_OFF();
        delay(1000);
            
        // TEST UBLOX
        if ( ensureUbloxIsReset() )
        {
            if( ensureATReturnsOK() == 1 ) {
                PASS_GREEN();
                Serial.println("Ublox test: PASS");
            }
            else {
                testSTATUS = 0;
                Serial.println("Ublox test: FAIL");
            }  
                
        }
        else {
            //FAIL_BLUE();
            Serial.println("Ublox test: FAIL");
            testSTATUS = 0;
        }

        Serial.println("--");

        // TEST PMIC BQ24195
        byte pmicVersion = PMIC.getVersion();
        delay(50);
        Serial.print("PMIC Version Number: ");
        Serial.println(pmicVersion,HEX);
        if (PMIC.getVersion() == 35) {
            Serial.println("PMIC Test: PASS");
        }
        else {
            Serial.println("PMIC Test: FAIL");
            testSTATUS = 0;
        }

        Serial.println("--");

        // Check power source quality
        Serial.print("Power: ");
        if(PMIC.isPowerGood()) Serial.println("GOOD");
        else {
            Serial.println("BAD");
            testSTATUS = 0;
        }

        Serial.println("--");

        // TEST MAX17043 FUEL GAUGE
        Serial.print("Fuel Gauge Version Number: ");
        Serial.println(fuel.getVersion());
        if (fuel.getVersion() == 3) {
            Serial.println("Fuel Gauge Test: PASS");
        }
        else {
            Serial.println("Fuel Gauge Test: FAIL");
            testSTATUS = 0;
        }

        Serial.println("--"); 

        // Final step is to notify test status using the RGB LED
        if(testSTATUS) {
            PASS_GREEN();
            Serial.println("Electron Test: PASS");
        }
        else {
            FAIL_RED();
            Serial.println("Electron Test: FAIL");
        }
        
        Serial.println("- - - - - - - - - - - - - - - - - - - - ");
        Serial.println("");

        

    } 

}


void intialize_IO(void) {

    // Initialize the buffer chip enable pin
    pinMode(LVLOE_UC, OUTPUT);
    digitalWrite(LVLOE_UC, HIGH);
    delay(500);
    digitalWrite(LVLOE_UC, LOW);
    delay(50);

    // Initialize IO relevant to Ublox-uC comm
    pinMode(D7, OUTPUT);
    pinMode(PWR_UC, OUTPUT);
    pinMode(RESET_UC, OUTPUT);
    digitalWrite(PWR_UC, HIGH);
    digitalWrite(RESET_UC, HIGH);
    pinMode(RTS_UC, OUTPUT);
    digitalWrite(RTS_UC, LOW); // VERY IMPORTANT FOR CORRECT OPERATION!!

    // Initialize the user accessible GPIOs
    pinMode(D0,OUTPUT);
    pinMode(D1,OUTPUT);
    pinMode(D2,OUTPUT);
    pinMode(D3,OUTPUT);
    pinMode(D4,OUTPUT);
    pinMode(D5,OUTPUT);
    pinMode(D6,OUTPUT);
    pinMode(D7,OUTPUT);
    
    pinMode(A0,OUTPUT);
    pinMode(A1,OUTPUT);
    pinMode(A2,OUTPUT);
    pinMode(A3,OUTPUT);
    pinMode(A4,OUTPUT);
    pinMode(A5,OUTPUT);
    pinMode(A6,OUTPUT);
    pinMode(A7,OUTPUT);

    pinMode(B0,OUTPUT);
    pinMode(B1,OUTPUT);
    pinMode(B2,OUTPUT);
    pinMode(B3,OUTPUT);
    pinMode(B4,OUTPUT);
    pinMode(B5,OUTPUT);

    pinMode(C0,OUTPUT);
    pinMode(C1,OUTPUT);
    pinMode(C2,OUTPUT);
    pinMode(C3,OUTPUT);
    pinMode(C4,OUTPUT);
    pinMode(C5,OUTPUT);
    
    pinMode(RX,OUTPUT);
    
    // Initialize all IO pins to LOW state
    allLOW();

    pinMode(TX,INPUT_PULLDOWN);

}


void runIOTEST(void) {

    int wait = 200;

    // RX - A0
    for (int i=18;i>9;i--)
    {
        digitalWrite(i,HIGH);
        delay(wait);
        digitalWrite(i,LOW);
        delay(wait);
    }

    // B5 - B0
    for (int i=29;i>23;i--)
    {
        digitalWrite(i,HIGH);
        delay(wait);
        digitalWrite(i,LOW);
        delay(wait);
    }

    // C0 - C5
    for (int i=30;i<36;i++)
    {
        digitalWrite(i,HIGH);
        delay(wait);
        digitalWrite(i,LOW);
        delay(wait);
    }

    // D0 - D7
    for (int i=0;i<8;i++)
    {
        digitalWrite(i,HIGH);
        delay(wait);
        digitalWrite(i,LOW);
        delay(wait);
    }

    allHIGH();
    delay(1000);
    allLOW();
    delay(1000);
    allHIGH();
    
}

void allHIGH(void) {

    for (int i=18;i>9;i--)
    {
        digitalWrite(i,HIGH);
    }

    // B5 - B0
    for (int i=29;i>23;i--)
    {
        digitalWrite(i,HIGH);
    }

    // C0 - C5
    for (int i=30;i<36;i++)
    {
        digitalWrite(i,HIGH);
    }

    // D0 - D7
    for (int i=0;i<8;i++)
    {
        digitalWrite(i,HIGH);
    }

}

void allLOW(void) {

    for (int i=18;i>9;i--)
    {
        digitalWrite(i,LOW);
    }

    // B5 - B0
    for (int i=29;i>23;i--)
    {
        digitalWrite(i,LOW);
    }

    // C0 - C5
    for (int i=30;i<36;i++)
    {
        digitalWrite(i,LOW);
    }

    // D0 - D7
    for (int i=0;i<8;i++)
    {
        digitalWrite(i,LOW);
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
    Serial3.println("AT+CMGS=\"+15555555555\"");
    delay(1500);
    Serial3.println(msg);
    Serial3.println((char)26); // End AT command with a ^Z, ASCII code 26
}
