#include "catch.hpp"
#include "spark_wiring_string.h"

// Mock Tinker dependencies
typedef uint16_t pin_t;
enum PinT {
    // Don't have all the pins in order to simulate platforms where D1 != D0 + 1
    D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D0,
    A1, A2, A3, A4, A5, A6, A7, A0,
    B1, B2, B3, B4, B5, B6, B7, B0,
    C1, C2, C3, C4, C5, C6, C7, C0,
    GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9,
    GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19,
    GPIO20, GPIO21, GPIO22, GPIO23, GPIO24, GPIO25, GPIO26, GPIO27, GPIO0,
    TX, RX,
    NUM_PINS
};

enum PinModeT {
    INPUT_PULLDOWN,
    OUTPUT
};

static PinT pinModePin;
static PinModeT pinModeMode[NUM_PINS];
static void pinMode(int pin, int mode) {
    pinModePin = (PinT)pin;
    pinModeMode[pin] = (PinModeT)mode;
}

static PinT digitalWritePin;
static int digitalWriteValue[NUM_PINS];
static void digitalWrite(int pin, int value) {
    digitalWritePin = (PinT)pin;
    digitalWriteValue[pin] = value;
}

static PinT digitalReadPin;
static int digitalReadValue[NUM_PINS];
static int digitalRead(int pin) {
    digitalReadPin = (PinT)pin;
    return digitalReadValue[pin];
}

static PinT analogWritePin;
static int analogWriteValue[NUM_PINS];
static void analogWrite(int pin, int value) {
    analogWritePin = (PinT)pin;
    analogWriteValue[pin] = value;
}

static PinT analogReadPin;
static int analogReadValue[NUM_PINS];
static int analogRead(int pin) {
    analogReadPin = (PinT)pin;
    return analogReadValue[pin];
}

// Include the Tinker code
#include "../../src/application.cpp"


SCENARIO("Parse commands") {
    GIVEN("a blank command") {
        TinkerCommand command = parseCommand("");
        THEN("The result is blank") {
            REQUIRE(command.pinType.equals(""));
            REQUIRE(command.pinNumber == 0);
            REQUIRE(command.value == 0);
            REQUIRE(command.hasNumber == false);
            REQUIRE(command.hasValue == false);
        }
    }
    GIVEN("a pin without a number") {
        TinkerCommand command = parseCommand("TX");
        THEN("the result contains the pin type") {
            REQUIRE(command.pinType.equals("TX"));
            REQUIRE(command.pinNumber == 0);
            REQUIRE(command.value == 0);
            REQUIRE(command.hasNumber == false);
            REQUIRE(command.hasValue == false);
        }
    }
    GIVEN("a pin with number") {
        WHEN("a short type and number") {
            TinkerCommand command = parseCommand("D7");
            THEN("the result contains the pin info") {
                REQUIRE(command.pinType.equals("D"));
                REQUIRE(command.pinNumber == 7);
                REQUIRE(command.value == 0);
                REQUIRE(command.hasNumber == true);
                REQUIRE(command.hasValue == false);
            }
        }
        WHEN("a long type and number") {
            TinkerCommand command = parseCommand("GPIO17");
            THEN("the result contains the pin info") {
                REQUIRE(command.pinType.equals("GPIO"));
                REQUIRE(command.pinNumber == 17);
                REQUIRE(command.value == 0);
                REQUIRE(command.hasNumber == true);
                REQUIRE(command.hasValue == false);
            }
        }
    }
    GIVEN("a pin and digital value") {
        WHEN("HIGH") {
            TinkerCommand command = parseCommand("D7=HIGH");
            THEN("the result contains the pin info and value") {
                REQUIRE(command.pinType.equals("D"));
                REQUIRE(command.pinNumber == 7);
                REQUIRE(command.value == 1);
                REQUIRE(command.hasNumber == true);
                REQUIRE(command.hasValue == true);
            }
        }
        WHEN("LOW") {
            TinkerCommand command = parseCommand("A0=LOW");
            THEN("the result contains the pin info and value") {
                REQUIRE(command.pinType.equals("A"));
                REQUIRE(command.pinNumber == 0);
                REQUIRE(command.value == 0);
                REQUIRE(command.hasNumber == true);
                REQUIRE(command.hasValue == true);
            }
        }
        WHEN("no pin number") {
            TinkerCommand command = parseCommand("TX=HIGH");
            THEN("the result contains the pin info and value") {
                REQUIRE(command.pinType.equals("TX"));
                REQUIRE(command.pinNumber == 0);
                REQUIRE(command.value == 1);
                REQUIRE(command.hasNumber == false);
                REQUIRE(command.hasValue == true);
            }
        }
    }
    GIVEN("a pin and analog value") {
        TinkerCommand command = parseCommand("A6=150");
        THEN("the result contains the pin info and value") {
            REQUIRE(command.pinType.equals("A"));
            REQUIRE(command.pinNumber == 6);
            REQUIRE(command.value == 150);
            REQUIRE(command.hasNumber == true);
            REQUIRE(command.hasValue == true);
        }
    }
}

