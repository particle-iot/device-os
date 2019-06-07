#include "tools/catch.h"
#include "dynalib_abi.h"


struct struct_contains_test {
    uint16_t size;
    uint32_t item1;
    uint32_t item2;
};


TEST_CASE("dynalib abi") {

    constexpr struct_contains_test size2 { .size = 2 };
    constexpr struct_contains_test size4 { .size = 4 };
    constexpr struct_contains_test size6 { .size = 6 };
    constexpr struct_contains_test size8 { .size = 8 };

     SECTION("offset_of") {
// this doesn't compile on clang on OSX Apple LLVM version 10.0.0 (clang-1000.10.44.4)
 //         constexpr auto offsetItem1 = offset_of(&struct_contains_test::item1);
//         static_assert(offsetItem1==4, "expected item1 at offset 4");
         REQUIRE(offset_of(&struct_contains_test::size)==0);
         REQUIRE(offset_of(&struct_contains_test::item1)==4);
         REQUIRE(offset_of(&struct_contains_test::item2)==8);
     }

     SECTION("dynamic size") {
         REQUIRE(dynamic_size(size2)==2);
         REQUIRE(dynamic_size(size4)==4);
         REQUIRE(dynamic_size(size6)==6);
     }

     SECTION("struct_has") {
         REQUIRE(!struct_contains(size2, &struct_contains_test::item1));
         REQUIRE(!struct_contains(size4, &struct_contains_test::item1));
         REQUIRE(!struct_contains(size6, &struct_contains_test::item1));
         REQUIRE(struct_contains(size8, &struct_contains_test::item1));
     }
}
