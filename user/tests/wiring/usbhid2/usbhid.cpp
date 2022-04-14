#include "application.h"
#include "unit-test/unit-test.h"
#include <cmath>

void startup();
STARTUP(startup());

typedef struct {
    char ev;
    int x;
    int y;
    int v1;
    int v2;
    int v3;
} HidEvent;

void startup()
{
    Keyboard.begin();
    Mouse.begin();
}

// asciimap is defined in spark_wiring_usbkeyboard.cpp
extern const uint8_t usb_hid_asciimap[128];
#define SHIFT_MOD 0x80

static uint16_t s_aScreenWidth = 0;
static uint16_t s_aScreenHeight = 0;

static uint16_t s_rScreenWidth = 0;
static uint16_t s_rScreenHeight = 0;

static uint8_t s_relativeWorks = 0;

int randomString(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t d = random(0, 15);
        char c = d + 48;
        if (57 < c)
            c += 7;
        buf[i] = c;
    }

    return len;
}

void consume(Stream& serial)
{
    while (serial.available() > 0) {
        (void)serial.read();
    }
}

int serialReadLineNoEcho(Stream *serialObj, uint8_t *dst, int max_len, system_tick_t timeout)
{
    uint16_t c = 0, i = 0;
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
                *dst = '\0';
                break;
            }

            *dst++ = c;
            ++i;
        }
    }

    return (*dst == '\0');
}

bool readHidEvent(Stream& serial, HidEvent& ev, uint32_t timeout = 2000)
{
    char msg[255];
    memset(msg, 'a', sizeof(msg));
    msg[sizeof(msg) - 1] = '\0';
    if (serialReadLineNoEcho(&serial, (uint8_t*)msg, sizeof(msg), timeout)) {
        if (sscanf(msg, "%c %d %d %d %d %d", &ev.ev, &ev.x, &ev.y, &ev.v1, &ev.v2, &ev.v3) == 6) {
            return true;
        }
    }

    return false;
}

test(USBHID_01_Keyboard_Print_Random_Printable_Characters) {
    delay(1000);
    srand(millis());

    char randStr[65];
    HidEvent ev;
    memset(randStr, 0, sizeof(randStr));
    randomString(randStr, sizeof(randStr) - 1);
    Serial.println(randStr);

    consume(Serial);
    for (int i = 0; i < sizeof(randStr) - 1; i++) {
        Keyboard.print(randStr[i]);
        for(;;) {
            assertTrue(readHidEvent(Serial, ev));
            assertEqual(ev.ev, 'k');
            // Skip SHIFT
            if (ev.v1 == KEY_LSHIFT)
                continue;
            assertEqual((char)ev.v3, randStr[i]);
            break;
        }
    }
}

test(USBHID_02_Keyboard_Print_Full_Ascii_Map) {
    HidEvent ev;
    consume(Serial);
    for (char i = 0; i < sizeof(usb_hid_asciimap); i++) {
        // Skip KEY_RESERVED (0x00)
        if (usb_hid_asciimap[(int)i] == 0x00)
            continue;
        Keyboard.print(i);
        assertTrue(readHidEvent(Serial, ev));
        assertEqual(ev.ev, 'k');
        if (usb_hid_asciimap[(int)i] & SHIFT_MOD) {
            // Skip SHIFT
            assertTrue(readHidEvent(Serial, ev));
            assertEqual(ev.ev, 'k');
        }
        if (ev.v2 & MOD_LSHIFT) { // SHIFT
            ev.v1 |= SHIFT_MOD;
        }
        assertEqual(usb_hid_asciimap[(int)i], (uint8_t)ev.v1);
        if (isprint(i)) {
            assertEqual(i, (char)ev.v3);
        }
    }
}

test(USBHID_03_Keyboard_Up_To_Six_Buttons_Can_Pressed_Simultaneously) {
    HidEvent ev;
    consume(Serial);

    assertTrue((bool)Keyboard.press(KEY_1));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_1);

    assertTrue((bool)Keyboard.press(KEY_2));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_2);

    assertTrue((bool)Keyboard.press(KEY_3));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_3);

    assertTrue((bool)Keyboard.press(KEY_4));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_4);

    assertTrue((bool)Keyboard.press(KEY_5));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_5);

    assertTrue((bool)Keyboard.press(KEY_6));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_6);

    assertFalse((bool)Keyboard.press(KEY_7));

    assertTrue((bool)Keyboard.release(KEY_6));

    assertTrue((bool)Keyboard.press(KEY_7));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_7);

    assertFalse((bool)Keyboard.press(KEY_8));

    Keyboard.releaseAll();
}

