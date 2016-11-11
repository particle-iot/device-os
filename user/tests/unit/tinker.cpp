#include "catch.hpp"
#include "spark_wiring_string.h"

// Include the Tinker code
#include "../../src/application.cpp"

SCENARIO("Parse commands") {
    GIVEN("A blank command") {
        TinkerCommand command = parseCommand("");
        THEN("The result is blank") {
            REQUIRE(command.pinType.equals(""));
            REQUIRE(command.pinNumber == 0);
            REQUIRE(command.value == 0);
        }
    }
    GIVEN("Parsing pin type") {
        TinkerCommand command = parseCommand("TX");
        THEN("The result contains the pin type") {
            REQUIRE(command.pinType.equals("TX"));
            REQUIRE(command.pinNumber == 0);
            REQUIRE(command.value == 0);
        }
    }
    GIVEN("Parsing pin number") {
        WHEN("a short number") {
            TinkerCommand command = parseCommand("D7");
            THEN("The result contains the pin info") {
                REQUIRE(command.pinType.equals("D"));
                REQUIRE(command.pinNumber == 7);
                REQUIRE(command.value == 0);
            }
        }
        WHEN("a long pin number") {
            TinkerCommand command = parseCommand("GPIO17");
            THEN("The result contains the pin info") {
                REQUIRE(command.pinType.equals("GPIO"));
                REQUIRE(command.pinNumber == 17);
                REQUIRE(command.value == 0);
            }
        }
    }
    GIVEN("Parsing pin and digital value") {
        WHEN("HIGH") {
            TinkerCommand command = parseCommand("D7=HIGH");
            THEN("The result contains the pin info") {
                REQUIRE(command.pinType.equals("D"));
                REQUIRE(command.pinNumber == 7);
                REQUIRE(command.value == 1);
            }
        }
        WHEN("LOW") {
            TinkerCommand command = parseCommand("A0=LOW");
            THEN("The result contains the pin info") {
                REQUIRE(command.pinType.equals("A"));
                REQUIRE(command.pinNumber == 0);
                REQUIRE(command.value == 0);
            }
        }
        WHEN("no pin number") {
            TinkerCommand command = parseCommand("TX=HIGH");
            THEN("The result contains the pin info") {
                REQUIRE(command.pinType.equals("TX"));
                REQUIRE(command.pinNumber == 0);
                REQUIRE(command.value == 1);
            }
        }
    }
    GIVEN("Parsing pin and analog value") {
        TinkerCommand command = parseCommand("A6=150");
        THEN("The result contains the pin info") {
            REQUIRE(command.pinType.equals("A"));
            REQUIRE(command.pinNumber == 6);
            REQUIRE(command.value == 150);
        }
    }
}

//SCENARIO("LED_RGB_Get can retrieve correct 8-bit values from 16-bit CCR counters") {
//    GIVEN("The RGB Led setup with a color") {
//        LED_Signaling_Start();
//        LED_SetBrightness(96);
//        LED_SetSignalingColor(0xFEDCBA);
//    }
//    WHEN ("The LED is turned on and values fetched") {
//        LED_On(LED_RGB);
//        uint8_t rgb[3];
//        LED_RGB_Get(rgb);
//        THEN("The corresponding values match the original") {
//            REQUIRE(rgb[0] == ledAdjust(0xFE, 96));
//            REQUIRE(rgb[1] == ledAdjust(0xDC, 96));
//            REQUIRE(rgb[2] == ledAdjust(0xBA, 96));
//        }
//    }
//}
//
//
