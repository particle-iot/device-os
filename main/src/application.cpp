/**
 ******************************************************************************
 * @file    application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/  
#include "application.h"
#include "Ymodem\Ymodem.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

/* This function is called once at start up ----------------------------------*/
void setup()
{
    Serial.begin(9600);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    //This will run in a loop
    if(Serial.available())
    {
        char c = Serial.read();
        if('f' == c)
        {
            System.serialFirmwareUpdate(&Serial);
        }
        else if('b' == c)
        {
            System.bootloader();
        }
        else if('r' == c)
        {
            System.factoryReset();
        }
    }
}
