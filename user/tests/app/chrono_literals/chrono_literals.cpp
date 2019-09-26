#include <Particle.h>

/*
 * Project: chrono_literals
 * Description: Test chrono literal timing in `delay` API
 * Author: Zachary J. Fields
 * Date: 3 SEP 2019
 */

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  pinMode(D7, OUTPUT);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  digitalWrite(D7, HIGH);
  delay(3000);
  digitalWrite(D7, LOW);
  delay(3s);
}
