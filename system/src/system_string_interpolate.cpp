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

#include "system_string_interpolate.h"
#include <string.h>
#include <ctype.h>

inline bool is_variable_char(char c)
{
	return isalnum(c) || c=='_';
}

/**
 * Determines the length of an interpolatable varaible.
 * @param s the buffer containing the variable name
 *
 * The variable name may be zero terminated, or impliicitly terminated
 * by the next non-alphanumeric or non-underscore character.
 */
size_t variable_length(const char* s)
{
	const char* start = s;
	while (is_variable_char(*s)) { s++; }
	return s-start;
}

size_t system_string_interpolate(const char* source, char* dest, size_t dest_len, string_interpolate_source_t vars)
{
	char* dest_end = dest+dest_len;
	for (;dest<dest_end;)
	{
		char c = *source++;
		if (!c) {
			*dest = 0;
			break;
		}
		if (c=='$')
		{
			// here source points to the first char in the variable name
			size_t variable_len = variable_length(source);
			if (variable_len)
			{
				size_t added = vars(source, variable_len, dest, dest_end-dest);
				dest += added;
				source += variable_len;
			}
		}
		else
		{
			*dest++ = c;
		}
	}
	*(dest_end-1) = 0;
	return dest_len-(dest_end-dest);
}

