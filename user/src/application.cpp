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

SYSTEM_MODE(SEMI_AUTOMATIC);

extern void test_get_semaphore_breaks_serial_printing(void);

/* This function is called once at start up ----------------------------------*/
void setup()
{
    pinMode(D7, OUTPUT);
    Serial.begin(9600);
    while (!Serial.available());
}

bool passed(uint32_t modulo, uint32_t& last) {
    if (millis()/modulo > last/modulo) {
        last = millis();
        return true;
    }
    return false;
}

/* This function loops forever --------------------------------------------*/
void loop()
{    
    //This will run in a loop
    char c = 0;
    if(Serial.available())
    {
        c = Serial.read();        
    }
    
    static uint32_t last = 0;
    
    if (passed(5000, last)) {
        c = 'g';
    }
    
    switch(c)
    {
    case 'g':
        Serial.println("before wiced_rtos_get_semaphore() call");
        //delay a while to get the above printed
        delay(1000);
        test_get_semaphore_breaks_serial_printing();
        //due to serial issue, the below is not printed
        Serial.println("after wiced_rtos_get_semaphore() call");
        break;

    case 'p':
        Serial.println("Serial check...OK");
        break;

    case 'w':
        WiFi.off();
        break;

    case 'W':
        WiFi.on();
        break;

    case 'n':
        WiFi.disconnect();
        break;

    case 'N':
        WiFi.connect();
        break;

    case 's':
        Spark.disconnect();
        break;

    case 'S':
        Spark.connect();
        break;

    case 'm':
        Serial.print("Your core MAC address is\r\n");
        WLanConfig ip_config;
        wlan_fetch_ipconfig(&ip_config);
        uint8_t* addr = ip_config.uaMacAddr;
        Serial.print(bytes2hex(addr++, 1).c_str());
        for (int i = 1; i < 6; i++)
        {
            Serial.print(":");
            Serial.print(bytes2hex(addr++, 1).c_str());
        }
        Serial.print("\r\n");
        break;
    }
    

    delay(50);
    digitalWrite(D7, HIGH);
    delay(50);
    digitalWrite(D7, LOW);
    Serial.print('.');
}
