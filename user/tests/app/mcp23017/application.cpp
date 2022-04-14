#include "application.h"
#include "Adafruit_MCP23017.h"


SYSTEM_THREAD(ENABLED);

Adafruit_MCP23017 myPE; 

void setup()
{
  Serial.begin(9600);

  
  Serial.println("Example program w/ real i2c traffic");

  myPE.begin();
  myPE.pinMode(0,OUTPUT);
  myPE.pinMode(1,INPUT);
}

void loop()
{
  delayMicroseconds(1000*50);

  //i2c transactions w/ our PE
  myPE.digitalWrite(0, myPE.digitalRead(1) );
  
}
