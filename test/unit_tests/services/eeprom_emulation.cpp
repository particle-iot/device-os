// Off device tests for the byte-oriented EEPROM emulation

#include "catch.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include "eeprom_emulation.h"
#include "flash_storage.h"

const size_t TestPageSize = 0x4000;
const uint8_t TestPageCount = 2;
const uintptr_t TestBase = 0xC000;

/* Simulate 2 Flash pages of different sizes used for EEPROM emulation */
const uintptr_t PageBase1 = TestBase;
const size_t PageSize1 = TestPageSize;
const uintptr_t PageBase2 = TestBase + TestPageSize;
const size_t PageSize2 = TestPageSize / 4;

using TestStore = RAMFlashStorage<TestBase, TestPageCount, TestPageSize>;
using TestEEPROM = EEPROMEmulation<TestStore, PageBase1, PageSize1, PageBase2, PageSize2>;
using Record = TestEEPROM::Record;

// Alias some constants, otherwise the linker is having issues when
// those are used inside REQUIRE() assertions
auto NoPage = TestEEPROM::LogicalPage::NoPage;
auto Page1 = TestEEPROM::LogicalPage::Page1;
auto Page2 = TestEEPROM::LogicalPage::Page2;

auto PAGE_ERASED = TestEEPROM::PageHeader::ERASED;
auto PAGE_COPY = TestEEPROM::PageHeader::COPY;
auto PAGE_ACTIVE = TestEEPROM::PageHeader::ACTIVE;
auto PAGE_INACTIVE = TestEEPROM::PageHeader::INACTIVE;

// Test helper class to pre-write EEPROM records and validate written
// records
class EEPROMTester
{
public:
    EEPROMTester(TestEEPROM &eeprom)
        : eeprom(eeprom)
    {
    }

    // Populate a page of flash with a page header and 0 or more records
    void populate(uintptr_t address,
            uint32_t pageStatus,
            std::initializer_list<Record> recordList = {})
    {
        eeprom.store.eraseSector(address);

        eeprom.store.write(address, &pageStatus, sizeof(pageStatus));
        address += sizeof(pageStatus);

        for(auto record: recordList)
        {
            eeprom.store.write(address, &record, sizeof(record));
            address += sizeof(record);
        }
    }

    // Validate that a page of flash matches exactly the expected page header
    // and records, including having erased space after the last expected record
    void requireContents(intptr_t address,
            uint32_t expectedPageStatus,
            std::initializer_list<Record> expectedRecordList = {})
    {
        uint32_t pageStatus;
        eeprom.store.read(address, &pageStatus, sizeof(pageStatus));
        INFO("Unexpected page status at 0x" << std::hex << address);
        REQUIRE(pageStatus == expectedPageStatus);
        address += sizeof(pageStatus);

        for(auto expectedRecord: expectedRecordList)
        {
            Record record;
            eeprom.store.read(address, &record, sizeof(record));
            address += sizeof(record);
            INFO("Unexpected record at 0x" << std::hex << address);
            REQUIRE(std::memcmp(&record, &expectedRecord, sizeof(record)) == 0);
        }

        uint32_t erased;
        eeprom.store.read(address, &erased, sizeof(erased));
        INFO("Expected erased space at 0x" << std::hex << address);
        REQUIRE(erased == 0xFFFFFFFF);
    }

    // Debugging helper to view the storage contents
    // Usage:
    // WARN(tester.dumpStorage(PageBase1, 30));
    std::string dumpStorage(uintptr_t address, uint16_t length)
    {
        std::stringstream ss;
        const uint8_t *begin = eeprom.store.dataAt(address);
        const uint8_t *end = eeprom.store.dataAt(address + length);

        ss << std::hex << address << ": ";
        while(begin < end)
        {
            ss << std::hex << std::setw(2) << std::setfill('0');
            ss << (int)*begin++ << " ";
        }
        return ss.str();
    }
private:

    TestEEPROM &eeprom;
};

