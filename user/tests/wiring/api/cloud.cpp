/**
 ******************************************************************************
 * @file    spark.cpp
 * @authors Matthew McGowan
 * @date    13 January 2015
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

#include "testapi.h"

test(api_spark_variable) {

    int valueInt = 0;
    int32_t valueInt32 = 0;
    uint32_t valueUint32 = 0;
    double valueDouble = 0;
    const char* constValueString = "oh no!";
    char valueString[] = "oh yeah!";

    String valueSmartString(valueString);
    bool boolValue = true;
    uint8_t uint8Value = 1;

    API_COMPILE(Particle.variable("mybool", boolValue));
    API_COMPILE(Particle.variable("mybool", uint8Value));

    API_COMPILE(Particle.variable("myint", &valueInt, INT));

    API_COMPILE(Particle.variable("mydouble", &valueDouble, DOUBLE));

    API_COMPILE(Particle.variable("mystring", valueString, STRING));
    // This doesn't compile and shouldn't
    //API_COMPILE(Particle.variable("mystring", &valueString, STRING));

    API_NO_COMPILE(Particle.variable("mystring", constValueString, STRING));

    API_COMPILE(Particle.variable("mystring", &valueSmartString, STRING));

    API_COMPILE(Particle.variable("mystring", &valueInt32, INT));
    API_COMPILE(Particle.variable("mystring", &valueUint32, INT));

    API_NO_COMPILE(Particle.variable("mystring", valueUint32));
    API_COMPILE(Particle.variable("mystring", valueInt));
    API_COMPILE(Particle.variable("mystring", valueInt32));
    API_COMPILE(Particle.variable("mystring", valueUint32));

    API_COMPILE(Particle.variable("mystring", valueDouble));

    API_COMPILE(Particle.variable("mystring", valueString));
    API_COMPILE(Particle.variable("mystring", constValueString));
    API_COMPILE(Particle.variable("mystring", valueSmartString));

    // This should gives a compiler error about too long name
    //API_COMPILE(Particle.variable("mystring123456789", valueString));

}

test(api_spark_function) {
    int (*handler)(String) = NULL;

    API_COMPILE(Particle.function("name", handler));

    // This should gives a compiler error about too long name
    //API_COMPILE(Particle.function("superlongfunctionname", handler));

    // Length not checked until run time
    API_COMPILE(Particle.function(String("name"), handler));
    const char *longname = "superlongfunctionname";
    API_COMPILE(Particle.function(longname, handler));

    class MyClass {
      public:
        int handler(String arg) { return 0; }
    } myObj;
    API_COMPILE(Particle.function("name", &MyClass::handler, &myObj));
}

test(api_spark_publish) {
    // Particle.publish(const char*, const char*, ...)
    API_COMPILE(Particle.publish("event"));
    API_COMPILE(Particle.publish("event", PUBLIC));
    API_COMPILE(Particle.publish("event", PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish("event", PUBLIC | NO_ACK)); // traditional syntax

    API_COMPILE(Particle.publish("event", "data"));
    API_COMPILE(Particle.publish("event", "data", PUBLIC));
    API_COMPILE(Particle.publish("event", "data", PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish("event", "data", PUBLIC | NO_ACK));

    API_COMPILE(Particle.publish("event", "data", 60));
    API_COMPILE(Particle.publish("event", "data", 60, PUBLIC));
    API_COMPILE(Particle.publish("event", "data", 60, PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish("event", "data", 60, PUBLIC | NO_ACK));

    // Particle.publish(String, String, ...)
    API_COMPILE(Particle.publish(String("event")));
    API_COMPILE(Particle.publish(String("event"), PUBLIC));
    API_COMPILE(Particle.publish(String("event"), PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish(String("event"), PUBLIC | NO_ACK));

    API_COMPILE(Particle.publish(String("event"), String("data")));
    API_COMPILE(Particle.publish(String("event"), String("data"), PUBLIC));
    API_COMPILE(Particle.publish(String("event"), String("data"), PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish(String("event"), String("data"), PUBLIC | NO_ACK));

    API_COMPILE(Particle.publish(String("event"), String("data"), 60));
    API_COMPILE(Particle.publish(String("event"), String("data"), 60, PUBLIC));
    API_COMPILE(Particle.publish(String("event"), String("data"), 60, PUBLIC, NO_ACK));
    API_COMPILE(Particle.publish(String("event"), String("data"), 60, PUBLIC | NO_ACK));
}

test(api_spark_publish_vitals) {
    API_COMPILE(Particle.publishVitals()); // publish vitals immediately
    API_COMPILE(Particle.publishVitals(particle::NOW)); // publish vitals immediately
    API_COMPILE(Particle.publishVitals(0)); // disable periodic publishing
    API_COMPILE(Particle.publishVitals(5)); // publish vitals at 5 second intervals
}

test(api_spark_subscribe) {

    void (*handler)(const char *event_name, const char *data) = NULL;

    API_COMPILE(Particle.subscribe("name", handler));

    API_COMPILE(Particle.subscribe("name", handler, MY_DEVICES));

    API_COMPILE(Particle.subscribe("name", handler, "1234"));


    API_COMPILE(Particle.subscribe(String("name"), handler));

    API_COMPILE(Particle.subscribe(String("name"), handler, MY_DEVICES));

    API_COMPILE(Particle.subscribe(String("name"), handler, "1234"));

    class MyClass {
      public:
        void handler(const char *event_name, const char *data) { }
    } myObj;

    API_COMPILE(Particle.subscribe("name", &MyClass::handler, &myObj));

    API_COMPILE(Particle.subscribe("name", &MyClass::handler, &myObj, MY_DEVICES));

    API_COMPILE(Particle.subscribe("name", &MyClass::handler, &myObj, "1234"));

}

test(api_spark_sleep) {

    API_COMPILE(System.sleep(60));

    API_COMPILE(System.sleep(SLEEP_MODE_WLAN, 60));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP, 60));

    API_COMPILE(System.sleep(A0, CHANGE));
    API_COMPILE(System.sleep(A0, RISING));
    API_COMPILE(System.sleep(A0, FALLING));
    API_COMPILE(System.sleep(A0, FALLING, 20));

    API_COMPILE(System.sleep(SLEEP_MODE_DEEP));

}

test(api_spark_connection) {
    bool connected = false;
    API_COMPILE(connected=Particle.connected());
    connected++;
    API_COMPILE(Particle.connect());
    API_COMPILE(Particle.disconnect());
    API_COMPILE(Particle.process());

#if HAL_PLATFORM_CLOUD_UDP
    API_COMPILE(Particle.keepAlive(20 * 60));
#endif
}

test(api_spark_deviceID) {
    String id;
    API_COMPILE(id = Particle.deviceID());
}

test(api_spark_syncTime) {
    API_COMPILE(Particle.syncTime());
    API_COMPILE(Particle.syncTimeDone());
    API_COMPILE(Particle.syncTimePending());
    API_COMPILE(Particle.timeSyncedLast());
}
