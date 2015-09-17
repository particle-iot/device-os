
#include "testapi.h"

test(time_format)
{
    const char* format;
    API_COMPILE(Time.timeStr());
    API_COMPILE(Time.timeStr(1234));
    API_COMPILE(Time.format(TIME_FORMAT_DEFAULT));
    API_COMPILE(Time.format(TIME_FORMAT_ISO8601_FULL));
    API_COMPILE(Time.format(1234, TIME_FORMAT_ISO8601_FULL));
    API_COMPILE(Time.setFormat(TIME_FORMAT_ISO8601_FULL));
    API_COMPILE((void)(format=Time.getFormat()));

    API_COMPILE(Time.format(1234, "abcd"))

}