SCENARIO("Digital read") {
    GIVEN("a valid pin number") {
        WHEN("D pin") {
            int pin = D2;
            digitalReadValue[pin] = 1;
            int result = tinkerDigitalRead("D2");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == INPUT_PULLDOWN);
            }
            THEN("it returns value for correct pin") {
                REQUIRE(digitalReadPin == pin);
                REQUIRE(result == 1);
            }
        }
        WHEN("A pin") {
            int pin = A2;
            digitalReadValue[pin] = 1;
            int result = tinkerDigitalRead("A2");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == INPUT_PULLDOWN);
            }
            THEN("it returns value for correct pin") {
                REQUIRE(digitalReadPin == pin);
                REQUIRE(result == 1);
            }
        }
        WHEN("B pin") {
            int pin = B2;
            digitalReadValue[pin] = 1;
            int result = tinkerDigitalRead("B2");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == INPUT_PULLDOWN);
            }
            THEN("it returns value for correct pin") {
                REQUIRE(digitalReadPin == pin);
                REQUIRE(result == 1);
            }
        }
        WHEN("C pin") {
            int pin = C2;
            digitalReadValue[pin] = 1;
            int result = tinkerDigitalRead("C2");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == INPUT_PULLDOWN);
            }
            THEN("it returns value for correct pin") {
                REQUIRE(digitalReadPin == pin);
                REQUIRE(result == 1);
            }
        }
        WHEN("GPIO pin") {
            int pin = GPIO2;
            digitalReadValue[pin] = 1;
            int result = tinkerDigitalRead("GPIO2");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == INPUT_PULLDOWN);
            }
            THEN("returns value for correct pin") {
                REQUIRE(digitalReadPin == pin);
                REQUIRE(result == 1);
            }
        }
    }

    GIVEN("an invalid pin number") {
        int result = tinkerDigitalRead("D99");
        THEN("it return error") {
            REQUIRE(result == -1);
        }
    }

    GIVEN("a missing pin number") {
        int result = tinkerDigitalRead("D");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("invalid input") {
        int result = tinkerDigitalRead("foo");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }
}

