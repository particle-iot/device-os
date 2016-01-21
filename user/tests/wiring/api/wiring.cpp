
#include "testapi.h"
#include "spark_wiring_i2c.h"

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


    API_COMPILE(attachSystemInterrupt(SysInterrupt_TIM1_CC_IRQ, D0_callback));

    API_COMPILE(attachInterrupt(D0, D0_callback, RISING, 14));
    API_COMPILE(attachInterrupt(D0, D0_callback, RISING, 14, 0));
    API_COMPILE(attachInterrupt(D0, &MyClass::handler, &myObj, RISING, 14));
    API_COMPILE(attachInterrupt(D0, &MyClass::handler, &myObj, RISING, 14, 0));
}

test(api_wiring_usartserial) {

    API_COMPILE(Serial1.halfduplex(true));
    API_COMPILE(Serial1.halfduplex(false));

}

void TIM3_callback()
{
}

#if PLATFORM_ID>=6
// system interrupt not available for the core yet.
test(api_wiring_system_interrupt) {

    API_COMPILE(attachSystemInterrupt(SysInterrupt_TIM3_IRQ, TIM3_callback));
    API_COMPILE(detachSystemInterrupt(SysInterrupt_TIM3_IRQ));
}
#endif

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
    map(0x01,0x00,0xFF,0,255);
}
