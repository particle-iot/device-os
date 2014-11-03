
#include "application.h"
#include "unit-test/unit-test.h"

test(Spark_Publish_Silently_Fails_When_Not_Connected) {
    Spark.disconnect();
    SPARK_WLAN_Loop();    
    Spark.publish("hello", "world", 60, PRIVATE);
}


