#include "testapi.h"
#include "Particle.h"

test(Pin_digital_methods)
{
	bool value;
    API_COMPILE(value=D0.read());
    API_COMPILE(D0.write(HIGH));
}

test(Pin_digital_conversion_to_primitive) {
	API_COMPILE(digitalWrite(D0, HIGH));
	API_COMPILE(digitalRead(D0));
}
