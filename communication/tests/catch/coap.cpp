
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

SCENARIO("CoAP::header")
{
	GIVEN("A CoAP instance and a 4-byte buffer")
	{
		CoAP coap;
		uint8_t buf[4];

		WHEN("header is called with arguments")
		{
			int size = coap.header(buf, CoAPType::CON, 3, CoAPCode::CONTINUE, 0x1234);
			THEN("The buffer is filled out correctly")
			{
				REQUIRE(buf[0]==0x43);
				REQUIRE(buf[1]==40);
				REQUIRE(buf[2]==0x12);
				REQUIRE(buf[3]==0x34);
			}
		}
	}

}

SCENARIO("CoAP::path is retrieved from a CoAP message")
{
	GIVEN("a simple message")
	{
		CoAP coap;
		unsigned char msg[] = { 0x40, 40, 0x12, 0x34, 0x91, 'Z', 0 };
		WHEN("requesting the path")
		{
			const unsigned char* path = coap.path(msg);
			THEN("the result is the path")
			{
				REQUIRE(path==msg+5);
			}
		}
	}
}