test(USBHID_04_Keyboard_Modifier_Keys_Are_Sent_As_Modifiers_And_Not_Key_Pressess) {
    HidEvent ev;
    consume(Serial);

    Keyboard.releaseAll();

    // Press 6 buttons
    assertTrue((bool)Keyboard.press(KEY_1));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_1);

    assertTrue((bool)Keyboard.press(KEY_2));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_2);

    assertTrue((bool)Keyboard.press(KEY_3));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_3);

    assertTrue((bool)Keyboard.press(KEY_4));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_4);

    assertTrue((bool)Keyboard.press(KEY_5));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_5);

    assertTrue((bool)Keyboard.press(KEY_6));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_6);

    assertFalse((bool)Keyboard.press(KEY_7));

    assertTrue((bool)Keyboard.release(KEY_6));

    assertTrue((bool)Keyboard.press(KEY_7));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_7);

    assertFalse((bool)Keyboard.press(KEY_8));

    assertTrue((bool)Keyboard.click(KEY_LCTRL));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LCTRL);

    assertTrue((bool)Keyboard.click(KEY_LSHIFT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LSHIFT);

    assertTrue((bool)Keyboard.click(KEY_LALT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LALT);

    // assertTrue((bool)Keyboard.click(KEY_LGUI));
    // assertTrue(readHidEvent(Serial, ev));
    // assertEqual(ev.ev, 'k');
    // assertEqual(ev.v1, (int)KEY_LGUI);

    assertTrue((bool)Keyboard.click(KEY_RCTRL));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RCTRL);

    assertTrue((bool)Keyboard.click(KEY_RSHIFT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RSHIFT);

    assertTrue((bool)Keyboard.click(KEY_RALT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RALT);

    // assertTrue((bool)Keyboard.click(KEY_RGUI));
    // assertTrue(readHidEvent(Serial, ev));
    // assertEqual(ev.ev, 'k');
    // assertEqual(ev.v1, (int)KEY_RGUI);

    Keyboard.releaseAll();
}

test(USBHID_05_Keyboard_Modifiers_Work_Correctly) {
    HidEvent ev;
    consume(Serial);

    Keyboard.releaseAll();

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_LCTRL));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LCTRL);

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_LSHIFT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LSHIFT);

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_LALT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_LALT);

    // assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_LGUI));
    // assertTrue(readHidEvent(Serial, ev));
    // assertEqual(ev.ev, 'k');
    // assertEqual(ev.v1, (int)KEY_LGUI);

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_RCTRL));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RCTRL);

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_RSHIFT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RSHIFT);

    assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_RALT));
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'k');
    assertEqual(ev.v1, (int)KEY_RALT);

    // assertTrue((bool)Keyboard.click(KEY_RESERVED, MOD_RGUI));
    // assertTrue(readHidEvent(Serial, ev));
    // assertEqual(ev.ev, 'k');
    // assertEqual(ev.v1, (int)KEY_RGUI);

    Keyboard.releaseAll();
}

test(USBHID_06_Keyboard_Multiple_Modifiers_Can_Be_Sent_Simultaneously) {
    /*
     * This test should fail on Windows
     */
    HidEvent ev;
    consume(Serial);

    Keyboard.releaseAll();

    uint16_t modifiers = MOD_LCTRL | MOD_LSHIFT | MOD_LALT | /* MOD_LGUI */
                         MOD_RCTRL | MOD_RSHIFT | MOD_RALT /* | MOD_RGUI */;
    assertTrue((bool)Keyboard.click(KEY_RESERVED, modifiers));
    while (modifiers) {
        assertTrue(readHidEvent(Serial, ev));
        assertEqual(ev.ev, 'k');
        switch (ev.v1) {
            case KEY_LCTRL:
                modifiers &= ~(MOD_LCTRL);
                break;
            case KEY_LSHIFT:
                modifiers &= ~(MOD_LSHIFT);
                break;
            case KEY_LALT:
                modifiers &= ~(MOD_LALT);
                break;
            case KEY_LGUI:
                modifiers &= ~(MOD_LGUI);
                break;
            case KEY_RCTRL:
                modifiers &= ~(MOD_RCTRL);
                break;
            case KEY_RSHIFT:
                modifiers &= ~(MOD_RSHIFT);
                break;
            case KEY_RALT:
                modifiers &= ~(MOD_RALT);
                break;
            case KEY_RGUI:
                modifiers &= ~(MOD_RGUI);
                break;
            default:
                assertTrue(false);
                break;
        }
    }

    Keyboard.releaseAll();
}

