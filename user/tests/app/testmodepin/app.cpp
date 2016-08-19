
/**
 *
 * Setup:
 *
 * On the P1 breakout board, connect TESTMODE to SPI1_MISO (A4).
 * Run the code below.
 */

#include "application.h"

retained int step = 0;


void setup_testmode()
{
	System.enableFeature(FEATURE_RETAINED_MEMORY);

	switch(++step) {
	case 1:
		System.enableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
		break;

	case 2: // leave clock still active from before;
		break;

	case 3:
		System.disableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
		break;

	case 4:	// leave clock inactive
		break;
	}
}

const int max_steps = 5;

bool assertPinPeriod(const int pin, const int expectedPeriod, const int variancePercent=20, const int count=100)
{
	int avgPulseHigh = 0;
    for(int i=0;i<count;i++) {
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= count;

    const int variance = expectedPeriod*variancePercent/100;

    bool success = true;
    if (avgPulseHigh<(expectedPeriod-variance)) {
    		Serial1.printlnf("expected minimum period of %d, got %d", (expectedPeriod-variance), avgPulseHigh);
		success = false;
    }
    if (avgPulseHigh>(expectedPeriod+variance)) {
    		Serial1.printlnf("expected maximum period of %d, got %d", (expectedPeriod+variance), avgPulseHigh);
		success = false;
    }
    return success;
}

const int TESTMODE = 33;

bool wifiPowersaveClockRunning(int mirror) {
	return assertPinPeriod(mirror, 1000/64);
}

bool validate_testmode(int mirror, int step)
{
	if (step==1 || step==2) {
		Serial1.println("checking powersave clock is active.");
		return wifiPowersaveClockRunning(mirror);
	}
	else if (step==3 || step==4) {
		bool success = true;

		// pulseIn doesn't work well with static signals - needs a configurable timeout.
//		if (wifiPowersaveClockRunning(mirror)) {
//			Serial1.println("WiFi powersave clock still running");
//			return false;
//		}

		Serial1.println("checking pin can be controlled while wifi is cycled");
		Particle.disconnect();
		pinMode(mirror, INPUT);
		pinMode(TESTMODE, OUTPUT);
		for (int i=0; i<16; i++) {
			Serial1.printlnf("iteration %d", i);
			int state = (i&1) ? LOW : HIGH;
			digitalWrite(TESTMODE, state);
			int sense = digitalRead(mirror);
			if (!!sense != !!state) {
				success = false;
				Serial1.printlnf("expected pin state after setting to be %d was %d", state, sense);
			}
			WiFi.off();
			WiFi.on();
			delay(16);
			sense = digitalRead(mirror);
			if (!!sense != !!state) {
				success = false;
				Serial1.printlnf("expected pin state after toggling wifi to be %d was %d", state, sense);
			}
		}
		return success;
	}
	return true;
}


STARTUP(setup_testmode());

const int testModeMirror = A4;

void setup() {
	Serial1.begin(9600);
	pinMode(testModeMirror, INPUT);
	Serial1.printlnf("step %d", step);
}

void loop() {

	static bool run = true;

	if (run) {
		run = false;

		RGB.control(true);
		RGB.color(255,255,0);

		if (validate_testmode(testModeMirror, step)) {
			Serial1.println("test successful");

			if (step<max_steps) {
				RGB.color(0,64,0);
				delay(1000);
				System.reset();
			}
			else {
				Serial1.println("all tests complete.");
				step = 0;	// restart on reset.
				RGB.color(0,255,0);
			}
		}
		else {
			Serial1.println("test FAILED");
			Serial1.printlnf("current step %d", step);
			RGB.color(255,0,0);
			step = 0;	// restart on reset.
		}
	}
}