TEST_CASE("Get byte", "[eeprom]")
{
    TestEEPROM eeprom;

    eeprom.init();

    uint8_t value;
    uint16_t eepromIndex = 10;

    SECTION("The index was not programmed")
    {
        SECTION("No other records")
        {
            THEN("get returns the value as erased")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xFF);
            }
        }

        SECTION("With other records")
        {
            eeprom.put(0, 0xAA);

            THEN("get returns the value as erased")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xFF);
            }
        }

        SECTION("With a partially written record")
        {
            eeprom.store.discardWritesAfter(1, [&] {
                eeprom.put(eepromIndex, 0xEE);
            });

            THEN("get returns the value as erased")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xFF);
            }
        }
    }

    SECTION("The index was programmed")
    {
        eeprom.put(eepromIndex, 0xCC);

        SECTION("No other records")
        {
            THEN("get extracts the value")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xCC);
            }
        }

        SECTION("Followed by a partially written record")
        {
            eeprom.store.discardWritesAfter(1, [&] {
                eeprom.put(eepromIndex, 0xEE);
            });

            THEN("get extracts the value of the last valid record")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xCC);
            }
        }

        SECTION("Followed by a fully written record")
        {
            eeprom.put(eepromIndex, 0xEE);

            THEN("get extracts the value of the new valid record")
            {
                eeprom.get(eepromIndex, value);
                REQUIRE(value == 0xEE);
            }
        }

    }

    SECTION("The index was programmed by a multi-byte put")
    {
        uint16_t eepromIndex = 0;
        uint8_t values[] = { 1, 2, 3 };

        eeprom.put(eepromIndex, values, sizeof(values));

        THEN("get extracts each value")
        {
            eeprom.get(eepromIndex, value);
            REQUIRE(value == 1);

            eeprom.get(eepromIndex + 1, value);
            REQUIRE(value == 2);

            eeprom.get(eepromIndex + 2, value);
            REQUIRE(value == 3);
        }
    }

    SECTION("The index is out of range")
    {
        THEN("get returns the value as erased")
        {
            eeprom.get(65000, value);
            REQUIRE(value == 0xFF);
        }
    }
}

TEST_CASE("Get multi-byte", "[eeprom]")
{
    TestEEPROM eeprom;
    eeprom.init();

    uint16_t eepromIndex = 10;
    uint8_t values[3];
    auto requireValues = [&](uint8_t v1, uint8_t v2, uint8_t v3)
    {
        REQUIRE(values[0] == v1);
        REQUIRE(values[1] == v2);
        REQUIRE(values[2] == v3);
    };

    SECTION("The offsets were not programmed")
    {
        SECTION("No other records")
        {
            THEN("get returns the values as erased")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(0xFF, 0xFF, 0xFF);
            }
        }

        SECTION("With other records")
        {
            eeprom.put(0, 0xAA);

            THEN("get returns the values as erased")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(0xFF, 0xFF, 0xFF);
            }
        }

        SECTION("With a partially written block of records")
        {
            // It takes 12 byte writes to write the 3 data records
            // so discard everything after the first record write
            eeprom.store.discardWritesAfter(4, [&] {
                uint8_t partialValues[] = { 1, 2, 3 };
                eeprom.put(eepromIndex, partialValues, sizeof(partialValues));
            });

            THEN("get returns the values as erased")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(0xFF, 0xFF, 0xFF);
            }
        }

        SECTION("With a partially validated block of records")
        {
            // It takes 12 byte writes to write the 3 data records,
            // so discard the write of the last index
            eeprom.store.discardWritesAfter(10, [&] {
                uint8_t partialValues[] = { 1, 2, 3 };
                eeprom.put(eepromIndex, partialValues, sizeof(partialValues));
            });

            THEN("get returns the values as erased")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(0xFF, 0xFF, 0xFF);
            }
        }
    }

    SECTION("The offsets were programmed")
    {
        uint8_t previousValues[] = { 10, 20, 30 };

        eeprom.put(eepromIndex, previousValues, sizeof(previousValues));

        SECTION("No other records")
        {
            THEN("get returns the values as previously programmed")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(10, 20, 30);
            }
        }

        SECTION("With other records")
        {
            eeprom.put(0, 0xAA);

            THEN("get returns the values as previously programmed")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(10, 20, 30);
            }
        }

        SECTION("With a partially written block of records")
        {
            // It takes 4 writes to write the 2 data records, followed
            // by the 2 valid statuses, so discard everything after the
            // first invalid record write
            eeprom.store.discardWritesAfter(1, [&] {
                uint8_t partialValues[] = { 2, 3 };
                eeprom.put(eepromIndex + 1, partialValues, sizeof(partialValues));
            });

            THEN("get returns the values as previously programmed")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(10, 20, 30);
            }
        }

        SECTION("With a partially validated block of records")
        {
            // It takes 4 writes to write the 2 data records, followed
            // by the 2 valid statuses, so discard the 4th write
            eeprom.store.discardWritesAfter(3, [&] {
                uint8_t partialValues[] = { 2, 3 };
                eeprom.put(eepromIndex + 1, partialValues, sizeof(partialValues));
            });

            THEN("get returns the values as previously programmed")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(10, 20, 30);
            }
        }

        SECTION("With a fully written block of records")
        {
            uint8_t newValues[] = { 2, 3 };
            eeprom.put(eepromIndex + 1, newValues, sizeof(newValues));

            THEN("get returns the new values")
            {
                eeprom.get(eepromIndex, values, sizeof(values));
                requireValues(10, 2, 3);
            }
        }
    }

    SECTION("Some offsets were programmed")
    {
        uint8_t previousValues[] = { 10, 20 };

        eeprom.put(eepromIndex, previousValues, sizeof(previousValues));

        THEN("get returns the values as previously programmed and erase for missing values")
        {
            eeprom.get(eepromIndex, values, sizeof(values));
            requireValues(10, 20, 0xFF);
        }
    }

    SECTION("The record is put after partially written records")
    {
        eeprom.store.discardWritesAfter(8, [&] {
            uint8_t partialValues[] = { 1, 2, 3 };
            eeprom.put(eepromIndex, partialValues, sizeof(partialValues));
        });

        eeprom.put(eepromIndex, 10);

        THEN("get returns only the fully written value")
        {
            eeprom.get(eepromIndex, values, sizeof(values));
            requireValues(10, 0xFF, 0xFF);
        }
    }
}

