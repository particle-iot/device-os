
// not hardware specific so test on the photon only to free up space for the core
#if PLATFORM_ID>=3

#include "application.h"
#include "unit-test/unit-test.h"
#include "rgbled.h"
#include "rgbled_hal.h"
#include <stdio.h>

#ifdef abs
#undef abs
#endif

/**
 * Handles the notification of LED change
 */
uint8_t rgbNotify[3];
volatile uint32_t rgbNotifyCount;
void onChangeRGBLED(uint8_t r, uint8_t g, uint8_t b) {
    rgbNotify[0] = r;
    rgbNotify[1] = g;
    rgbNotify[2] = b;
    rgbNotifyCount++;
}

void assertLEDNotify(uint8_t r, uint8_t g, uint8_t b) {
    assertEqual(rgbNotify[0], r);
    assertEqual(rgbNotify[1], g);
    assertEqual(rgbNotify[2], b);
}


void assertLEDColor(uint8_t r, uint8_t g, uint8_t b, bool equal) {
    uint8_t rgb[3];
    LED_RGB_Get(rgb);

    if (equal) {
        assertEqual(rgb[0], r);
        assertEqual(rgb[1], g);
        assertEqual(rgb[2], b);
    }
    else if(rgb[0]==r && rgb[1]==g && rgb[2]==b) {
        char buf[50];
        sprintf(buf, "%d,%d,%d",r,g,b);
        assertNotEqual(buf, buf);
    }
}

void assertLEDColorIs(uint8_t r, uint8_t g, uint8_t b) {
    assertLEDColor(r, g, b, true);
}

void assertLEDColorIsNot(byte r, byte g, byte b) {
    assertLEDColor(r, g, b, false);
}

void assertLEDColorIs(int rgb) {
    assertLEDColor(rgb>>16 & 0xFF, rgb>>8 & 0xFF, rgb&0xFF, true);
}

void assertLEDColorIsNot(int rgb) {
    assertLEDColor(rgb>>16 & 0xFF, rgb>>8 & 0xFF, rgb&0xFF, false);
}

/**
 */
uint8_t ledAdjust(uint8_t value, uint8_t brightness=255) {
    return (value*brightness)>>8;
}

test(LED_01_Updated) {
    // Force the LED to show a breathing pattern for this test
    LEDStatus status(LED_PATTERN_FADE, LED_PRIORITY_IMPORTANT);
    status.setActive();

    RGB.control(false);
    RGB.onChange(onChangeRGBLED);
    uint32_t start = rgbNotifyCount;
    delay(500);
    uint32_t end = rgbNotifyCount;
    RGB.onChange(NULL);

    // onChange callback is called every 25ms, so 500ms / 25ms = 20
    assertMoreOrEqual((end-start), uint32_t(20));
}


test(LED_02_ControlledReturnsFalseWhenNoControl) {
    // when
    RGB.control(false);
    // then
    assertFalse(RGB.controlled());
}

test(LED_03_ControlledReturnsTrueWhenControlled) {
    // when
    RGB.control(true);
    // then
    assertTrue(RGB.controlled());
}

test(LED_04_ChangesWhenNotControlled) {
    // when
    RGB.control(false);
    // then
    uint8_t rgbInitial[3];
    uint8_t rgbChanged[3];
    LED_RGB_Get(rgbInitial);
    delay(75);
    LED_RGB_Get(rgbChanged);

    assertFalse(rgbInitial[0]==rgbChanged[0] && rgbInitial[1]==rgbChanged[1] && rgbInitial[2]==rgbChanged[2]);
}

test(LED_05_StaticWhenControlled) {
    // given
    RGB.control(true);
    RGB.brightness(255);
    RGB.onChange(onChangeRGBLED);

    // when
    RGB.color(30,60,90);
    // then
    uint8_t rgbExpected[3] = {ledAdjust(30), ledAdjust(60), ledAdjust(90)};
    uint8_t rgbInitial[3];
    uint8_t rgbChanged[3];
    LED_RGB_Get(rgbInitial);
    delay(75);
    LED_RGB_Get(rgbChanged);
    for (int i=0; i<3; i++)
        assertEqual(rgbInitial[i], rgbExpected[i]);

    for (int i=0; i<3; i++)
        assertEqual(rgbInitial[i], rgbChanged[i]);

    for (int i=0; i<3; i++)
        assertEqual(rgbInitial[i], rgbNotify[i]);

    RGB.onChange(NULL);
}

test(LED_06_SettingRGBAfterOverrideShouldChangeLED) {
    // given
    RGB.control(true);
    RGB.brightness(255);

    // when
    RGB.color(10,20,30);

    // then
    assertLEDColorIs(ledAdjust(10),ledAdjust(20),ledAdjust(30));
}

test(LED_07_SettingRGBWithoutOverrideShouldNotChangeLED) {
    // given
    RGB.control(false);

    // when
    RGB.color(10,20,30);

    // then
    assertLEDColorIsNot(ledAdjust(10),ledAdjust(20),ledAdjust(30));
}

