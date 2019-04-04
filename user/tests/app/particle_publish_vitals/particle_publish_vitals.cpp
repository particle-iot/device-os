/*
 * Project: particle_publish_vitals
 * Description: Confirm the functionality of the `Particle.publishVitals()` API
 * Author: Zachary J. Fields
 * Date: 13th March, 2019
 */

#include "Particle.h"

static const uint16_t ALT_LED = D7;
size_t setup_ms = 0;
bool publish = true;
bool final_publish = false;

// setup() runs once, when the device is first turned on.
void setup()
{
    // Put initialization like pinMode and begin functions here.
    Particle.publishVitals(3);
    setup_ms = millis();
    pinMode(ALT_LED, OUTPUT);
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
    if (publish)
    {
        // Disable publish after 10 seconds
        if (10000 < (millis() - setup_ms))
        {
            publish = false;
            Particle.publishVitals(0);
            for (size_t i = 0; i < 5; ++i)
            {
                digitalWrite(ALT_LED, HIGH);
                delay(100);
                digitalWrite(ALT_LED, LOW);
                delay(100);
            }
        }
        else
        {
            // Visually demonstrate loop
            digitalWrite(ALT_LED, HIGH);
            delay(100);
            digitalWrite(ALT_LED, LOW);
        }
    }
    else if (!final_publish && (15000 < (millis() - setup_ms)))
    {
        final_publish = true;
        digitalWrite(ALT_LED, HIGH);
        delay(100);
        digitalWrite(ALT_LED, LOW);
        Particle.publishVitals();
        delay(500);
        Particle.publishVitals(particle::NOW);
    }
    delay(1000);
}
