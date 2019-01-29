
#ifndef ARDUINO_H
#define	ARDUINO_H

#ifdef PARTICLE_WIRING_ARDUINO_COMPATIBILTY
#undef PARTICLE_WIRING_ARDUINO_COMPATIBILTY
#endif

#define PARTICLE_WIRING_ARDUINO_COMPATIBILTY 1

#include "Particle.h"

#if PARTICLE_WIRING_ARDUINO_COMPATIBILTY == 1

#ifndef ARDUINO
#define ARDUINO 10800
#endif

#include "math.h"

#ifndef isnan
#error isnan is not defined please ensure this header is included before any STL headers
#endif


#include "avr/pgmspace.h"
#include "spark_wiring_arduino_constants.h"
#include "spark_wiring_arduino_binary.h"

typedef particle::__SPISettings SPISettings;

#undef F
#define F(X) (reinterpret_cast<const __FlashStringHelper*>(X))

#ifndef makeWord
inline uint16_t makeWord(uint16_t w) {
  return w;
}

inline uint16_t makeWord(uint8_t h, uint8_t l) {
  return ((uint16_t)h << 8) | ((uint16_t)l);
}
#endif

#ifndef word
#define word(...) makeWord(__VA_ARGS__)
#endif

#ifndef WEAK
#define WEAK __attribute__ ((weak))
#endif

#ifndef SYSTEM_CORE_CLOCK
#define SYSTEM_CORE_CLOCK             SystemCoreClock
#endif

#ifndef clockCyclesPerMicrosecond
#define clockCyclesPerMicrosecond()   (SystemCoreClock / 1000000L)
#endif

#ifndef clockCyclesToMicroseconds
#define clockCyclesToMicroseconds(a)  (((a) * 1000L) / (SystemCoreClock / 1000L))
#endif

#ifndef microsecondsToClockCycles
#define microsecondsToClockCycles(a)  ((a) * (SystemCoreClock / 1000000L))
#endif

#ifndef digitalPinToInterrupt
#define digitalPinToInterrupt(P)      (P)
#endif

inline void yield() {
#if PLATFORM_THREADING
  os_thread_yield();
#endif // PLATFORM_THREADING
}

#ifndef PINS_COUNT
#define PINS_COUNT           TOTAL_PINS
#endif

#ifndef NUM_DIGITAL_PINS
#define NUM_DIGITAL_PINS     TOTAL_PINS
#endif

#ifndef NUM_ANALOG_INPUTS
#define NUM_ANALOG_INPUTS    TOTAL_ANALOG_PINS
#endif

#ifndef NUM_ANALOG_OUTPUTS
#define NUM_ANALOG_OUTPUTS   TOTAL_DAC_PINS
#endif

#ifndef analogInputToDigitalPin
#define analogInputToDigitalPin(p)  (((p < TOTAL_ANALOG_PINS) && (p >= 0)) ? (p) + FIRST_ANALOG_PIN : -1)
#endif

// XXX
#if PLATFORM_ID == PLATFORM_SPARK_CORE || PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

# ifndef digitalPinToPort
# define digitalPinToPort(P)        ( HAL_Pin_Map()[P].gpio_peripheral )
# endif

# ifndef digitalPinToBitMask
# define digitalPinToBitMask(P)     ( HAL_Pin_Map()[P].gpio_pin )
# endif
//#define analogInPinToBit(P)        ( )
# ifndef portOutputRegister
# define portOutputRegister(port)   ( &(port->ODR) )
# endif

# ifndef portInputRegister
# define portInputRegister(port)    ( &(port->IDR) )
# endif

//#define portModeRegister(port)     ( &(port->CRL) )
# ifndef digitalPinHasPWM
# define digitalPinHasPWM(P)        ( HAL_Validate_Pin_Function(P, PF_TIMER) == PF_TIMER )
# endif

#elif HAL_PLATFORM_NRF52840

# ifndef digitalPinToPort
# define digitalPinToPort(P)        ( HAL_Pin_Map()[P].gpio_port ? NRF_P1 : NRF_P0 )
# endif

# ifndef digitalPinToBitMask
# define digitalPinToBitMask(P)     ( HAL_Pin_Map()[P].gpio_pin )
# endif

# ifndef portOutputRegister
# define portOutputRegister(port)   ( &(port->OUT) )
# endif

# ifndef portInputRegister
# define portInputRegister(port)    ( &(port->IN) )
# endif

