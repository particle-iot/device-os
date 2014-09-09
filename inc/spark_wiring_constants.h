/**
 ******************************************************************************
 * @file    spark_wiring.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Defines constants and types for the wiring API.
 ******************************************************************************
  Copyright (c) 2013-4 Spark Labs, Inc.  All rights reserved.

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


#ifndef SPARK_WIRING_CONSTANTS_H
#define	SPARK_WIRING_CONSTANTS_H


/*
* Basic variables
*/

#if !defined(min)
#   define min(a,b)                ((a)<(b)?(a):(b))
#endif
#if !defined(max)
#   define max(a,b)                ((a)>(b)?(a):(b))
#endif
#if !defined(constrain)
#   define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif
#if !defined(round)
#   define round(x)                ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif

#define HIGH 0x1
#define LOW 0x0

#define boolean bool

//#define NULL ((void *)0)
#define NONE ((uint8_t)0xFF)



#endif	/* SPARK_WIRING_CONSTANTS_H */

