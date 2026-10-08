#include "system_sleep_configuration.h"
#include "spark_wiring_system.h"

#include "util/catch.h"

#include <cstdlib>
#include <new>
#include <utility>

using namespace particle;

namespace {

class AllocTracker {
public:
    static AllocTracker& instance() {
        static AllocTracker tracker;
        return tracker;
    }

    void start() {
        for (auto& p : live_) {
            p = nullptr;
        }
        overflow_ = false;
        enabled_ = true;
    }

    void stop() {
        enabled_ = false;
    }

    size_t liveCount() const {
        size_t count = 0;
        for (auto p : live_) {
            if (p) {
                ++count;
            }
        }
        return count;
    }

    bool overflow() const {
        return overflow_;
    }

    void allocated(void* ptr) {
        if (!enabled_ || !ptr) {
            return;
        }
        for (auto& p : live_) {
            if (!p) {
                p = ptr;
                return;
            }
        }
        overflow_ = true;
    }

    void freed(void* ptr) {
        if (!enabled_ || !ptr) {
            return;
        }
        for (auto& p : live_) {
            if (p == ptr) {
                p = nullptr;
                return;
            }
        }
    }

private:
    void* live_[64] = {};
    bool enabled_ = false;
    bool overflow_ = false;
};

class TrackAllocations {
public:
    TrackAllocations() {
        AllocTracker::instance().start();
    }

    ~TrackAllocations() {
        AllocTracker::instance().stop();
    }

    size_t liveCount() const {
        return AllocTracker::instance().liveCount();
    }

    bool overflow() const {
        return AllocTracker::instance().overflow();
    }
};

size_t countWakeupSources(const SystemSleepConfiguration& config) {
    size_t count = 0;
    for (auto source = config.halConfig()->wakeup_sources; source; source = source->next) {
        ++count;
    }
    return count;
}

const hal_wakeup_source_network_t* findNetworkWakeupSource(const SystemSleepConfiguration& config) {
    for (auto source = config.halConfig()->wakeup_sources; source; source = source->next) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            return reinterpret_cast<const hal_wakeup_source_network_t*>(source);
        }
    }
    return nullptr;
}

void configure(SystemSleepConfiguration& config) {
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(D2, RISING).gpio(D3, FALLING).duration(5000);
}

hal_wakeup_source_gpio_t makeGpioWakeupSource(hal_pin_t pin) {
    hal_wakeup_source_gpio_t gpio = {};
    gpio.base.size = sizeof(gpio);
    gpio.base.version = HAL_SLEEP_VERSION;
    gpio.base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
    gpio.pin = pin;
    return gpio;
}

} // namespace

void* operator new(size_t size) {
    auto ptr = std::malloc(size ? size : 1);
    if (!ptr) {
        throw std::bad_alloc();
    }
    AllocTracker::instance().allocated(ptr);
    return ptr;
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    auto ptr = std::malloc(size ? size : 1);
    AllocTracker::instance().allocated(ptr);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    AllocTracker::instance().freed(ptr);
    std::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    AllocTracker::instance().freed(ptr);
    std::free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    AllocTracker::instance().freed(ptr);
    std::free(ptr);
}

