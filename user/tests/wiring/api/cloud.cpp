/**
 ******************************************************************************
 * @file    spark.cpp
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

test(api_spark_variable) {
    
    int valueInt = 0;
    double valueDouble = 0;
    const char* UNUSED(constValueString) = "oh no!";
    char valueString[] = "oh yeah!";
    String valueSmartString(valueString);
    
    API_COMPILE(Spark.variable("myint", &valueInt, INT));
    
    API_COMPILE(Spark.variable("mydouble", &valueDouble, DOUBLE));
    
    API_COMPILE(Spark.variable("mystring", valueString, STRING));
        
    API_NO_COMPILE(Spark.variable("mystring", constValueString, STRING));        
}

test(api_spark_function) {
    int (*handler)(String) = NULL;    
    API_COMPILE(Spark.function("name", handler));    
}

test(api_spark_publish) {
    
    API_COMPILE(Spark.publish("public event name"));
    
    API_COMPILE(Spark.publish("public event name", "event data"));
    
    API_COMPILE(Spark.publish("public event name", "event data"));
    
    API_COMPILE(Spark.publish("public event name", "event data", 60));

    API_COMPILE(Spark.publish("public event name", "event data", 60, PUBLIC));

    API_COMPILE(Spark.publish("private event name", "event data", 60, PRIVATE));

    API_COMPILE(Spark.publish("public event name", PRIVATE));
    
    API_COMPILE(Spark.publish("public event name", "event data", PRIVATE));
    
    API_COMPILE(Spark.publish("public event name", PUBLIC));

    
    API_COMPILE(Spark.publish(String("public event name")));
    
    API_COMPILE(Spark.publish(String("public event name"), String("event data")));
    
    API_COMPILE(Spark.publish(String("public event name"), String("event data")));
    
    API_COMPILE(Spark.publish(String("public event name"), String("event data"), 60));

    API_COMPILE(Spark.publish(String("public event name"), String("event data"), 60, PUBLIC));

    API_COMPILE(Spark.publish(String("public event name"), String("event data"), 60, PRIVATE));
    
    API_COMPILE(Spark.publish(String("public event name"), PRIVATE));
    
    API_COMPILE(Spark.publish(String("public event name"), String("event data"), PRIVATE));
    
    API_COMPILE(Spark.publish(String("public event name"), PUBLIC));
    
}

test(api_spark_subscribe) {

    void (*handler)(const char *event_name, const char *data) = NULL;
    
    API_COMPILE(Spark.subscribe("name", handler));
    
    API_COMPILE(Spark.subscribe("name", handler, MY_DEVICES));
    
    API_COMPILE(Spark.subscribe("name", handler, "1234"));
    

    API_COMPILE(Spark.subscribe(String("name"), handler));
    
    API_COMPILE(Spark.subscribe(String("name"), handler, MY_DEVICES));
    
    API_COMPILE(Spark.subscribe(String("name"), handler, "1234"));
    
}

test(api_spark_sleep) {
    
    API_COMPILE(Spark.sleep(60));

    API_COMPILE(Spark.sleep(SLEEP_MODE_WLAN, 60));
    
    API_COMPILE(Spark.sleep(SLEEP_MODE_DEEP, 60));

    API_COMPILE(Spark.sleep(A0, CHANGE));
    API_COMPILE(Spark.sleep(A0, RISING));
    API_COMPILE(Spark.sleep(A0, FALLING));
    API_COMPILE(Spark.sleep(A0, FALLING, 20));
        
}

test(api_spark_connection) {
    bool connected = false;
    API_COMPILE(connected=Spark.connected());
    connected++;
    API_COMPILE(Spark.connect());
    API_COMPILE(Spark.disconnect());
    API_COMPILE(Spark.process());
}

test(api_spark_deviceID) {
    String id;
    API_COMPILE(id = Spark.deviceID());
}    
