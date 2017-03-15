#ifndef TEST_TOOLS_CATCH_H
#define TEST_TOOLS_CATCH_H

#define CATCH_CONFIG_PREFIX_ALL
#include <catch.hpp>

// Non-prefixed aliases for some typical macros that don't clash with the firmware
#define CHECK(...) CATCH_CHECK(__VA_ARGS__)
#define CHECK_FALSE(...) CATCH_CHECK_FALSE(__VA_ARGS__)
#define REQUIRE(...) CATCH_REQUIRE(__VA_ARGS__)
#define REQUIRE_FALSE(...) CATCH_REQUIRE_FALSE(__VA_ARGS__)
#define FAIL(...) CATCH_FAIL(__VA_ARGS__)
#define TEST_CASE(...) CATCH_TEST_CASE(__VA_ARGS__)
#define SECTION(...) CATCH_SECTION(__VA_ARGS__)

#endif // TEST_TOOLS_CATCH_H