test(LED_08_BrightnessChangesColor) {
    // given
    RGB.control(true);
    RGB.brightness(255);
    RGB.color(255,127,0);

    // when
    RGB.brightness(128);

    // then
    assertLEDColorIs(ledAdjust(255,128), ledAdjust(127,128), ledAdjust(0,128));
}

test(LED_09_BrightnessIsPersisted) {
    // given
    RGB.control(true);
    RGB.brightness(128);
    RGB.color(255,255,255);

    // when
    RGB.control(false);
    RGB.control(true);
    RGB.color(255,255,0);

    assertLEDColorIs(ledAdjust(255,128),ledAdjust(255,128),0);
}

uint8_t rgbUser[3];
void userLEDChangeHandler(uint8_t r, uint8_t g, uint8_t b) {
    rgbUser[0] = r;
    rgbUser[1] = g;
    rgbUser[2] = b;
}

void assertChangeHandlerCalledWith(uint8_t r, uint8_t g, uint8_t b) {
    assertEqual(rgbUser[0], r);
    assertEqual(rgbUser[1], g);
    assertEqual(rgbUser[2], b);
}

test(LED_10_ChangeHandlerCalled) {
    // given
    RGB.onChange(userLEDChangeHandler);

    // when
    RGB.control(true);
    RGB.brightness(255);
    RGB.color(10, 20, 30);

    // then
    assertChangeHandlerCalledWith(ledAdjust(10),ledAdjust(20),ledAdjust(30));

    RGB.onChange(NULL);
}

static void assertRgbLedMirrorPinsColor(const pin_t pins[3], uint16_t r, uint16_t g, uint16_t b)
{
    // Convert to CCR
    r = (uint16_t)((((uint32_t)(r)) * 255 * HAL_Led_Rgb_Get_Max_Value(nullptr)) >> 16);
    g = (uint16_t)((((uint32_t)(g)) * 255 * HAL_Led_Rgb_Get_Max_Value(nullptr)) >> 16);
    b = (uint16_t)((((uint32_t)(b)) * 255 * HAL_Led_Rgb_Get_Max_Value(nullptr)) >> 16);
    assertLessOrEqual(std::abs((int32_t)(HAL_PWM_Get_AnalogValue_Ext(pins[0])) - (int32_t)(r * ((1UL << HAL_PWM_Get_Resolution(pins[0])) - 1) / HAL_Led_Rgb_Get_Max_Value(nullptr))), 1);
    assertLessOrEqual(std::abs((int32_t)(HAL_PWM_Get_AnalogValue_Ext(pins[1])) - (int32_t)(g * ((1UL << HAL_PWM_Get_Resolution(pins[1])) - 1) / HAL_Led_Rgb_Get_Max_Value(nullptr))), 1);
    assertLessOrEqual(std::abs((int32_t)(HAL_PWM_Get_AnalogValue_Ext(pins[2])) - (int32_t)(b * ((1UL << HAL_PWM_Get_Resolution(pins[2])) - 1) / HAL_Led_Rgb_Get_Max_Value(nullptr))), 1);
}

test(LED_11_MirroringWorks) {
    RGB.control(true);
    RGB.brightness(255);

#if !HAL_PLATFORM_NRF52840
    const pin_t pins[3] = {A4, A5, A7};
#else
# if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
    const pin_t pins[3] = {A4, A5, A3};
# else
    // SoM
    const pin_t pins[3] = {A1, A0, A7};
# endif // PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON
#endif
    // Mirror to r=A4, g=A5, b=A7. Non-inverted (common cathode).
    // RGB led mirroring in bootloader is not enabled
    RGB.mirrorTo(pins[0], pins[1], pins[2], false, false);

    RGB.color(0, 0, 0);
    assertRgbLedMirrorPinsColor(pins, 0, 0, 0);

    RGB.color(127, 255, 127);
    assertRgbLedMirrorPinsColor(pins, 127, 255, 127);

    RGB.color(255, 0, 127);
    assertRgbLedMirrorPinsColor(pins, 255, 0, 127);

    RGB.mirrorDisable();
    RGB.control(false);
}

namespace {

// Handler class for RGB.onChange() that counts number of its instances
class OnChangeHandler {
public:
    OnChangeHandler() {
        ++s_count;
    }

    OnChangeHandler(const OnChangeHandler&) {
        ++s_count;
    }

    ~OnChangeHandler() {
        --s_count;
    }

    static unsigned instanceCount() {
        return s_count;
    }

    void operator()(uint8_t r, uint8_t g, uint8_t b) {
    }

private:
    static unsigned s_count;
};

unsigned OnChangeHandler::s_count = 0;

} // namespace

test(LED_12_NoLeakWhenOnChangeHandlerIsOverridden) {
    RGB.onChange(OnChangeHandler());
    assertEqual(OnChangeHandler::instanceCount(), 1);
    RGB.onChange(OnChangeHandler());
    assertEqual(OnChangeHandler::instanceCount(), 1); // Previous handler has been destroyed
    RGB.onChange(nullptr);
    assertEqual(OnChangeHandler::instanceCount(), 0); // Current handler has been destroyed
}

#endif // PLATFORM_ID >= 3
