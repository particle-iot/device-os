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

RetainedIntegerDiagnosticData<INT_1> int1(1); // NoConcurrency (default)
RetainedIntegerDiagnosticData<INT_2> int2(2); // NoConcurrency (default)
RetainedIntegerDiagnosticData<INT_3, AtomicConcurrency> int3(3);
RetainedIntegerDiagnosticData<INT_4, AtomicConcurrency> int4(4);

RetainedEnumDiagnosticData<Enum, ENUM_1> enum1(ONE); // NoConcurrency (default)
RetainedEnumDiagnosticData<Enum, ENUM_2> enum2(TWO); // NoConcurrency (default)
RetainedEnumDiagnosticData<Enum, ENUM_3, AtomicConcurrency> enum3(THREE);
RetainedEnumDiagnosticData<Enum, ENUM_4, AtomicConcurrency> enum4(FOUR);

retained unsigned magic;

} // namespace

//RETAINED_INTEGER_DIAGNOSTIC_DATA(int1, INT_1, 0);

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
