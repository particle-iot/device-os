#include "application.h"

#include "Serial3/Serial3.h"
#include "core_hal.h"

SYSTEM_MODE(MANUAL);

#define now() millis()
uint32_t lastFlash = now();
String com = "";
#define PASS_GREEN() RGB.color(0,255,0)
#define FAIL_RED() RGB.color(255,0,0)
#define FAIL_BLUE() RGB.color(0,0,255)
#define RGB_OFF() RGB.color(0,0,0)
#define _BKPT __ASM("bkpt 0")
#ifndef UBLOX_PHONE_NUM
	#define UBLOX_PHONE_NUM "5555555555"
#endif

#if 0
#define COL(c) "\e[" c
#define DEF COL("39m")
#define BLA COL("30m")
#define RED COL("31m")
#define GRE COL("32m")
#define YEL COL("33m")
#define BLU COL("34m")
#define MAG COL("35m")
#define CYA COL("36m")
#define WHY COL("37m")
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

void setup()
{
	RGB.control(true);

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

	Serial.begin(9600);
	Serial3.begin(9600);

	//ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
	//Serial1DebugOutput debugOutput(115200, ALL_LEVEL);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	Serial.println("\e[0;36mHi, I'm your friendly neighborhood Electron!");
	Serial.print("\e[0;36mIf you send a SMS, it will go here: \e[0;33m+1");
	Serial.println(UBLOX_PHONE_NUM);

	// TEST RGB LED
	FAIL_RED();
	delay(500);
	PASS_GREEN();
	delay(500);
	FAIL_BLUE();
	delay(500);
	RGB_OFF();
	delay(500);

	// Test for AK OK, power cycle until we see it
	Serial.println("\e[0;32mTest if AT == OK...");
	while ( !testATOK(500) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	// Serial.println("\e[1;32mTurning ECHO OFF...");
	// while ( !testEchoOff() ) { FAIL_RED(); } PASS_GREEN();
	// delay(500);

	Serial.println("\e[0;32mTest if SIM card is inserted...");
	// This will require a power cycle if user hotswapping the card
	while ( !testSIMReady(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	Serial.println("\e[0;32mTest if SIM card is 3G...");
	// This will require a power cycle if user hotswapping the card
	while ( !testSIM3G(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	// Test if we are connected to a network
	Serial.println("\e[0;32mWait until we're connected to a network...");
	while ( !testNetworkRegistration(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	//sendSMS("Particle says, \"Hello!\"", UBLOX_PHONE_NUM);

	Serial.println("\e[0;32mDefine the PDP context 1 with PDP type \"IP\" and APN \"broadband\"...");
	while ( !testDefinePDP1(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	Serial.println("\e[0;32mDefine a QoS profile for PDP context 1, with Traffic Class 3 (background),\
			\r\nmaximum bit rate 64 kb/s both for UL and for DL, no Delivery Order requirements,\
			\r\na maximum SDU size of 320 octets, an SDU error ratio of 10-4, a residual bit error\
			\r\nratio of 10-5, delivery of erroneous SDUs allowed and Traffic Handling Priority 3.");
	while ( !testDefineQoSPDP1(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	Serial.println("\e[0;32mActivate PDP context 1...");
	while ( !testActivatePDP1(20000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	Serial.println("\e[0;32mTest PDP context 1 for non-zero IP address...");
	while ( !testPDP1nonZeroIP(2000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	Serial.println("\e[0;32mRead the PDP contexts’ parameters...");
	while ( !testReadPDP1params(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	Serial.println("\e[0;32mRead the negotiated QoS profile for PDP context 1...");
	while ( !testReadPDP1QoSprofile(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	Serial.println("\e[0;32mSetup GPRS Right NAO!!!");
	while ( !sendATcommand("AT+CGATT?;+UPSND=0,8;","+UPSND: 0,8,0",10000) ) { FAIL_RED(); } PASS_GREEN();
	delay(2000);

	while ( !sendATcommand("AT+UPSD=0,1,\"broadband\";+UPSD=0,7,\"0.0.0.0\";+UPSDA=0,1;+UPSDA=0,3;+UPSND=0,8;+UPSND=0,0","+UPSND:",20000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	Serial.println("\e[0;32mCreate Socket #0...");
	while ( !testCreateSocketZero(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	Serial.println("\e[0;32mDNS Lookup of \"device.spark.io\"...");
	while ( !testDNSLookupServer(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	Serial.println("\e[0;32mConnect to Server on Socket Zero, Expect 40 bytes of data...");
	while ( !testSocketConnectZero(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);

	// while(!Serial.available()) Spark.process();
	// while(Serial.available()) Serial.read();

	Serial.println("\e[0;32mRead 40-Byte Handshake Message from Particle Cloud...");
	while ( !testSocketReadZero(2000) ) { FAIL_RED(); } PASS_GREEN();
		delay(2000);
}

void loop()
{
	// TEST SETUP button (requires manual press)
	if (HAL_Core_Mode_Button_Pressed(100)) {
		FAIL_BLUE();
		delay(1000);
		PASS_GREEN();
		HAL_Core_Mode_Button_Reset();
	}

#ifndef USE_SWD_JTAG
	// TEST USER LED
	if ( now() - lastFlash > 100UL ) {
		lastFlash = now();
		digitalWrite(D7, !digitalRead(D7));
	}
#endif

	if ( Serial.available() ) {
		char c = Serial.read();
		com += c;
		Serial.write(c); //echo input
		if (c == '\r') {
			if (com.charAt(0)==';') {
				// ~a<ENTER> = AT OK test, ~3<ENTER> = signal strentgh test
				executeCommand(com.charAt(1));
			}
			else {
				// Anything not preceeded by ~ will be sent as plain text
				Serial3.print(com);
			}
			com = "";
		}
	}

	// if (Serial.available()) {
	//  char c = Serial.read();
	//  Serial3.write(c);
	// }

	if (Serial3.available()) {
		char c = Serial3.read();
		Serial.write(c);
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
		Serial.println("\e[0;35mUblox is on already.");
		clearUbloxBuffer();
		return 1;
	}
	else
	{
		// Ublox is off, so power it on
		Serial.println("\e[0;35mUblox is off, turning it on.");
		digitalWrite(PWR_UC,LOW);
		delay(500);
		digitalWrite(PWR_UC,HIGH);
		delay(2000);

		Serial3.println("AT");
		delay(100);

		if (Serial3.available())
		{
			// Ublox is on, just clear the buffer
			Serial.println("\e[0;35mUblox was off, now it is on.");
			clearUbloxBuffer();
			return 1;
		}
		else
		{
			// Ublox is off, so power it on
			Serial.println("\e[0;35mUblox remains off, error!");
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

	Serial.print("\e[0;35m");
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
