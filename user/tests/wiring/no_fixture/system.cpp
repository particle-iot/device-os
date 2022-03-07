#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"
#include "scope_guard.h"
#include "ota_flash_hal.h"
#include "storage_hal.h"
#include "endian_util.h"
#include "softcrc32.h"

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

#if HAL_PLATFORM_STM32F2XX // TODO

#if defined(BUTTON1_MIRROR_SUPPORTED) && PLATFORM_ID != PLATFORM_P1
static int s_button_clicks = 0;
static void onButtonClick(system_event_t ev, int data) {
    s_button_clicks = data;
}

test(SYSTEM_04_button_mirror)
{
    System.buttonMirror(D1, FALLING, false);
    auto pinmap = hal_pin_map();
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

#endif // HAL_PLATFORM_STM32F2XX

void findUserAndFactoryModules(hal_system_info_t& info, hal_module_t** user, hal_module_t** factory) {
    for (unsigned i = 0; i < info.module_count; i++) {

        if (info.modules[i].bounds.store == MODULE_STORE_FACTORY && !*factory) {
            *factory = &info.modules[i];
        } else if (info.modules[i].bounds.store == MODULE_STORE_MAIN && info.modules[i].bounds.module_function == MODULE_FUNCTION_USER_PART) {
            // NOTE: there shouldn't be multiple user modules present, but just in case take the one with the highest index
            if (!*user || info.modules[i].bounds.module_index > (*user)->bounds.module_index) {
                *user = &info.modules[i];
            }
        }
    }
}

#if !HAL_PLATFORM_RTL872X // P2 doesn't have factory module
test(SYSTEM_06_system_describe_is_not_overflowed_when_factory_module_present)
{
    hal_system_info_t info = {};
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0, nullptr);
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr);
    });
    assertFalse(info.modules == nullptr);
    hal_module_t* user = nullptr;
    hal_module_t* factory = nullptr;
    findUserAndFactoryModules(info, &user, &factory);
    assertFalse(user == nullptr);
    assertFalse(factory == nullptr);

    // Clear the session data
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    assertTrue(waitFor(Particle.disconnected, 1000));
    // Copy current user-part into factory location
    auto storageId = factory->bounds.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH ? HAL_STORAGE_ID_EXTERNAL_FLASH : HAL_STORAGE_ID_INTERNAL_FLASH;
    int r = hal_storage_erase(storageId, factory->bounds.start_address, factory->bounds.maximum_size);
    SCOPE_GUARD({
        hal_storage_erase(storageId, factory->bounds.start_address, factory->bounds.maximum_size);
    });
    assertEqual(factory->bounds.maximum_size, r);
    module_info_t patchedModuleInfo = user->info;
    module_info_suffix_t patchedModuleSuffix = user->suffix;
    patchedModuleInfo.module_version = 123;
    memset(patchedModuleSuffix.sha, 0x5a, sizeof(patchedModuleSuffix.sha));
    // Using software implementation here bundled with the test, because the appropriate variant of HAL_Core_Compute_CRC32
    // is not exposed (the one that takes the current remainder value) and some platforms have a hardware peripheral for CRC
    // which complicates this even further.
    uint32_t crc = 0;
    if (user->module_info_offset > 0) {
        crc = particle::softCrc32((const uint8_t*)user->bounds.start_address, user->module_info_offset, &crc);
    }
    size_t userModuleSize = (size_t)user->info.module_end_address - (size_t)user->info.module_start_address;
    crc = particle::softCrc32((const uint8_t*)&patchedModuleInfo, sizeof(patchedModuleInfo), &crc);
    crc = particle::softCrc32((const uint8_t*)user->bounds.start_address + user->module_info_offset + sizeof(patchedModuleInfo),
            userModuleSize - sizeof(patchedModuleInfo) - user->module_info_offset - sizeof(patchedModuleSuffix), &crc);
    crc = particle::softCrc32((const uint8_t*)&patchedModuleSuffix, sizeof(patchedModuleSuffix), &crc);
    crc = particle::nativeToBigEndian(crc);

    char buf[256]; // some platforms cannot write into e.g. an external flash directly from internal
    size_t pos = 0;
    while (pos < user->module_info_offset) {
        size_t sz = std::min<size_t>(sizeof(buf), user->module_info_offset - pos);
        memcpy(buf, (const char*)user->bounds.start_address + pos, sz);
        r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)buf, sz);
        assertEqual(sz, r);
        pos += sz;
    }
    r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)&patchedModuleInfo, sizeof(patchedModuleInfo));
    assertEqual(sizeof(patchedModuleInfo), r);
    pos += sizeof(patchedModuleInfo);
    while (pos < userModuleSize - sizeof(patchedModuleSuffix)) {
        size_t sz = std::min<size_t>(sizeof(buf), userModuleSize - sizeof(patchedModuleSuffix) - pos);
        memcpy(buf, (const char*)user->bounds.start_address + pos, sz);
        r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)buf, sz);
        assertEqual(sz, r);
        pos += sz;
    }
    r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)&patchedModuleSuffix, sizeof(patchedModuleSuffix));
    assertEqual(sizeof(patchedModuleSuffix), r);
    pos += sizeof(patchedModuleSuffix);
    r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)&crc, sizeof(crc));
    assertEqual(sizeof(crc), r);

    // Re-request module info to check that the factory binary is in fact valid now
    system_info_free_unstable(&info, nullptr);
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0, nullptr);
    assertFalse(info.modules == nullptr);

    factory = nullptr;
    user = nullptr;
    findUserAndFactoryModules(info, &user, &factory);
    assertFalse(user == nullptr);
    assertFalse(factory == nullptr);
    assertEqual(factory->validity_checked, factory->validity_result);
    assertNotEqual(factory->validity_checked, 0);
    assertTrue(!memcmp(&factory->info, &patchedModuleInfo, sizeof(patchedModuleInfo)));
    assertTrue(!memcmp(&factory->suffix, &patchedModuleSuffix, sizeof(patchedModuleSuffix)));

    // Connect to the cloud, if there is a system describe overflow, we'll trigger assertion failure here
    assertTrue(Particle.disconnected);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(SYSTEM_07_system_describe_is_not_overflowed_when_factory_module_present_but_invalid)
{
    hal_system_info_t info = {};
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0, nullptr);
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr);
    });

    assertFalse(info.modules == nullptr);
    hal_module_t* user = nullptr;
    hal_module_t* factory = nullptr;
    findUserAndFactoryModules(info, &user, &factory);

    assertFalse(user == nullptr);
    assertFalse(factory == nullptr);

    // Clear the session data
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    assertTrue(waitFor(Particle.disconnected, 1000));

    // Copy HALF of current user-part into factory location
    size_t toCopy = std::min<size_t>(((uintptr_t)user->info.module_end_address - (uintptr_t)user->info.module_start_address) / 2, factory->bounds.maximum_size);
    auto storageId = factory->bounds.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH ? HAL_STORAGE_ID_EXTERNAL_FLASH : HAL_STORAGE_ID_INTERNAL_FLASH;
    int r = hal_storage_erase(storageId, factory->bounds.start_address, factory->bounds.maximum_size);
    SCOPE_GUARD({
        hal_storage_erase(storageId, factory->bounds.start_address, factory->bounds.maximum_size);
    });
    assertEqual(factory->bounds.maximum_size, r);
    char buf[256]; // some platforms cannot write into e.g. an external flash directly from internal
    for (size_t pos = 0; pos < toCopy; pos += sizeof(buf)) {
        size_t sz = std::min(sizeof(buf), toCopy - pos);
        memcpy(buf, (const char*)user->bounds.start_address + pos, sz);
        r = hal_storage_write(storageId, factory->bounds.start_address + pos, (const uint8_t*)buf, sz);
        assertEqual(sz, r);
    }
    // Re-request module info to check that the factory binary is in fact invalid now
    system_info_free_unstable(&info, nullptr);
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0, nullptr);
    assertFalse(info.modules == nullptr);

    factory = nullptr;
    user = nullptr;
    findUserAndFactoryModules(info, &user, &factory);
    assertFalse(user == nullptr);
    assertFalse(factory == nullptr);
    assertNotEqual(factory->validity_checked, factory->validity_result);
    assertNotEqual(factory->validity_checked, 0);

    // Connect to the cloud, if there is a system describe overflow, we'll trigger assertion failure here
    assertTrue(Particle.disconnected);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}
