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

// 1.2.0-alpha.1 < 1.2.0-alpha.63 < 1.2.0-beta.1 < 1.2.0-beta.63 < 1.2.0-rc.1 < 1.2.0-rc.2 < 1.2.0
#define SYSTEM_VERSION_ALPHA(x, y, z, p) ((((x) & 0xFF) << 24) | \
                                          (((y) & 0xFF) << 16) | \
                                          (((z) & 0xFF) <<  8) | \
                                          (((p) & 0x3F) | 0x00))
#define SYSTEM_VERSION_BETA(x, y, z, p)  ((((x) & 0xFF) << 24) | \
                                          (((y) & 0xFF) << 16) | \
                                          (((z) & 0xFF) <<  8) | \
                                          (((p) & 0x3F) | 0x40))
#define SYSTEM_VERSION_RC(x, y, z, p)    ((((x) & 0xFF) << 24) | \
                                          (((y) & 0xFF) << 16) | \
                                          (((z) & 0xFF) <<  8) | \
                                          (((p) & 0x3F) | 0x80))
// (((p)) | 0xC0)) is reserved (0xC1 - 0xFE), unit-tests will fail if this range is detected.
#define SYSTEM_VERSION_DEFAULT(x, y, z)  ((((x) & 0xFF) << 24) | \
                                          (((y) & 0xFF) << 16) | \
                                          (((z) & 0xFF) <<  8) | \
                                           ((0xFF)))

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
#define SYSTEM_VERSION_v054     0x00050400
#define SYSTEM_VERSION_v055     0x00050500
#define SYSTEM_VERSION_v060RC1  0x00060001
#define SYSTEM_VERSION_v060RC2  0x00060002
#define SYSTEM_VERSION_v060     0x00060000
#define SYSTEM_VERSION_v061RC1  0x00060101
#define SYSTEM_VERSION_v061RC2  0x00060102
#define SYSTEM_VERSION_v061     0x00060100
#define SYSTEM_VERSION_v062RC1  0x00060201
#define SYSTEM_VERSION_v062RC2  0x00060202
#define SYSTEM_VERSION_v062     0x00060200
#define SYSTEM_VERSION_v063     0x00060300
#define SYSTEM_VERSION_v064     0x00060400
#define SYSTEM_VERSION_v070RC1  0x00070001
#define SYSTEM_VERSION_v070RC2  0x00070002
#define SYSTEM_VERSION_v070RC3  0x00070003
#define SYSTEM_VERSION_v070RC4  0x00070004
#define SYSTEM_VERSION_v070RC5  0x00070005
#define SYSTEM_VERSION_v070RC6  0x00070006
#define SYSTEM_VERSION_v070RC7  0x00070007
#define SYSTEM_VERSION_v070     0x00070000
#define SYSTEM_VERSION_v080RC1  0x00080001
#define SYSTEM_VERSION_v080RC2  0x00080002
#define SYSTEM_VERSION_v080RC3  0x00080003
#define SYSTEM_VERSION_v080RC4  0x00080004
#define SYSTEM_VERSION_v080RC5  0x00080005
#define SYSTEM_VERSION_v080RC6  0x00080006
#define SYSTEM_VERSION_v080RC7  0x00080007
#define SYSTEM_VERSION_v080RC8  0x00080008
#define SYSTEM_VERSION_v080RC9  0x00080009
#define SYSTEM_VERSION_v080RC10 0x0008000a
#define SYSTEM_VERSION_v080RC11 0x0008000b
#define SYSTEM_VERSION_v080RC12 0x0008000c
#define SYSTEM_VERSION_v080RC13 0x0008000d
#define SYSTEM_VERSION_v080RC14 0x0008000e
#define SYSTEM_VERSION_v080RC15 0x0008000f
#define SYSTEM_VERSION_v080RC16 0x00080010
#define SYSTEM_VERSION_v080RC17 0x00080011
#define SYSTEM_VERSION_v080RC18 0x00080012
#define SYSTEM_VERSION_v080RC19 0x00080013
#define SYSTEM_VERSION_v080RC20 0x00080014
#define SYSTEM_VERSION_v080RC21 0x00080015
#define SYSTEM_VERSION_v080RC22 0x00080016
#define SYSTEM_VERSION_v080RC23 0x00080017
#define SYSTEM_VERSION_v080RC24 0x00080018
#define SYSTEM_VERSION_v080RC25 0x00080019
#define SYSTEM_VERSION_v080RC26 0x0008001a
#define SYSTEM_VERSION_v080RC27 0x0008001b
#define SYSTEM_VERSION_v090RC1  0x00090001
#define SYSTEM_VERSION_v090RC2  0x00090002
#define SYSTEM_VERSION_v090RC3  0x00090003
#define SYSTEM_VERSION_v100     0x01000000
#define SYSTEM_VERSION_v101RC1  0x01000101
#define SYSTEM_VERSION_v101     0x01000100
#define SYSTEM_VERSION_v110RC1  0x01010001
#define SYSTEM_VERSION_v110RC2  0x01010002
#define SYSTEM_VERSION_v110     0x01010000
#define SYSTEM_VERSION_v111RC1  0x01010101
#define SYSTEM_VERSION_v111     0x01010100
#define SYSTEM_VERSION_v120ALPHA1  SYSTEM_VERSION_ALPHA(1, 2, 0, 1)
#define SYSTEM_VERSION_v120BETA1    SYSTEM_VERSION_BETA(1, 2, 0, 1)
#define SYSTEM_VERSION_v120RC1        SYSTEM_VERSION_RC(1, 2, 0, 1)
#define SYSTEM_VERSION_v121RC1        SYSTEM_VERSION_RC(1, 2, 1, 1)
#define SYSTEM_VERSION_v121RC2        SYSTEM_VERSION_RC(1, 2, 1, 2)
#define SYSTEM_VERSION_v121RC3        SYSTEM_VERSION_RC(1, 2, 1, 3)
#define SYSTEM_VERSION_v121         SYSTEM_VERSION_DEFAULT(1, 2, 1)
#define SYSTEM_VERSION SYSTEM_VERSION_v121

