#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "test.h"
#include "softcrc32.h"
#include "check.h"
#include "storage_hal.h"

namespace {

volatile int updateResult = firmware_update_failed;

struct ReportAsset {
    ApplicationAsset asset;
    uint32_t crc = 0;
    int error = 0;
    bool readable = false;
};

uint16_t getProductVersion() {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    const int r = system_info_get_unstable(&info, 0 /* flags */, nullptr /* reserved */);
    if (r != 0) {
        return 0xffff;
    }
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr /* reserved */);
    });
    hal_module_t* module = nullptr;
    for (size_t i = 0; i < info.module_count; ++i) {
        auto mod = &info.modules[i];
        if (mod->info.module_function == MODULE_FUNCTION_USER_PART && mod->bounds.store == MODULE_STORE_MAIN) {
            if (!module || mod->bounds.module_index > module->bounds.module_index) {
                module = mod;
            }
        }
    }
    auto storageId = HAL_STORAGE_ID_INVALID;
    if (module->bounds.location == MODULE_BOUNDS_LOC_INTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_INTERNAL_FLASH;
    } else if (module->bounds.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_EXTERNAL_FLASH;
    }
    if (storageId == HAL_STORAGE_ID_INVALID) {
        return 0xffff;
    }

    module_info_product_data_ext_t product = {};
    uintptr_t productDataStart = (uintptr_t)module->info.module_end_address - sizeof(module_info_suffix_base_t) - sizeof(module_info_product_data_ext_t);
    if (hal_storage_read(storageId, productDataStart, (uint8_t*)&product, sizeof(product))) {
        return 0xffff;
    }
    return product.version;
}

// This is a replacement for `PRODUCT_VERSION(xxx)`. We are going to be modifying product extension in JS side of tests
// so, need to read from the actual suffix/extension.
STARTUP(spark_protocol_set_product_firmware_version(spark_protocol_instance(), getProductVersion()));

int readAndCalculateAssetCrc(ApplicationAsset& asset, uint32_t* outCrc, bool resetAndSkip) {
    uint32_t crc = 0;
    SCOPE_GUARD({
        asset.reset();
    });
    asset.reset();
    CHECK_TRUE(asset.isValid() && asset.isReadable(), SYSTEM_ERROR_BAD_DATA);
    char buf[256];
    size_t size = asset.size();
    size_t pos = 0;
    while (asset.available() > 0) {
        size_t r = CHECK(asset.read(buf, sizeof(buf)));
        pos += r;
        crc = particle::softCrc32((const uint8_t*)buf, r, &crc);
        // This call is here so that test runner readMailbox() requests
        // are processed in a timely manner and we don't lose any messages.
        Particle.process();
        if (resetAndSkip && asset.available() <= size / 2) {
            asset.reset();
            CHECK(asset.skip(pos));
            resetAndSkip = false;
        }
    }
    *outCrc = crc;
    return 0;
}

void serializeAsset(JSONBufferWriter& w, ReportAsset& asset) {
    w.beginObject();
    w.name("name");
    w.value(asset.asset.name());
    w.name("size");
    w.value(asset.asset.size());
    w.name("storageSize");
    w.value(asset.asset.storageSize());
    w.name("valid");
    w.value(asset.asset.isValid());
    w.name("readable");
    w.value(asset.readable);
    w.name("hash");
    w.value(asset.asset.hash().toString());
    w.name("crc");
    w.value(asset.crc);
    w.name("error");
    w.value(asset.error);
    w.endObject();
}

size_t serializeAssetReportAsJson(char* buf, size_t size, spark::Vector<ReportAsset> available, spark::Vector<ReportAsset> required) {
    JSONBufferWriter w(buf, size);

    w.beginObject();
    w.name("available");
    w.beginArray();
    for (auto& asset: available) {
        serializeAsset(w, asset);
    }
    w.endArray();

    w.name("required");
    w.beginArray();
    for (auto& asset: required) {
        serializeAsset(w, asset);
    }
    w.endArray();
    w.endObject();

    return w.dataSize();
}

int validateAndReportAssets(int limit = 0, bool resetAndSkip = false) {
    auto required = System.assetsRequired();
    auto available = System.assetsAvailable();

    spark::Vector<ReportAsset> reportedAvailable;
    spark::Vector<ReportAsset> reportedRequired;

    for (auto& asset: available) {
        int i = required.indexOf(asset);
        if (i < 0 || asset != required[i]) {
            continue;
        }

        ReportAsset rep;
        rep.asset = asset;
        rep.readable = rep.asset.isReadable();
        rep.error = readAndCalculateAssetCrc(rep.asset, &rep.crc, resetAndSkip);
        rep.asset.reset();
        reportedAvailable.append(rep);

        if (limit > 0 && reportedAvailable.size() == limit) {
            break;
        }
    }

    for (auto& asset: required) {
        ReportAsset rep;
        rep.asset = asset;
        reportedRequired.append(rep);
    }

    auto size = serializeAssetReportAsJson(nullptr, 0, reportedAvailable, reportedRequired);
    // TODO: some issue in size calculation? Last '}' sometimes does not get filled
    auto buf = std::make_unique<char[]>(size + 10);
    if (buf) {
        memset(buf.get(), 0, size + 10);
        serializeAssetReportAsJson(buf.get(), size + 9, reportedAvailable, reportedRequired);
        int r = TestRunner::instance()->pushMailboxMsg(buf.get(), 10000);
        return r;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

bool hookExecuted = false;
spark::Vector<ApplicationAsset> hookAssets;

void handleAvailableAssets(spark::Vector<ApplicationAsset> assets) {
    // Called before setup
    hookExecuted = true;
    hookAssets = assets;
}

} // namespace

STARTUP(System.onAssetOta(handleAvailableAssets));

test(01_ad_hoc_ota_start) {
    assertEqual(0, System.assetsRequired().size());
    assertEqual(0, asset_manager_format_storage(nullptr));
    assertEqual(0, System.assetsAvailable().size());
    System.disableReset();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 10 * 60 * 1000));

    System.on(firmware_update, [](system_event_t ev, int data, void* context) {
        updateResult = data;
    });

    updateResult = SYSTEM_ERROR_OTA;

    System.assetsHandled();
}

