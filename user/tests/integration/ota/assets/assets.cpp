#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "test.h"
#include "softcrc32.h"
#include "check.h"

PRODUCT_VERSION(999);

namespace {

volatile int updateResult = firmware_update_failed;

struct ReportAsset {
    ApplicationAsset asset;
    uint32_t crc = 0;
    int error = 0;
};

int readAndCalculateAssetCrc(ApplicationAsset& asset, uint32_t* outCrc, bool resetAndSkip) {
    uint32_t crc = 0;
    SCOPE_GUARD({
        asset.reset();
    });
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
    w.name("valid");
    w.value(asset.asset.isValid());
    w.name("readable");
    w.value(asset.asset.isReadable());
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
        rep.error = readAndCalculateAssetCrc(rep.asset, &rep.crc, resetAndSkip);
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
    auto buf = std::make_unique<char[]>(size + 1);
    if (buf) {
        memset(buf.get(), 0, size);
        serializeAssetReportAsJson(buf.get(), size + 1, reportedAvailable, reportedRequired);
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

STARTUP(System.onAssetsOta(handleAvailableAssets));

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
    
}