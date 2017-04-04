/**
  ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

#ifndef SPARK_WIRING_ARDUINO_CONSTANTS_H
#define SPARK_WIRING_ARDUINO_CONSTANTS_H

#ifndef true
#define true        0x1
#endif

#ifndef false
#define false       0x0
#endif

#ifndef SERIAL
#define SERIAL      0x0
#endif

#ifndef DISPLAY
#define DISPLAY     0x1
#endif

#ifndef DEFAULT
#define DEFAULT     1
#endif

#ifndef EXTERNAL
#define EXTERNAL    0
#endif

#ifndef PI
#define PI          3.1415926535897932384626433832795
#endif

#ifndef HALF_PI
#define HALF_PI     1.5707963267948966192313216916398
#endif

#ifndef TWO_PI
#define TWO_PI      6.283185307179586476925286766559
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD  0.017453292519943295769236907684886
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG  57.295779513082320876798154814105
#endif

#ifndef EULER
#define EULER       2.718281828459045235360287471352
#endif

#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif

#ifndef radians
#define radians(deg) ((deg)*DEG_TO_RAD)
#endif

#ifndef degrees
#define degrees(rad) ((rad)*RAD_TO_DEG)
#endif

#ifndef sq
#define sq(x)        ((x)*(x))
#endif


#if PARTICLE_WIRING_ARDUINO_TEMPLATES
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef constrain
#undef constrain
#endif

#ifdef round
#undef round
#endif

#else
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

#ifndef round
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif

#endif  // #ifdef PARTICLE_WIRING_ARDUINO_TEMPLATES

#ifndef lowByte
#define lowByte(w)   ((uint8_t) ((w) & 0xff))
#endif

#ifndef highByte
#define highByte(w)  ((uint8_t) ((w) >> 8))
#endif

#ifndef bitRead
#define bitRead(value, bit)            (((value) >> (bit)) & 0x01)
#endif

#ifndef bitSet
#define bitSet(value, bit)             ((value) |= (1UL << (bit)))
#endif

#ifndef bitClear
#define bitClear(value, bit)           ((value) &= ~(1UL << (bit)))
#endif

#ifndef bitWrite
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#endif

#ifndef bit
#define bit(b) (1UL << (b))
#endif

typedef uint32_t word;

#endif // SPARK_WIRING_ARDUINO_CONSTANTS_H
