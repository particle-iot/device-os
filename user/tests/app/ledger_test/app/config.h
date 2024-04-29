#pragma once

#include <system_ledger.h>

namespace particle::test {

struct Config {
    bool autoConnect;
    bool restoreConnection;
    bool wasConnected;
    bool removeLedger;
    bool removeAllLedgers;
    bool debugEnabled;

    char removeLedgerName[LEDGER_MAX_NAME_LENGTH + 1];

    void setRestoreConnectionFlag();

    static Config& get();
};

} // namespace particle::test
