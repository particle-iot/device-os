#include "application.h"
#if (PLATFORM_ID == 6 || PLATFORM_ID == 8)
#include "Serial2/Serial2.h"
#define TESTSERIAL1 Serial1
#define TESTSERIAL2 Serial2
#define TX1 TX
#elif (PLATFORM_ID == 10)
#include "Serial4/Serial4.h"
#include "Serial5/Serial5.h"
#define TESTSERIAL1 Serial4
#define TESTSERIAL2 Serial5
#define TX1 C3
#elif (PLATFORM_ID == 0)
#include "Serial2/Serial2.h"
#define TESTSERIAL1 Serial1
#define TESTSERIAL2 Serial2
#define TX1 TX
#else
#error Unsupported platform
#endif
#include "unit-test/unit-test.h"

#define TEST_MESSAGE "All work and no play makes Jack a dull boy"

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
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[pin].gpio_pin;
    GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
    GPIO_InitTypeDef GPIO_InitStructure = {0};
#if PLATFORM_ID != 0
    GPIO_InitStructure.GPIO_Pin = gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio_port, &GPIO_InitStructure);
#else
    GPIO_InitStructure.GPIO_Pin = gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio_port, &GPIO_InitStructure);
#endif // PLATFORM_ID != 0
}

void consume(Stream& serial)
{
    while (serial.available() > 0) {
        (void)serial.read();
    }
}

test(SERIAL_01_serial_in_default_half_duplex_mode_works_normally_when_first_serial_transmits)
{
    char tmp[64] = {0};
    TESTSERIAL1.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    TESTSERIAL2.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    switchPinToAfOpenDrainWithPullUp(TX1);
    assertEqual(TESTSERIAL1.isEnabled(), true);
    assertEqual(TESTSERIAL2.isEnabled(), true);

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

test(SERIAL_01_serial_in_default_half_duplex_mode_works_normally_when_second_serial_transmits)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
    TESTSERIAL2.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX);
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


test(SERIAL_03_serial_in_half_duplex_mode_with_echo_disabled_works_normally)
{
    char tmp[64] = {0};

    TESTSERIAL1.end();
    TESTSERIAL2.end();

    TESTSERIAL1.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);
    TESTSERIAL2.begin(1200, SERIAL_8N1 | SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);
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