TEST_CASE("Put record", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    eeprom.init();

    uint16_t eepromIndex = 0;

    SECTION("The record doesn't exist")
    {
        THEN("put creates the record")
        {
            eeprom.put(eepromIndex, 0xCC);

            tester.requireContents(PageBase1, PAGE_ACTIVE, {
                Record(eepromIndex, 0xCC)
            });
        }

        THEN("get returns the put record")
        {
            eeprom.put(eepromIndex, 0xCC);

            uint8_t newRecord;
            eeprom.get(eepromIndex, newRecord);

            REQUIRE(newRecord == 0xCC);
        }
    }

    SECTION("A bad record exists")
    {
        eeprom.store.discardWritesAfter(1, [&] {
            eeprom.put(eepromIndex, 0xEE);
        });

        THEN("put triggers a page swap")
        {
            REQUIRE(eeprom.getActivePage() == Page1);

            eeprom.put(eepromIndex, 0xCC);

            REQUIRE(eeprom.getActivePage() == Page2);
        }

        THEN("put creates the record")
        {
            eeprom.put(eepromIndex, 0xCC);

            uint8_t newRecord;
            eeprom.get(eepromIndex, newRecord);

            REQUIRE(newRecord == 0xCC);
        }
    }

    SECTION("The record exists")
    {
        eeprom.put(eepromIndex, 0xCC);

        THEN("put creates a new copy of the record")
        {
            eeprom.put(eepromIndex, 0xDD);

            tester.requireContents(PageBase1, PAGE_ACTIVE, {
                Record(eepromIndex, 0xCC),
                Record(eepromIndex, 0xDD)
            });
        }

        THEN("get returns the put record")
        {
            eeprom.put(eepromIndex, 0xDD);

            uint8_t newRecord;
            eeprom.get(eepromIndex, newRecord);

            REQUIRE(newRecord == 0xDD);
        }
    }

    SECTION("The new record value is the same as the current one")
    {
        eeprom.put(eepromIndex, 0xCC);

        THEN("put doesn't create a new copy of the record")
        {
            eeprom.put(eepromIndex, 0xCC);

            tester.requireContents(PageBase1, PAGE_ACTIVE, {
                Record(eepromIndex, 0xCC)
            });
        }
    }

    SECTION("The address is out of range")
    {
        THEN("put doesn't create a new record")
        {
            eeprom.put(65000, 0xEE);

            tester.requireContents(PageBase1, PAGE_ACTIVE, {
                /* no records */
            });
        }
    }

    SECTION("Page swap is required")
    {
        uint16_t writesToFillPage1 = PageSize1 / sizeof(TestEEPROM::Record) - 1;

        // Write multiple copies of the same record until page 1 is full
        for(uint32_t i = 0; i < writesToFillPage1; i++)
        {
            eeprom.put(eepromIndex, (uint8_t)i);
        }

        REQUIRE(eeprom.getActivePage() == Page1);
    
        THEN("The next write performs a page swap")
        {
            uint8_t newRecord = 0;
            eeprom.put(eepromIndex, newRecord);

            REQUIRE(eeprom.getActivePage() == Page2);
        }
    }
}

