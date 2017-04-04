// including particle first should not stop the arduino chnages being added later
// the one exception is the template functinos min/max/constrain/range - these become the C++
// template version when Arduino.h is included after particle.h

#include "Particle.h"
#include "Arduino.h"

#include "testapi.h"

// SPISettings part of the API
void somefunc(SPISettings& s) {

}

test(has_min_and_max) {
	min(0L, 3U);
	max(0L, 3U);
}

test(has_sq) {
	(void)sq(5);
}

test(abs) {
	abs(5.0);
}
