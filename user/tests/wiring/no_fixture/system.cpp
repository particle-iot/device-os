
#include "application.h"
#include "unit-test/unit-test.h"


#if PLATFORM_ID >= 3
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
#endif

test(SYSTEM_02_version)
{
    uint32_t versionNumber = System.versionNumber();
    // Serial.println(System.versionNumber()); // 328193 -> 0x00050201
    // Serial.println(System.version().c_str()); // 0.5.2-rc.1
    char expected[20];
    if (SYSTEM_VERSION & 0xFF)
        sprintf(expected, "%d.%d.%d-rc.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0));
    else
        sprintf(expected, "%d.%d.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1));

    assertTrue(strcmp(expected,System.version().c_str())==0);
}

// todo - use platform feature flags
#if defined(STM32F2XX)
    // subtract 4 bytes for signature (3068 bytes)
    #define USER_BACKUP_RAM ((1024*3)-4)
#endif // defined(STM32F2XX)

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

    if (int(&app_backup) < 0x40024000) {
        Serial.printlnf("ERROR: expected app_backup in user backup memory, but was at %x", &app_backup);
    }
    assertTrue(int(&app_backup)>=0x40024000);

    if (int(&app_ram) >= 0x40024000) {
        Serial.printlnf("ERROR: expected app_ram in user sram memory, but was at %x", &app_ram);
    }
    assertTrue(int(&app_ram)<0x40024000);
}

#endif // defined(USER_BACKUP_RAM)

#if defined(BUTTON1_MIRROR_SUPPORTED)
static int s_button_clicks = 0;
static void onButtonClick(system_event_t ev, int data) {
    s_button_clicks = data;
}

test(SYSTEM_04_button_mirror)
{
    // Known bug:
    // events posted from an ISR might not be delivered to the application queue
    // when threading is enabled
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        skip();
        return;
    }
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