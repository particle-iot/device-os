#include "catch.hpp"

#include "bytes2hexbuf.h"
#include <cstring>

TEST_CASE("bytes2hex_lower_") {
	SECTION("bytes are converted to lowercase hex") {
		uint8_t bytes[]  = { 0x01, 0xFF, 0xA0, 0xb9 };
		char out[4*2+1] = {0};
		char* result = bytes2hexbuf_lower_case(bytes, 4, out);
		REQUIRE(!strcmp(out, "01ffa0b9"));
		REQUIRE(result==out);
	}
}
