#include <cstring>

#include "application.h"
#include "unit-test/unit-test.h"

#include "scope_guard.h"

#if Wiring_Ledger

namespace {

const auto LEDGER_NAME = "test";

struct CountingCallback {
    static int instanceCount;

    CountingCallback() {
        ++instanceCount;
    }

    CountingCallback(const CountingCallback&) {
        ++instanceCount;
    }

    ~CountingCallback() {
        --instanceCount;
    }

    void operator()(Ledger ledger) {
    }

    CountingCallback& operator=(const CountingCallback&) {
        ++instanceCount;
        return *this;
    }
};

int CountingCallback::instanceCount = 0;

} // namespace

test(01_remove_all) {
    assertEqual(Ledger::removeAll(), 0);
}

test(02_initial_state) {
    auto ledger = Particle.ledger(LEDGER_NAME);
    assertTrue(ledger.isValid());
    assertEqual(std::strcmp(ledger.name(), LEDGER_NAME), 0);
    assertTrue(ledger.scope() == LedgerScope::UNKNOWN);
    assertTrue(ledger.isWritable());
    assertEqual(ledger.lastUpdated(), 0);
    assertEqual(ledger.lastSynced(), 0);
    assertEqual(ledger.dataSize(), 0);
}

test(03_replace_data) {
    assertEqual(ledger_purge(LEDGER_NAME, nullptr), 0);
    {
        auto ledger = Particle.ledger(LEDGER_NAME);
        LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        assertEqual(ledger.set(d, Ledger::REPLACE), 0);
        assertMore(ledger.dataSize(), 0);
    }
    {
        auto ledger = Particle.ledger(LEDGER_NAME);
        auto d = ledger.get();
        assertTrue((d == LedgerData{ { "a", 1 }, { "b", 2 }, { "c", 3 } }));
    }
}

test(04_merge_data) {
    assertEqual(ledger_purge(LEDGER_NAME, nullptr), 0);
    {
        auto ledger = Particle.ledger(LEDGER_NAME);
        LedgerData d = { { "d", 4 }, { "f", 6 } };
        assertEqual(ledger.set(d), 0); // Replaces the current data by default
        d = { { "e", 5 } };
        assertEqual(ledger.set(d, Ledger::MERGE), 0);
    }
    {
        auto ledger = Particle.ledger(LEDGER_NAME);
        auto d = ledger.get();
        assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
    }
}

test(05_concurrent_writing) {
    assertEqual(ledger_purge(LEDGER_NAME, nullptr), 0);

    // Open two ledger streams for writing
    ledger_instance* lr = nullptr;
    assertEqual(ledger_get_instance(&lr, LEDGER_NAME, nullptr), 0);
    SCOPE_GUARD({
        ledger_release(lr, nullptr);
    });
    ledger_stream* ls1 = nullptr;
    assertEqual(ledger_open(&ls1, lr, LEDGER_STREAM_MODE_WRITE, nullptr), 0);
    NAMED_SCOPE_GUARD(g1, {
        ledger_close(ls1, 0, nullptr);
    });
    ledger_stream* ls2 = nullptr;
    assertEqual(ledger_open(&ls2, lr, LEDGER_STREAM_MODE_WRITE, nullptr), 0);
    NAMED_SCOPE_GUARD(g2, {
        ledger_close(ls2, 0, nullptr);
    });

    // Write some data to both streams
    char cbor1[] = { 0xa3, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02, 0x61, 0x63, 0x03 }; // {"a":1,"b":2,"c":3}
    assertEqual(ledger_write(ls1, cbor1, sizeof(cbor1), nullptr), sizeof(cbor1));
    char cbor2[] = { 0xa3, 0x61, 0x64, 0x04, 0x61, 0x65, 0x05, 0x61, 0x66, 0x06 }; // {"d":4,"e":5,"f":6}
    assertEqual(ledger_write(ls2, cbor2, sizeof(cbor2), nullptr), sizeof(cbor2));

    // Close the first stream and check that the data can be read back
    g1.dismiss();
    assertEqual(ledger_close(ls1, 0, nullptr), 0);
    auto ledger = Particle.ledger(LEDGER_NAME);
    auto d = ledger.get();
    assertTrue((d == LedgerData{ { "a", 1 }, { "b", 2 }, { "c", 3 } }));

    // Close the second stream and check that the data can be read back
    g2.dismiss();
    assertEqual(ledger_close(ls2, 0, nullptr), 0);
    d = ledger.get();
    assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
}

