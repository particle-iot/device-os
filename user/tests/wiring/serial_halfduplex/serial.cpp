#include "application.h"
#if (PLATFORM_ID == 6 || PLATFORM_ID == 8)
#include "Serial2/Serial2.h"
#define TESTSERIAL1 Serial1
#define TESTSERIAL2 Serial2
#define TX1 TX
#define TX2 RGBG
#elif (PLATFORM_ID == 10)
#include "Serial4/Serial4.h"
#include "Serial5/Serial5.h"
#define TESTSERIAL1 Serial4
#define TESTSERIAL2 Serial5
#define TX1 C3
#define TX2 C1
#elif (PLATFORM_ID == 0)
#include "Serial2/Serial2.h"
#define TESTSERIAL1 Serial1
#define TESTSERIAL2 Serial2
#define TX1 TX
#define TX2 D1
#warning Core requires external pull-up resistor attached to TX or D1, as it doesn't support Open-Drain or Push-Pull mode with pull-up
#else
#error Unsupported platform
#endif
#include "unit-test/unit-test.h"

#define BAUD_RATE 2400
#define TEST_MESSAGE "All work and no play makes Jack a dull boy"


/*
 * TX1 and TX2 (for the actual pin names see the defines ^^) pins need to be jumpered for this test:
 *            WIRE
 * (TX1) --==========-- (TX2)
 *
 * NOTE: Core requires external pull-up resistor attached to TX or D1,
 * as it doesn't support Open-Drain or Push-Pull mode with pull-up.
 *
 *            3V3
 *           _____
 *             |
 *            +++
 *            | | 4k7
 *            | |
 *            +++
 *             |
 *             |
 * (TX) --=====+=====-- (D1)
 *
 */

void serialReadLineNoEcho(Stream *serialObj, char *dst, int max_len, system_tick_t timeout)
{
    char c = 0, i = 0;
    system_tick_t last_millis = millis();

    while (1)
    {
        if((timeout > 0) && ((millis()-last_millis) > timeout))
        {
            //Abort after a specified timeout
            break;
        }

        if (0 < serialObj->available())
        {
            c = serialObj->read();

            if (i == max_len || c == '\r' || c == '\n')
            {
                if (c == '\r' && (serialObj->available() > 0) && (serialObj->peek() == '\n')) {
                    // consume it too
                    (void)serialObj->read();
                }
                *dst = '\0';
                break;
            }

            if (c == 8 || c == 127)
            {
                //for backspace or delete
                if (i > 0)
                {
                    --dst;
                    --i;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                *dst++ = c;
                ++i;
            }
        }
    }
}


void switchPinToAfOpenDrainWithPullUp(pin_t pin)
{
#if PLATFORM_ID != 0
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio_port, &GPIO_InitStructure);
#endif // PLATFORM_ID != 0
}

void consume(Stream& serial)
{
    while (serial.available() > 0) {
        (void)serial.read();
    }
}

static bool pinIsFloating(const pin_t pin)
{
#if PLATFORM_ID != 0
    PinMode mode = getPinMode(pin);
    pinMode(pin, INPUT_PULLUP);
    delay(1);
    int32_t pup = digitalRead(pin);
    pinMode(pin, INPUT_PULLDOWN);
    delay(1);
    int32_t pud = digitalRead(pin);

    pinMode(pin, mode);

    return (pup == HIGH && pud == LOW);
#else
    return true;
#endif // PLATFORM_ID != 0
}

test(SERIAL_01_serial_in_half_duplex_mode_with_opendrain_tx_works_normally_when_first_serial_transmits)
{
    char tmp[64] = {0};

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN);
    assertEqual(getPinMode(TX1), AF_OUTPUT_DRAIN);
    assertEqual(getPinMode(TX2), AF_OUTPUT_DRAIN);
    switchPinToAfOpenDrainWithPullUp(TX1);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL1.println(TEST_MESSAGE);
    delay(1000);

    // By default echo is enabled, so both Serials under test should have received the transmitted buffer
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}

test(SERIAL_02_serial_in_half_duplex_mode_with_opendrain_tx_works_normally_when_second_serial_transmits)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN);
    assertEqual(getPinMode(TX1), AF_OUTPUT_DRAIN);
    assertEqual(getPinMode(TX2), AF_OUTPUT_DRAIN);
    switchPinToAfOpenDrainWithPullUp(TX1);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL2.println(TEST_MESSAGE);
    delay(1000);

    // By default echo is enabled, so both Serials under test should have received the transmitted buffer
    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}


test(SERIAL_03_serial_in_half_duplex_mode_with_opendrain_tx_with_echo_disabled_works_normally)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN | SERIAL_HALF_DUPLEX_NO_ECHO);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX_OPEN_DRAIN | SERIAL_HALF_DUPLEX_NO_ECHO);
    assertEqual(getPinMode(TX1), AF_OUTPUT_DRAIN);
    assertEqual(getPinMode(TX2), AF_OUTPUT_DRAIN);
    switchPinToAfOpenDrainWithPullUp(TX1);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL1.println(TEST_MESSAGE);
    delay(1000);

    assertEqual(TESTSERIAL1.available(), 0);
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    TESTSERIAL2.println(TEST_MESSAGE);
    delay(1000);

    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL2.available(), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}

test(SERIAL_04_serial_in_half_duplex_mode_with_pushpull_tx_works_normally_when_first_serial_transmits)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    // Both should be floating when not transmitting
    assertTrue(pinIsFloating(TX1));
    assertTrue(pinIsFloating(TX2));

    // Reconfigure with one of USARTS pulling up to avoid noise
    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_TX_PULL_UP);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL1.println(TEST_MESSAGE);
    delay(1000);

    // By default echo is enabled, so both Serials under test should have received the transmitted buffer
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}

test(SERIAL_05_serial_in_half_duplex_mode_with_pushpull_tx_works_normally_when_second_serial_transmits)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    // Both should be floating when not transmitting
    assertTrue(pinIsFloating(TX1));
    assertTrue(pinIsFloating(TX2));

    // Reconfigure with one of USARTS pulling up to avoid noise
    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_TX_PULL_UP);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL2.println(TEST_MESSAGE);
    delay(1000);

    // By default echo is enabled, so both Serials under test should have received the transmitted buffer
    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}


test(SERIAL_06_serial_in_half_duplex_mode_with_pushpull_tx_with_echo_disabled_works_normally)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    // Both should be floating when not transmitting
    assertTrue(pinIsFloating(TX1));
    assertTrue(pinIsFloating(TX2));

    // Reconfigure with one of USARTS pulling up to avoid noise
    TESTSERIAL1.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_TX_PULL_UP | SERIAL_HALF_DUPLEX_NO_ECHO);
    TESTSERIAL2.begin(BAUD_RATE, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

    consume(TESTSERIAL1);
    consume(TESTSERIAL2);

    TESTSERIAL1.println(TEST_MESSAGE);
    delay(1000);

    assertEqual(TESTSERIAL1.available(), 0);
    assertEqual(TESTSERIAL2.available(), strlen(TEST_MESSAGE) + 2);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL2, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);

    TESTSERIAL2.println(TEST_MESSAGE);
    delay(1000);

    assertEqual(TESTSERIAL1.available(), strlen(TEST_MESSAGE) + 2);
    assertEqual(TESTSERIAL2.available(), 0);

    memset(tmp, 0, sizeof(tmp));
    serialReadLineNoEcho(&TESTSERIAL1, tmp, sizeof(tmp) - 1, 1000);
    assertEqual(strcmp(tmp, TEST_MESSAGE), 0);
}
