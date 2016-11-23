#ifndef _ARDUINO_H_
#define _ARDUINO_H_

/*
 * Arduino compatability shim for Particle applications and libraries
 *
 * 2015, John Plocher
 * Released to the public domain
 *
 * Leverage the normal include <Arduino.h> mechanism to do the right thing.
 *
 * The "right thing" means
 *  assume the caller wants an Arduino/wiring-like environment,
 *  define the things that are defined for 'duino users in the official IDE env
 *  don't require libs and apps to sprinkle  ifdef PARTICLE or SPARK directives
 */

#include <application.h>

#define PI		3.1415926535897932384626433832795
#define HALF_PI 	1.5707963267948966192313216916398
#define TWO_PI		6.283185307179586476925286766559
#define DEG_TO_RAD	0.017453292519943295769236907684886
#define RAD_TO_DEG	57.295779513082320876798154814105
#define EULER		2.718281828459045235360287471352

#define abs(x)	     ((x)>0?(x):-(x))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x)        ((x)*(x))

#define lowByte(w)   ((uint8_t) ((w) & 0xff))
#define highByte(w)  ((uint8_t) ((w) >> 8))

#define bitRead(value, bit)            (((value) >> (bit)) & 0x01)
#define bitSet(value, bit)             ((value) |= (1UL << (bit)))
#define bitClear(value, bit)           ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#endif

