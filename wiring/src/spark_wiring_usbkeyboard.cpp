/**
 ******************************************************************************
 * @file    spark_wiring_usbkeyboard.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Wrapper for wiring usb keyboard hid module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "usb_hal.h"

#ifdef SPARK_USB_KEYBOARD
#include "spark_wiring_usbkeyboard.h"

#define SHIFT_MOD 0x80

extern const uint8_t usb_hid_asciimap[128] =
{
    KEY_RESERVED,               // 0x00 NUL
    KEY_RESERVED,               // 0x01 SOH
    KEY_RESERVED,               // 0x02 STX
    KEY_RESERVED,               // 0x03 ETX
    KEY_RESERVED,               // 0x04 EOT
    KEY_RESERVED,               // 0x05 ENQ
    KEY_RESERVED,               // 0x06 ACK
    KEY_RESERVED,               // 0x07 BEL
    KEY_BACKSPACE,              // 0x08 BS Backspace
    KEY_TAB,                    // 0x09 TAB Tab
    KEY_ENTER,                  // 0x0A LF Line Feed
    KEY_RESERVED,               // 0x0B VT
    KEY_RESERVED,               // 0x0C FF
    KEY_RESERVED,               // 0x0D CR Carriage return
    KEY_RESERVED,               // 0x0E SO
    KEY_RESERVED,               // 0x0F SI
    KEY_RESERVED,               // 0x10 DEL
    KEY_RESERVED,               // 0x11 DC1
    KEY_RESERVED,               // 0x12 DC2
    KEY_RESERVED,               // 0x13 DC3
    KEY_RESERVED,               // 0x14 DC4
    KEY_RESERVED,               // 0x15 NAK
    KEY_RESERVED,               // 0x16 SYN
    KEY_RESERVED,               // 0x17 ETB
    KEY_RESERVED,               // 0x18 CAN
    KEY_RESERVED,               // 0x19 EM
    KEY_RESERVED,               // 0x1A SUB
    KEY_ESCAPE,                 // 0x1B ESC
    KEY_RESERVED,               // 0x1C FS
    KEY_RESERVED,               // 0x1D GS
    KEY_RESERVED,               // 0x1E RS
    KEY_RESERVED,               // 0x1F US
    KEY_SPACE,                  // 0x20  ' '
    SHIFT_MOD | KEY_1,          // 0x21 !
    SHIFT_MOD | KEY_APOSTROPHE, // 0x22 "
    SHIFT_MOD | KEY_3,          // 0x23 #
    SHIFT_MOD | KEY_4,          // 0x24 $
    SHIFT_MOD | KEY_5,          // 0x25 %
    SHIFT_MOD | KEY_7,          // 0x26 &
    KEY_APOSTROPHE,             // 0x27 '
    SHIFT_MOD | KEY_9,          // 0x28 (
    SHIFT_MOD | KEY_0,          // 0x29 )
    SHIFT_MOD | KEY_8,          // 0x2A *
    SHIFT_MOD | KEY_EQUAL,      // 0x2B +
    KEY_COMMA,                  // 0x2C ,
    KEY_MINUS,                  // 0x2D -
    KEY_DOT,                    // 0x2E .
    KEY_SLASH,                  // 0x2F /
    KEY_0,                      // 0x30 0
    KEY_1,                      // 0x31 1
    KEY_2,                      // 0x32 2
    KEY_3,                      // 0x33 3
    KEY_4,                      // 0x34 4
    KEY_5,                      // 0x35 5
    KEY_6,                      // 0x36 6
    KEY_7,                      // 0x37 7
    KEY_8,                      // 0x38 8
    KEY_9,                      // 0x39 9
    SHIFT_MOD | KEY_SEMICOLON,  // 0x3A :
    KEY_SEMICOLON,              // 0x3B ;
    SHIFT_MOD | KEY_COMMA,      // 0x3C <
    KEY_EQUAL,                  // 0x3D =
    SHIFT_MOD | KEY_DOT,        // 0x3E >
    SHIFT_MOD | KEY_SLASH,      // 0x3F ?
    SHIFT_MOD | KEY_2,          // 0x40 @
    SHIFT_MOD | KEY_A,          // 0x41 A
    SHIFT_MOD | KEY_B,          // 0x42 B
    SHIFT_MOD | KEY_C,          // 0x43 C
    SHIFT_MOD | KEY_D,          // 0x44 D
    SHIFT_MOD | KEY_E,          // 0x45 E
    SHIFT_MOD | KEY_F,          // 0x46 F
    SHIFT_MOD | KEY_G,          // 0x47 G
    SHIFT_MOD | KEY_H,          // 0x48 H
    SHIFT_MOD | KEY_I,          // 0x49 I
    SHIFT_MOD | KEY_J,          // 0x4A J
    SHIFT_MOD | KEY_K,          // 0x4B K
    SHIFT_MOD | KEY_L,          // 0x4C L
    SHIFT_MOD | KEY_M,          // 0x4D M
    SHIFT_MOD | KEY_N,          // 0x4E N
    SHIFT_MOD | KEY_O,          // 0x4F O
    SHIFT_MOD | KEY_P,          // 0x50 P
    SHIFT_MOD | KEY_Q,          // 0x51 Q
    SHIFT_MOD | KEY_R,          // 0x52 R
    SHIFT_MOD | KEY_S,          // 0x53 S
    SHIFT_MOD | KEY_T,          // 0x54 T
    SHIFT_MOD | KEY_U,          // 0x55 U
    SHIFT_MOD | KEY_V,          // 0x56 V
    SHIFT_MOD | KEY_W,          // 0x57 W
    SHIFT_MOD | KEY_X,          // 0x58 X
    SHIFT_MOD | KEY_Y,          // 0x59 Y
    SHIFT_MOD | KEY_Z,          // 0x5A Z
    KEY_LEFTBRACE,              // 0x5B [
    KEY_BACKSLASH,              // 0x5C bslash
    KEY_RIGHTBRACE,             // 0x5D ]
    SHIFT_MOD | KEY_6,          // 0x5E ^
    SHIFT_MOD | KEY_MINUS,      // 0x5F _
    KEY_GRAVE,                  // 0x60 `
    KEY_A,                      // 0x61 a
    KEY_B,                      // 0x62 b
    KEY_C,                      // 0x63 c
    KEY_D,                      // 0x64 d
    KEY_E,                      // 0x65 e
    KEY_F,                      // 0x66 f
    KEY_G,                      // 0x67 g
    KEY_H,                      // 0x68 h
    KEY_I,                      // 0x69 i
    KEY_J,                      // 0x6A j
    KEY_K,                      // 0x6B k
    KEY_L,                      // 0x6C l
    KEY_M,                      // 0x6D m
    KEY_N,                      // 0x6E n
    KEY_O,                      // 0x6F o
    KEY_P,                      // 0x70 p
    KEY_Q,                      // 0x71 q
    KEY_R,                      // 0x72 r
    KEY_S,                      // 0x73 s
    KEY_T,                      // 0x74 t
    KEY_U,                      // 0x75 u
    KEY_V,                      // 0x76 v
    KEY_W,                      // 0x77 w
    KEY_X,                      // 0x78 x
    KEY_Y,                      // 0x79 y
    KEY_Z,                      // 0x7A z
    SHIFT_MOD | KEY_LEFTBRACE,  // 0x7B {
    SHIFT_MOD | KEY_BACKSLASH,  // 0x7C |
    SHIFT_MOD | KEY_RIGHTBRACE, // 0x7D }
    SHIFT_MOD | KEY_GRAVE,      // 0x7E ~
    KEY_DELETE                  // 0x7F DEL
};

//
// Constructor
//
USBKeyboard::USBKeyboard(void)
{
    memset((void*)&keyReport, 0, sizeof(keyReport));
    keyReport.reportId = 0x02;
    HAL_USB_HID_Init(0, NULL);
}

void USBKeyboard::begin(void)
{
    HAL_USB_HID_Begin(0, NULL);
}

void USBKeyboard::end(void)
{
    HAL_USB_HID_End(0);
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t USBKeyboard::press(uint16_t key, uint16_t modifiers)
{
    bool doReport = false;
    if (key >= KEY_LEFTCTRL && key <= KEY_RIGHTGUI)
    {
       // it's a modifier key
       keyReport.modifiers |= (1 << (key - KEY_LEFTCTRL));
       key = 0;
       doReport = true;
    }

    if (key > KEY_KPHEX)
        return 0;

    if (modifiers) {
        if (modifiers > MOD_RESERVED && modifiers <= (MOD_RESERVED | 0xFF)) {
            modifiers &= 0x00FF;
            keyReport.modifiers |= modifiers;
            doReport = true;
        } else {
            modifiers = 0;
        }
    }

    // Add key to the keyReport only if it's not already present
    // and if there is an empty slot.
    if (key && keyReport.keys[0] != key && keyReport.keys[1] != key &&
        keyReport.keys[2] != key && keyReport.keys[3] != key &&
        keyReport.keys[4] != key && keyReport.keys[5] != key)
    {
        uint8_t i;
        for (i = 0; i < 6; i++)
        {
            if (keyReport.keys[i] == 0x00)
            {
                keyReport.keys[i] = key;
                doReport = true;
                break;
            }
        }
        if (i == 6)
        {
            setWriteError();
            return 0;
        }
    }

    if (doReport) {
        sendReport();
        return 1;
    }
    return 0;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t USBKeyboard::release(uint16_t key, uint16_t modifiers)
{
    bool doReport = false;
    if (key >= KEY_LEFTCTRL && key <= KEY_RIGHTGUI)
    {
       // it's a modifier key
       keyReport.modifiers &= ~(1 << (key - KEY_LEFTCTRL));
       key = 0;
       doReport = true;
    }

    if (key > KEY_KPHEX)
        return 0;

    if (modifiers) {
        if (modifiers > MOD_RESERVED && modifiers <= (MOD_RESERVED | 0xFF)) {
            modifiers &= 0x00FF;
            keyReport.modifiers &= ~(modifiers);
            doReport = true;
        } else {
            modifiers = 0;
        }
    }

    // Test the key report to see if key is present.  Clear it if it exists.
    // Check all positions in case the key is present more than once (which it shouldn't be)
    if (key) {
        for (uint8_t i = 0; i < 6; i++)
        {
            if (0 != key && keyReport.keys[i] == key)
            {
                keyReport.keys[i] = 0x00;
                doReport = true;
            }
        }
    }

    if (doReport) {
        sendReport();
        return 1;
    }

    return 0;
}

void USBKeyboard::releaseAll(void)
{
    keyReport.keys[0] = 0;
    keyReport.keys[1] = 0;
    keyReport.keys[2] = 0;
    keyReport.keys[3] = 0;
    keyReport.keys[4] = 0;
    keyReport.keys[5] = 0;
    keyReport.modifiers = 0;
    sendReport();
}

size_t USBKeyboard::writeKey(uint16_t key, uint16_t modifiers)
{
    // Keydown
    uint8_t p = press(key, modifiers);
    // Keyup
    uint8_t r = release(key, modifiers);
    (void)r;
    // just return the result of press() since release() almost always returns 1
    return (p);
}

size_t USBKeyboard::click(uint16_t key, uint16_t modifiers)
{
    return writeKey(key, modifiers);
}

size_t USBKeyboard::write(uint8_t ch)
{
    uint16_t key = 0;
    uint16_t modifiers = 0;

    if (ch < sizeof(usb_hid_asciimap)) {
        // Lookup key code from ascii -> keycode map
        key = usb_hid_asciimap[ch];
        if (key) {
            modifiers = (key & SHIFT_MOD) ? MOD_LEFTSHIFT : 0;
        }
        key &= 0x7F;
    }
    
    if (key)
        return writeKey(key, modifiers);
    return 0;
}

void USBKeyboard::sendReport()
{
    uint32_t m = millis();
    while(HAL_USB_HID_Status(0, nullptr)) {
       // Wait 1 bInterval (1ms)
       delay(1);
       if ((millis() - m) >= 50)
         return;
    }
    HAL_USB_HID_Send_Report(0, &keyReport, sizeof(keyReport), NULL);
    m = millis();
    while(HAL_USB_HID_Status(0, nullptr)) {
       // Wait 1 bInterval (1ms)
       delay(1);
       if ((millis() - m) >= 50)
         return;
    }
}

USBKeyboard& _fetch_usbkeyboard()
{
  static USBKeyboard _usbkeyboard;
  return _usbkeyboard;
}

#endif
