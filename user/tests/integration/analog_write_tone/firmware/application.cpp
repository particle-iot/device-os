#include "application.h"

SerialDebugOutput debugOutput(9600, ALL_LEVEL);

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  // Particle.connect();
  pinMode(B0, OUTPUT);
  pinMode(B1, OUTPUT);
  pinMode(B2, OUTPUT);
  pinMode(B3, OUTPUT);
  pinMode(C4, OUTPUT);
  pinMode(C5, OUTPUT);
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);

  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A7, OUTPUT); //WKP
  pinMode(RX, OUTPUT);
  pinMode(TX, OUTPUT);

  delay(5000);
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>  starting the PWM test  <<<<<<<<<<<<<<<<<<<<<<<<<");
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void loop() {
  // write to each PWM pin
  analogWrite(B0, 10);
  analogWrite(B1, 20);
  analogWrite(B2, 30);
  analogWrite(B3, 40);
  analogWrite(C4, 50);
  analogWrite(C5, 60);
  analogWrite(D0, 70);
  analogWrite(D1, 80);

  analogWrite(D2, 200); // shares timer with A5
  analogWrite(D3, 100); // shares timer with A4
  analogWrite(A4, 100); // shares timer with D3
  analogWrite(A5, 200); // shares timer with D2

  analogWrite(A7, 130); //WKP
  analogWrite(RX, 140);
  analogWrite(TX, 150);

  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>  loop  <<<<<<<<<<<<<<<<<<<<<<<<<");
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  delay(2000);
}
