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

class Stream;

class SystemClass {

public:
  SystemClass(System_Mode_TypeDef mode=AUTOMATIC) {
      set_system_mode(mode);
  }
  static System_Mode_TypeDef mode(void) {
      return system_mode();
  }
  static bool serialSaveFile(Stream *serialObj, uint32_t sFlashAddress) { return serialSaveFile(serialObj, sFlashAddress); }
  static bool serialFirmwareUpdate(Stream *serialObj) { return system_serialFirmwareUpdate(serialObj); }
  static void factoryReset(void);
  static void bootloader(void);
  static void reset(void);
    
};

extern SystemClass System;

#define SYSTEM_MODE(mode)  SystemClass SystemMode(mode);


#endif	/* SPARK_WIRING_SYSTEM_H */

