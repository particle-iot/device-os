
#include "testapi.h"

#if Wiring_Watchdog == 1

using namespace std::placeholders;

class Handlers {
public:
    void onWatchdogExpired() {
    }
};

Handlers watchdogHandler;

void onWatchdogExpiredHandlerFunc(void* context) {
}

test(watchdog_caps) {
    API_COMPILE({
        EnumFlags<WatchdogCaps> caps = WatchdogCaps::NONE |
                                       WatchdogCaps::RESET |
                                       WatchdogCaps::NOTIFY |
                                       WatchdogCaps::NOTIFY_ONLY |
                                       WatchdogCaps::RECONFIGURABLE |
                                       WatchdogCaps::STOPPABLE |
                                       WatchdogCaps::SLEEP_RUNNING |
                                       WatchdogCaps::DEBUG_RUNNING |
                                       WatchdogCaps::ALL;
        (void)caps;
    });
}

test(watchdog_configuration_class) {
    API_COMPILE(WatchdogConfiguration().timeout(1234).capabilities(WatchdogCaps::RESET));
    API_COMPILE(WatchdogConfiguration().timeout(10s).capabilities(WatchdogCaps::RESET));
}

test(watchdog_class) {
    WatchdogConfiguration config = {};
    WatchdogInfo info = {};

    API_COMPILE({ int ret = Watchdog.init(config); (void)ret; });
    API_COMPILE({ int ret = Watchdog.start(); (void)ret; });
    API_COMPILE({ bool ret = Watchdog.started(); (void)ret; });
    API_COMPILE({ int ret = Watchdog.stop(); (void)ret; });
    API_COMPILE({ int ret = Watchdog.refresh(); (void)ret; });
    API_COMPILE({ int ret = Watchdog.getInfo(info); (void)ret; });

    API_COMPILE({ int ret = Watchdog.onExpired(onWatchdogExpiredHandlerFunc); (void)ret; });
    API_COMPILE({ int ret = Watchdog.onExpired(onWatchdogExpiredHandlerFunc, nullptr); (void)ret; });
    API_COMPILE({ int ret = Watchdog.onExpired([]() {}); (void)ret; });
    API_COMPILE({ int ret = Watchdog.onExpired(&Handlers::onWatchdogExpired, &watchdogHandler); (void)ret; });
    API_COMPILE({ int ret = Watchdog.onExpired(std::bind(&Handlers::onWatchdogExpired, &watchdogHandler)); (void)ret; });
}

#endif
