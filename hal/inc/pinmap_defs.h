#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

typedef uint16_t pin_t;

typedef enum PinMode {
  INPUT,
  OUTPUT,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  AF_OUTPUT_PUSHPULL, //Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
  AF_OUTPUT_DRAIN,    //Used internally for Alternate Function Output Drain(I2C etc). External pullup resistors required.
  AN_INPUT,           //Used internally for ADC Input
  AN_OUTPUT,          //Used internally for DAC Output
  PIN_MODE_NONE=0xFF
} PinMode;

typedef enum {
    PF_NONE,
    PF_DIO,
    PF_TIMER,
    PF_ADC,
  PF_DAC
} PinFunction;



#ifdef __cplusplus

namespace particle { namespace pin {

#define PIN_COUNT(name, count) \
	const int name = count;

template<pin_t PIN> class AbstractPin {
public:
	const pin_t pin = PIN;
};

template<pin_t PIN> class GPIOPin : public AbstractPin<PIN> {
public:
	const pin_t pin = PIN;

	PinMode mode() const;
	void mode(PinMode mode);

	operator pin_t() const { return pin; }

};

extern "C" void digitalWrite(uint16_t pin, uint8_t value);
extern "C" int32_t digitalRead(uint16_t pin);

template<pin_t PIN> class DigitalPin : public GPIOPin<PIN> {
public:
	bool read() { return digitalRead(PIN)!=0; }
	void write(bool value) { digitalWrite(PIN, value ? 1 : 0); }

	bool isHigh() { return read(); }
	bool isLow() { return read(); }

	DigitalPin& operator =(bool value) { write(value); return *this; }
};

}}  // particle::pin

#endif