test(02_ad_hoc_ota_wait) {
    SCOPE_GUARD({
        System.off(all_events);
        System.enableReset();
    });

    for (auto start = millis(); millis() - start <= 5 * 60 * 1000;) {
        if (updateResult == firmware_update_complete || updateResult == firmware_update_failed || updateResult == firmware_update_pending) {
            break;
        } else if (updateResult == SYSTEM_ERROR_OTA && millis() - start >= 1 * 60 * 1000) {
            break;
        }
        Particle.process();
        delay(100);
    }

    assertNotEqual((int)updateResult, (int)firmware_update_failed);
    assertNotEqual((int)updateResult, (int)SYSTEM_ERROR_OTA);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::SAFE_MODE_PENDING), 5000));
}

test(03_ad_hoc_ota_complete) {
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 10 * 60 * 1000));
    assertEqual(0, validateAndReportAssets());
    assertTrue(hookExecuted);
    assertTrue(hookAssets == System.assetsAvailable());
}

test(04_ad_hoc_ota_restore) {
    updateResult = firmware_update_failed;
    Particle.disconnect();
    System.assetsHandled();
    // Just in case
    System.enableReset();
}

test(05_product_ota_start) {
    assertFalse(hookExecuted);
    assertEqual(0, hookAssets.size());
    assertEqual(0, System.assetsRequired().size());
    assertEqual(0, asset_manager_format_storage(nullptr));
    assertEqual(0, System.assetsAvailable().size());
    System.disableReset();
    // IMPORTANT: report version as 0xffff, so that we can get flashed
    spark_protocol_set_product_firmware_version(spark_protocol_instance(), 0xffff);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 10 * 60 * 1000));

    System.on(firmware_update, [](system_event_t ev, int data, void* context) {
        updateResult = data;
    });

    updateResult = SYSTEM_ERROR_OTA;
    System.assetsHandled();
}

test(06_product_ota_wait) {
    SCOPE_GUARD({
        System.off(all_events);
        System.enableReset();
    });

    for (auto start = millis(); millis() - start <= 5 * 60 * 1000;) {
        if (updateResult == firmware_update_complete || updateResult == firmware_update_failed || updateResult == firmware_update_pending) {
            break;
        } else if (updateResult == SYSTEM_ERROR_OTA && millis() - start >= 1 * 60 * 1000) {
            break;
        }
        Particle.process();
        delay(100);
    }

    assertNotEqual((int)updateResult, (int)firmware_update_failed);
    assertNotEqual((int)updateResult, (int)SYSTEM_ERROR_OTA);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::SAFE_MODE_PENDING), 5000));
}

test(07_product_ota_complete) {
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 10 * 60 * 1000));
    assertEqual(0, validateAndReportAssets());
    assertTrue(hookExecuted);
    assertTrue(hookAssets == System.assetsAvailable());
}

test(08_product_ota_complete_handled) {
    System.assetsHandled(true);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.reset();
}

test(09_assets_handled_hook) {
    assertFalse(hookExecuted);
    assertEqual(0, hookAssets.size());
    assertNotEqual(0, System.assetsAvailable().size());
}

test(10_assets_read_skip_reset) {
    assertEqual(0, validateAndReportAssets(0, true /* resetAndSkip */));
}

test(11_assets_available_after_eof_reports_zero) {
    auto asset = System.assetsAvailable()[0];
    char buf[256];
    while (asset.available()) {
        assertLessOrEqual(asset.read(buf, sizeof(buf)), (int)sizeof(buf));
    }
    assertEqual(asset.available(), 0);
    assertEqual(asset.read(buf, sizeof(buf)), (int)SYSTEM_ERROR_END_OF_STREAM);
    assertEqual(asset.available(), 0);
    asset.reset();
    assertMore(asset.available(), 0);
    assertLessOrEqual(asset.read(buf, sizeof(buf)), (int)sizeof(buf));
    asset.reset();
}

test(12_assets_read_using_filesystem) {
#if HAL_PLATFORM_INFLATE_USE_FILESYSTEM
    const size_t FREE_MEM_TO_LEAVE = 30 * 1024;
    auto freeMem = System.freeMemory();
    std::unique_ptr<uint8_t[]> dummy;
    if (freeMem > FREE_MEM_TO_LEAVE) {
        dummy.reset(new uint8_t[freeMem - FREE_MEM_TO_LEAVE]);
        assertTrue(dummy.get());
    }
    assertEqual(0, validateAndReportAssets(1));
#else
    skip();
#endif // HAL_PLATFORM_INFLATE_USE_FILESYSTEM
}

test(99_product_ota_restore) {
    // Just in case
    System.enableReset();
}