/**
 * Previously we would set the least significant byte to 0 for the final release, but to make
 * version comparisons simpler, the final version LSB is now 0xFF
 * 1.2.0-alpha.1 < 1.2.0-alpha.63 < 1.2.0-beta.1 < 1.2.0-beta.63 < 1.2.0-rc.1 < 1.2.0-rc.2 < 1.2.0
 */

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
#define SYSTEM_VERSION_054
#define SYSTEM_VERSION_055
#define SYSTEM_VERSION_060RC1
#define SYSTEM_VERSION_060RC2
#define SYSTEM_VERSION_060
#define SYSTEM_VERSION_061RC1
#define SYSTEM_VERSION_061RC2
#define SYSTEM_VERSION_061
#define SYSTEM_VERSION_062RC1
#define SYSTEM_VERSION_062RC2
#define SYSTEM_VERSION_062
#define SYSTEM_VERSION_063
#define SYSTEM_VERSION_064
#define SYSTEM_VERSION_070RC1
#define SYSTEM_VERSION_070RC2
#define SYSTEM_VERSION_070RC3
#define SYSTEM_VERSION_070RC4
#define SYSTEM_VERSION_070RC5
#define SYSTEM_VERSION_070RC6
#define SYSTEM_VERSION_070RC7
#define SYSTEM_VERSION_070
#define SYSTEM_VERSION_080RC1
#define SYSTEM_VERSION_080RC2
#define SYSTEM_VERSION_080RC3
#define SYSTEM_VERSION_080RC4
#define SYSTEM_VERSION_080RC5
#define SYSTEM_VERSION_080RC6
#define SYSTEM_VERSION_080RC7
#define SYSTEM_VERSION_080RC8
#define SYSTEM_VERSION_080RC9
#define SYSTEM_VERSION_080RC10
#define SYSTEM_VERSION_080RC11
#define SYSTEM_VERSION_080RC12
#define SYSTEM_VERSION_080RC13
#define SYSTEM_VERSION_080RC14
#define SYSTEM_VERSION_080RC15
#define SYSTEM_VERSION_080RC16
#define SYSTEM_VERSION_080RC17
#define SYSTEM_VERSION_080RC18
#define SYSTEM_VERSION_080RC19
#define SYSTEM_VERSION_080RC20
#define SYSTEM_VERSION_080RC21
#define SYSTEM_VERSION_080RC22
#define SYSTEM_VERSION_080RC23
#define SYSTEM_VERSION_080RC24
#define SYSTEM_VERSION_080RC25
#define SYSTEM_VERSION_080RC26
#define SYSTEM_VERSION_080RC27
#define SYSTEM_VERSION_090RC1
#define SYSTEM_VERSION_090RC2
#define SYSTEM_VERSION_090RC3
#define SYSTEM_VERSION_100
#define SYSTEM_VERSION_101RC1
#define SYSTEM_VERSION_101
#define SYSTEM_VERSION_110RC1
#define SYSTEM_VERSION_110RC2
#define SYSTEM_VERSION_110
#define SYSTEM_VERSION_111RC1
#define SYSTEM_VERSION_111
#define SYSTEM_VERSION_120ALPHA1
#define SYSTEM_VERSION_120BETA1
#define SYSTEM_VERSION_120RC1
#define SYSTEM_VERSION_121RC1
#define SYSTEM_VERSION_121RC2
#define SYSTEM_VERSION_121RC3
#define SYSTEM_VERSION_121

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

