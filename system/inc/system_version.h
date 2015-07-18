/**
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
#ifndef VERSION_H
#define	VERSION_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * This file is referenced from https://github.com/spark/firmware/wiki/Firmware-Release-Checklist
 */

const int PARTICLE_v040 = 0x00040000;
const int PARTICLE_v041 = 0x00040100;
const int PARTICLE_v042 = 0x00040200;
const int PARTICLE_v043 = 0x00040300;
const int PARTICLE_v044 = 0x00040400;

const int SYSTEM_VERSION = PARTICLE_v044;

#define SYSTEM_VERSION_STRING "0.4.4"


#ifdef	__cplusplus
}
#endif

#endif	/* VERSION_H */

