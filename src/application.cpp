#include "application.h"

int sensor_value = 0;

void setup()
{
  Spark.variable("sensor", &sensor_value, INT);
  pinMode(A0, INPUT);
}

void loop()
{
  sensor_value = analogRead(A0);
}
