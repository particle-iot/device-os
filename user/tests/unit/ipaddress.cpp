
#include "catch.hpp"

#include "spark_wiring_ipaddress.h"

TEST_CASE("Can Construct From Uint32") {

    IPAddress ip(1<<24 | 2<<16 | 3<<8 | 4);

    CHECK(ip[3]==4);
    CHECK(ip[2]==3);
    CHECK(ip[1]==2);
    CHECK(ip[0]==1);

}
