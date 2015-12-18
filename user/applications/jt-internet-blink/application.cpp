
#include "application.h"
// #include "stdarg.h"

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
SerialDebugOutput debugOutput(9600, ALL_LEVEL);

const int led = D7;
int ledToggle(String command);

// PRODUCT_ID(PLATFORM_ID);
// PRODUCT_VERSION(2);
SYSTEM_MODE(AUTOMATIC);

void setup() {
   pinMode(D7, OUTPUT);
  // Particle.connect();
  Particle.function("led",ledToggle);
  digitalWrite(led, LOW);
}


void loop()
{
  // noop
}


int ledToggle(String command) {
   if (command=="on") {
       digitalWrite(led,HIGH);
       return 1;
   }
   else if (command=="off") {
       digitalWrite(led,LOW);
       return 0;
   }
   else {
       return -1;
   }
}
