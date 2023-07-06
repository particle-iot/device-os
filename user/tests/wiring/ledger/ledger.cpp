#include "application.h"
#include "unit-test/unit-test.h"

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

test(03_replace) {
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        assertEqual(ledger.set(d, Ledger::REPLACE), 0);
        assertMore(ledger.dataSize(), 0);
    }
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = ledger.get();
        assertTrue((d == LedgerData{ { "a", 1 }, { "b", 2 }, { "c", 3 } }));
    }
}

test(04_merge) {
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = { { "d", 4 }, { "f", 6 } };
        assertEqual(ledger.set(d), 0); // Replaces the current data by default
        d = { { "e", 5 } };
        assertEqual(ledger.set(d, Ledger::MERGE), 0);
    }
    {
        auto ledger = Particle.ledger("test");
        LedgerData d = ledger.get();
        assertTrue((d == LedgerData{ { "d", 4 }, { "e", 5 }, { "f", 6 } }));
    }
}

test(05_sync_callback) {
    auto ledger = Particle.ledger("test");

    // Register a functor callback
    ledger.onSync(CountingCallback());
    assertEqual(CountingCallback::instanceCount, 1);

    // Register a C callback
    Ledger::OnSyncCallback cb = [](Ledger ledger, void* arg) {};
    ledger.onSync(cb);
    assertEqual(CountingCallback::instanceCount, 0);

    // Register a functor callback again callback
    ledger.onSync(CountingCallback());
    assertEqual(CountingCallback::instanceCount, 1);

    // Unregister the callback
    ledger.onSync(nullptr);
    assertEqual(CountingCallback::instanceCount, 0);
}

test(06_purge) {
    // Remove the test ledger files
    assertEqual(ledger_purge("test", nullptr), 0);
}
