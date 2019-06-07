
#include "application.h"
#include "unit-test/unit-test.h"
#include "dynalib_abi.h"


struct test_struct {
    uint16_t size;
    uint32_t data;
};

test_struct t8 = { .size = 8 };
test_struct t4 = { .size = 4 };


test(dynalib_abi_struct_does_not_contain)
{
    assertFalse(struct_contains(t4, &test_struct::data));
}

test(dynalib_abi_struct_does_contain)
{
    assertTrue(struct_contains(t8, &test_struct::data));
}

test(dynalib_api_manual_struct_check) {
    assertTrue(t8.size >= 9)
}

bool test_struct_pointer(test_struct* t) {
    return struct_contains(t, &test_struct::data);
}

test(dynalib_api_indirect_check) {
    assertTrue(test_struct_pointer(&t8));
}
