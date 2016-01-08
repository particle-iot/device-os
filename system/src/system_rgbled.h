/**
 ******************************************************************************
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

#pragma once

#include "system_task.h"
#include "rgbled.h"

// This is temporary until we can get scoped LED management in place.
class RGBLEDState
{
	volatile uint8_t fade;
	volatile uint8_t override;
	volatile uint32_t color;

public:

	void save()
	{
		fade = SPARK_LED_FADE;
		override = LED_RGB_IsOverRidden();
		color = LED_GetColor(override ? 1 : 0, NULL);
	}

	void restore()
	{
		SPARK_LED_FADE = fade;
		if (override)
		{
			LED_SetSignalingColor(color);
			LED_Signaling_Start();
			LED_On(LED_RGB);
		}
		else
		{
			LED_SetRGBColor(color);
			LED_Signaling_Stop();
		}
	}

};