#endif // !HAL_PLATFORM_RTL872X

test(SYSTEM_08_system_event_subscription) {
    SCOPE_GUARD({
        System.off(all_events);
    });
    int lastEvent = 0;
    auto subscription = System.on(cloud_status, [&lastEvent](system_event_t ev) {
        lastEvent = ev;
    });
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(lastEvent, (int)cloud_status);

    System.off(subscription);
    assertTrue(Particle.disconnected);
    lastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(lastEvent, 0);

    subscription = System.on(cloud_status, [&lastEvent](system_event_t ev) {
        lastEvent = ev;
    });
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(lastEvent, (int)cloud_status);

    System.off(cloud_status);
    assertTrue(Particle.disconnected);
    lastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(lastEvent, 0);
}

namespace {
int sLastEvent = 0;
} // anonymous

test(SYSTEM_09_system_event_subscription_funcptr_or_non_capturing_lambda) {
    SCOPE_GUARD({
        System.off(all_events);
    });
    auto handler = [](system_event_t ev, int data, void*) {
        sLastEvent = ev;
    };
    assertTrue((bool)System.on(cloud_status, handler));
    assertTrue(Particle.connected());
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, (int)cloud_status);

    // System.off(event)
    System.off(cloud_status);
    assertTrue(Particle.disconnected);
    sLastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, 0);

    assertTrue((bool)System.on(cloud_status, handler));
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, (int)cloud_status);

    // System.off(event, handler)
    System.off(cloud_status, handler);
    assertTrue(Particle.disconnected);
    sLastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, 0);

    assertTrue((bool)System.on(cloud_status, handler));
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, (int)cloud_status);

    // System.off(handler)
    System.off(handler);
    assertTrue(Particle.disconnected);
    sLastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, 0);

    assertTrue((bool)System.on(cloud_status, handler));
    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, 5000));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, (int)cloud_status);

    // System.off(event)
    System.off(cloud_status);
    assertTrue(Particle.disconnected);
    sLastEvent = 0;
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // Events are delivered on application thread when threading is enabled, make sure to process queue
    delay(100);
    Particle.process();
    assertEqual(sLastEvent, 0);
}