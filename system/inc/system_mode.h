/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSTEM_MODE_H
#define	SYSTEM_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  DEFAULT=0, AUTOMATIC = 1, SEMI_AUTOMATIC = 2, MANUAL = 3, SAFE_MODE=4
} System_Mode_TypeDef;

void set_system_mode(System_Mode_TypeDef mode);
System_Mode_TypeDef system_mode();

#ifdef __cplusplus
}
#endif

#endif	/* SYSTEM_MODE_H */

