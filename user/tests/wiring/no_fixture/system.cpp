
#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"

test(SYSTEM_01_freeMemory)
{
    // this test didn't work on the core attempting to allocate the current value of
    // freeMemory(), presumably because of fragmented heap from
    // relatively large allocations during the handshake, so the request was satisfied
    // without calling _sbrk()
    // 4096 was chosen to be small enough to allocate, but large enough to force _sbrk() to be called.)

    uint32_t free1 = System.freeMemory();
    if (free1>128) {
        void* m1 = malloc(1024*6);
        uint32_t free2 = System.freeMemory();
        free(m1);
        assertLess(free2, free1);
    }
}

test(SYSTEM_02_version)
{
    uint32_t versionNumber = System.versionNumber();
    // Serial.println(System.versionNumber()); // 16908417 -> 0x01020081
    // Serial.println(System.version().c_str()); // 1.2.0-rc.1
    char expected[64] = {};
    // Order of testing here is important to retain
    if ((versionNumber & 0xFF) == 0xFF) {
        sprintf(expected, "%d.%d.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1));
    } else if ((versionNumber & 0xC0) == 0x00) {
        sprintf(expected, "%d.%d.%d-alpha.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((versionNumber & 0xC0) == 0x40) {
        sprintf(expected, "%d.%d.%d-beta.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((versionNumber & 0xC0) == 0x80) {
        sprintf(expected, "%d.%d.%d-rc.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((versionNumber & 0xC0) >= 0xC0) {
        Serial.println("expected \"alpha\", \"beta\", \"rc\", or \"default\" version!");
        assertFalse(true);
    }

    assertEqual( expected, System.version().c_str());
}

// todo - use platform feature flags
#if HAL_PLATFORM_BACKUP_RAM
	// subtract 64 bytes for test runner purposes
    #define USER_BACKUP_RAM ((1024*3)-64)
#endif // HAL_PLATFORM_BACKUP_RAM

#if defined(USER_BACKUP_RAM)

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

static retained uint8_t app_backup[USER_BACKUP_RAM];
static uint8_t app_ram[USER_BACKUP_RAM];

test(SYSTEM_03_user_backup_ram)
{
    int total_backup = 0;
    int total_ram = 0;
    for (unsigned i=0; i<(sizeof(app_backup)/sizeof(app_backup[0])); i++) {
        app_backup[i] = 1;
        app_ram[i] = 1;
        total_backup += app_backup[i];
        total_ram += app_ram[i];
    }
    // Serial.printlnf("app_backup(0x%x), app_ram(0x%x)", &app_backup, &app_ram);
    // Serial.printlnf("total_backup: %d, total_ram: %d", total_backup, total_ram);
    assertTrue(total_backup==(USER_BACKUP_RAM));
    assertTrue(total_ram==(USER_BACKUP_RAM));

	extern uintptr_t link_global_data_start;
	extern uintptr_t link_bss_end;

	uintptr_t ramStart = (uintptr_t)&link_global_data_start;
	uintptr_t ramEnd = (uintptr_t)&link_bss_end;

	assertFalse((uintptr_t)app_backup >= ramStart && (uintptr_t)app_backup <= ramEnd);
}

#endif // defined(USER_BACKUP_RAM)

#if !HAL_PLATFORM_NRF52840 // TODO

#if defined(BUTTON1_MIRROR_SUPPORTED) && PLATFORM_ID != PLATFORM_P1
static int s_button_clicks = 0;
static void onButtonClick(system_event_t ev, int data) {
    s_button_clicks = data;
}

test(SYSTEM_04_button_mirror)
{
    System.buttonMirror(D1, FALLING, false);
    auto pinmap = HAL_Pin_Map();
    System.on(button_click, onButtonClick);

    // "Click" setup button 3 times
    // First click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(100);

    // Second click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(100);

    // Third click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(300);

    assertEqual(s_button_clicks, 3);
}

test(SYSTEM_05_button_mirror_disable)
{
    System.disableButtonMirror(false);
}
#endif // defined(BUTTON1_MIRROR_SUPPORTED)

#endif // !HAL_PLATFORM_NRF52840