# ifndef portModeRegister
# define portModeRegister(port)     ( &(port->DIR) )
# endif

# ifndef digitalPinHasPWM
# define digitalPinHasPWM(P)        ( HAL_Validate_Pin_Function(P, PF_TIMER) == PF_TIMER )
# endif

#endif // PLATFORM_ID == PLATFORM_SPARK_CORE || PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#ifndef _BV
#define _BV(x)  (((uint32_t)1) << (x))
#endif

// XXX
#ifndef sei
#define sei() HAL_enable_irq(0)
#endif

// XXX
#ifndef cli
#define cli() (void)HAL_disable_irq()
#endif

#ifndef cbi
#define cbi(sfr, bit) ((sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) ((sfr) |= _BV(bit))
#endif

// XXX
#ifndef RwReg
typedef volatile uint32_t RwReg;
#endif

// Pins

// LED
#if PLATFORM_ID == PLATFORM_SPARK_CORE || PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || \
    PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION || \
    HAL_PLATFORM_MESH
# ifndef LED_BUILTIN
# define LED_BUILTIN D7
# endif

# ifndef ATN
# define ATN SS
# endif

#endif // PLATFORM_ID == PLATFORM_SPARK_CORE || PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION ||
       // PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION ||
       // HAL_PLATFORM_MESH

// C++ only
#ifdef __cplusplus

#ifndef isnan
using std::isnan
#endif

#ifndef isinf
using std::isinf
#endif


// Hardware serial defines

#ifndef UBRRH
#define UBRRH
#endif

#ifndef UBRR1H
#define UBRR1H
#endif

#if Wiring_Serial2
#ifndef UBRR2H
#define UBRR2H
#endif
#endif

#if Wiring_Serial3
#ifndef UBRR3H
#define UBRR3H
#endif
#endif

#if Wiring_Serial4
#ifndef UBRR4H
#define UBRR4H
#endif
#endif
#if Wiring_Serial5
#ifndef UBRR5H
#define UBRR5H
#endif
#endif

typedef USARTSerial HardwareSerial;

#ifndef SERIAL_PORT_MONITOR
#define SERIAL_PORT_MONITOR               Serial
#endif

#ifndef SERIAL_PORT_USBVIRTUAL
#define SERIAL_PORT_USBVIRTUAL            Serial
#endif

#ifndef SERIAL_PORT_HARDWARE_OPEN
#define SERIAL_PORT_HARDWARE_OPEN         Serial1
#endif

#ifndef SERIAL_PORT_HARDWARE_OPEN1
#define SERIAL_PORT_HARDWARE_OPEN1        Serial1
#endif

#if !defined(SERIAL_PORT_HARDWARE_OPEN2) && Wiring_Serial2
#define SERIAL_PORT_HARDWARE_OPEN2        Serial2
#endif

#if !defined(SERIAL_PORT_HARDWARE_OPEN3) && Wiring_Serial3
#define SERIAL_PORT_HARDWARE_OPEN3        Serial3
#endif

#if !defined(SERIAL_PORT_HARDWARE_OPEN4) && Wiring_Serial4
#define SERIAL_PORT_HARDWARE_OPEN4        Serial4
#endif

#if !defined(SERIAL_PORT_HARDWARE_OPEN5) && Wiring_Serial5
#define SERIAL_PORT_HARDWARE_OPEN5        Serial5
#endif

#ifndef SERIAL_PORT_HARDWARE
#define SERIAL_PORT_HARDWARE              Serial1
#endif

#ifndef SERIAL_PORT_HARDWARE1
#define SERIAL_PORT_HARDWARE1             Serial1
#endif

#if !defined(SERIAL_PORT_HARDWARE2) && Wiring_Serial2
#define SERIAL_PORT_HARDWARE2             Serial2
#endif

#if !defined(SERIAL_PORT_HARDWARE3) && Wiring_Serial3
#define SERIAL_PORT_HARDWARE3             Serial3
#endif

#if !defined(SERIAL_PORT_HARDWARE4) && Wiring_Serial4
#define SERIAL_PORT_HARDWARE4             Serial4
#endif

#if !defined(SERIAL_PORT_HARDWARE5) && Wiring_Serial5
#define SERIAL_PORT_HARDWARE5             Serial5
#endif

#ifndef SerialUSB
#define SerialUSB                         Serial
#endif

#endif // __cplusplus

#endif // PARTICLE_WIRING_ARDUINO_COMPATIBILTY


#endif	/* ARDUINO_H */

