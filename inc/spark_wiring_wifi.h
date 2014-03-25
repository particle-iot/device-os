/**
 ******************************************************************************
 * @file    spark_wiring_wifi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    7-Mar-2014
 * @brief   Header for spark_wiring_wifi.cpp module
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#ifndef __SPARK_WIRING_WIFI_H
#define __SPARK_WIRING_WIFI_H

#include "spark_wiring.h"

typedef enum
{
	WIFI_OFF = 0, WIFI_CONNECTING = 1, WIFI_ON = 2
} WiFi_Status_TypeDef;

class WiFiClass
{
public:
	WiFiClass() {}
    ~WiFiClass() {}

	static void on(void);
	static void off(void);
	static WiFi_Status_TypeDef status(void);
};

extern WiFiClass WiFi;

#endif
