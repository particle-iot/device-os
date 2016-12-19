#include "spark_wiring_led.h"

#include "led_service.h"

#include "rgbled_hal.h"

#include "catch.hpp"
#include "hippomocks.h"

namespace {

using namespace particle;

class Color {
public:
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

    // Returns `true` if this color is equal to another color with given precision
    bool equalsTo(const Color& color, int d = 0) const {
        return (std::abs(r() - color.r()) <= d &&
                std::abs(g() - color.g()) <= d &&
                std::abs(b() - color.b()) <= d);
    }

    operator uint32_t() const {
        return rgb();
    }

    bool operator==(const Color& color) const {
        return equalsTo(color, 2); // Small difference is tolerated
    }

    bool operator!=(const Color& color) const {
        return !operator==(color);
    }

    static const Color BLACK;
    static const Color BLUE;
    static const Color GREEN;
    static const Color CYAN;
    static const Color RED;
    static const Color MAGENTA;
    static const Color YELLOW;
    static const Color WHITE;
    static const Color GRAY;

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
const Color Color::GRAY = Color(RGB_COLOR_GREY);

inline std::ostream& operator<<(std::ostream& strm, const Color& color) {
    return strm << "Color(" << color.r() << ", " << color.g() << ", " << color.b() << ')';
}

class Led {
public:
    Led() {
        mocks_.OnCallFunc(HAL_Led_Rgb_Set_Values).Do([&](uint16_t r, uint16_t g, uint16_t b, void*) {
            color_ = Color(normalize(r), normalize(g), normalize(b));
        });
        mocks_.OnCallFunc(Set_RGB_LED_Values).Do([&](uint16_t r, uint16_t g, uint16_t b) {
            color_ = Color(normalize(r), normalize(g), normalize(b));
        });
        mocks_.OnCallFunc(HAL_Led_Rgb_Get_Max_Value).Return(MAX_COLOR_VALUE);
        mocks_.OnCallFunc(Get_RGB_LED_Max_Value).Return(MAX_COLOR_VALUE);
        LED_SetBrightness(255); // Set maximum brightness by default
        reset();
    }

    const Color& color() const {
        return color_;
    }

    static void reset() {
        // Reset cached LED state
        led_set_update_enabled(0, nullptr);
        led_set_update_enabled(1, nullptr);
    }

private:
    MockRepository mocks_;
    Color color_;

    static const uint16_t MAX_COLOR_VALUE = 10000;

    static uint8_t normalize(uint16_t val) {
        return ((uint32_t)val << 8) / MAX_COLOR_VALUE;
    }
};

inline void update(system_tick_t ticks = 0) {
    led_update(ticks, nullptr, nullptr);
}

class PatternChecker {
public:
    typedef std::function<Color(double)> Function;

    PatternChecker(const Led& led, Function func) :
            led_(led),
            func_(std::move(func)) {
    }

    void operator()(int period) {
        const int step = std::round(period / 5.0);
        int ticks = 0;
        while (ticks < period * 2) {
            update((ticks > 0) ? step : 0);
            const double t = (ticks % period) / (double)(period - 1);
            const Color c = func_(t);
            REQUIRE(led_.color() == c);
            ticks += step;
        }
    }

private:
    const Led& led_;
    Function func_;
};

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
        update();
        CHECK(led.color() == Color(0x00123456));
        LEDStatus s(Color::WHITE);
        s.setActive(); // Start status indication
        update();
        CHECK(led.color() == Color::WHITE);
        s.setActive(false); // Stop status indication
        update();
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

    SECTION("changing LED brightness globally") {
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
        SECTION("LED_PATTERN_BLINK") {
            LEDStatus s(Color::WHITE, LED_PATTERN_BLINK);
            s.setActive();

            PatternChecker check(led, [](double t) {
                if (t < 0.5) {
                    return Color::WHITE; // LED is on
                } else {
                    return Color::BLACK; // LED is off
                }
            });

            SECTION("slow speed") {
                s.setSpeed(LED_SPEED_SLOW);
                check(1000); // Pattern period is 1s
            }

            SECTION("normal speed") {
                s.setSpeed(LED_SPEED_NORMAL);
                check(500); // Pattern period is 500ms
            }

            SECTION("fast speed") {
                s.setSpeed(LED_SPEED_FAST);
                check(200); // Pattern period is 200ms
            }

            SECTION("custom period") {
                s.setPeriod(2000);
                check(2000); // Pattern period is 2s
            }
        }

        SECTION("LED_PATTERN_FADE") {
            LEDStatus s(Color::WHITE, LED_PATTERN_FADE);
            s.setActive();

            PatternChecker check(led, [](double t) {
                if (t < 0.5) {
                    return Color::WHITE.scaled(1.0 - t / 0.5); // Fading out
                } else {
                    return Color::WHITE.scaled((t - 0.5) / 0.5); // Fading in
                }
            });

            SECTION("slow speed") {
                s.setSpeed(LED_SPEED_SLOW);
                check(7000); // Pattern period is 7s
            }

            SECTION("normal speed") {
                s.setSpeed(LED_SPEED_NORMAL);
                check(4000); // Pattern period is 4s
            }

            SECTION("fast speed") {
                s.setSpeed(LED_SPEED_FAST);
                check(1000); // Pattern period is 1s
            }

            SECTION("custom period") {
                s.setPeriod(3000);
                check(3000); // Pattern period is 3s
            }
        }
    }
}
