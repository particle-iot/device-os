
#include "catch.hpp"
#include <string>
#include "flash_storage.h"
#include "dcd.h"
#include <string.h>
#include "hippomocks.h"
#include <random>

const int TestSectorSize = 16000;
const int TestSectorCount = 2;
const int TestBase = 4000;

using std::string;
using TestStore = RAMFlashStorage<TestBase, TestSectorCount, TestSectorSize>;

unsigned int crc32b(const unsigned char* message, size_t length) {
   int i, j;
   uint32_t byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (length--> 0) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

uint32_t calcCrc(const void* address, size_t length) {
	return crc32b((const unsigned char*)address, length);
}

using TestDCDBase = DCD<TestStore, TestSectorSize, TestBase, TestBase+TestSectorSize, calcCrc>;

class TestDCD : public TestDCDBase {
	friend class DCDFixture;
};

/**
 * Utility method to sum a range of values.
 */
unsigned sum_bytes(TestStore& store, unsigned start, unsigned length)
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
    int fingerprint = sum_bytes(store, TestBase, TestSectorSize*TestSectorCount);
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

    int fingerprint = sum_bytes(store, TestBase+TestSectorSize, TestSectorSize);   // 2nd sector
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
    for (unsigned i=0; i<5; i++)
    {
        CAPTURE( i );
        REQUIRE(data[i] == 0xFFu);
    }
}

SCENARIO("DCD Length is SectorSize minus the header size minus footer size", "[dcd]")
{
    TestDCD dcd;
    REQUIRE(dcd.Length == TestSectorSize-8-32);
}

SCENARIO("DCD can save data", "[dcd]")
{
    TestDCD dcd;

    uint8_t expected[dcd.Length];
    memset(expected, 0xFF, sizeof(expected));
    memcpy(expected+13, "batman", 6);

    REQUIRE_FALSE(dcd.write(23, "batman", 6));

    const uint8_t* data = dcd.read(10);
    assertMemoryEqual(data, expected, dcd.Length-10);
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
    for (unsigned i=0; i<dcd.Length; i++) {
        expected[i] = 0xFF;
    }
    memmove(expected+23, "bbatman", 7);

    // overwrite data swapping a b to an a and vice versa
    REQUIRE_FALSE(dcd.write(23, "batman", 6));
    REQUIRE_FALSE(dcd.write(24, "batman", 6));

    const uint8_t* data = dcd.read(0);
    assertMemoryEqual(data, expected, dcd.Length);

    REQUIRE_FALSE(dcd.write(25, "batman", 6));
    data = dcd.read(0);
    memmove(expected+23, "bbbatman", 8);
    assertMemoryEqual(data, expected, dcd.Length);
}

