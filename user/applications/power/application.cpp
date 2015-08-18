#include "application.h"


#include "Serial3/Serial3.h"
#include "core_hal.h"
#include "pmic.h"
#include "fuelgauge.h"

SYSTEM_MODE(MANUAL);

PMIC PMIC;
FuelGauge fuel;

#define now() millis()
uint32_t lastFlash = now();
String com = "";
#define PASS_GREEN() RGB.color(0,255,0)
#define FAIL_RED() RGB.color(255,0,0)
#define FAIL_BLUE() RGB.color(0,0,255)
#define RGB_OFF() RGB.color(0,0,0)
#define RGB_WHITE RGB.color(255,255,255)
#define _BKPT __ASM("bkpt 0")
#ifndef UBLOX_PHONE_NUM
    #define UBLOX_PHONE_NUM "+16129997293"
#endif

int8_t testUbloxIsReset();
void   clearUbloxBuffer();
int8_t sendATcommand(const char* ATcommand, const char* expected_answer1, system_tick_t timeout);
int8_t testATOK(system_tick_t timeout);
void   sendSMS(const char * msg, char* num);
void   executeCommand(char c);
int8_t testEchoOff(system_tick_t timeout);
int8_t testSIMReady(system_tick_t timeout);
int8_t testSIM3G(system_tick_t timeout);
int8_t testNetworkRegistration(system_tick_t timeout);
int8_t testDefinePDP1(system_tick_t timeout);
int8_t testDefineQoSPDP1(system_tick_t timeout);
int8_t testActivatePDP1(system_tick_t timeout);
int8_t testPDP1nonZeroIP(system_tick_t timeout);
int8_t testReadPDP1params(system_tick_t timeout);
int8_t testReadPDP1QoSprofile(system_tick_t timeout);
int8_t testCreateSocketZero(system_tick_t timeout);
int8_t testDNSLookupServer(system_tick_t timeout);
int8_t testSocketConnectZero(system_tick_t timeout);
int8_t testSocketReadZero(system_tick_t timeout);

void intialize_IO(void);
void runIOTEST(void);
void allHIGH(void);
void allLOW(void);

bool testSTATUS=1;
bool testRunning = 0;

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
        testSTATUS = 1;
        Serial.println("Button press detected, starting test now...");
        RGB.color(0,0,0);
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
        testUbloxIsReset();
        // Test for AK OK
        Serial.println("Test if AT == OK...");
        if ( !testATOK(500) ) { 
            testSTATUS = 0;
            Serial.println("Ublox test: FAIL");
        }
        else Serial.println("Ublox test: PASS"); 
        Serial.println("--");
        delay(1000);

        //Serial.println("Ublox test: PASS");
        Serial.println("Test if SIM card is inserted...");
        // This will require a power cycle if user hotswapping the card
        if ( !testSIMReady(2000) ) { 
            testSTATUS = 0;
            Serial.println("SIM card Test: FAIL");
        } 
        else Serial.println("SIM card Test: PASS");
        Serial.println("--");
        delay(1000);

        Serial.println("Test if SIM card is 3G...");
        // This will require a power cycle if user hotswapping the card
        if ( !testSIM3G(2000) ) { 
            //testSTATUS = 0; 
        } 
        Serial.println("--");
        delay(1000);

        //FAIL_BLUE();
        //Serial.println("Ublox test: FAIL");
        //testSTATUS = 0;


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

        testRunning = 1;
    } 

    else {
        if(!testRunning) {
            RGB.color(0,0,0);
            delay(100);
            RGB.color(255,255,255);
            delay(20); 
        }
        
    }

}


