#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include "application.h"
#include "unit-test/unit-test.h"

#include "scope_guard.h"

namespace {

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

test(01_purge_all) {
    // Remove all ledger files
    assertEqual(ledger_purge_all(nullptr), 0);
}

test(02_initial_state) {
    auto ledger = Particle.ledger("test");
    assertTrue(ledger.isValid());
    assertEqual(std::strcmp(ledger.name(), "test"), 0);
    assertTrue(ledger.scope() == LedgerScope::UNKNOWN);
    assertTrue(ledger.isWritable());
    assertEqual(ledger.lastUpdated(), 0);
    assertEqual(ledger.lastSynced(), 0);
    assertEqual(ledger.dataSize(), 0);
}

test(03_replace_data) {
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        assertEqual(ledger.set(d, Ledger::REPLACE), 0);
        assertMore(ledger.dataSize(), 0);
    }
    {
        auto ledger = Particle.ledger("test");
        auto d = ledger.get();
        assertTrue((d == LedgerData{ { "a", 1 }, { "b", 2 }, { "c", 3 } }));
    }
}

test(04_merge_data) {
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = { { "d", 4 }, { "f", 6 } };
        assertEqual(ledger.set(d), 0); // Replaces the current data by default
        d = { { "e", 5 } };
        assertEqual(ledger.set(d, Ledger::MERGE), 0);
    }
    {
        auto ledger = Particle.ledger("test");
        auto d = ledger.get();
        assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
    }
}

test(05_concurrent_writing) {
    // Open two ledger streams for writing
    ledger_instance* lr = nullptr;
    assertEqual(ledger_get_instance(&lr, "test", nullptr), 0);
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
    String s = "{\"a\":1,\"b\":2,\"c\":3}";
    assertEqual(ledger_write(ls1, s.c_str(), s.length(), nullptr), s.length());
    s = "{\"d\":4,\"e\":5,\"f\":6}";
    assertEqual(ledger_write(ls2, s.c_str(), s.length(), nullptr), s.length());

    // Close the first stream and check that the data can be read back
    g1.dismiss();
    assertEqual(ledger_close(ls1, 0, nullptr), 0);
    auto ledger = Particle.ledger("test");
    auto d = ledger.get();
    assertTrue((d == LedgerData{ { "a", 1 }, { "b", 2 }, { "c", 3 } }));

    // Close the second stream and check that the data can be read back
    g2.dismiss();
    assertEqual(ledger_close(ls2, 0, nullptr), 0);
    d = ledger.get();
    assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
}

test(06_concurrent_reading_and_writing) {
    // Set some initial data
    auto ledger = Particle.ledger("test");
    LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    assertEqual(ledger.set(d), 0);

    // Open the ledger for reading
    ledger_instance* lr = nullptr;
    assertEqual(ledger_get_instance(&lr, "test", nullptr), 0);
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
    String s;
    OutputStringStream ss(s);
    char buf[128];
    for (;;) {
        int r = ledger_read(ls, buf, sizeof(buf), nullptr);
        if (r == SYSTEM_ERROR_END_OF_STREAM) {
            break;
        }
        assertMore(r, 0);
        ss.write((uint8_t*)buf, r);
    }

    // The reader opened before the modification should see the original data
    assertTrue(s == "{\"a\":1,\"b\":2,\"c\":3}");

    // The reader opened after the modification should see the actual data
    d = ledger.get();
    assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
}

test(07_set_sync_callback) {
    auto ledger = Particle.ledger("test");

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

test(08_purge) {
    // Remove the test ledger files
    assertEqual(ledger_purge("test", nullptr), 0);
}

#endif // HAL_PLATFORM_LEDGER
