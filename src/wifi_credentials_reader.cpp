/**
  ******************************************************************************
  * @file    wifi_credentials_reader.cpp
  * @author  Zachary Crockett and Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief  
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

#include "wifi_credentials_reader.h"

WiFiCredentialsReader::WiFiCredentialsReader(ConnectCallback connect_callback)
{
  this->connect_callback = connect_callback;
  serial.begin(9600);
}

void WiFiCredentialsReader::read(void)
{
  if (0 < serial.available())
  {
    char c = serial.read();
    if ('w' == c)
    {
      memset(ssid, 0, 33);
      memset(password, 0, 65);
      memset(security_type_string, 0, 2);

      print("SSID: ");
      read_line(ssid, 32);

      do {
        print("Security 0=unsecured, 1=WEP, 2=WPA, 3=WPA2: ");
        read_line(security_type_string, 1);
      } while ('0' > security_type_string[0] || '3' < security_type_string[0]);

      if ('1' == security_type_string[0]) {
        print("\r\n ** Even though the CC3000 supposedly supports WEP,");
        print("\r\n ** we at Spark have never seen it work.");
        print("\r\n ** If you control the network, we recommend changing it to WPA2.\r\n");
      }

      unsigned long security_type = security_type_string[0] - '0';
      if (0 < security_type) {
        print("Password: ");
        read_line(password, 64);
      }

      print("Thanks! Wait about 7 seconds while I save those credentials...\r\n\r\n");

      connect_callback(ssid, password, security_type);

      print("Awesome. Now we'll connect!\r\n\r\n");
      print("If you see a pulsing cyan light, your Spark Core\r\n");
      print("has connected to the Cloud and is ready to go!\r\n\r\n");
      print("If your LED flashes red or you encounter any other problems,\r\n");
      print("visit https://www.spark.io/support to debug.\r\n\r\n");
      print("    Spark <3 you!\r\n\r\n");
    }
    else if ('i' == c)
    {
      char id[12];
      memcpy(id, (char *)ID1, 12);
      print("Your core id is ");
      char hex_digit;
      for (int i = 0; i < 12; ++i)
      {
        hex_digit = 48 + (id[i] >> 4);
        if (57 < hex_digit)
          hex_digit += 39;
        serial.write(hex_digit);
        hex_digit = 48 + (id[i] & 0xf);
        if (57 < hex_digit)
          hex_digit += 39;
        serial.write(hex_digit);
      }
      print("\r\n");
    }
  }
}


/* private methods */

void WiFiCredentialsReader::print(const char *s)
{
  for (size_t i = 0; i < strlen(s); ++i)
  {
    serial.write(s[i]);
    Delay(1); // ridonkulous, but required
  }
}

void WiFiCredentialsReader::read_line(char *dst, int max_len)
{
  char c = 0, i = 0;
  while (1)
  {
    if (0 < serial.available())
    {
      c = serial.read();

      if (i == max_len || c == '\r' || c == '\n')
      {
        *dst = '\0';
        break;
      }

      if (c == 8 || c == 127)
      {
    	//for backspace or delete
    	if (i > 0)
    	{
          --dst;
          --i;
    	}
    	else
    	{
    	  continue;
    	}
      }
      else
      {
        *dst++ = c;
        ++i;
      }

      serial.write(c);
    }
  }
  print("\r\n");
  while (0 < serial.available())
    serial.read();
}