TEST_CASE("Capacity", "[eeprom]")
{
    TestEEPROM eeprom;
    // Each record is 4 bytes, and some space is used by the page header
    // Capacity if 50% of the theoretical max
    size_t expectedByteCapacity = PageSize2 / 4 / 2;

    REQUIRE(eeprom.capacity() == expectedByteCapacity);
}

TEST_CASE("Initialize EEPROM", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    SECTION("Random flash")
    {
        eeprom.init();

        THEN("Page 1 is active, page 2 is erased")
        {
            tester.requireContents(PageBase1, PAGE_ACTIVE);
            tester.requireContents(PageBase2, PAGE_ERASED);
        }
    }

    SECTION("Erased flash")
    {
        tester.populate(PageBase1, PAGE_ERASED);
        tester.populate(PageBase2, PAGE_ERASED);

        eeprom.init();

        THEN("Page 1 is active, page 2 is erased")
        {
            tester.requireContents(PageBase1, PAGE_ACTIVE);
            tester.requireContents(PageBase2, PAGE_ERASED);
        }
    }

    SECTION("Page 1 active")
    {
        tester.populate(PageBase1, PAGE_ACTIVE);
        tester.populate(PageBase2, PAGE_ERASED);

        eeprom.init();

        THEN("Page 1 remains active, page 2 remains erased")
        {
            tester.requireContents(PageBase1, PAGE_ACTIVE);
            tester.requireContents(PageBase2, PAGE_ERASED);
        }
    }

    SECTION("Page 2 active")
    {
        tester.populate(PageBase1, PAGE_ERASED);
        tester.populate(PageBase2, PAGE_ACTIVE);

        eeprom.init();

        THEN("Page 1 remains erased, page 2 remains active")
        {
            tester.requireContents(PageBase1, PAGE_ERASED);
            tester.requireContents(PageBase2, PAGE_ACTIVE);
        }
    }
}

TEST_CASE("Clear", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    eeprom.init();
    // Add some records
    eeprom.put(0, 0xAA);
    eeprom.put(1, 0xBB);

    eeprom.clear();

    THEN("Page 1 is active, page 2 is erased")
    {
        tester.requireContents(PageBase1, PAGE_ACTIVE);
        tester.requireContents(PageBase2, PAGE_ERASED);
    }
}

TEST_CASE("Verify page", "[eeprom]")
{
    TestEEPROM eeprom;

    SECTION("random flash")
    {
        REQUIRE(eeprom.verifyPage(Page1) == false);
    }

    SECTION("erased flash")
    {
        eeprom.store.eraseSector(PageBase1);
        REQUIRE(eeprom.verifyPage(Page1) == true);
    }

    SECTION("partially erased flash")
    {
        eeprom.store.eraseSector(PageBase1);

        uint8_t garbage = 0xCC;
        eeprom.store.write(PageBase1 + 100, &garbage, sizeof(garbage));

        REQUIRE(eeprom.verifyPage(Page1) == false);
    }
}

