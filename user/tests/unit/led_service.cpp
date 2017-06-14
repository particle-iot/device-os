#include "spark_wiring_led.h"

#include "rgbled_hal.h"

#include "tools/random.h"
#include "tools/catch.h"

#include "hippomocks.h"

namespace {

using namespace particle;

class Color {
public:
    // Predefined colors
    static const Color BLACK;
    static const Color BLUE;
    static const Color GREEN;
    static const Color CYAN;
    static const Color RED;
    static const Color MAGENTA;
    static const Color YELLOW;
    static const Color WHITE;
    static const Color GRAY;

    Color() :
            Color(0, 0, 0) {
    }

    Color(uint32_t rgb) :
            rgb_(rgb) {
    }

    Color(int r, int g, int b) :
            rgb_(((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b) {
    }

    int r() const {
        return (rgb_ >> 16) & 0xff;
    }

    int g() const {
        return (rgb_ >> 8) & 0xff;
    }

    int b() const {
        return rgb_ & 0xff;
    }

    uint32_t rgb() const {
        return rgb_;
    }

    Color scaled(double value) const {
        value = std::max(0.0, std::min(value, 1.0));
        return Color(std::round(r() * value), std::round(g() * value), std::round(b() * value));
    }

    // Returns `true` if this color is equal to another color with a given precision
    bool equalsTo(const Color& color, int d = 0) const {
        return (std::abs(r() - color.r()) <= d &&
                std::abs(g() - color.g()) <= d &&
                std::abs(b() - color.b()) <= d);
    }

    operator uint32_t() const {
        return rgb();
    }

    bool operator==(const Color& color) const {
        // Since the LED service uses integer arithmetic internally, a small difference between colors
        // under comparison is tolerated by default
        return equalsTo(color, 2);
    }

    bool operator!=(const Color& color) const {
        return !operator==(color);
    }

private:
    uint32_t rgb_;
};

const Color Color::BLACK = Color();
const Color Color::BLUE = Color(RGB_COLOR_BLUE);
const Color Color::GREEN = Color(RGB_COLOR_GREEN);
const Color Color::CYAN = Color(RGB_COLOR_CYAN);
const Color Color::RED = Color(RGB_COLOR_RED);
const Color Color::MAGENTA = Color(RGB_COLOR_MAGENTA);
const Color Color::YELLOW = Color(RGB_COLOR_YELLOW);
const Color Color::WHITE = Color(RGB_COLOR_WHITE);
const Color Color::GRAY = Color(RGB_COLOR_GRAY);

inline std::ostream& operator<<(std::ostream& strm, const Color& color) {
    return strm << "Color(" << color.r() << ", " << color.g() << ", " << color.b() << ')';
}

// Class mocking HAL functions for RGB LED control
class Led {
public:
    Led() {
        mocks_.OnCallFunc(Set_RGB_LED_Values).Do([&](uint16_t r, uint16_t g, uint16_t b) {
            color_ = Color(normalize(r), normalize(g), normalize(b));
        });
        mocks_.OnCallFunc(Get_RGB_LED_Max_Value).Return(MAX_COLOR_VALUE);
        LED_SetBrightness(255); // Set maximum brightness by default
        reset();
    }

    const Color& color() const {
        return color_;
    }

    void reset() {
        // Reset cached LED state
        led_set_update_enabled(0, nullptr);
        led_set_update_enabled(1, nullptr);
        color_ = Color::BLACK;
    }

private:
    MockRepository mocks_;
    Color color_;

    static const uint16_t MAX_COLOR_VALUE = 10000;

    static uint8_t normalize(uint16_t val) {
        return std::round(val * 255.0 / MAX_COLOR_VALUE);
    }
};

inline void update(system_tick_t ticks = 0) {
    led_update(ticks, nullptr, nullptr);
}

// Class checking LED signaling patterns
class PatternChecker {
public:
    // Function returning expected LED color for specified time within pattern period. Time argument
    // takes values in the range [0.0, 1.0)
    typedef std::function<Color(double)> Function;

    PatternChecker(const Led& led, Function func) :
            led_(led),
            func_(std::move(func)) {
    }

    PatternChecker(const Led& led, const Color& color, LEDPattern pattern) :
            led_(led),
            func_(patternFunction(color, pattern)) {
    }

    void check(int period) {
        const int step = std::round(period / 11.0);
        if (step > 0) {
            int ticks = 0;
            while (ticks < period * 2) {
                update((ticks > 0) ? step : 0);
                const double t = (ticks % period) / (double)period;
                const Color c = func_(t);
                REQUIRE(led_.color() == c);
                ticks += step;
            }
        } else {
            update();
            const Color c = func_(0.0);
            REQUIRE(led_.color() == c);
        }
    }

    static void check(const Led& led, const Color& color, LEDPattern pattern, int period) {
        PatternChecker check(led, color, pattern);
        check(period);
    }

    void operator()(int period) {
        check(period);
    }

private:
    const Led& led_;
    Function func_;

    static Function patternFunction(const Color& color, LEDPattern pattern) {
        switch (pattern) {
        case LED_PATTERN_SOLID:
            return [=](double t) {
                return color; // LED is always on
            };
        case LED_PATTERN_BLINK:
            return [=](double t) {
                if (t < 0.5) {
                    return color; // LED is on
                } else {
                    return Color::BLACK; // LED is off
                }
            };
        case LED_PATTERN_FADE:
            return [=](double t) {
                if (t < 0.5) {
                    return color.scaled(1.0 - t / 0.5); // Fading out
                } else {
                    return color.scaled((t - 0.5) / 0.5); // Fading in
                }
            };
        default:
            return Function();
        }
    }
};

// Adapter class allowing to use std::function to define a custom LED pattern
class CustomStatus: public LEDStatus {
public:
    typedef std::function<Color(double)> Function; // See PatternChecker::Function

    CustomStatus(int period, Function func) :
            CustomStatus(LED_PRIORITY_NORMAL, period, func) {
    }

    CustomStatus(LEDPriority priority, int period, Function func) :
            CustomStatus(priority, LED_SOURCE_DEFAULT, period, func) {
    }

    CustomStatus(LEDPriority priority, LEDSource source, int period, Function func) :
            LEDStatus(LED_PATTERN_CUSTOM, priority, source),
            func_(std::move(func)),
            period_(period),
            ticks_(0) {
    }

    void reset() {
        ticks_ = 0;
    }

protected:
    virtual void update(system_tick_t ticks) override {
        ticks_ += ticks;
        const double t = (ticks_ % period_) / (double)period_;
        const Color c = func_(t);
        setColor(c);
    }

private:
    Function func_;
    const int period_;
    int ticks_;
};

void checkThemeSignal(LEDSignal signal, const Color& color, LEDPattern pattern, int period, const LEDSystemTheme& theme,
        const Led& led) {
    // Check theme settings
    CHECK(theme.color(signal) == color);
    CHECK(theme.pattern(signal) == pattern);
    CHECK(theme.period(signal) == period);
    // Start signal
    CHECK(led_signal_started(signal, nullptr) == 0);
    led_start_signal(signal, LED_PRIORITY_VALUE(LED_PRIORITY_NORMAL, LED_SOURCE_SYSTEM), 0, nullptr);
    CHECK(led_signal_started(signal, nullptr) == 1);
    // Check signal pattern
    PatternChecker::check(led, color, pattern, period);
    // Stop signal
    led_stop_signal(signal, 0, nullptr);
    CHECK(led_signal_started(signal, nullptr) == 0);
    // Calling led_update() after stopping a signal allows to reset pattern state, cached by the
    // LED service, before checking other signals via this function
    update();
}

} // namespace

TEST_CASE("LEDStatus") {
    Led led;

    SECTION("default status parameters") {
        LEDStatus s;
        CHECK(s.color() == Color::WHITE);
        CHECK(s.pattern() == LED_PATTERN_SOLID);
        CHECK(s.period() == 0);
        CHECK(s.priority() == LED_PRIORITY_NORMAL);
        CHECK(s.isOn() == true);
        CHECK(s.isOff() == false);
        CHECK(s.isActive() == false); // Not active after construction
    }

    SECTION("LED is not affected when no active status is available") {
        LED_SetRGBColor(0x00123456); // Set LED color directly
        LED_On(LED_RGB);
        update(); // No active status
        CHECK(led.color() == Color(0x00123456));
        LEDStatus s(Color::WHITE);
        s.setActive(); // Start status indication
        update();
        CHECK(led.color() == Color::WHITE);
        s.setActive(false); // Stop status indication
        update(); // No active status
        CHECK(led.color() == Color::WHITE); // LED remains white
    }

    SECTION("changing color of active status") {
        LEDStatus s(Color::RED);
        CHECK(s.color() == Color::RED);
        s.setActive();
        update();
        CHECK(led.color() == Color::RED);
        s.setColor(Color::GREEN);
        CHECK(s.color() == Color::GREEN);
        update();
        CHECK(led.color() == Color::GREEN);
        s.setColor(Color::BLUE);
        CHECK(s.color() == Color::BLUE);
        update();
        CHECK(led.color() == Color::BLUE);
    }

    SECTION("turning LED on and off") {
        LEDStatus s(Color::WHITE);
        s.setActive();
        update();
        CHECK(led.color() == Color::WHITE);
        s.off(); // Turn off
        CHECK(s.isOn() == false);
        CHECK(s.isOff() == true);
        update();
        CHECK(led.color() == Color::BLACK);
        s.on(); // Turn on
        CHECK(s.isOn() == true);
        CHECK(s.isOff() == false);
        update();
        CHECK(led.color() == Color::WHITE);
        s.toggle(); // Toggle
        CHECK(s.isOn() == false);
        CHECK(s.isOff() == true);
        update();
        CHECK(led.color() == Color::BLACK);
        s.toggle(); // Toggle
        CHECK(s.isOn() == true);
        CHECK(s.isOff() == false);
        update();
        CHECK(led.color() == Color::WHITE);
    }

    SECTION("changing LED brightness") {
        LEDStatus s(Color::WHITE);
        s.setActive();
        for (int i = 0; i <= 255; ++i) {
            LED_SetBrightness(i);
            update();
            REQUIRE(led.color() == Color(i, i, i));
        }
    }

    SECTION("temporary disabling LED updates") {
        LEDStatus s(Color::WHITE);
        s.setActive();
        update();
        s.setColor(Color::RED); // Override color
        led_set_update_enabled(0, nullptr); // Disable updates
        update();
        CHECK(led.color() == Color::WHITE); // LED color has not changed
        led_set_update_enabled(1, nullptr); // Enable updates
        update();
        CHECK(led.color() == Color::RED); // LED color has changed
    }

    SECTION("newly activated status overrides already active status with same priority") {
        LEDStatus s1(Color::WHITE);
        s1.setActive();
        LEDStatus s2(Color::RED);
        s2.setActive();
        update();
        CHECK(led.color() == Color::RED);
        s2.setActive(false);
        update();
        CHECK(led.color() == Color::WHITE);
    }

    SECTION("status with higher priority always overrides status with lower priority") {
        LEDStatus s1(Color::WHITE, LED_PRIORITY_NORMAL);
        s1.setActive(); // Activate normal status
        LEDStatus s2(Color::RED, LED_PRIORITY_CRITICAL);
        s2.setActive(); // Activate critical status
        update();
        CHECK(led.color() == Color::RED); // Shows critical status
        LEDStatus s3(Color::GRAY, LED_PRIORITY_BACKGROUND);
        s3.setActive(); // Activate background status
        LEDStatus s4(Color::YELLOW, LED_PRIORITY_IMPORTANT);
        s4.setActive(); // Activate important status
        update();
        CHECK(led.color() == Color::RED); // Still shows critical status
        s2.setActive(false); // Deactivate critical status
        update();
        CHECK(led.color() == Color::YELLOW); // Shows important status
        s4.setActive(false); // Deactivate important status
        update();
        CHECK(led.color() == Color::WHITE); // Shows normal status
        s1.setActive(false); // Deactivate normal status
        update();
        CHECK(led.color() == Color::GRAY); // Shows background status
    }

    SECTION("system status can be overriden by application status with higher priority") {
        // Activate system status for each priority
        LEDStatus s1(Color::WHITE, LED_PRIORITY_BACKGROUND, LED_SOURCE_SYSTEM);
        s1.setActive();
        LEDStatus s2(Color::BLUE, LED_PRIORITY_NORMAL, LED_SOURCE_SYSTEM);
        s2.setActive();
        LEDStatus s3(Color::GREEN, LED_PRIORITY_IMPORTANT, LED_SOURCE_SYSTEM);
        s3.setActive();
        LEDStatus s4(Color::RED, LED_PRIORITY_CRITICAL, LED_SOURCE_SYSTEM);
        s4.setActive();
        // Activate application status for each priority
        LEDStatus a1(Color::GRAY, LED_PRIORITY_BACKGROUND, LED_SOURCE_APPLICATION);
        a1.setActive();
        LEDStatus a2(Color::CYAN, LED_PRIORITY_NORMAL, LED_SOURCE_APPLICATION);
        a2.setActive();
        LEDStatus a3(Color::YELLOW, LED_PRIORITY_IMPORTANT, LED_SOURCE_APPLICATION);
        a3.setActive();
        LEDStatus a4(Color::MAGENTA, LED_PRIORITY_CRITICAL, LED_SOURCE_APPLICATION);
        a4.setActive();
        update();
        CHECK(led.color() == Color::RED); // Shows critical status (system)
        s4.setActive(false); // Deactivate critical status (system)
        update();
        CHECK(led.color() == Color::MAGENTA); // Shows critical status (application)
        a4.setActive(false); // Deactivate critical status (application)
        update();
        CHECK(led.color() == Color::GREEN); // Shows important status (system)
        s3.setActive(false); // Deactivate important status (system)
        update();
        CHECK(led.color() == Color::YELLOW); // Shows important status (application)
        a3.setActive(false); // Deactivate important status (application)
        update();
        CHECK(led.color() == Color::BLUE); // Shows normal status (system)
        s2.setActive(false); // Deactivate normal status (system)
        update();
        CHECK(led.color() == Color::CYAN); // Shows normal status (application)
        a2.setActive(false); // Deactivate normal status (application)
        update();
        CHECK(led.color() == Color::WHITE); // Shows background status (system)
        s1.setActive(false); // Deactivate background status (system)
        update();
        CHECK(led.color() == Color::GRAY); // Shows background status (application)
        a1.setActive(false); // Deactivate background status (application)
        update();
        CHECK(led.color() == Color::GRAY); // No active status available (LED remains gray)
    }

    SECTION("predefined patterns") {
        SECTION("LED_PATTERN_SOLID") {
            LEDStatus s(Color::WHITE, LED_PATTERN_SOLID);
            s.setActive();

            PatternChecker check(led, Color::WHITE, LED_PATTERN_SOLID);
            check(10000); // Should pass for any period
        }

        SECTION("LED_PATTERN_BLINK") {
            LEDStatus s(Color::WHITE, LED_PATTERN_BLINK);
            s.setActive();

            PatternChecker check(led, Color::WHITE, LED_PATTERN_BLINK);

            SECTION("slow speed") {
                s.setSpeed(LED_SPEED_SLOW);
                check(500); // Expected period is 500ms
            }

            SECTION("normal speed") {
                s.setSpeed(LED_SPEED_NORMAL);
                check(200); // Expected period is 200ms
            }

            SECTION("fast speed") {
                s.setSpeed(LED_SPEED_FAST);
                check(100); // Expected period is 100ms
            }

            SECTION("custom period") {
                s.setPeriod(1000);
                check(1000); // Expected period is 1s
            }
        }

        SECTION("LED_PATTERN_FADE") {
            LEDStatus s(Color::WHITE, LED_PATTERN_FADE);
            s.setActive();

            PatternChecker check(led, Color::WHITE, LED_PATTERN_FADE);

            SECTION("slow speed") {
                s.setSpeed(LED_SPEED_SLOW);
                check(8000); // Expected period is 8s
            }

            SECTION("normal speed") {
                s.setSpeed(LED_SPEED_NORMAL);
                check(4000); // Expected period is 4s
            }

            SECTION("fast speed") {
                s.setSpeed(LED_SPEED_FAST);
                check(1000); // Expected period is 1s
            }

            SECTION("custom period") {
                s.setPeriod(2000);
                check(2000); // Expected period is 2s
            }
        }
    }

    SECTION("custom patterns") {
        // Simple pattern alternating between some predefined colors
        auto patternFunc = [](double t) {
            static const std::vector<Color> colors = { Color::RED, Color::GREEN, Color::BLUE };
            return colors.at(std::floor(t * colors.size())); // `t` takes values in the range [0.0, 1.0)
        };

        SECTION("activating custom status") {
            const int period = 1000;

            CustomStatus s(period, patternFunc);
            s.setActive();

            PatternChecker check(led, patternFunc);
            check(period);
        }
    }

    SECTION("user callback is invoked when LED changes its color") {
        struct LedUpdateHandler {
            static void callback(void* data, uint8_t r, uint8_t g, uint8_t b, void* reserved) {
                *static_cast<Color*>(data) = Color(r, g, b);
            }
        };

        Color color;
        LED_RGB_SetChangeHandler(LedUpdateHandler::callback, &color);

        LEDStatus s(Color::WHITE);
        s.setActive();
        update();
        CHECK(color == Color::WHITE);
        s.setColor(Color::RED);
        update();
        CHECK(color == Color::RED);
        color = Color::BLACK;
        update(); // Update without changing status color
        CHECK(color == Color::BLACK); // User callback has not been called

        LED_RGB_SetChangeHandler(nullptr, nullptr);
    }
}

TEST_CASE("LEDSystemTheme") {
    Led led;

    SECTION("default theme") {
        // Get current theme (set to default theme initially)
        LEDSystemTheme t;
        // Check theme settings and default signaling
        checkThemeSignal(LED_SIGNAL_NETWORK_OFF, Color::WHITE, LED_PATTERN_FADE, 4000, t, led);
        checkThemeSignal(LED_SIGNAL_NETWORK_ON, Color::BLUE, LED_PATTERN_FADE, 4000, t, led);
        checkThemeSignal(LED_SIGNAL_NETWORK_CONNECTING, Color::GREEN, LED_PATTERN_BLINK, 200, t, led);
        checkThemeSignal(LED_SIGNAL_NETWORK_DHCP, Color::GREEN, LED_PATTERN_BLINK, 100, t, led);
        checkThemeSignal(LED_SIGNAL_NETWORK_CONNECTED, Color::GREEN, LED_PATTERN_FADE, 4000, t, led);
        checkThemeSignal(LED_SIGNAL_CLOUD_CONNECTING, Color::CYAN, LED_PATTERN_BLINK, 200, t, led);
        checkThemeSignal(LED_SIGNAL_CLOUD_HANDSHAKE, Color::CYAN, LED_PATTERN_BLINK, 100, t, led);
        checkThemeSignal(LED_SIGNAL_CLOUD_CONNECTED, Color::CYAN, LED_PATTERN_FADE, 4000, t, led);
        checkThemeSignal(LED_SIGNAL_SAFE_MODE, Color::MAGENTA, LED_PATTERN_FADE, 4000, t, led);
        checkThemeSignal(LED_SIGNAL_LISTENING_MODE, Color::BLUE, LED_PATTERN_BLINK, 500, t, led);
        checkThemeSignal(LED_SIGNAL_DFU_MODE, Color::YELLOW, LED_PATTERN_BLINK, 200, t, led);
        checkThemeSignal(LED_SIGNAL_FIRMWARE_UPDATE, Color::MAGENTA, LED_PATTERN_BLINK, 100, t, led);
        checkThemeSignal(LED_SIGNAL_POWER_OFF, 0x00111111, LED_PATTERN_SOLID, 0, t, led);
    }

    SECTION("setting custom theme and restoring default theme") {
        // Get current theme (set to default theme initially)
        LEDSystemTheme t1;
        // Set custom theme
        LEDSystemTheme t2;
        for (int i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const LEDSignal s = (LEDSignal)i;
            t2.setColor(s, Color(i * 10, i * 10, i * 10));
            t2.setPattern(s, test::anyOf(LED_PATTERN_SOLID, LED_PATTERN_BLINK, LED_PATTERN_FADE));
            t2.setPeriod(s, test::randomInt(1000, 10000));
        }
        t2.apply();
        // Check current theme
        LEDSystemTheme t3;
        for (int i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const LEDSignal s = (LEDSignal)i;
            checkThemeSignal(s, t2.color(s), t2.pattern(s), t2.period(s), t3, led);
        }
        // Restore default theme
        LEDSystemTheme::restoreDefault();
        // Check current theme
        LEDSystemTheme t4;
        for (int i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const LEDSignal s = (LEDSignal)i;
            checkThemeSignal(s, t1.color(s), t1.pattern(s), t1.period(s), t4, led);
        }
    }

    // Stop all signals
    led_stop_signal(0, LED_SIGNAL_FLAG_ALL_SIGNALS, nullptr);

    // Restore default theme
    LEDSystemTheme::restoreDefault();
}
