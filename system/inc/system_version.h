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

#define SYSTEM_VERSION_v040     0x00040000
#define SYSTEM_VERSION_v041     0x00040100
#define SYSTEM_VERSION_v042     0x00040200
#define SYSTEM_VERSION_v043     0x00040300
#define SYSTEM_VERSION_v044     0x00040400
#define SYSTEM_VERSION_v045     0x00040500
#define SYSTEM_VERSION_v046     0x00040600
#define SYSTEM_VERSION_v047     0x00040700
#define SYSTEM_VERSION_v048     0x00040800	// photon RTM Dec 2015
#define SYSTEM_VERSION_v048RC6  0x00040806	// electron RTM Jan 2016
#define SYSTEM_VERSION_v049     0x00040900
#define SYSTEM_VERSION_v050     0x00050000
#define SYSTEM_VERSION_v051     0x00050100
#define SYSTEM_VERSION_v052RC1  0x00050201
#define SYSTEM_VERSION_v052     0x00050200
#define SYSTEM_VERSION_v053RC1  0x00050301
#define SYSTEM_VERSION_v053RC2  0x00050302
#define SYSTEM_VERSION_v053RC3  0x00050303
#define SYSTEM_VERSION_v053     0x00050300
#define SYSTEM_VERSION_v060RC1  0x00060001
#define SYSTEM_VERSION_v060RC2  0x00060002
#define SYSTEM_VERSION_v060     0x00060000
#define SYSTEM_VERSION_v061RC1  0x00060101
#define SYSTEM_VERSION_v061RC2  0x00060102
#define SYSTEM_VERSION_v061     0x00060100
#define SYSTEM_VERSION_v062RC1  0x00060201
#define SYSTEM_VERSION_v062RC2  0x00060202
#define SYSTEM_VERSION_v062     0x00060200
#define SYSTEM_VERSION_v070RC1  0x00070001
#define SYSTEM_VERSION_v070RC2  0x00070002

#define SYSTEM_VERSION  SYSTEM_VERSION_v070RC2

/**
 * For Library/App creators. Can be used to ensure features/api's are present.
 * Add a new define for every XYZRCN or final XYZ release.
 */
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
#define SYSTEM_VERSION_051
#define SYSTEM_VERSION_052RC1
#define SYSTEM_VERSION_052
#define SYSTEM_VERSION_053RC1
#define SYSTEM_VERSION_053RC2
#define SYSTEM_VERSION_053RC3
#define SYSTEM_VERSION_053
#define SYSTEM_VERSION_060RC1
#define SYSTEM_VERSION_060RC2
#define SYSTEM_VERSION_060
#define SYSTEM_VERSION_061RC1
#define SYSTEM_VERSION_061RC2
#define SYSTEM_VERSION_061
#define SYSTEM_VERSION_062RC1
#define SYSTEM_VERSION_062RC2
#define SYSTEM_VERSION_062
#define SYSTEM_VERSION_070RC1
#define SYSTEM_VERSION_070RC2

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

