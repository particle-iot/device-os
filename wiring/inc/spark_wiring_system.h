/**
 ******************************************************************************
 * @file    spark_wiring_system.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
 ******************************************************************************
  Copyright (c) 2013-2015 Spark Labs, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_SYSTEM_H
#define	SPARK_WIRING_SYSTEM_H

#include "system_mode.h"
#include "system_update.h"
#include "system_cloud.h"

typedef enum
{
    SLEEP_MODE_WLAN = 0, SLEEP_MODE_DEEP = 1
} Spark_Sleep_TypeDef;


class Stream;

class SystemClass {
public:

    SystemClass(System_Mode_TypeDef mode = DEFAULT) {
        set_system_mode(mode);
    }

    static System_Mode_TypeDef mode(void) {
        return system_mode();
    }

    static bool serialSaveFile(Stream *serialObj, uint32_t sFlashAddress) {
        return serialSaveFile(serialObj, sFlashAddress);
    }

    static bool serialFirmwareUpdate(Stream *serialObj) {
        return system_serialFirmwareUpdate(serialObj);
    }
    static void factoryReset(void);
    static void bootloader(void);
    static void reset(void);

    static void sleep(Spark_Sleep_TypeDef sleepMode, long seconds);
    static void sleep(long seconds) { sleep(SLEEP_MODE_WLAN, seconds); }    
    static void sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds=0);
    static String deviceID(void) { return spark_deviceID(); }
    
};

extern SystemClass System;

#define SYSTEM_MODE(mode)  SystemClass SystemMode(mode);


#endif	/* SPARK_WIRING_SYSTEM_H */

