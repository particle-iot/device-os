
#include "testapi.h"
#include "spark_wiring_i2c.h"
#include "Serial2/Serial2.h"
#include "Serial3/Serial3.h"
#include "Serial4/Serial4.h"
#include "Serial5/Serial5.h"

test(api_i2c)
{
    int buffer;
    API_COMPILE(buffer=I2C_BUFFER_LENGTH);
    (void)buffer;
}

test(api_wiring_pinMode) {

    PinMode mode = PIN_MODE_NONE;
    API_COMPILE(mode=getPinMode(D0));
    API_COMPILE(pinMode(D0, mode));
    (void)mode;
}

test(api_wiring_analogWrite) {
  API_COMPILE(analogWrite(D0, 50));
  API_COMPILE(analogWrite(D0, 50, 10000));
}

test(api_wiring_wire_setSpeed)
{
    API_COMPILE(Wire.setSpeed(CLOCK_SPEED_100KHZ));
}
void D0_callback()
{
}

test(api_wiring_interrupt) {

    API_COMPILE(interrupts());
    API_COMPILE(noInterrupts());

    API_COMPILE(attachInterrupt(D0, D0_callback, RISING));
    API_COMPILE(detachInterrupt(D0));

    class MyClass {
      public:
        void handler() { }
    } myObj;

    API_COMPILE(attachInterrupt(D0, &MyClass::handler, &myObj, RISING));

#if PLATFORM_ID >= 6 && PLATFORM_ID <= 10
    API_COMPILE(attachSystemInterrupt(SysInterrupt_TIM1_CC_IRQ, D0_callback));
#endif // PLATFORM_ID >= 6 && PLATFORM_ID <= 10

    API_COMPILE(attachInterrupt(D0, D0_callback, RISING, 14));
    API_COMPILE(attachInterrupt(D0, D0_callback, RISING, 14, 0));
    API_COMPILE(attachInterrupt(D0, &MyClass::handler, &myObj, RISING, 14));
    API_COMPILE(attachInterrupt(D0, &MyClass::handler, &myObj, RISING, 14, 0));
}

test(api_wiring_usartserial) {

    API_COMPILE(Serial1.halfduplex(true));
    API_COMPILE(Serial1.halfduplex(false));

    API_COMPILE(Serial1.blockOnOverrun(false));
    API_COMPILE(Serial1.blockOnOverrun(true));

    API_COMPILE(Serial1.availableForWrite());

    API_COMPILE(Serial1.begin(9600, SERIAL_8N1));
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, SERIAL_8N2));
    API_COMPILE(Serial1.end());

    API_COMPILE(Serial1.begin(9600, SERIAL_8E1));
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, SERIAL_8E2));
    API_COMPILE(Serial1.end());

    API_COMPILE(Serial1.begin(9600, SERIAL_8O1));
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, SERIAL_8O2));
    API_COMPILE(Serial1.end());


    API_COMPILE(Serial1.begin(9600, SERIAL_9N1));
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, SERIAL_9N2));
    API_COMPILE(Serial1.end());

    // LIN mode
    API_COMPILE(Serial1.begin(9600, LIN_MASTER_13B));
    API_COMPILE(Serial1.breakTx());
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, LIN_SLAVE_10B));
    API_COMPILE(Serial1.breakRx());
    API_COMPILE(Serial1.end());
    API_COMPILE(Serial1.begin(9600, LIN_SLAVE_11B));
    API_COMPILE(Serial1.breakRx());
    API_COMPILE(Serial1.end());
}

test(api_wiring_usbserial) {
    API_COMPILE(Serial.blockOnOverrun(false));
    API_COMPILE(Serial.blockOnOverrun(true));
    API_COMPILE(Serial.availableForWrite());
    API_COMPILE(Serial.isConnected());

#if Wiring_USBSerial1
    API_COMPILE(USBSerial1.blockOnOverrun(false));
    API_COMPILE(USBSerial1.blockOnOverrun(true));
    API_COMPILE(USBSerial1.availableForWrite());
    API_COMPILE(USBSerial1.isConnected());
#endif
}

