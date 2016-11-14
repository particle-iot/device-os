#include "led_service.h"

#include "catch.hpp"
#include "hippomocks.h"

namespace {

class Color {
public:
    Color() :
            Color(0, 0, 0, 255) {
    }

    Color(uint32_t rgba) :
            rgba_(rgba) {
    }

    Color(int r, int g, int b, int a = 255) :
            rgba_(((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b) {
    }

    int r() const {
        return (rgba_ >> 16) & 0xff;
    }

    int g() const {
        return (rgba_ >> 8) & 0xff;
    }

    int b() const {
        return rgba_ & 0xff;
    }

    int a() const {
        return rgba_ >> 24;
    }

    uint32_t rgba() const {
        return rgba_;
    }

    bool operator==(const Color& color) const {
        return rgba_ == color.rgba_;
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
    static const Color GREY;

private:
    uint32_t rgba_;
};

const Color Color::BLACK = Color(0xff000000);
const Color Color::BLUE = Color(0xff0000ff);
const Color Color::GREEN = Color(0xff00ff00);
const Color Color::CYAN = Color(0xff00ffff);
const Color Color::RED = Color(0xffff0000);
const Color Color::MAGENTA = Color(0xffff00ff);
const Color Color::YELLOW = Color(0xffffff00);
const Color Color::WHITE = Color(0xffffffff);
const Color Color::GREY = Color(0xff1f1f1f);

class Led {
public:
    Led() {
        mocks_.OnCallFunc(Set_RGB_LED_Values).Do([&](uint16_t r, uint16_t g, uint16_t b) {
            color_ = Color(normalize(r), normalize(g), normalize(b));
        });
        mocks_.OnCallFunc(Get_RGB_LED_Max_Value).Return(MAX_COLOR_VALUE);
        reset();
    }

    const Color& color() const {
        return color_;
    }

    static void reset() {
        led_update_disable(nullptr);
        led_update_enable(nullptr);
    }

private:
    MockRepository mocks_;
    Color color_;

    static const uint16_t MAX_COLOR_VALUE = 2048;

    static uint8_t normalize(uint16_t val) {
        return ((uint32_t)val << 8) / MAX_COLOR_VALUE;
    }
};

inline void update(system_tick_t ticks = 0) {
    led_update(ticks, nullptr);
}

inline std::ostream& operator<<(std::ostream& strm, const Color& color) {
    return strm << "Color(" << color.r() << ", " << color.g() << ", " << color.b() << ')';
}

} // namespace

TEST_CASE("LEDState") {
    Led led;

    SECTION("empty queue") {
        update();
        CHECK(led.color() == Color(0, 0, 0));
    }

    SECTION("solid colors") {
        LEDStatus s = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s, NORMAL);
        update();
        CHECK(led.color() == Color::WHITE);
        LED_COLOR(s, RED);
        update();
        CHECK(led.color() == Color::RED);
        LED_COLOR(s, GREEN);
        update();
        CHECK(led.color() == Color::GREEN);
        LED_COLOR(s, BLUE);
        update();
        CHECK(led.color() == Color::BLUE);
        LED_STOP(s);
    }

    SECTION("brightness") {
        LEDStatus s = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s, NORMAL);
        for (int i = 0; i <= 255; ++i) {
            LED_BRIGHTNESS(s, i);
            update();
            CHECK(led.color() == Color(i, i, i));
        }
        LED_STOP(s);
    }

    SECTION("on / off / toggle") {
        LEDStatus s = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s, NORMAL);
        update();
        CHECK(led.color() == Color::WHITE);
        LED_OFF(s);
        update();
        CHECK(led.color() == Color::BLACK);
        LED_ON(s);
        update();
        CHECK(led.color() == Color::WHITE);
        LED_TOGGLE(s);
        update();
        CHECK(led.color() == Color::BLACK);
        LED_TOGGLE(s);
        update();
        CHECK(led.color() == Color::WHITE);
        LED_STOP(s);
    }

    SECTION("enable / disable updates") {
        LEDStatus s = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s, NORMAL);
        update();
        CHECK(led.color() == Color::WHITE);
        LED_COLOR(s, RED);
        led_update_disable(nullptr); // Disable updates
        update();
        CHECK(led.color() == Color::WHITE); // Not changed
        led_update_enable(nullptr); // Enable updates
        update();
        CHECK(led.color() == Color::RED);
        LED_STOP(s);
    }

    SECTION("system priorities") {
        // Background
        LEDStatus s1 = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s1, BACKGROUND);
        update();
        CHECK(led.color() == Color::WHITE);
        // Normal
        LEDStatus s2 = LED_STATUS(BLUE, SOLID, SYSTEM);
        LED_START(s2, NORMAL);
        // update();
        CHECK(led.color() == Color::BLUE); // Overrides background status
/*
        // Important
        LEDStatus s3 = LED_STATUS(BLUE, SOLID, SYSTEM);
        LED_START(s3, IMPORTANT);
        update();
        CHECK(led.color() == Color::BLUE); // Overrides normal status
        // Critical
        LEDStatus s4 = LED_STATUS(WHITE, SOLID, SYSTEM);
        LED_START(s4, CRITICAL);
        update();
        CHECK(led.color() == Color::WHITE); // Overrides important status
        // Cleanup
        LED_STOP(s4);
        LED_STOP(s3);
        LED_STOP(s2);
*/
        LED_STOP(s1);
    }
/*
    SECTION("system status overrides application status with same priority") {

    }
*/
}
