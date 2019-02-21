#include "catch.hpp"
#include <string.h>
#include "system_string_interpolate.h"

size_t id_interpolate(const char* var, size_t var_len, char* dest, size_t dest_len)
{
	if (memcmp("id", var, var_len)==0)
	{
		if (dest_len>4)
		{
			memcpy(dest, "ABCD", 4);
			return 4;
		}
	}
	return 0;
}

SCENARIO("attempting to interpolate into a smaller buffer does not interpolate")
{
	GIVEN("an interpolation function")
	{
		// this isn't stricly correct - if the buffer size "cuts" a variable substution
		// in half, then the entire variable substitution is discarded and characters appended
		// after the varaible.
		char buf[5];
		size_t written;
		string_interpolate_source_t fn = id_interpolate;
		WHEN("a string is interpolated into a too small buffer")
		{
			written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
			THEN("the result is incomplete, but the buffer is not overflowed")
			{
				REQUIRE(!strcmp("abc.", buf));
				REQUIRE(written==5);
			}
		}
	}
}

SCENARIO("can interpolate a simple ID into a larger buffer")
{
	GIVEN("an interpolation function")
	{
		char buf[40];
		size_t written;
		string_interpolate_source_t fn = id_interpolate;
		WHEN("a string is interpolated into a larger buffer")
		{
			written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
			THEN("the variable is interpolated and non-interpolated parts are correctly copied")
			{
				REQUIRE(!strcmp(buf,"abcABCD.123"));
				REQUIRE(written==strlen(buf));
			}
		}
	}
}

SCENARIO("can interpolate a simple ID into a buffer that is exactly the size required")
{
	GIVEN("an interpolation function")
	{
		char buf[12];
		size_t written;
		string_interpolate_source_t fn = id_interpolate;
		WHEN("a string is interpolated into a larger buffer")
		{
			written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
			THEN("the variable is interpolated and non-interpolated parts are correctly copied")
			{
				REQUIRE(!strcmp(buf,"abcABCD.123"));
				REQUIRE(written==strlen(buf));
			}
		}
	}
}


