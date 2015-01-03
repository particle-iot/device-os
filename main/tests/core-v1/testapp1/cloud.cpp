
#include "application.h"
#include "unit-test/unit-test.h"

test(Spark_Publish_Silently_Fails_When_Not_Connected) {
    Spark.disconnect();
    Spark.process();    
    Spark.publish("hello", "world", 60, PRIVATE);
}


