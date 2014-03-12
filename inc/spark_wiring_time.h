/**
 ******************************************************************************
 * @file    spark_wiring_time.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Header for spark_wiring_time.cpp module
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

#ifndef __SPARK_WIRING_TIME_H
#define __SPARK_WIRING_TIME_H

#include "spark_wiring.h"

#ifdef __cplusplus
extern "C"
{
#endif

void Time_Init(void);
void Time_DateUpdate(void);

#ifdef __cplusplus
}
#endif

class TimeClass {
public:
	// Arduino time and date functions
	static int     hour();            			// current hour
	static int     hourFormat12();    			// current hour in 12 hour format
	static uint8_t isAM();            			// returns true if time now is AM
	static uint8_t isPM();            			// returns true if time now is PM
	static int     minute();          			// current minute
	static int     second();          			// current seconds
	static int     day();             			// current day
	static int     month();           			// current month
	static int     year();            			// current four digit year
	static void    setTime(uint32_t datetime);	// set the given time as system time
};

extern TimeClass Time;	//eg. usage: Time.day();

#endif