TEST_CASE("Active page", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    SECTION("No valid page")
    {
        SECTION("Page 1 erased, page 2 erased (blank flash)")
        {
            tester.populate(PageBase1, PAGE_ERASED);
            tester.populate(PageBase2, PAGE_ERASED);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == NoPage);
        }

        SECTION("Page 1 garbage, page 2 garbage (invalid state)")
        {
            tester.populate(PageBase1, 0xDEADC0DE);
            tester.populate(PageBase2, 0xDEADC0DE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == NoPage);
        }
    }

    SECTION("Page 1 valid")
    {
        SECTION("Page 1 active, page 2 erased (normal case)")
        {
            tester.populate(PageBase1, PAGE_ACTIVE);
            tester.populate(PageBase2, PAGE_ERASED);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page1);
        }
    }

    SECTION("Steps of swap from page 1 to page 2")
    {
        SECTION("Page 1 active, page 2 copy (interrupted swap)")
        {
            tester.populate(PageBase1, PAGE_ACTIVE);
            tester.populate(PageBase2, PAGE_COPY);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page1);
        }

        SECTION("Page 1 active, page 2 active (almost completed swap)")
        {
            tester.populate(PageBase1, PAGE_ACTIVE);
            tester.populate(PageBase2, PAGE_ACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page1);
        }
        
        SECTION("Page 1 inactive, page 2 active (completed swap, pending erase)")
        {
            tester.populate(PageBase1, PAGE_INACTIVE);
            tester.populate(PageBase2, PAGE_ACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page2);
        }
    }

    SECTION("Page 2 valid")
    {
        SECTION("Page 1 erased, page 2 active (normal case)")
        {
            tester.populate(PageBase1, PAGE_ERASED);
            tester.populate(PageBase2, PAGE_ACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page2);
        }
    }

    SECTION("Steps of swap from page 2 to page 1")
    {
        SECTION("Page 1 copy, page 2 active (interrupted swap)")
        {
            tester.populate(PageBase1, PAGE_COPY);
            tester.populate(PageBase2, PAGE_ACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page2);
        }

        SECTION("Page 1 active, page 2 active (almost completed swap)")
        {
            tester.populate(PageBase1, PAGE_ACTIVE);
            tester.populate(PageBase2, PAGE_ACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page1);
        }
        
        SECTION("Page 1 active, page 2 inactive (completed swap, pending erase)")
        {
            tester.populate(PageBase1, PAGE_ACTIVE);
            tester.populate(PageBase2, PAGE_INACTIVE);

            eeprom.updateActivePage();

            REQUIRE(eeprom.getActivePage() == Page1);
        }
    }
}

TEST_CASE("Alternate page", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    SECTION("Page 1 is active")
    {
        tester.populate(PageBase1, PAGE_ACTIVE);
        tester.populate(PageBase2, PAGE_ERASED);

        eeprom.updateActivePage();

        REQUIRE(eeprom.getAlternatePage() == Page2);
    }

    SECTION("Page 2 is active")
    {
        tester.populate(PageBase1, PAGE_ERASED);
        tester.populate(PageBase2, PAGE_ACTIVE);

        eeprom.updateActivePage();

        REQUIRE(eeprom.getAlternatePage() == Page1);
    }

    SECTION("No page is valid")
    {
        // Not necessary to test when no page is active since that
        // condition will be fixed at boot in init()
    }
}

