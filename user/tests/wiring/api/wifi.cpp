/**
 ******************************************************************************
 * @file    cloud.cpp
 * @authors Matthew McGowan
 * @date    13 January 2015
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

#include "testapi.h"

test(api_wifi_resolve) {
    
    API_COMPILE(WiFi.resolve(String("abc.def.com")));
    API_COMPILE(WiFi.resolve("abc.def.com"));
}

test(api_wifi_selectantennt) {
    
    API_COMPILE(WiFi.selectAntenna(ANT_AUTO));
    API_COMPILE(WiFi.selectAntenna(ANT_INTERNAL));
    API_COMPILE(WiFi.selectAntenna(ANT_EXTERNAL));
    
}
    

