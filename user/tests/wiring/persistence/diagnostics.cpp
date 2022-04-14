#include "application.h"
#include "unit-test/unit-test.h"

namespace {

using namespace particle;

enum DiagnosticId {
    INT_1 = DIAG_ID_USER,
    INT_2,
    INT_3,
    INT_4,
    ENUM_1,
    ENUM_2,
    ENUM_3,
    ENUM_4
};

enum Enum {
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5
};

const unsigned MAGIC = 0x12345678;

PARTICLE_RETAINED_INTEGER_DIAGNOSTIC_DATA(int1, INT_1, "int1", 1); // NoConcurrency
PARTICLE_RETAINED_INTEGER_DIAGNOSTIC_DATA(int2, INT_2, "int2", 2); // NoConcurrency
PARTICLE_RETAINED_INTEGER_DIAGNOSTIC_DATA(int3, INT_3, "int3", 3, AtomicConcurrency);
PARTICLE_RETAINED_INTEGER_DIAGNOSTIC_DATA(int4, INT_4, "int4", 4, AtomicConcurrency);

PARTICLE_RETAINED_ENUM_DIAGNOSTIC_DATA(enum1, ENUM_1, "enum1", ONE); // NoConcurrency
PARTICLE_RETAINED_ENUM_DIAGNOSTIC_DATA(enum2, ENUM_2, "enum2", TWO); // NoConcurrency
PARTICLE_RETAINED_ENUM_DIAGNOSTIC_DATA(enum3, ENUM_3, "enum3", THREE, AtomicConcurrency);
PARTICLE_RETAINED_ENUM_DIAGNOSTIC_DATA(enum4, ENUM_4, "enum4", FOUR, AtomicConcurrency);

retained unsigned magic;

} // namespace

test(diagnostics_01) {
    if (magic != MAGIC) {
        Serial.println("Checking initial values...");
        assertEqual(int1, 1);
        assertEqual(int2, 2);
        assertEqual(int3, 3);
        assertEqual(int4, 4);
        assertEqual(enum1, ONE);
        assertEqual(enum2, TWO);
        assertEqual(enum3, THREE);
        assertEqual(enum4, FOUR);
        Serial.println("Updating values...");
        int1 = -1;
        int2 = -2;
        int3 = -3;
        int4 = -4;
        enum1 = TWO;
        enum2 = THREE;
        enum3 = FOUR;
        enum4 = FIVE;
        magic = MAGIC;
        Serial.println("Please reset the device and restart the test");
    } else {
        Serial.println("Checking updated values...");
        assertEqual(int1, -1);
        assertEqual(int2, -2);
        assertEqual(int3, -3);
        assertEqual(int4, -4);
        assertEqual(enum1, TWO);
        assertEqual(enum2, THREE);
        assertEqual(enum3, FOUR);
        assertEqual(enum4, FIVE);
    }
}
