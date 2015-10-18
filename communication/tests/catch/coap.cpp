
#include "catch.hpp"
#include "coap.h"


SCENARIO("CoAP::code")
{
    CoAP coap;

    unsigned char msg[2];
    
    msg[1] = 0x00;
    REQUIRE(coap.code(msg)==CoAPCode::EMPTY);

    msg[1] = 0x1;
    REQUIRE(coap.code(msg)==CoAPCode::GET);

    msg[1] = 0x2;
    REQUIRE(coap.code(msg)==CoAPCode::POST);

    msg[1] = 0x3;
    REQUIRE(coap.code(msg)==CoAPCode::PUT);

    msg[1] = 0x45;
    REQUIRE(coap.code(msg)==CoAPCode::CONTENT);

    msg[1] = 10;
    REQUIRE(coap.code(msg)==CoAPCode::ERROR);

}
