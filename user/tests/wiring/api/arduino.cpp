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
	min(0L, short(3));
	max(0L, short(3));
}

test(has_sq) {
	(void)sq(5);
}

test(abs) {
	(void)abs(5.0);
}

// FlashStringHelper
// construct, assign, copy, concat, +,
test(String_FlashStringHelper) {
	const __FlashStringHelper* flash = reinterpret_cast<const __FlashStringHelper*>("hello");

	String s(flash);
	s = flash;
	String s2 = s + flash;
	String s3 = String(flash);
	s.concat(flash);
}

class ShowThatFlashStringHelperIsDistinctType {
	void f(const char* s) {

	}

	void f(const __FlashStringHelper* s) {

	}
};

