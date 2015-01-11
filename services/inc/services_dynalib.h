/**
 ******************************************************************************
 * @file    services-dynalib.h
 * @author  Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

#ifndef SERVICES_DYNALIB_H
#define	SERVICES_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(services)
DYNALIB_FN(services,LED_SetRGBColor,0)
DYNALIB_FN(services,LED_SetSignalingColor,1)
DYNALIB_FN(services,LED_Signaling_Start,2)
DYNALIB_FN(services,LED_Signaling_Stop,3)
DYNALIB_FN(services,LED_SetBrightness,4)
DYNALIB_FN(services,LED_SetBrightness,5)
DYNALIB_FN(services,LED_RGB_Get,6)
DYNALIB_FN(services,LED_Init,7)
DYNALIB_FN(services,LED_On,8)
DYNALIB_FN(services,LED_Off,9)
DYNALIB_FN(services,LED_Toggle,10)
DYNALIB_FN(services,LED_Fade,11)
DYNALIB_FN(services,Get_LED_Brightness,12)        
DYNALIB_END(services)        



#endif	/* SERVICES_DYNALIB_H */

