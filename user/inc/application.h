/**
  ******************************************************************************
  * @file    application.h
  * @authors  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   User Application File Header
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "system_version.h"

#ifdef PARTICLE_PLATFORM
#include "platform_headers.h"
#endif


#include "particle_wiring.h"
#include "particle_wiring_cloud.h"
#include "particle_wiring_interrupts.h"
#include "particle_wiring_string.h"
#include "particle_wiring_print.h"
#include "particle_wiring_usartserial.h"
#include "particle_wiring_usbserial.h"
#include "particle_wiring_usbmouse.h"
#include "particle_wiring_usbkeyboard.h"
#include "particle_wiring_spi.h"
#include "particle_wiring_i2c.h"
#include "particle_wiring_servo.h"
#include "particle_wiring_wifi.h"
#include "particle_wiring_network.h"
#include "particle_wiring_client.h"
#include "particle_wiring_startup.h"
#include "particle_wiring_tcpclient.h"
#include "particle_wiring_tcpserver.h"
#include "particle_wiring_udp.h"
#include "particle_wiring_time.h"
#include "particle_wiring_tone.h"
#include "particle_wiring_eeprom.h"
#include "particle_wiring_version.h"
#include "particle_wiring_thread.h"
#include "fast_pin.h"
#include "string_convert.h"
#include "debug_output_handler.h"

// this was being implicitly pulled in by some of the other headers
// adding here for backwards compatibility.
#include "system_task.h"
#include "system_user.h"

#include "stdio.h"

using namespace particle;

#endif /* APPLICATION_H_ */