test(06_concurrent_reading_and_writing) {
    assertEqual(ledger_purge(LEDGER_NAME, nullptr), 0);

    // Set some initial data
    auto ledger = Particle.ledger(LEDGER_NAME);
    LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    assertEqual(ledger.set(d), 0);

    // Open the ledger for reading
    ledger_instance* lr = nullptr;
    assertEqual(ledger_get_instance(&lr, LEDGER_NAME, nullptr), 0);
    SCOPE_GUARD({
        ledger_release(lr, nullptr);
    });
    ledger_stream* ls = nullptr;
    assertEqual(ledger_open(&ls, lr, LEDGER_STREAM_MODE_READ, nullptr), 0);
    SCOPE_GUARD({
        ledger_close(ls, 0, nullptr);
    });

    // Set some new data
    d = { { "d", 4 }, { "e", 5 }, { "f", 6 } };
    assertEqual(ledger.set(d), 0);

    // Proceed to read the data
    char expectedCbor[] = { 0xa3, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02, 0x61, 0x63, 0x03 }; // {"a":1,"b":2,"c":3}
    char cbor[128];
    int n = ledger_read(ls, cbor, sizeof(cbor), nullptr);

    // The reader opened before the modification should see the original data
    assertEqual(n, (int)sizeof(expectedCbor));
    assertEqual(std::memcmp(cbor, expectedCbor, sizeof(expectedCbor)), 0);

    // The reader opened after the modification should see the actual data
    d = ledger.get();
    assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
}

test(07_multiple_staged_data_files) {
    assertEqual(ledger_purge(LEDGER_NAME, nullptr), 0);

    // Open the ledger for reading
    ledger_instance* lr = nullptr;
    assertEqual(ledger_get_instance(&lr, LEDGER_NAME, nullptr), 0);
    SCOPE_GUARD({
        ledger_release(lr, nullptr);
    });
    ledger_stream* ls = nullptr;
    assertEqual(ledger_open(&ls, lr, LEDGER_STREAM_MODE_READ, nullptr), 0);
    NAMED_SCOPE_GUARD(closeStreamGuard, {
        ledger_close(ls, 0, nullptr);
    });

    // Write some data to the ledger. This will cause the ledger to create a staged data file as it
    // can't overwrite the current one
    Ledger ledger = Particle.ledger(LEDGER_NAME);
    LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    assertEqual(ledger.set(d), 0);

    // Open another ledger stream that would read the staged data. This will cause the ledger to
    // create a new staged data file rather than overwrite the existing one when Ledger::set() is
    // called below
    ledger_stream* ls2 = nullptr;
    assertEqual(ledger_open(&ls2, lr, LEDGER_STREAM_MODE_READ, nullptr), 0);
    NAMED_SCOPE_GUARD(closeStreamGuard2, {
        ledger_close(ls2, 0, nullptr);
    });

    // Write some new data to the ledger
    d = { { "d", 4 }, { "e", 5 }, { "f", 6 } };
    assertEqual(ledger.set(d), 0);

    // Close the streams
    closeStreamGuard.dismiss();
    assertEqual(ledger_close(ls, 0, nullptr), 0);
    closeStreamGuard2.dismiss();
    assertEqual(ledger_close(ls2, 0, nullptr), 0);

    // Validate the ledger data
    d = ledger.get();
    assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
}

test(08_set_sync_callback) {
    auto ledger = Particle.ledger(LEDGER_NAME);

    // Register a functor callback
    ledger.onSync(CountingCallback());
    assertEqual(CountingCallback::instanceCount, 1);

    // Register a C callback
    Ledger::OnSyncCallback cb = [](Ledger ledger, void* arg) {};
    ledger.onSync(cb);
    assertEqual(CountingCallback::instanceCount, 0);

    // Register a functor callback again
    ledger.onSync(CountingCallback());
    assertEqual(CountingCallback::instanceCount, 1);

    // Unregister the callback
    ledger.onSync(nullptr);
    assertEqual(CountingCallback::instanceCount, 0);
}

test(09_remove) {
    // Remove the test ledger files
    assertEqual(Ledger::remove(LEDGER_NAME), 0);
}

#endif // Wiring_Ledger
