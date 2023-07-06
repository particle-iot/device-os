#include "application.h"
#include "unit-test/unit-test.h"

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
        assertEqual(ledger.set(d), 0); // Replaces by default
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

    // Register a C callback
    Ledger::OnSyncCallback cb = [](Ledger ledger, void* arg) {
    };
    ledger.onSync(cb);

    // Register a functor callback
    struct Callback {
        bool& destroyed;

        explicit Callback(bool& destroyed) :
                destroyed(destroyed) {
        }

        ~Callback() {
            destroyed = true;
        }

        void operator()(Ledger ledger) {
        }
    };

    bool functorDestroyed = false;
    ledger.onSync(Callback(functorDestroyed));
    assertFalse(functorDestroyed);

    // Unregister the callback
    ledger.onSync(nullptr);
    assertTrue(functorDestroyed);
}

test(06_purge) {
    assertEqual(ledger_purge("test", nullptr), 0);
}