void intialize_IO(void) {

#ifndef USE_SWD_JTAG
    pinMode(D7, OUTPUT);
#endif

    pinMode(PWR_UC, OUTPUT);
    pinMode(RESET_UC, OUTPUT);
    digitalWrite(PWR_UC, HIGH);
    digitalWrite(RESET_UC, HIGH);
    pinMode(RTS_UC, OUTPUT);
    digitalWrite(RTS_UC, LOW); // VERY IMPORTANT FOR CORRECT OPERATION!!
    pinMode(LVLOE_UC, OUTPUT);
    digitalWrite(LVLOE_UC, LOW); // VERY IMPORTANT FOR CORRECT OPERATION!!

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

void executeCommand(char c) {
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
        String temp = UBLOX_PHONE_NUM;
        char temp2[15] = "";
        temp.toCharArray(temp2, temp.length());
        sendSMS("Hello from Particle!", temp2);
    }
    else if (c == '0') {
        // Check if the module is registered on the network
        Serial.println("Check Network Registration...");
        sendATcommand("AT+CREG?", "OK", 500);
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
    else if (c == '3') {
        // Check signal strength
        Serial.println("Checking Signal Strength...");
        sendATcommand("AT+CSQ", "OK", 500);
    }
    else if (c == 'E') {
        // Turn UBLOX UART Echo ON
        Serial.println("Turn UBLOX UART Echo ON...");
        sendATcommand("AT E1", "OK", 500);
    }
    else if (c == 'e') {
        // Turn UBLOX UART Echo OFF
        Serial.println("Turn UBLOX UART Echo OFF...");
        sendATcommand("AT E0", "OK", 500);
    }
}

void clearUbloxBuffer() {
  while (Serial3.available()) {
    Serial3.read();
  }
}

int8_t testUbloxIsReset() {
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

int8_t testATOK(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT", "OK", timeout);
    return rv;
}

int8_t testEchoOff(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT E0", "OK", timeout);
    return rv;
}

int8_t testSIMReady(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CPIN?", "READY", timeout);
    return rv;
}

int8_t testSIM3G(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+UUICC?", "UUICC: 1", timeout);
    return rv;
}

int8_t testNetworkRegistration(system_tick_t timeout) {
    // CREG: x,0: not registered, the MT is not currently searching a new operator to register to
    // CREG: x,1: registered, home network
    // CREG: x,2: not registered, but the MT is currently searching a new operator to register to
    // CREG: x,3: registration denied
    int8_t rv = sendATcommand("AT+CREG?", "CREG: 0,1", timeout);
    return rv;
}

int8_t testDefinePDP1(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGDCONT=1,\"IP\",\"broadband\"", "OK", timeout);
    return rv;
}

int8_t testDefineQoSPDP1(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGEQREQ=1,3,64,64,,,0,320,\"1E4\",\"1E5\",1,,3", "OK", timeout);
    return rv;
}

int8_t testActivatePDP1(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGACT=1,1", "OK", timeout);
    return rv;
}

int8_t testPDP1nonZeroIP(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGPADDR=1", "+CGPADDR: 1,\"0.0.0.0\"", timeout);
    if (rv == 0) rv = 1; // if IP is non-zero, return true
    else (rv = 0);
    return rv;
}

int8_t testReadPDP1params(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGDCONT?", "OK", timeout);
    return rv;
}

int8_t testReadPDP1QoSprofile(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+CGEQNEG=1", "OK", timeout);
    return rv;
}

int8_t testCreateSocketZero(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+USOCR=6", "+USOCR: 0", timeout);
    return rv;
}

int8_t testDNSLookupServer(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+UDNSRN=0,\"device.spark.io\"", "OK", timeout);
    return rv;
}

int8_t testSocketConnectZero(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+USOCO=0,\"52.0.31.156\",5683", "+UUSORD: 0,40", timeout);
    return rv;
}

int8_t testSocketReadZero(system_tick_t timeout) {
    int8_t rv = sendATcommand("AT+USORD=0,40", "+USORD: 0,40", timeout);
    return rv;
}

void sendSMS(const char * msg, char* num)
{
    Serial.print("\e[0;32mSending SMS: ");
    Serial.println(msg);
    Serial.print("\e[0;35m");

    Serial3.print("AT+CMGF=1\r");
    delay(500);
    Serial3.print("AT+CMGS=\"+1");
    Serial3.print(num);
    Serial3.print("\"\r");
    delay(1500);
    Serial3.print(msg);
    Serial3.print((char)26); // End AT command with a ^Z, ASCII code 26
    Serial3.print("\r");
}