SCENARIO("Digital write") {
    GIVEN("a valid pin number") {
        WHEN("D pin") {
            int pin = D3;
            int result = tinkerDigitalWrite("D3=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(digitalWritePin == pin);
                REQUIRE(digitalWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("A pin") {
            int pin = A3;
            int result = tinkerDigitalWrite("A3=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(digitalWritePin == pin);
                REQUIRE(digitalWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("B pin") {
            int pin = B3;
            int result = tinkerDigitalWrite("B3=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(digitalWritePin == pin);
                REQUIRE(digitalWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("C pin") {
            int pin = C3;
            int result = tinkerDigitalWrite("C3=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(digitalWritePin == pin);
                REQUIRE(digitalWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("GPIO pin") {
            int pin = GPIO3;
            int result = tinkerDigitalWrite("GPIO3=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(digitalWritePin == pin);
                REQUIRE(digitalWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
    }

    GIVEN("an invalid pin number") {
        int result = tinkerDigitalWrite("D99=HIGH");
        THEN("it return error") {
            REQUIRE(result == -1);
        }
    }

    GIVEN("a missing pin number") {
        int result = tinkerDigitalWrite("D=HIGH");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("a missing value") {
        int result = tinkerDigitalWrite("D0");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("a bad value") {
        int result = tinkerDigitalWrite("D3=2");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("invalid input") {
        int result = tinkerDigitalWrite("foo");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }
}

SCENARIO("Analog read") {
    GIVEN("a valid pin number") {
        WHEN("A pin") {
            int pin = A4;
            analogReadValue[pin] = 50;
            int result = tinkerAnalogRead("A4");
            THEN("returns value for correct pin") {
                REQUIRE(analogReadPin == pin);
                REQUIRE(result == 50);
            }
        }
        WHEN("B pin") {
            int pin = B4;
            analogReadValue[pin] = 50;
            int result = tinkerAnalogRead("B4");
            THEN("returns value for correct pin") {
                REQUIRE(analogReadPin == pin);
                REQUIRE(result == 50);
            }
        }
    }

    GIVEN("an invalid pin number") {
        int result = tinkerAnalogRead("A99");
        THEN("it return error") {
            REQUIRE(result == -1);
        }
    }

    GIVEN("a missing pin number") {
        int result = tinkerAnalogRead("A");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("an unsupported pin") {
        int result = tinkerAnalogRead("D2");
        THEN("it return error") {
            REQUIRE(result == -3);
        }
    }

    GIVEN("invalid input") {
        int result = tinkerAnalogRead("foo");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }
}

SCENARIO("Analog write") {
    GIVEN("a valid pin number") {
        WHEN("D pin") {
            int pin = D5;
            int result = tinkerAnalogWrite("D5=50");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 50);
                REQUIRE(result == 1);
            }
        }
        WHEN("A pin") {
            int pin = A5;
            int result = tinkerAnalogWrite("A5=50");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 50);
                REQUIRE(result == 1);
            }
        }
        WHEN("TX pin") {
            int pin = TX;
            int result = tinkerAnalogWrite("TX=50");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 50);
                REQUIRE(result == 1);
            }
        }
        WHEN("RX pin") {
            int pin = RX;
            int result = tinkerAnalogWrite("RX=50");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 50);
                REQUIRE(result == 1);
            }
        }
        WHEN("B pin") {
            int pin = B5;
            int result = tinkerAnalogWrite("B5=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("C pin") {
            int pin = C5;
            int result = tinkerAnalogWrite("C5=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
        WHEN("GPIO pin") {
            int pin = GPIO5;
            int result = tinkerAnalogWrite("GPIO5=HIGH");
            THEN("it sets correct pin mode") {
                REQUIRE(pinModePin == pin);
                REQUIRE(pinModeMode[pin] == OUTPUT);
            }
            THEN("it sets value for correct pin") {
                REQUIRE(analogWritePin == pin);
                REQUIRE(analogWriteValue[pin] == 1);
                REQUIRE(result == 1);
            }
        }
    }

    GIVEN("an invalid pin number") {
        int result = tinkerAnalogWrite("D99=50");
        THEN("it return error") {
            REQUIRE(result == -1);
        }
    }

    GIVEN("a missing pin number") {
        int result = tinkerAnalogWrite("D=50");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("a missing value") {
        int result = tinkerAnalogWrite("D0");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }

    GIVEN("invalid input") {
        int result = tinkerAnalogWrite("foo");
        THEN("it return error") {
            REQUIRE(result == -2);
        }
    }
}