test(USBHID_07_Keyboard_Most_Keycodes_Are_Correctly_Delivered) {
    HidEvent ev;
    consume(Serial);

    Keyboard.releaseAll();
    int errors = 0;
    for (uint16_t k = KEY_A; k <= KEY_KPHEX; k++) {
        // Avoid sending these keys
        switch (k) {
            case KEY_POWER:
            case KEY_APPLICATION:
            case KEY_PRINTSCREEN:
            case KEY_F13:
            case KEY_F14:
            case KEY_F15:
            case KEY_F16:
            case KEY_F17:
            case KEY_F18:
            case KEY_F19:
            case KEY_F20:
            case KEY_F21:
            case KEY_F22:
            case KEY_F23:
            case KEY_F24:
                continue;
                break;
        }

        assertTrue(Keyboard.click(k));
        if (!readHidEvent(Serial, ev, 200)) {
            errors++;
        } else {
            assertEqual(ev.ev, 'k');
        }
    }
}

test(USBHID_08_Mouse_Absolute_Movement_Figure_Out_Resolution) {
    HidEvent ev;
    consume(Serial);

    Keyboard.releaseAll();

    Mouse.moveTo(INT16_MAX, INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x > 0 && ev.y > 0);
    Serial.printf("Resolution: %d %d\r\n", ev.x, ev.y);
    Mouse.screenSize(ev.x, ev.y);
    s_aScreenWidth = ev.x;
    s_aScreenHeight = ev.y;
}

test(USBHID_09_Mouse_Absolute_Movement_With_Screen_Size_Works_Correctly) {
    HidEvent ev;
    consume(Serial);

    Mouse.moveTo(0, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);

    Mouse.moveTo(s_aScreenWidth, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == s_aScreenWidth && ev.y == 0);

    Mouse.moveTo(0, s_aScreenHeight);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == s_aScreenHeight);

    Mouse.moveTo(s_aScreenWidth, s_aScreenHeight);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == s_aScreenWidth && ev.y == s_aScreenHeight);

    Mouse.moveTo(0, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);
}

test(USBHID_10_Mouse_Relative_Movement_Figure_Out_Resolution) {
    /*
     * This test should fail on Linux
     */
    HidEvent ev;
    consume(Serial);

    Mouse.moveTo(100, 100);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');

    Mouse.move(-INT16_MAX, -INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);
    s_relativeWorks = 1;

    Mouse.move(INT16_MAX, INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x >= s_aScreenWidth && ev.y >= s_aScreenHeight);
    s_rScreenWidth = ev.x;
    s_rScreenHeight = ev.y;

    Mouse.move(-INT16_MAX, -INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);
}

test(USBHID_11_Mouse_Relative_Mixed_With_Absolute_Works_Correctly) {
    /*
     * This test should fail on OSX and Linux
     */

    if (!s_relativeWorks) {
        fail();
        return;
    }
    HidEvent ev;
    consume(Serial);

    Mouse.moveTo(s_aScreenWidth, s_aScreenHeight);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == s_aScreenWidth && ev.y == s_aScreenHeight);

    Mouse.move(-INT16_MAX, -INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);

    Mouse.move(INT16_MAX, INT16_MAX);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == s_rScreenWidth && ev.y == s_rScreenHeight);

    Mouse.moveTo(0, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);

    Mouse.move(0, 100);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y != 0);

    Mouse.moveTo(0, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);

    Mouse.move(100, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x != 0 && ev.y == 0);

    Mouse.moveTo(0, 0);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue(ev.x == 0 && ev.y == 0);
}