TEST_CASE("Copy records during page swap", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    eeprom.init();

    uint16_t eepromIndex = 0;
    uintptr_t toAddress = PageBase2;

    auto performSwap = [&]() {
        // Don't write any new records
        eeprom.swapPagesAndWrite(100, NULL, 0);
    };

    SECTION("Single record")
    {
        eeprom.put(eepromIndex, 0xBB);

        performSwap();

        THEN("The record is copied")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(eepromIndex, 0xBB)
            });
        }
    }

    SECTION("Multiple copies of a record")
    {
        eeprom.put(eepromIndex, 0xBB);
        eeprom.put(eepromIndex, 0xCC);

        performSwap();

        THEN("The last record is copied")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(eepromIndex, 0xCC)
            });
        }
    }

    SECTION("Multiple copies of a record followed by an invalid record")
    {
        eeprom.put(eepromIndex, 0xBB);
        eeprom.put(eepromIndex, 0xCC);
        eeprom.store.discardWritesAfter(1, [&] {
            eeprom.put(eepromIndex, 0xEE);
        });

        performSwap();

        THEN("The last valid record is copied")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(eepromIndex, 0xCC)
            });
        }
    }

    SECTION("Record with 0xFF value")
    {
        eeprom.put(eepromIndex, 0xBB);
        eeprom.put(eepromIndex, 0xFF);

        performSwap();

        THEN("The record is not copied")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                /* no records */
            });
        }
    }

    SECTION("Multiple records")
    {
        eeprom.put(3, 0xDD);
        eeprom.put(1, 0xBB);
        eeprom.put(0, 0xAA);
        eeprom.put(2, 0xCC);

        performSwap();

        THEN("The records are copied from small ids to large ids")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(0, 0xAA),
                Record(1, 0xBB),
                Record(2, 0xCC),
                Record(3, 0xDD)
            });
        }
    }

    SECTION("Except specified records")
    {
        eeprom.put(3, 0xDD);
        eeprom.put(1, 0xBB);
        eeprom.put(0, 0xAA);
        eeprom.put(2, 0xCC);

        uint8_t newData[] = { 0x11, 0x22 };
        eeprom.swapPagesAndWrite(1, newData, sizeof(newData));

        THEN("The specified records are not copied, new values are written")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(0, 0xAA),
                Record(3, 0xDD),
                Record(1, 0x11),
                Record(2, 0x22)
            });
        }
    }

    SECTION("Multiple records with interrupted block write")
    {
        eeprom.put(3, 0xDD);
        eeprom.put(1, 0xBB);
        eeprom.put(0, 0xAA);
        eeprom.put(2, 0xCC);

        // It takes 6 writes to write the 3 data records, followed
        // by the 3 valid statuses, so discard the 6th write
        eeprom.store.discardWritesAfter(5, [&] {
            uint8_t partialValues[] = { 1, 2, 3 };
            eeprom.put(0, partialValues, sizeof(partialValues));
        });

        performSwap();

        THEN("Records up to the invalid record are copied")
        {
            tester.requireContents(toAddress, PAGE_ACTIVE, {
                Record(0, 0xAA),
                Record(1, 0xBB),
                Record(2, 0xCC),
                Record(3, 0xDD)
            });
        }
    }

    SECTION("Full page")
    {
        for(uint16_t index = 0; index < eeprom.capacity(); index++)
        {
            eeprom.put(index, 0);
        }

        performSwap();

        THEN("All records are copied")
        {
            for(uint16_t index = 0; index < eeprom.capacity(); index++)
            {
                CAPTURE(index);
                uint8_t value;
                eeprom.get(index, value);
                REQUIRE(value == 0);
            }
        }
    }
}

TEST_CASE("Swap pages recovery", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    // Write some data
    tester.populate(PageBase1, PAGE_ACTIVE, {
        Record(0, 1),
        Record(1, 2),
        Record(2, 3)
    });

    // Have a record to write after the swap
    uint16_t newIndex = 1;
    uint8_t newData[] = { 20, 30 };

    eeprom.init();

    auto requireSwapCompleted = [&]()
    {
        tester.requireContents(PageBase1, PAGE_INACTIVE, {
            Record(0, 1),
            Record(1, 2),
            Record(2, 3)
        });

        tester.requireContents(PageBase2, PAGE_ACTIVE, {
            Record(0, 1),
            Record(1, 20),
            Record(2, 30)
        });
    };

    auto performSwap = [&]()
    {
        eeprom.swapPagesAndWrite(newIndex, newData, sizeof(newData));
    };

    SECTION("No interruption")
    {
        performSwap();

        requireSwapCompleted();
    }

    SECTION("Interrupted page swap 1: during erase")
    {
        // Garbage status
        tester.populate(PageBase2, 0xDEADC0DE);

        eeprom.store.discardWritesAfter(0, [&] {
            performSwap();
        });

        // Verify that the alternate page is not yet erased
        tester.requireContents(PageBase2, 0xDEADC0DE);

        THEN("Redoing the page swap works")
        {
            performSwap();

            requireSwapCompleted();
        }
    }

    SECTION("Interrupted page swap 2: during copy")
    {
        // Enough writes for the status to be written, but no records
        eeprom.store.discardWritesAfter(4, [&] {
            performSwap();
        });

        // Verify that the alternate page is still copy
        tester.requireContents(PageBase2, PAGE_COPY);

        THEN("Redoing the page swap works")
        {
            performSwap();

            requireSwapCompleted();
        }
    }

    SECTION("Interrupted page swap 3: before old page becomes inactive")
    {
        // Enough writes to copy all records, but not enough to make old
        // page inactive
        eeprom.store.discardWritesAfter(19, [&] {
            performSwap();
        });

        // Verify that both pages are active
        tester.requireContents(PageBase1, PAGE_ACTIVE, {
            Record(0, 1),
            Record(1, 2),
            Record(2, 3)
        });

        tester.requireContents(PageBase2, PAGE_ACTIVE, {
            Record(0, 1),
            Record(1, 20),
            Record(2, 30)
        });

        THEN("Page 1 remains the active page")
        {
            REQUIRE(eeprom.getActivePage() == Page1);
        }

        THEN("Redoing the page swap works")
        {
            performSwap();

            requireSwapCompleted();
        }
    }
}

