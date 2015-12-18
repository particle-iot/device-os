#include "application.h"
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialDebugOutput debugOutput(9600, ALL_LEVEL);

const int led = D7;

void setup() {
  delay(3000);
  DEBUG_D("Hello from the Electron! Boot time is: %d\r\n ms", millis());
	Particle.connect(); // blocking call to connect
  pinMode(led, OUTPUT);
}

void loop(){
  digitalWrite(led, HIGH);
  delay(750);
  digitalWrite(led, LOW);
  delay(750);
  DEBUG_D("Loop end. %d ms since boot.\r\n", millis());
}