test(USBHID_12_Mouse_Relative_Figure_Out_Maximum) {
    /*
     * This test should fail on Linux
     */
    if (!s_relativeWorks) {
        fail();
        return;
    }
    HidEvent ev;
    consume(Serial);
    //
    int relMax = 0;
    int relMaxC = 0;
    int relMaxPx = 0;
    for (int p = 0; p <= 32767;) {
        Mouse.moveTo(relMaxC % s_aScreenWidth, 0);
        readHidEvent(Serial, ev, 1000);

        memset(&ev, 0, sizeof(ev));
        Mouse.move(0, p);
        if (!readHidEvent(Serial, ev, 100)) {
            if (p > 100)
                break;
        } else {
            assertEqual(ev.ev, 'm');
        }
        relMaxC++;
        int l = floor(std::log10((float)p));
        switch(l) {
            case 3:
                p += 10;
                break;
            case 4:
                p += 100;
                break;
            case 5:
                p += 1000;
                break;
            default:
                p++;
                break;
        }

        relMax = p;
        if (ev.y > 0) {
            relMaxPx = ev.y;
            if (relMaxPx >= s_aScreenHeight)
                break;
        }
    }

    Serial.printf("Maximum relative movement: %d (%d px)\r\n", relMax, relMaxPx);

    relMax = 0;
    relMaxC = 0;
    relMaxPx = 0;
    for (int p = 0; p >= -32767;) {
        Mouse.moveTo(relMaxC % s_aScreenWidth, s_aScreenHeight);
        readHidEvent(Serial, ev, 1000);

        memset(&ev, 0, sizeof(ev));
        Mouse.move(0, p);
        if (!readHidEvent(Serial, ev, 100)) {
            if (p < -100)
                break;
        } else {
            assertEqual(ev.ev, 'm');
        }
        relMaxC++;
        int l = floor(std::log10(std::fabs((float)p)));
        switch(l) {
            case 3:
                p -= 10;
                break;
            case 4:
                p -= 100;
                break;
            case 5:
                p -= 1000;
                break;
            default:
                p--;
                break;
        }

        relMax = p;
        if (ev.y < s_aScreenHeight)
            relMaxPx = ev.y;
        if (ev.y == 0 && p < -100)
            break;
    }
}

test(USBHID_13_Mouse_Clicks_Work_Correctly) {
    HidEvent ev;
    consume(Serial);

    Mouse.moveTo(s_aScreenWidth / 2, s_aScreenHeight / 2);
    assertTrue(readHidEvent(Serial, ev));
    assertEqual(ev.ev, 'm');
    assertTrue((ev.x == (s_aScreenWidth / 2)) && (ev.y == (s_aScreenHeight / 2)));

    Mouse.click(MOUSE_LEFT);
    assertTrue(readHidEvent(Serial, ev, 1000));
    assertEqual(ev.ev, 'c');
    assertTrue((ev.x == (s_aScreenWidth / 2)) && (ev.y == (s_aScreenHeight / 2)));
    assertEqual(ev.v1, 1);

    Mouse.click(MOUSE_MIDDLE);
    assertTrue(readHidEvent(Serial, ev, 1000));
    assertEqual(ev.ev, 'c');
    assertTrue((ev.x == (s_aScreenWidth / 2)) && (ev.y == (s_aScreenHeight / 2)));
    assertEqual(ev.v1, 2);

    Mouse.click(MOUSE_RIGHT);
    assertTrue(readHidEvent(Serial, ev, 1000));
    assertEqual(ev.ev, 'c');
    assertTrue((ev.x == (s_aScreenWidth / 2)) && (ev.y == (s_aScreenHeight / 2)));
    assertEqual(ev.v1, 3);
}

test(USBHID_14_Mouse_Wheel_Works_Correctly) {
    HidEvent ev;
    consume(Serial);

    for (int p = -127; p <= 127; p++) {
        if (p == 0)
            continue;

        memset(&ev, 0, sizeof(ev));
        Mouse.scroll((int8_t)p);
        while(readHidEvent(Serial, ev, 200)) {
            assertEqual(ev.ev, 'w');
            if (p < 0) {
                assertTrue(ev.v1 < 0);
            } else {
                assertTrue(ev.v1 > 0);
            }
        }
    }
}