TEST_CASE("Erasable page", "[eeprom]")
{
    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    SECTION("One active page, one erased page")
    {
        tester.populate(PageBase1, PAGE_ACTIVE);
        tester.populate(PageBase2, PAGE_ERASED);

        eeprom.updateActivePage();

        THEN("No page needs to be erased")
        {
            REQUIRE(eeprom.getPendingErasePage() == NoPage);
            REQUIRE(eeprom.hasPendingErase() == false);
        }
    }

    SECTION("One active page, one inactive page")
    {
        tester.populate(PageBase1, PAGE_ACTIVE);
        tester.populate(PageBase2, PAGE_INACTIVE);

        eeprom.updateActivePage();

        THEN("The old page needs to be erased")
        {
            REQUIRE(eeprom.getPendingErasePage() == Page2);
            REQUIRE(eeprom.hasPendingErase() == true);
        }

        THEN("Erasing the old page clear it")
        {
            eeprom.performPendingErase();

            REQUIRE(eeprom.getPendingErasePage() == NoPage);
            REQUIRE(eeprom.hasPendingErase() == false);
        }
    }

    SECTION("2 active pages")
    {
        tester.populate(PageBase1, PAGE_ACTIVE);
        tester.populate(PageBase2, PAGE_ACTIVE);

        eeprom.updateActivePage();

        THEN("Page 2 needs to be erased")
        {
            REQUIRE(eeprom.getPendingErasePage() == Page2);
            REQUIRE(eeprom.hasPendingErase() == true);
        }
    }
}

TEST_CASE("Flash wear validation", "[eeprom]")
{
    struct Point
    {
        double x, y;
    };

    TestEEPROM eeprom;
    EEPROMTester tester(eeprom);

    tester.populate(PageBase1, PAGE_ACTIVE);
    tester.populate(PageBase2, PAGE_ERASED);
    eeprom.store.resetEraseCount();

    eeprom.init();

    int writeCount = 5000;

    INFO("Writing " << writeCount << " records of " << sizeof(Point) << " bytes");
    for(int i = 0; i < writeCount; i++)
    {
        Point p { (double)i, (double)i };
        eeprom.put(0, &p, sizeof(p));
    }

    int expectedErases = 3;
    INFO("Expected at most " << expectedErases << " erases");
    REQUIRE(eeprom.store.getEraseCount() <= expectedErases);
}

void loadEEPROMFromFile(const char *filename, TestEEPROM &eeprom, uintptr_t pageAddress, size_t pageSize)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(!file)
    {
        FAIL("Could not load EEPROM dump " << filename);
        return;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
    file.read((char *)buffer.get(), size);

    eeprom.store.eraseSector(pageAddress);
    eeprom.store.write(pageAddress, buffer.get(), std::min(size, pageSize));
}