test(api_wiring_keyboard) {
#if Wiring_Keyboard
    API_COMPILE(Keyboard.begin());
    API_COMPILE(Keyboard.end());
#endif
}

test(api_wiring_mouse) {
#if Wiring_Mouse
    API_COMPILE(Mouse.begin());
    API_COMPILE(Mouse.end());
#endif
}


void TIM3_callback()
{
}

#if PLATFORM_ID >= 6 && PLATFORM_ID <= 10
// system interrupt not available for the core yet.
test(api_wiring_system_interrupt) {

    API_COMPILE(attachSystemInterrupt(SysInterrupt_TIM3_IRQ, TIM3_callback));
    API_COMPILE(detachSystemInterrupt(SysInterrupt_TIM3_IRQ));
}
#endif // PLATFORM_ID >= 6 && PLATFORM_ID <= 10

void externalLEDHandler(uint8_t r, uint8_t g, uint8_t b) {
}

class ExternalLed {
  public:
    void handler(uint8_t r, uint8_t g, uint8_t b) {
    }
} externalLed;

test(api_rgb) {
    bool flag; uint8_t value;
    API_COMPILE(RGB.brightness(50));
    API_COMPILE(RGB.brightness(50, false));
    API_COMPILE(flag=RGB.controlled());
    API_COMPILE(RGB.control(true));
    API_COMPILE(RGB.color(255,255,255));
    API_COMPILE(RGB.color(RGB_COLOR_WHITE));
    API_COMPILE(flag=RGB.brightness());
    API_COMPILE(RGB.onChange(externalLEDHandler));
    API_COMPILE(RGB.onChange(&ExternalLed::handler, &externalLed));
#if !HAL_PLATFORM_NRF52840 // (GEN 2)
    API_COMPILE(RGB.mirrorTo(A4, A5, A7));
    API_COMPILE(RGB.mirrorTo(A4, A5, A7, false));
    API_COMPILE(RGB.mirrorTo(A4, A5, A7, true, true));
#else // HAL_PLATFORM_NRF52840 (GEN 3)
#if (PLATFORM_ID == PLATFORM_XENON) || (PLATFORM_ID == PLATFORM_ARGON) || (PLATFORM_ID == PLATFORM_BORON)
    API_COMPILE(RGB.mirrorTo(A4, A5, A3));
    API_COMPILE(RGB.mirrorTo(A4, A5, A3, false));
    API_COMPILE(RGB.mirrorTo(A4, A5, A3, true, true));
#else
    // SoM
    API_COMPILE(RGB.mirrorTo(A1, A0, A7));
    API_COMPILE(RGB.mirrorTo(A1, A0, A7, false));
    API_COMPILE(RGB.mirrorTo(A1, A0, A7, true, true));
#endif // PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
#endif
    API_COMPILE(RGB.mirrorDisable());
    (void)flag; (void)value; // unused
}


test(api_servo_trim)
{
    Servo servo;
    servo.setTrim(234);
}

test(api_wire)
{
    API_COMPILE(Wire.begin());
    API_COMPILE(Wire.reset());
    API_COMPILE(Wire.end());
}

test(api_map)
{
    int i = 0;
    API_COMPILE(i = map(0x01, 0x00, 0xFF, 0, 255));
    double d = 0;
    API_COMPILE(d = map(5.0, 0.0, 10.0, 0.0, 20.0));
    (void)i; // avoid unused variable warning
    (void)d;
}

/**
 * Ensures that we can stil take the address of the global instances.
 *
 */
test(api_wiring_globals)
{
	void* ptrs[] = {
			&SPI,
#if Wiring_SPI1
			&SPI1,
#endif
#if Wiring_SPI2
			&SPI2,
#endif
			&Serial,
#if Wiring_USBSerial1
            &USBSerial1,
#endif
			&Wire,
#if Wiring_Wire1
			&Wire1,
#endif
#if Wiring_Wire3
			&Wire3,
#endif
			&Serial1,
#if Wiring_Serial2
			&Serial2,
#endif
#if Wiring_Serial3
			&Serial3,
#endif
#if Wiring_Serial4
			&Serial4,
#endif
#if Wiring_Serial5
			&Serial5,
#endif
			&EEPROM,
	};
	(void)ptrs;
}

