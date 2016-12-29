#include "rgbled.h"
#include "rgbled_hal.h"

#include "catch.hpp"
#include "hippomocks.h"

namespace {

static uint16_t rgb_values[3];

// The functions for the low-level hardware delegate to mocks so the tests
// can easily stub/verify calls.
class Mocks {
public:
    Mocks() {
        mocks_.OnCallFunc(Set_RGB_LED_Values).Do([&](uint16_t r, uint16_t g, uint16_t b) {
            rgb_values[0] = r;
            rgb_values[1] = g;
            rgb_values[2] = b;
        });
        mocks_.OnCallFunc(Get_RGB_LED_Values).Do([&](uint16_t* rgb) {
            for (int i=0; i<3; i++) {
                rgb[i] = rgb_values[i];
            }
        });
        mocks_.OnCallFunc(Get_RGB_LED_Max_Value).Return(2048);
        // Mock new HAL functions
        mocks_.OnCallFunc(HAL_Led_Rgb_Set_Values).Do([&](uint16_t r, uint16_t g, uint16_t b, void*) {
            Set_RGB_LED_Values(r, g, b);
        });
        mocks_.OnCallFunc(HAL_Led_Rgb_Get_Values).Do([&](uint16_t* rgb, void*) {
            Get_RGB_LED_Values(rgb);
        });
        mocks_.OnCallFunc(HAL_Led_Rgb_Get_Max_Value).Do([](void*) {
            return Get_RGB_LED_Max_Value();
        });
    }

private:
    MockRepository mocks_;
};

/**
 */
uint8_t ledAdjust(uint8_t value, uint8_t brightness=255) {
    return (uint16_t(value)*brightness)>>8;
}

/**
 * Verifies the current LED values have the RGB values scaled to the given
 * brightness.
 * @param r
 * @param g
 * @param b
 * @param brightness
 */
void assertLEDRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t fade=99) {
    uint16_t actual[3];
    actual[0] = r;
    actual[1] = g;
    actual[2] = b;
    for (int i=0; i<3; i++) {
        actual[i] = (uint32_t(actual[i])*brightness*HAL_Led_Rgb_Get_Max_Value(nullptr))>>16;
        actual[i] = actual[i]*fade/99;
        REQUIRE((actual[i]>>8) == (rgb_values[i]>>8) );
    }
}

void assertLEDRGB(uint8_t r, uint8_t g, uint8_t b) {
    assertLEDRGB(r, g, b, Get_LED_Brightness());
}

} // namespace

SCENARIO( "User can set the LED Color", "[led]" ) {
    Mocks mocks;
    GIVEN("The RGB led is in override mode") {
        LED_Signaling_Start();
    }
    WHEN("The LED color is set to RGB_COLOR_RED") {
        LED_SetSignalingColor(RGB_COLOR_RED);
        LED_On(LED_RGB);
        THEN("The rgb values of the LED should be (255,0,0,)") {
            assertLEDRGB(255,0,0);
        }
    }
}

SCENARIO( "User can turn the LED off", "[led]" ) {
    Mocks mocks;
    GIVEN("The RGB led is in override mode") {
        LED_Signaling_Start();
        WHEN("The LED is turned off") {
            LED_Off(LED_RGB);
            THEN("The rgb values of the LED should be (0,0,0)") {
                assertLEDRGB(0,0,0);
            }
        }

    }
}

SCENARIO("Led can be toggled off then on again") {
    Mocks mocks;
    GIVEN("The RGB led is in override mode and a color set") {
        LED_Signaling_Start();
        LED_SetSignalingColor(0xFEDCBA);

        WHEN ("The LED turned on and is toggled") {
            LED_On(LED_RGB);
            LED_Toggle(LED_RGB);
            THEN("The rgb values of the LED should be (0,0,0)") {
                assertLEDRGB(0,0,0);
            }
            AND_WHEN ("The LED is toggled again") {
                LED_Toggle(LED_RGB);
                THEN("The rgb values match the original") {
                    assertLEDRGB(0xFE,0xDC,0xBA);
                }
            }
        }
    }
}


SCENARIO("Led can be toggled on") {
    Mocks mocks;
    GIVEN("The RGB led is in override mode, off, and a color set") {
        LED_Signaling_Start();
        LED_SetSignalingColor(0xFEDCBA);
        WHEN ("The LED is switched on and toggled") {
            LED_Off(LED_RGB);
            LED_Toggle(LED_RGB);
            THEN("The rgb values match the original") {
                assertLEDRGB(0xFE,0xDC,0xBA);
            }
            AND_WHEN ("The LED is toggled again") {
                LED_Toggle(LED_RGB);
                THEN("The rgb values of the LED should be (0,0,0)") {
                    assertLEDRGB(0,0,0);
                }
            }
        }
    }
}

SCENARIO("LED Brightness can be set") {
    Mocks mocks;
    GIVEN("The RGB led is in override mode, on, and a color set") {
        LED_Signaling_Start();
        LED_SetSignalingColor(0xFEDCBA);

        WHEN("The brightness is set to 48 (50%)") {
            LED_SetBrightness(48);
            LED_On(LED_RGB);
            THEN("The RGB values are half of the original") {
                assertLEDRGB(0xFE,0xDC,0xBA, 48);
            }
        }
    }
}

SCENARIO("LED can fade to half brightness in 50 steps", "[led]") {
    Mocks mocks;
    GIVEN("The RGB led is in override mode, and a color set") {
        LED_Signaling_Start();
        LED_SetBrightness(96);
        LED_SetSignalingColor(0xFEDCBA);
    }
    WHEN ("The led is turned on and faded 50 times") {
        LED_On(LED_RGB);
        // fade starts at 99, so 49 takes us to 50.
        for (int i=0; i<50; i++) {
            LED_Fade(LED_RGB);
        }
        THEN("The LED values are 1/2 of the original") {
            assertLEDRGB(0xFE,0xDC,0xBA,96, 49);
        }
    }
}

SCENARIO("LED_RGB_Get can retrieve correct 8-bit values from 16-bit CCR counters") {
    Mocks mocks;
    GIVEN("The RGB Led setup with a color") {
        LED_Signaling_Start();
        LED_SetBrightness(96);
        LED_SetSignalingColor(0xFEDCBA);
    }
    WHEN ("The LED is turned on and values fetched") {
        LED_On(LED_RGB);
        uint8_t rgb[3];
        LED_RGB_Get(rgb);
        THEN("The corresponding values match the original") {
            REQUIRE(rgb[0] == ledAdjust(0xFE, 96));
            REQUIRE(rgb[1] == ledAdjust(0xDC, 96));
            REQUIRE(rgb[2] == ledAdjust(0xBA, 96));
        }
    }
}
