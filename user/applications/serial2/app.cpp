#include "application.h"

#include "Serial2/Serial2.h"

SYSTEM_MODE(MANUAL);

uint32_t now = millis();
uint32_t lastFlash = now;
    
void setup()
{
	// RGB.control(true);
	pinMode(RGBR, OUTPUT); 
	// pinMode(RGBG, OUTPUT);
	// pinMode(RGBB, OUTPUT);
	Serial.begin(9600);
	Serial.println("Hi, I'm Serial USB!");
}

void loop()
{    
	now = millis();
	if (now - lastFlash > 100UL) {
		lastFlash = now;
		digitalWrite(RGBR, !digitalRead(RGBR)); // This blinks the RED segment of the RGB led
		// digitalWrite(RGBG, !digitalRead(RGBG));
		// digitalWrite(RGBB, !digitalRead(RGBB));
	}
	
	if(Serial.available()) {
		char c = Serial.read();
		if(c == '1') Serial1.begin(9600); 
		else if(c == '2') Serial2.begin(9600); 
		else if (c == 'o') Serial1.println("Hi, I'm Serial 1!");
		else if (c == 't') Serial2.println("Hi, I'm Serial 2!");
	}

	if (Serial1.available()) {
		char c = Serial1.read();
		Serial.write(c);
	}

	if (Serial2.available()) {
		char c = Serial2.read();
		Serial.write(c);
	}
}