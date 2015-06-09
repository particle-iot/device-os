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

#ifdef SPARK_USB_KEYBOARD
#include "spark_wiring_usbkeyboard.h"

#define SHIFT 0x80

const uint8_t asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,             // BS  Backspace
	0x2b,             // TAB Tab
	0x28,             // LF  Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US
	0x2c,		      //  ' '
	0x1e|SHIFT,	      // !
	0x34|SHIFT,	      // "
	0x20|SHIFT,       // #
	0x21|SHIFT,       // $
	0x22|SHIFT,       // %
	0x24|SHIFT,       // &
	0x34,             // '
	0x26|SHIFT,       // (
	0x27|SHIFT,       // )
	0x25|SHIFT,       // *
	0x2e|SHIFT,       // +
	0x36,             // ,
	0x2d,             // -
	0x37,             // .
	0x38,             // /
	0x27,             // 0
	0x1e,             // 1
	0x1f,             // 2
	0x20,             // 3
	0x21,             // 4
	0x22,             // 5
	0x23,             // 6
	0x24,             // 7
	0x25,             // 8
	0x26,             // 9
	0x33|SHIFT,       // :
	0x33,             // ;
	0x36|SHIFT,       // <
	0x2e,             // =
	0x37|SHIFT,       // >
	0x38|SHIFT,       // ?
	0x1f|SHIFT,       // @
	0x04|SHIFT,       // A
	0x05|SHIFT,       // B
	0x06|SHIFT,       // C
	0x07|SHIFT,       // D
	0x08|SHIFT,       // E
	0x09|SHIFT,       // F
	0x0a|SHIFT,       // G
	0x0b|SHIFT,       // H
	0x0c|SHIFT,       // I
	0x0d|SHIFT,       // J
	0x0e|SHIFT,       // K
	0x0f|SHIFT,       // L
	0x10|SHIFT,       // M
	0x11|SHIFT,       // N
	0x12|SHIFT,       // O
	0x13|SHIFT,       // P
	0x14|SHIFT,       // Q
	0x15|SHIFT,       // R
	0x16|SHIFT,       // S
	0x17|SHIFT,       // T
	0x18|SHIFT,       // U
	0x19|SHIFT,       // V
	0x1a|SHIFT,       // W
	0x1b|SHIFT,       // X
	0x1c|SHIFT,       // Y
	0x1d|SHIFT,       // Z
	0x2f,             // [
	0x31,             // bslash
	0x30,             // ]
	0x23|SHIFT,       // ^
	0x2d|SHIFT,       // _
	0x35,             // `
	0x04,             // a
	0x05,             // b
	0x06,             // c
	0x07,             // d
	0x08,             // e
	0x09,             // f
	0x0a,             // g
	0x0b,             // h
	0x0c,             // i
	0x0d,             // j
	0x0e,             // k
	0x0f,             // l
	0x10,             // m
	0x11,             // n
	0x12,             // o
	0x13,             // p
	0x14,             // q
	0x15,             // r
	0x16,             // s
	0x17,             // t
	0x18,             // u
	0x19,             // v
	0x1a,             // w
	0x1b,             // x
	0x1c,             // y
	0x1d,             // z
	0x2f|SHIFT,       //
	0x31|SHIFT,       // |
	0x30|SHIFT,       // }
	0x35|SHIFT,       // ~
	0				  // DEL
};

//
// Constructor
//
USBKeyboard::USBKeyboard(void)
{
}

void USBKeyboard::begin(void)
{
	SPARK_USB_Setup();
}

void USBKeyboard::end(void)
{
	//To Do
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t USBKeyboard::press(uint8_t key)
{
	uint8_t i;
	if (key >= 136)
	{
		// it's a non-printing key (not a modifier)
		key = key - 136;
	}
	else if (key >= 128)
	{
		// it's a modifier key
		keyReport.modifiers |= (1 << (key - 128));
		key = 0;
	}
	else
	{
		// it's a printing key
		key = asciimap[key];
		if (!key)
		{
			setWriteError();
			return 0;
		}
		if (key & 0x80)
		{
			// it's a capital letter or other character reached with shift
			keyReport.modifiers |= 0x02;	// the left shift modifier
			key &= 0x7F;
		}
	}

	// Add key to the keyReport only if it's not already present
	// and if there is an empty slot.
	if (keyReport.keys[0] != key && keyReport.keys[1] != key &&
		keyReport.keys[2] != key && keyReport.keys[3] != key &&
		keyReport.keys[4] != key && keyReport.keys[5] != key)
	{

		for (i=0; i<6; i++)
		{
			if (keyReport.keys[i] == 0x00)
			{
				keyReport.keys[i] = key;
				break;
			}
		}
		if (i == 6)
		{
			setWriteError();
			return 0;
		}
	}

	USB_HID_Send_Report(&keyReport, sizeof(KeyReport));
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t USBKeyboard::release(uint8_t key)
{
	uint8_t i;
	if (key >= 136)
	{
		// it's a non-printing key (not a modifier)
		key = key - 136;
	}
	else if (key >= 128)
	{
		// it's a modifier key
		keyReport.modifiers &= ~(1 << (key - 128));
		key = 0;
	}
	else
	{
		// it's a printing key
		key = asciimap[key];
		if (!key)
		{
			return 0;
		}
		if (key & 0x80)
		{
			// it's a capital letter or other character reached with shift
			keyReport.modifiers &= ~(0x02);	// the left shift modifier
			key &= 0x7F;
		}
	}

	// Test the key report to see if key is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i = 0 ; i < 6 ; i++)
	{
		if (0 != key && keyReport.keys[i] == key)
		{
			keyReport.keys[i] = 0x00;
		}
	}

	USB_HID_Send_Report(&keyReport, sizeof(KeyReport));
	return 1;
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
	USB_HID_Send_Report(&keyReport, sizeof(KeyReport));
}

size_t USBKeyboard::write(uint8_t key)
{
	uint8_t p = press(key);		// Keydown
	delay(100);
	uint8_t r = release(key);	// Keyup
	return (p);					// just return the result of press() since release() almost always returns 1
}

//Preinstantiate Object
USBKeyboard Keyboard;
#endif
