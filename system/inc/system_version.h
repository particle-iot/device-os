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

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * This file is referenced from https://github.com/spark/firmware/wiki/Firmware-Release-Checklist
 */

#define SYSTEM_VERSION_v040  0x00040000
#define SYSTEM_VERSION_v041  0x00040100
#define SYSTEM_VERSION_v042  0x00040200
#define SYSTEM_VERSION_v043  0x00040300
#define SYSTEM_VERSION_v044  0x00040400
#define SYSTEM_VERSION_v045  0x00040500
#define SYSTEM_VERSION_v046  0x00040600
#define SYSTEM_VERSION_v047  0x00040700
#define SYSTEM_VERSION_v048  0x00040800		// photon RTM Dec 2015
#define SYSTEM_VERSION_v048RC6  0x00040806	// electron RTM Jan 2016
#define SYSTEM_VERSION_v049  0x00040900
#define SYSTEM_VERSION_v050  0x00050000
#define SYSTEM_VERSION_v060_rc1  0x00060001

#define SYSTEM_VERSION  SYSTEM_VERSION_v060_rc1

#define SYSTEM_VERSION_040
#define SYSTEM_VERSION_041
#define SYSTEM_VERSION_042
#define SYSTEM_VERSION_043
#define SYSTEM_VERSION_044
#define SYSTEM_VERSION_045
#define SYSTEM_VERSION_046
#define SYSTEM_VERSION_047
#define SYSTEM_VERSION_048
#define SYSTEM_VERSION_048RC6
#define SYSTEM_VERSION_049
#define SYSTEM_VERSION_050
#define SYSTEM_VERSION_060

typedef struct __attribute__((packed)) SystemVersionInfo
{
    uint16_t size = sizeof(SystemVersionInfo);
    uint16_t reserved;      // use this if you need to.
    uint32_t versionNumber;
    char     versionString[20];



} SystemVersionInfo;

int system_version_info(SystemVersionInfo* target, void* reserved);

#ifdef	__cplusplus
}
#endif

#endif	/* VERSION_H */

