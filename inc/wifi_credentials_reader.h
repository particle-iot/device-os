/**
  ******************************************************************************
  * @file    wifi_credentials_reader.h
  * @author  Zachary Crockett and Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   header for wifi_credentials_reader.cpp
  ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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
#include <string.h>
#include "spark_wiring_usbserial.h"

typedef void (*ConnectCallback)(const char *ssid,
                                const char *password,
                                unsigned long security_type);

class WiFiCredentialsReader
{
  public:
    WiFiCredentialsReader(ConnectCallback connect_callback);
    void read(void);

  private:
    USBSerial serial;
    ConnectCallback connect_callback;
    char ssid[33];
    char password[65];
    char security_type_string[2];

    void print(const char *s);
    void read_line(char *dst, int max_len);
};
