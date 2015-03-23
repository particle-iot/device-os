#include "application.h"

int result;

void setup() {
	pinMode(D7,OUTPUT);
	digitalWrite(D7,LOW);
}

void loop() {
  // Measure control lines on RF switch with a multimeter
  // -3.3V
  digitalWrite(D7,HIGH);
  delay(500);
  digitalWrite(D7,LOW);
  result = WiFi.selectAntenna(ANT_INTERNAL);
  if(result != 0) {
    while(1) {
      digitalWrite(D7,!digitalRead(D7));
      delay(100);
    }
  }
  delay(3000);

  // 3.3V
  for (int x=0; x<2; x++) {
    digitalWrite(D7,HIGH);
    delay(500);
    digitalWrite(D7,LOW);
    delay(500);
  }
  result = WiFi.selectAntenna(ANT_EXTERNAL);
  if(result != 0) {
    while(1) {
      digitalWrite(D7,!digitalRead(D7));
      delay(500);
    }
  }
  delay(3000);

  // Toggling very fast between 3.3V and -3.3V
  for (int x=0; x<3; x++) {
    digitalWrite(D7,HIGH);
    delay(500);
    digitalWrite(D7,LOW);
    delay(500);
  }
  result = WiFi.selectAntenna(ANT_AUTO);
  if(result != 0) {
    while(1) {
      digitalWrite(D7,!digitalRead(D7));
      delay(1000);
    }
  }
  delay(3000);

  // Force an error
  //-------------------------------
  // WiFi.selectAntenna(0);
  // WiFi.selectAntenna(ANT_BOGUS);
}