SCENARIO("DCD uses 2nd sector if both are valid v1 sectors", "[dcd]")
{
    TestDCD dcd;
    TestStore& store = dcd.store;

    TestDCD::Header header;
    header.makeValid();

    // directly manipulate the flash to create desired state
    store.eraseSector(TestBase);
    store.eraseSector(TestBase+TestSectorSize);
    store.write(TestBase, &header, header.size());
    store.write(TestBase+header.size(), "abcd", 4);
    store.write(TestBase+TestSectorSize, &header, header.size());
    store.write(TestBase+TestSectorSize+header.size(), "1234", 4);

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

TEST_CASE("initialized header has version 1", "[header]") {
	TestDCD dcd;
	TestDCD::Header header;
	header.makeValid();
	REQUIRE(header.isValid());
	REQUIRE(header.seal==0xEDA15E00);		// SEAL_VALID
}

SCENARIO_METHOD(TestDCD, "isCRCValid", "[dcd]") {
	TestDCD& dcd = *this;
	uint32_t crc = 0x1234ABCD;

    REQUIRE_FALSE(isInitialized());
    initialize(Sector_0);

	GIVEN("calculateCRC is mocked") {
		const uint8_t* start = store.dataAt(addressOf(Sector_0));
		MockRepository mocks;
		mocks.ExpectCallFunc(calcCrc).With((const void*)(start+sizeof(Header)), TestSectorSize-sizeof(Header)-sizeof(typename Footer::crc_type)).Return(crc);

		WHEN("the footer has a different CRC") {
			REQUIRE_FALSE(dcd.isCRCValid(Sector_0, 0x1234));
		}

		WHEN("the footer has the same CRC") {
			REQUIRE(dcd.isCRCValid(Sector_0, crc));
		}
	}
}

SCENARIO_METHOD(TestDCD, "initializing a sector initializes to 0xFF with a valid CRC", "[dcd") {
    REQUIRE_FALSE(isInitialized());
    initialize(Sector_0);
    REQUIRE(isCRCValid(Sector_0));
    REQUIRE(!isCRCValid(Sector_1));
    REQUIRE(isInitialized());
}

SCENARIO_METHOD(TestDCD, "writing data and then changing it on the fly invalidates the sector", "[dcd]") {
	TestDCD& dcd = *this;
    REQUIRE(!isCRCValid(Sector_0));
    REQUIRE_FALSE(dcd.write(23, "abcdef", 6));
	bool initialized = dcd.isInitialized();
    REQUIRE(initialized);
	// emulate a change in the CRC
	MockRepository mocks;
	uint32_t crc = 0xABCDABCD;
	mocks.OnCallFunc(calcCrc).Return(crc);
	REQUIRE(!dcd.isCRCValid(Sector_0));
}

SCENARIO_METHOD(TestDCD, "dcd is uninitialized by default", "[dcd]") {
	REQUIRE_FALSE(isInitialized());
}

SCENARIO_METHOD(TestDCD, "dcd can be initialized by writing", "[dcd]") {
	// assume uninitialized
	auto& dct = *this;
	dct.write(23, "abcd", 4);
	REQUIRE(dct.isInitialized());
	REQUIRE(dct.isCRCValid(Sector_0));
}

SCENARIO_METHOD(TestDCD, "current sector","[dcd]") {

	GIVEN("current sector") {
		TestDCD& dcd = *this;
		// these are needed to avoid a linker error on the Sector_xxx symbols from the DCD template
		//const int Sector_Unknown = 255;
		//const int Sector_0 = 0;
		//const int Sector_1 = 1;

		THEN("is the sector with the highest count") {
			REQUIRE(_currentSector(0, 1, Sector_0, Sector_1)==Sector_1);
			REQUIRE(_currentSector(3, 0, Sector_0, Sector_1)==Sector_1);
			REQUIRE(_currentSector(0, 3, Sector_0, Sector_1)==Sector_0);
			REQUIRE(_currentSector(1, 0, Sector_0, Sector_1)==Sector_0);

			REQUIRE(_currentSector(0, 1, Sector_1, Sector_0)==Sector_0);
			REQUIRE(_currentSector(3, 0, Sector_1, Sector_0)==Sector_0);
			REQUIRE(_currentSector(0, 3, Sector_1, Sector_0)==Sector_1);
			REQUIRE(_currentSector(1, 0, Sector_1, Sector_0)==Sector_1);
		}

		THEN("is the first sector when counts are not in sequence") {
			REQUIRE(_currentSector(2, 0, Sector_0, Sector_1)==Sector_0);
			REQUIRE(_currentSector(0, 2, Sector_0, Sector_1)==Sector_0);
			REQUIRE(_currentSector(1, 3, Sector_0, Sector_1)==Sector_0);
			REQUIRE(_currentSector(3, 1, Sector_0, Sector_1)==Sector_0);

			REQUIRE(_currentSector(2, 0, Sector_1, Sector_0)==Sector_1);
			REQUIRE(_currentSector(0, 2, Sector_1, Sector_0)==Sector_1);
			REQUIRE(_currentSector(1, 3, Sector_1, Sector_0)==Sector_1);
			REQUIRE(_currentSector(3, 1, Sector_1, Sector_0)==Sector_1);
		}
	}
}

SCENARIO_METHOD(TestDCD, "upgrade v1 to v2 format", "[dcd]") {
    Header header;
    header.makeValid();

    // directly manipulate the flash to create desired state
    store.eraseSector(TestBase);
    store.eraseSector(TestBase+TestSectorSize);
    store.write(TestBase, &header, header.size());

    uint8_t data[4096];

    for (int i=0; i<4096; i++) {
    		data[i] = random();
    }

    store.write(TestBase+header.size(), data, 4096);
    REQUIRE(!this->isCRCValid(Sector_0));
    const uint8_t* result = read(0);
    assertMemoryEqual(result, data, 4096);

    // now rewrite, which will change it to sector v2 format
    write(4, "56", 2);
    memmove(data+4, "56", 2);
    result = read(0);
	assertMemoryEqual(result, data, 4096);

	//sector should have a valid CRC
    REQUIRE(this->isCRCValid(Sector_1));

    // Footer should be valid
    const auto& footer = sectorFooter(currentSector());
    REQUIRE(footer.isValid());
}

static void fillRandom(uint8_t* data, size_t size) {
    static std::random_device r;
    static std::default_random_engine rnd(r());

    for (uint32_t* ptr = (uint32_t*)data; (uint32_t*)ptr < (uint32_t*)(data + size); ptr++) {
        *ptr = rnd();
    }
}

SCENARIO_METHOD(TestDCD, "secondary (invalid_v2) sector is selected as valid if primary (valid) is lost", "[dcd]") {
    // Erase both sectors
    store.eraseSector(TestBase);
    store.eraseSector(TestBase + TestSectorSize);

    // Generate some random data
    uint8_t temp[TestSectorSize / 2] = {};
    fillRandom(temp, sizeof(temp));

    // DCD is unitialized, both sectors are invalid
    REQUIRE(!isInitialized());
    REQUIRE(!isValid(Sector_0));
    REQUIRE(!isValid(Sector_1));

    // Write random data. This should end up in Sector_0
    REQUIRE(write(0, temp, sizeof(temp)) == DCD_SUCCESS);
    REQUIRE(isValid(Sector_0));

    // Validate data
    assertMemoryEqual(read(0), temp, sizeof(temp));

    // Write same data
    // This will cause a sector switch
    REQUIRE(write(0, temp, sizeof(temp)) == DCD_SUCCESS);
    // Both sectors should be valid
    REQUIRE(isValid(Sector_0));
    REQUIRE(isValid(Sector_1));

    // The data should be in Sector_1
    REQUIRE(currentSector() == Sector_1);
    // Validate data
    assertMemoryEqual(read(0), temp, sizeof(temp));

    // Erase second sector, imitating a write/erase failure that was not caught during a write operation itself
    store.eraseSector(TestBase + TestSectorSize);

    // DCD should be initialized, only Sector_0 should be valid
    REQUIRE(isInitialized());
    REQUIRE(isValid(Sector_0));
    REQUIRE(!isValid(Sector_1));

    // Change only first 8 bytes
    fillRandom(temp, sizeof(uint32_t) * 2);

    // Write these modified 8 bytes
    REQUIRE(write(0, temp, sizeof(uint32_t) * 2) == DCD_SUCCESS);
    REQUIRE(isInitialized());
    // The data should have ended up in Sector_1
    REQUIRE(currentSector() == Sector_1);
    // Both sectors should be valid
    REQUIRE(isValid(Sector_0));
    REQUIRE(isValid(Sector_1));

    // Validate data
    assertMemoryEqual(read(0), temp, sizeof(temp));
}
