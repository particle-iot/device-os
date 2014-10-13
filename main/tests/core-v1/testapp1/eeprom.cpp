
#include "application.h"
#include "unit-test/unit-test.h"

#define EEPROM_SIZE ((uint8_t)0x64) /* 100 bytes (Max 255/0xFF bytes) as per eeprom_hal.c */

test(EEPROM_ReadWriteSucceedsForAllAddressWithInRange) {
    // when
    for(int i=0;i<EEPROM_SIZE;i++)
    {
        EEPROM.write(i, i);
    }
    // then
    for(int i=0;i<EEPROM_SIZE;i++)
    {
        assertEqual(EEPROM.read(i), i);
    }
}

test(EEPROM_ReadWriteFailsForAnyAddressOutOfRange) {
    // when
    for(int i=EEPROM_SIZE;i<(2*EEPROM_SIZE);i++)
    {
        EEPROM.write(i, i);
    }
    // then
    for(int i=EEPROM_SIZE;i<(2*EEPROM_SIZE);i++)
    {
        assertNotEqual(EEPROM.read(i), i);
    }
}

