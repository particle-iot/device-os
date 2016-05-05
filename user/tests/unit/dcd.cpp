
#include "catch.hpp"
#include <string>
#include "flash_storage.h"
#include "dcd.h"
#include <string.h>

const int TestSectorSize = 16000;
const int TestSectorCount = 2;
const int TestBase = 4000;

using std::string;
using TestStore = RAMFlashStorage<TestBase, TestSectorCount, TestSectorSize>;

using TestDCD = DCD<TestStore, TestSectorSize, TestBase, TestBase+TestSectorSize>;


unsigned sum(TestStore& store, unsigned start, unsigned length)
{
    const uint8_t* data = store.dataAt(start);
    unsigned sum = 0;
    for (unsigned i=0; i<length; i++)
    {
        sum += *data++;
    }
    return sum;
}


void assertMemoryEqual(const uint8_t* actual, const uint8_t* expected, size_t length)
{
    int index = 0;
    while (length --> 0)
    {
        CAPTURE(index);
        REQUIRE(actual[index] == expected[index]);
        index++;
    }
}


SCENARIO("RAMRlashStore provides pointer to data", "[ramflash]")
{
    TestStore store;
    const uint8_t* data1 = store.dataAt(TestBase);
    const uint8_t* data2 = store.dataAt(TestBase+100);

    REQUIRE(data1 != nullptr);
    REQUIRE(data2 != nullptr);
    REQUIRE(long(data1+100) == long(data2));
}

SCENARIO("RAMFlashStore is initially random", "[ramflash]")
{
    TestStore store;
    int fingerprint = sum(store, TestBase, TestSectorSize*TestSectorCount);
    REQUIRE(fingerprint != 0);      // not all 0's
    REQUIRE(fingerprint != (TestSectorSize*TestSectorCount)*0xFF);
}

SCENARIO("RAMFlashStore can be erased","[ramflash]")
{
    TestStore store;
    REQUIRE_FALSE(store.eraseSector(TestBase+100+TestSectorSize));

    const uint8_t* data = store.dataAt(TestBase+TestSectorSize);
    for (unsigned i=0; i<TestSectorSize; i++) {
        CAPTURE(i);
        CHECK(data[i] == 0xFF);
    }

    int fingerprint = sum(store, TestBase+TestSectorSize, TestSectorSize);   // 2nd sector
    REQUIRE(fingerprint != 0);      // not all 0's
    REQUIRE(fingerprint == (TestSectorSize)*0xFF);
}

SCENARIO("RAMFlashStore can store data", "[ramflash]")
{
    TestStore store;
    REQUIRE_FALSE(store.eraseSector(TestBase));
    REQUIRE_FALSE(store.write(TestBase+3, (const uint8_t*)"batman", 7));

    const char* expected = "\xFF\xFF\xFF" "batman" "\x00\xFF\xFF";
    const char* actual = (const char*)store.dataAt(TestBase);
    REQUIRE(string(actual,12) == string(expected,12));
}

SCENARIO("RAMFlashStore emulates NAND flash", "[ramflash]")
{
    TestStore store;
    REQUIRE_FALSE(store.eraseSector(TestBase));
    REQUIRE_FALSE(store.write(TestBase+3, (const uint8_t*)"batman", 7));
    REQUIRE_FALSE(store.write(TestBase+0, (const uint8_t*)"\xA8\xFF\x00", 3));

    const char* actual = (const char*)store.dataAt(TestBase);

    const char* expected = "\xA8\xFF\x00" "batman" "\x00\xFF\xFF";
    REQUIRE(string(actual,12) == string(expected,12));

    // no change to flash storage
    REQUIRE_FALSE(store.write(TestBase, (const uint8_t*)"\xF7\x80\x00\xFF", 3));
    expected = "\xA0\x80\0batman\x00\xFF\xFF";
    REQUIRE(string(actual,12) == string(expected,12));
}


// DCD Tests


SCENARIO("DCD initialized returns 0xFF", "[dcd]")
{
    TestDCD dcd;

    const uint8_t* data = dcd.read(0);
    for (unsigned i=0; i<dcd.Length; i++)
    {
        CAPTURE( i );
        REQUIRE(data[i] == 0xFFu);
    }
}

SCENARIO("DCD Length is SectorSize minus 8", "[dcd]")
{
    TestDCD dcd;
    REQUIRE(dcd.Length == TestSectorSize-8);
}

SCENARIO("DCD can save data", "[dcd]")
{
    TestDCD dcd;

    uint8_t expected[dcd.Length];
    memset(expected, 0xFF, sizeof(expected));
    memcpy(expected+23, "batman", 6);

    REQUIRE_FALSE(dcd.write(23, "batman", 6));

    const uint8_t* data = dcd.read(0);
    assertMemoryEqual(data, expected, dcd.Length);
}

SCENARIO("DCD can write whole sector", "[dcd]")
{
    TestDCD dcd;

    uint8_t expected[dcd.Length];
    for (unsigned i=0; i<dcd.Length; i++)
        expected[i] = rand();

    dcd.write(0, expected, dcd.Length);
    const uint8_t* data = dcd.read(0);
    assertMemoryEqual(data, expected, dcd.Length);
}

SCENARIO("DCD can overwrite data", "[dcd]")
{
    TestDCD dcd;

    uint8_t expected[dcd.Length];
    for (unsigned i=0; i<dcd.Length; i++)
        expected[i] = 0xFF;
    memmove(expected+23, "bbatman", 7);

    // overwrite data swapping a b to an a and vice versa
    REQUIRE_FALSE(dcd.write(23, "batman", 6));
    REQUIRE_FALSE(dcd.write(24, "batman", 6));

    const uint8_t* data = dcd.read(0);
    assertMemoryEqual(data, expected, dcd.Length);
}

SCENARIO("DCD uses 2nd sector if both are valid", "[dcd]")
{
    TestDCD dcd;
    TestStore& store = dcd.store;

    TestDCD::Header header;
    header.make_valid();

    // directly manipulate the flash to create desired state
    store.eraseSector(TestBase);
    store.eraseSector(TestBase+TestSectorSize);
    store.write(TestBase, &header, sizeof(header));
    store.write(TestBase+sizeof(header), "abcd", 4);
    store.write(TestBase+TestSectorSize, &header, sizeof(header));
    store.write(TestBase+TestSectorSize+sizeof(header), "1234", 4);

    const uint8_t* result = dcd.read(0);
    assertMemoryEqual(result, (const uint8_t*)"1234", 4);
}


SCENARIO("DCD write is atomic if partial failure", "[dcd]")
{
    for (int write_count=1; write_count<5; write_count++)
    {
        TestDCD dcd;
        REQUIRE_FALSE(dcd.write(23, "abcdef", 6));
        REQUIRE_FALSE(dcd.write(23, "batman", 6));

        assertMemoryEqual(dcd.read(23), (const uint8_t*)"batman", 6);

        // mock a power failure after a certain number of writes
        dcd.store.setWriteCount(write_count);
        CAPTURE(write_count);

        // write should fail
        REQUIRE(dcd.write(23, "7890-!", 6));

        // last write is unsuccessful
        assertMemoryEqual(dcd.read(23), (const uint8_t*)"batman", 6);
    }
}