TEST_CASE("Migration from legacy format", "[eeprom]")
{
    TestEEPROM eeprom;

    // Load EEPROM extracted from a Photon into memory
    loadEEPROMFromFile("eeprom_page1.bin", eeprom, PageBase1, PageSize1);
    eeprom.store.eraseSector(PageBase2);

    eeprom.init();

    REQUIRE(eeprom.getActivePage() == Page1);

    // Instances of this struct were saved in the EEPROM
    struct Point
    {
        double x, y;

        bool valid()
        {
            return !std::isnan(x) && !std::isnan(y);
        }
    };

    Point point;

    // Make sure the old EEPROM format can be read
    eeprom.get(0, &point, sizeof(point));
    REQUIRE(point.valid() == true);
    REQUIRE(point.x == 21092.0);
    REQUIRE(point.y == 21095.0);

    // EEPROM was almost full so write a few more records...
    for(int i = 0; i < 2; i++)
    {
        point.x += 1;
        point.y += 1;

        eeprom.put(0, &point, sizeof(point));
    }

    REQUIRE(eeprom.getActivePage() == Page1);

    // ...then write another one to trigger a page swap
    point.x += 1;
    point.y += 1;

    eeprom.put(0, &point, sizeof(point));

    REQUIRE(eeprom.getActivePage() == Page2);

    REQUIRE(eeprom.hasPendingErase() == true);
    eeprom.performPendingErase();
    REQUIRE(eeprom.hasPendingErase() == false);

    // Data is still valid
    eeprom.get(0, &point, sizeof(point));
    REQUIRE(point.valid() == true);
    REQUIRE(point.x == 21095.0);
    REQUIRE(point.y == 21098.0);
}

TEST_CASE("Recover from data corruption", "[eeprom]")
{
    TestEEPROM eeprom;

    // Load corrupted EEPROM extracted from a Photon into memory
    loadEEPROMFromFile("corrupted_eeprom_page1.bin", eeprom, PageBase1, PageSize1);
    loadEEPROMFromFile("corrupted_eeprom_page2.bin", eeprom, PageBase2, PageSize2);

    eeprom.init();

    uint8_t number;

    eeprom.get(0, number);

    REQUIRE(number != 0xFF);

    number = 100;

    eeprom.put(0, number);

    uint8_t newNumber;
    eeprom.get(0, newNumber);

    REQUIRE(newNumber == number);
}

TEST_CASE("Write works for all indexes", "[eeprom]")
{
    TestEEPROM eeprom;
    eeprom.init();

    uint16_t index;
    uint8_t data;

    // when
    for(index = 0, data = 0; index < eeprom.capacity(); index++, data++)
    {
        eeprom.put(index, data);
    }

    // then
    for(index = 0, data = 0; index < eeprom.capacity(); index++, data++)
    {
        uint8_t dataRead;
        eeprom.get(index, dataRead);
        CAPTURE(index);
        REQUIRE(dataRead == data);
    }
}

TEST_CASE("Write ignored for any address out of range", "[eeprom]")
{
    TestEEPROM eeprom;
    eeprom.init();
    uint16_t index;
    uint8_t data;

    // when
    for(index = eeprom.capacity(), data = 0; index < eeprom.capacity() + 10; index++, data++)
    {
        eeprom.put(index, data);
    }

    // then
    for(index = eeprom.capacity(), data = 0; index < eeprom.capacity() + 10; index++, data++)
    {
        uint8_t dataRead;
        eeprom.get(index, dataRead);
        CAPTURE(index);
        REQUIRE(dataRead != data);
    }
}

TEST_CASE("Page swap with data in multiple batches", "[eeprom]")
{
    TestEEPROM eeprom;
    eeprom.init();
    uint16_t index;
    uint8_t data;

    // when
    for(index = 0, data = 10; index <= 500; index += 100, data += 10)
    {
        eeprom.put(index, data);
    }

    // Force a page swap
    eeprom.swapPagesAndWrite(1, nullptr, 0);

    // then
    for(index = 0, data = 10; index <= 500; index += 100, data += 10)
    {
        uint8_t dataRead;
        eeprom.get(index, dataRead);
        CAPTURE(index);
        REQUIRE(dataRead == data);
    }
}
