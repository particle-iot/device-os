/* Arduino SdFat Library
 * Copyright (C) 2008 by William Greiman
 *
 * This file is part of the Arduino SdFat Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef SdFatUtil_h
#define SdFatUtil_h
/**
 * \file
 * Useful utility functions.
 */
#include "application.h"
#include "Sd2Card.h"
//#include <string.h>
/*
#ifndef PSTR
#define PSTR(x) (x)
#endif
#ifndef PROGMEM
#define PROGMEM         // N/A
#endif
*/
#ifndef PGM_P
#define PGM_P const char*
#endif
#ifndef prog_uchar
typedef const unsigned char  prog_uchar;
#endif
#ifndef prog_short
typedef const unsigned short prog_short;
#endif
#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(prog_uchar*)x)
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(x)      (*(prog_uchar*)x)
#endif
#ifndef pgm_read_word
#define pgm_read_word(x)      (*(prog_short*)x)
#endif
#ifndef strcpy_P
#define strcpy_P strcpy
#endif
#ifndef strncpy_P
#define strncpy_P strncpy
#endif

//#include <avr/pgmspace.h>
/** Store and print a string in flash memory.*/
/*
#ifndef PgmPrint
#define PgmPrint(x) Serial.print(PSTR(x))
#endif
*/
/** Store and print a string in flash memory followed by a CR/LF.*/
/*
#ifndef PgmPrintln
#define PgmPrintln(x) Serial.println(PSTR(x))
#endif
*/
/** Defined so doxygen works for function definitions. */
#define NOINLINE __attribute__((noinline))
//------------------------------------------------------------------------------
/** Return the number of bytes currently free in RAM. */
#if 0
static int FreeRam(void) {
  extern int  __bss_end;
  extern int* __brkval;
  int free_memory;
  if (reinterpret_cast<int>(__brkval) == 0) {
    // if no heap use from end of bss section
    free_memory = reinterpret_cast<int>(&free_memory)
                  - reinterpret_cast<int>(&__bss_end);
  } else {
    // use from top of stack to heap
    free_memory = reinterpret_cast<int>(&free_memory)
                  - reinterpret_cast<int>(__brkval);
  }
  return free_memory;
}
#endif
//------------------------------------------------------------------------------
/**
 * %Print a string in flash memory to the serial port.
 *
 * \param[in] str Pointer to string stored in flash memory.
*/ 
/*
#ifndef SerialPrint_P
#define SerialPrint_P(str) { \
	const char *p=str; \
  for (uint8_t c; (c = pgm_read_byte(p)); p++) Serial.print((char)c); \
}
#endif
------------------------------------------------------------------------------
*/
/**
 * %Print a string in flash memory followed by a CR/LF.
 *
 * \param[in] str Pointer to string stored in flash memory.
*/
/* 
#ifndef SerialPrintln_P
#define SerialPrintln_P(str) { \
  SerialPrint_P(str); \
  Serial.println();   \
}
#endif
*/
#endif  // #define SdFatUtil_h