TEST_CASE("SystemSleepConfiguration") {
    SECTION("destructor frees all wakeup sources") {
        size_t count = 0;
        size_t configured = 0;
        size_t leaked = 0;
        {
            TrackAllocations tracker;
            {
                SystemSleepConfiguration config;
                configure(config);
                config.network(NETWORK_INTERFACE_ETHERNET).analog(A0, 1000, AnalogInterruptMode::CROSS);
                count = countWakeupSources(config);
                configured = tracker.liveCount();
            }
            leaked = tracker.liveCount();
            REQUIRE_FALSE(tracker.overflow());
        }
        CHECK(count == 5);
        CHECK(configured == 5);
        CHECK(leaked == 0);
    }

    SECTION("move assignment frees the wakeup sources it held") {
        size_t count = 0;
        size_t configured = 0;
        size_t reused = 0;
        size_t leaked = 0;
        {
            TrackAllocations tracker;
            {
                SystemSleepConfiguration config;
                configure(config);
                configured = tracker.liveCount();
                for (int i = 0; i < 100; ++i) {
                    config = SystemSleepConfiguration();
                    configure(config);
                }
                reused = tracker.liveCount();
                count = countWakeupSources(config);
            }
            leaked = tracker.liveCount();
            REQUIRE_FALSE(tracker.overflow());
        }
        CHECK(count == 3);
        CHECK(configured == 3);
        CHECK(reused == 3);
        CHECK(leaked == 0);
    }

    SECTION("move construction transfers ownership without allocating") {
        size_t configured = 0;
        size_t moved = 0;
        size_t otherCount = 0;
        size_t configCount = 0;
        size_t leaked = 0;
        {
            TrackAllocations tracker;
            {
                SystemSleepConfiguration config;
                configure(config);
                configured = tracker.liveCount();
                SystemSleepConfiguration other(std::move(config));
                moved = tracker.liveCount();
                otherCount = countWakeupSources(other);
                configCount = countWakeupSources(config);
            }
            leaked = tracker.liveCount();
            REQUIRE_FALSE(tracker.overflow());
        }
        CHECK(moved == configured);
        CHECK(otherCount == 3);
        CHECK(configCount == 0);
        CHECK(leaked == 0);
    }

    SECTION("move assignment takes over the wakeup sources of the source object") {
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::STOP).gpio(D2, RISING);
        SystemSleepConfiguration other;
        configure(other);
        config = std::move(other);
        CHECK(config.valid());
        CHECK(config.halConfig()->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER);
        CHECK(countWakeupSources(config) == 3);
        CHECK(countWakeupSources(other) == 0);
    }

    SECTION("self move assignment keeps the wakeup sources") {
        SystemSleepConfiguration config;
        configure(config);
        auto& ref = config;
        config = std::move(ref);
        CHECK(config.valid());
        CHECK(countWakeupSources(config) == 3);
    }

    SECTION("network wakeup source has the correct size") {
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::STOP).network(NETWORK_INTERFACE_ETHERNET);
        auto network = findNetworkWakeupSource(config);
        REQUIRE(network);
        CHECK(network->base.size == sizeof(hal_wakeup_source_network_t));
    }
}

TEST_CASE("SystemSleepResult") {
    auto gpio = makeGpioWakeupSource(D2);
    auto source = reinterpret_cast<hal_wakeup_source_base_t*>(&gpio);

    SECTION("self copy assignment keeps the wakeup source") {
        SystemSleepResult result(source, SYSTEM_ERROR_NONE);
        auto& ref = result;
        result = ref;
        CHECK(result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO);
        CHECK(result.wakeupPin() == D2);
    }

    SECTION("self move assignment keeps the wakeup source") {
        SystemSleepResult result(source, SYSTEM_ERROR_NONE);
        auto& ref = result;
        result = std::move(ref);
        CHECK(result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO);
        CHECK(result.wakeupPin() == D2);
    }

    SECTION("copy and move preserve the wakeup source") {
        SystemSleepResult result(source, SYSTEM_ERROR_NONE);
        SystemSleepResult copy(result);
        CHECK(copy.wakeupPin() == D2);
        SystemSleepResult moved(std::move(copy));
        CHECK(moved.wakeupPin() == D2);
        CHECK(copy.wakeupReason() == SystemSleepWakeupReason::UNKNOWN);
        SystemSleepResult assigned;
        assigned = result;
        CHECK(assigned.wakeupPin() == D2);
        SystemSleepResult moveAssigned;
        moveAssigned = std::move(assigned);
        CHECK(moveAssigned.wakeupPin() == D2);
        CHECK(assigned.wakeupReason() == SystemSleepWakeupReason::UNKNOWN);
    }
}
