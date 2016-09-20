
/**
 *
 * Setup:
 *
 * On the P1 breakout board, connect TESTMODE to SPI1_MISO (A4).
 * Run the code below.
 */

#include "application.h"

retained int step = 0;
const int max_steps = 6;
const int TESTMODE = P1S6;
const int TESTMODE_MIRROR = A4;
volatile int mirror_pin_state = 0;

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void setup_testmode()
{
	System.enableFeature(FEATURE_RETAINED_MEMORY);

	switch(++step) {
	case 1:
		System.enableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
		break;

	case 2: // leave clock still active from before;
		break;

	case 3: // leave clock still active from before;
		break;

	case 4:
		System.disableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
		break;

	case 5:	// leave clock inactive
		break;

	case 6:	// leave clock inactive
		break;
	}
}

STARTUP(setup_testmode());

void pinWentLow()
{
	mirror_pin_state = 0;
	detachInterrupt(TESTMODE_MIRROR);
}

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

bool wifiPowersaveClockRunning(int mirror) {
	return assertPinPeriod(mirror, 1000/64);
}

bool connectToCloud(void) {
	Particle.connect();
	if (!waitFor(Particle.connected, 30000)) {
		Serial1.println("Cloud did not connect within 30s!");
		return false;
	}
	return true;
}

bool validate_testmode(int mirror, int step)
{
	// check for active powersave clock
	if (step==1 || step==2) {
		if (!connectToCloud()) return false;
		Serial1.println("checking powersave clock is active.");
		return wifiPowersaveClockRunning(mirror);
	}
	// check for inactive powersave clock
	else if (step==4 || step==5) {
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
	// delay connection for step 3 and step 6 to test for retained control of P1S6 after Wi-Fi enabled
	else if ( step==3 || step==6 ) {
		Serial1.println("checking pin retains control after wifi is enabled.");
		digitalWrite(TESTMODE, HIGH);
		mirror_pin_state = 1;
		pinMode(mirror, INPUT_PULLUP);
		attachInterrupt(mirror, pinWentLow, FALLING);
		delay(500);
		if (digitalRead(mirror) == LOW || mirror_pin_state == 0) {
			Serial1.println("expected P1S6 to remain high while Wi-Fi is off for 500ms");
			return false;
		}
		if (!connectToCloud()) {
			return false;
		}
		if (step == 6) { // powersave clock is disabled, we expect P1S6 to remain high
			if (digitalRead(mirror) == LOW || mirror_pin_state == 0) {
				Serial1.println("powersave clock is disabled, expected P1S6 to remain high");
				return false;
			}
		}
		else { // step 3, powersave clock is enabled, we expect P1S6 to have gone LOW, and be clocking at 32kHz again
			if (mirror_pin_state == 1) {
				Serial1.println("powersave clock is enabled, expected P1S6 to have gone LOW, and be clocking at 32kHz again");
				return false;
			}
		}
	}
	return true;
}

void setup() {
	Serial1.begin(9600);
	pinMode(TESTMODE_MIRROR, INPUT);
	Serial1.printlnf("step %d", step);
	if (step==1) {
		Serial1.println("checking for jumper between TESTMODE (P1S6) to SPI1_MISO (A4)");
		pinMode(TESTMODE_MIRROR, INPUT_PULLDOWN);
		pinMode(TESTMODE, OUTPUT);
		if (digitalRead(TESTMODE_MIRROR)!=LOW) {
			Serial1.println("SPI1_MISO (A4) should be LOW, do you have it hooked to wrong pin?");
			while (digitalRead(TESTMODE_MIRROR)!=LOW) Particle.process(); // wait for error to be cleared.
		}
		digitalWrite(TESTMODE, HIGH);
		if (digitalRead(TESTMODE_MIRROR)!=HIGH) {
			Serial1.println("SPI1_MISO (A4) should be HIGH, do you have it hooked to wrong pin?");
			while (digitalRead(TESTMODE_MIRROR)!=HIGH) Particle.process(); // wait for error to be cleared.
		}
		pinMode(TESTMODE_MIRROR, INPUT);
	}
}

void loop() {

	static bool run = true;

	if (run) {
		run = false;

		RGB.control(true);
		RGB.color(255,255,0);

		if (validate_testmode(TESTMODE_MIRROR, step)) {
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

