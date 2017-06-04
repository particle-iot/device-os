/**
  ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_ARDUINO_H
#define	SPARK_WIRING_ARDUINO_H

#ifndef strcpy_P
#define strcpy_P strcpy
#endif

#ifndef strlcpy_P
#define strlcpy_P strncpy
#endif

#ifndef sprintf_P
#define sprintf_P sprintf
#endif

#ifndef strlen_P
#define strlen_P strlen
#endif

#ifndef strcmp_P
#define strcmp_P strcmp
#endif

#ifndef memcpy_P
#define memcpy_P memcpy
#endif

#ifndef vsnprintf_P
#define vsnprintf_P vsnprintf
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef PSTR
#define PSTR(x) (x)
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(x)      (*(x))
#endif

#ifndef pgm_read_word
#define pgm_read_word(x)      ((uint16_t)(*(x)))
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(x))
#endif

#ifndef pgm_read_word_near
#define pgm_read_word_near(x) ((uint16_t)(*(x)))
#endif

#endif	/* SPARK_WIRING_ARDUINO_H */
