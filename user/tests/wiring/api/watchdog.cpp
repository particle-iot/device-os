
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
        auto caps = WatchdogCap::NONE |
                                       WatchdogCap::RESET |
                                       WatchdogCap::NOTIFY |
                                       WatchdogCap::NOTIFY_ONLY |
                                       WatchdogCap::RECONFIGURABLE |
                                       WatchdogCap::STOPPABLE |
                                       WatchdogCap::SLEEP_RUNNING |
                                       WatchdogCap::DEBUG_RUNNING |
                                       WatchdogCap::ALL;
        (void)caps;
    });
}

test(watchdog_configuration_class) {
    API_COMPILE(WatchdogConfiguration().timeout(1234).capabilities(WatchdogCap::RESET));
    API_COMPILE(WatchdogConfiguration().timeout(10s).capabilities(WatchdogCap::RESET));
}

test(watchdog_class) {
    WatchdogConfiguration config;
    WatchdogInfo info;

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

    API_COMPILE({ auto v = info.mandatoryCapabilities(); });
    API_COMPILE({ auto v = info.capabilities(); });
    API_COMPILE({ auto v = info.configuration(); });
    API_COMPILE({ auto v = info.minTimeout(); });
    API_COMPILE({ auto v = info.maxTimeout(); });
    API_COMPILE({ auto v = info.state(); });
    API_COMPILE({ auto v = config.capabilities(); });
    API_COMPILE({ auto v = config.timeout(); }); 
}

test(external_watchdog_class) {
    WatchdogConfiguration config;
    WatchdogInfo info;

    API_COMPILE({ int ret = ExternalWatchdog.init(config); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.start(); (void)ret; });
    API_COMPILE({ bool ret = ExternalWatchdog.started(); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.stop(); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.refresh(); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.getInfo(info); (void)ret; });

    API_COMPILE({ int ret = ExternalWatchdog.onExpired(onWatchdogExpiredHandlerFunc); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.onExpired(onWatchdogExpiredHandlerFunc, nullptr); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.onExpired([]() {}); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.onExpired(&Handlers::onWatchdogExpired, &watchdogHandler); (void)ret; });
    API_COMPILE({ int ret = ExternalWatchdog.onExpired(std::bind(&Handlers::onWatchdogExpired, &watchdogHandler)); (void)ret; });
}

#endif
