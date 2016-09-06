#!/usr/bin/env python3

# Linux requirements: python3, pyserial, tkinter
# tkinter should usually be already installed
# $ pip3 install tkinter
# $ pip3 install pyserial
#
# OSX requirements: python3, pygame, pyserial
# $ brew install python3 hg sdl sdl_image sdl_mixer sdl_ttf portmidi
# $ pip3 install hg+https://bitbucket.org/pygame/pygame
# $ pip3 install pyserial
#
# Windows requirements: python3, pygame, pyserial
# $ C:\python35\Scripts\pip3.exe install pyserial
# Download pygame whl from http://www.lfd.uci.edu/~gohlke/pythonlibs/#pygame
# $ C:\python35\Scripts\pip3.exe install pygame-1.9.2b1-cp35-cp35m-win32.whl

import sys
import pprint
import platform
import serial
import re
import json

if platform.system() == 'Darwin' or platform.system() == 'Windows':
    import pygame
else:
    if sys.version_info[0] == 2:
        from Tkinter import *
    else:
        from tkinter import *

usb_keymap = {
    # taken from https://chromium.googlesource.com/chromium/chromium/+/master/ui/base/keycodes/usb_keycode_map.h
    # USB      XKB     Win     Mac
    0x0000: (0x0000, 0x0000, 0xffff),  # Reserved
    0x0001: (0x0000, 0x0000, 0xffff),  # ErrorRollOver
    0x0002: (0x0000, 0x0000, 0xffff),  # POSTFail
    0x0003: (0x0000, 0x0000, 0xffff),  # ErrorUndefined
    0x0004: (0x0026, 0x001e, 0x0000),  # aA
    0x0005: (0x0038, 0x0030, 0x000b),  # bB
    0x0006: (0x0036, 0x002e, 0x0008),  # cC
    0x0007: (0x0028, 0x0020, 0x0002),  # dD
    0x0008: (0x001a, 0x0012, 0x000e),  # eE
    0x0009: (0x0029, 0x0021, 0x0003),  # fF
    0x000a: (0x002a, 0x0022, 0x0005),  # gG
    0x000b: (0x002b, 0x0023, 0x0004),  # hH
    0x000c: (0x001f, 0x0017, 0x0022),  # iI
    0x000d: (0x002c, 0x0024, 0x0026),  # jJ
    0x000e: (0x002d, 0x0025, 0x0028),  # kK
    0x000f: (0x002e, 0x0026, 0x0025),  # lL
    0x0010: (0x003a, 0x0032, 0x002e),  # mM
    0x0011: (0x0039, 0x0031, 0x002d),  # nN
    0x0012: (0x0020, 0x0018, 0x001f),  # oO
    0x0013: (0x0021, 0x0019, 0x0023),  # pP
    0x0014: (0x0018, 0x0010, 0x000c),  # qQ
    0x0015: (0x001b, 0x0013, 0x000f),  # rR
    0x0016: (0x0027, 0x001f, 0x0001),  # sS
    0x0017: (0x001c, 0x0014, 0x0011),  # tT
    0x0018: (0x001e, 0x0016, 0x0020),  # uU
    0x0019: (0x0037, 0x002f, 0x0009),  # vV
    0x001a: (0x0019, 0x0011, 0x000d),  # wW
    0x001b: (0x0035, 0x002d, 0x0007),  # xX
    0x001c: (0x001d, 0x0015, 0x0010),  # yY
    0x001d: (0x0034, 0x002c, 0x0006),  # zZ
    0x001e: (0x000a, 0x0002, 0x0012),  # 1!
    0x001f: (0x000b, 0x0003, 0x0013),  # 2@
    0x0020: (0x000c, 0x0004, 0x0014),  # 3#
    0x0021: (0x000d, 0x0005, 0x0015),  # 4$
    0x0022: (0x000e, 0x0006, 0x0017),  # 5%
    0x0023: (0x000f, 0x0007, 0x0016),  # 6^
    0x0024: (0x0010, 0x0008, 0x001a),  # 7&
    0x0025: (0x0011, 0x0009, 0x001c),  # 8*
    0x0026: (0x0012, 0x000a, 0x0019),  # 9(
    0x0027: (0x0013, 0x000b, 0x001d),  # 0)
    0x0028: (0x0024, 0x001c, 0x0024),  # Return
    0x0029: (0x0009, 0x0001, 0x0035),  # Escape
    0x002a: (0x0016, 0x000e, 0x0033),  # Backspace
    0x002b: (0x0017, 0x000f, 0x0030),  # Tab
    0x002c: (0x0041, 0x0039, 0x0031),  # Spacebar
    0x002d: (0x0014, 0x000c, 0x001b),  # -_
    0x002e: (0x0015, 0x000d, 0x0018),  # =+
    0x002f: (0x0022, 0x001a, 0x0021),  # [{
    0x0030: (0x0023, 0x001b, 0x001e),  # }]
    0x0031: (0x0033, 0x002b, 0x002a),  # \| (US keyboard only)
    # USB#070032 is not present on US keyboard.
    # The keycap varies on international keyboards:
    #   Dan: '*  Dutch: <>  Ger: #'  UK: #~
    # For XKB, it uses the same scancode as the US \| key.
    # TODO(garykac): Verify Mac intl keyboard.
    # 0x0032: (0x0033, 0x002b, 0x002a),  # #~ (Non-US)
    0x0033: (0x002f, 0x0027, 0x0029),  # ;:
    0x0034: (0x0030, 0x0028, 0x0027),  # '"
    0x0035: (0x0031, 0x0029, 0x0032),  # `~
    0x0036: (0x003b, 0x0033, 0x002b),  # ,<
    0x0037: (0x003c, 0x0034, 0x002f),  # .>
    0x0038: (0x003d, 0x0035, 0x002c),  # /?
    # TODO(garykac): CapsLock requires special handling for each platform.
    0x0039: (0x0042, 0x003a, 0x0039),  # CapsLock
    0x003a: (0x0043, 0x003b, 0x007a),  # F1
    0x003b: (0x0044, 0x003c, 0x0078),  # F2
    0x003c: (0x0045, 0x003d, 0x0063),  # F3
    0x003d: (0x0046, 0x003e, 0x0076),  # F4
    0x003e: (0x0047, 0x003f, 0x0060),  # F5
    0x003f: (0x0048, 0x0040, 0x0061),  # F6
    0x0040: (0x0049, 0x0041, 0x0062),  # F7
    0x0041: (0x004a, 0x0042, 0x0064),  # F8
    0x0042: (0x004b, 0x0043, 0x0065),  # F9
    0x0043: (0x004c, 0x0044, 0x006d),  # F10
    0x0044: (0x005f, 0x0057, 0x0067),  # F11
    0x0045: (0x0060, 0x0058, 0x006f),  # F12
    0x0046: (0x006b, 0xe037, 0xffff),  # PrintScreen
    0x0047: (0x004e, 0x0046, 0xffff),  # ScrollLock
    0x0048: (0x007f, 0x0000, 0xffff),  # Pause
    # Labeled "Help/Insert" on Mac.
    0x0049: (0x0076, 0xe052, 0x0072),  # Insert
    0x004a: (0x006e, 0xe047, 0x0073),  # Home
    0x004b: (0x0070, 0xe049, 0x0074),  # PageUp
    0x004c: (0x0077, 0x0053, 0x0075),  # Delete (Forward Delete)
    0x004d: (0x0073, 0xe04f, 0x0077),  # End
    0x004e: (0x0075, 0xe051, 0x0079),  # PageDown
    0x004f: (0x0072, 0xe04d, 0x007c),  # RightArrow
    0x0050: (0x0071, 0xe04b, 0x007b),  # LeftArrow
    0x0051: (0x0074, 0xe050, 0x007d),  # DownArrow
    0x0052: (0x006f, 0xe048, 0x007e),  # UpArrow
    0x0053: (0x004d, 0x0045, 0x0047),  # Keypad_NumLock Clear
    0x0054: (0x006a, 0xe035, 0x004b),  # Keypad_/
    0x0055: (0x003f, 0x0037, 0x0043),  # Keypad_*
    0x0056: (0x0052, 0x004a, 0x004e),  # Keypad_-
    0x0057: (0x0056, 0x004e, 0x0045),  # Keypad_+
    0x0058: (0x0068, 0xe01c, 0x004c),  # Keypad_Enter
    0x0059: (0x0057, 0x004f, 0x0053),  # Keypad_1 End
    0x005a: (0x0058, 0x0050, 0x0054),  # Keypad_2 DownArrow
    0x005b: (0x0059, 0x0051, 0x0055),  # Keypad_3 PageDown
    0x005c: (0x0053, 0x004b, 0x0056),  # Keypad_4 LeftArrow
    0x005d: (0x0054, 0x004c, 0x0057),  # Keypad_5
    0x005e: (0x0055, 0x004d, 0x0058),  # Keypad_6 RightArrow
    0x005f: (0x004f, 0x0047, 0x0059),  # Keypad_7 Home
    0x0060: (0x0050, 0x0048, 0x005b),  # Keypad_8 UpArrow
    0x0061: (0x0051, 0x0049, 0x005c),  # Keypad_9 PageUp
    0x0062: (0x005a, 0x0052, 0x0052),  # Keypad_0 Insert
    0x0063: (0x005b, 0x0000, 0x0041),  # Keypad_. Delete
    # USB#070064 is not present on US keyboard.
    # This key is typically located near LeftShift key.
    # The keycap varies on international keyboards:
    #   Dan: <> Dutch: ][ Ger: <> UK: \|
    # TODO(garykac) Determine correct XKB scancode.
    0x0064: (0x0000, 0x0056, 0xffff),  # Non-US \|
    0x0065: (0x0087, 0xe05d, 0xffff),  # AppMenu (next to RWin key)
    0x0066: (0x007c, 0x0000, 0xffff),  # Power
    0x0067: (0x007d, 0x0000, 0x0051),  # Keypad_=
    0x0068: (0x0000, 0x0000, 0x0069),  # F13
    0x0069: (0x0000, 0x0000, 0x006b),  # F14
    0x006a: (0x0000, 0x005d, 0x0071),  # F15
    0x006b: (0x0000, 0x0063, 0x006a),  # F16
    0x006c: (0x0000, 0x0064, 0x0040),  # F17
    0x006d: (0x0000, 0x0065, 0x004f),  # F18
    0x006e: (0x0000, 0x0066, 0x0050),  # F19
    0x006f: (0x0000, 0x0067, 0x005a),  # F20
    0x0070: (0x0000, 0x0068, 0xffff),  # F21
    0x0071: (0x0000, 0x0069, 0xffff),  # F22
    0x0072: (0x0000, 0x006a, 0xffff),  # F23
    0x0073: (0x0000, 0x006b, 0xffff),  # F24
    0x0074: (0x0000, 0x0000, 0xffff),  # Execute
    0x0075: (0x0092, 0xe03b, 0xffff),  # Help
    0x0076: (0x0093, 0x0000, 0xffff),  # Menu
    0x0077: (0x0000, 0x0000, 0xffff),  # Select
    0x0078: (0x0000, 0x0000, 0xffff),  # Stop
    0x0079: (0x0089, 0x0000, 0xffff),  # Again (Redo)
    0x007a: (0x008b, 0xe008, 0xffff),  # Undo
    0x007b: (0x0091, 0xe017, 0xffff),  # Cut
    0x007c: (0x008d, 0xe018, 0xffff),  # Copy
    0x007d: (0x008f, 0xe00a, 0xffff),  # Paste
    0x007e: (0x0090, 0x0000, 0xffff),  # Find
    0x007f: (0x0079, 0xe020, 0x004a),  # Mute
    0x0080: (0x007b, 0xe030, 0x0048),  # VolumeUp
    0x0081: (0x007a, 0xe02e, 0x0049),  # VolumeDown
    0x0082: (0x0000, 0x0000, 0xffff),  # LockingCapsLock
    0x0083: (0x0000, 0x0000, 0xffff),  # LockingNumLock
    0x0084: (0x0000, 0x0000, 0xffff),  # LockingScrollLock
    # USB#070085 is used as Brazilian Keypad_.
    0x0085: (0x0000, 0x0000, 0x005f),  # Keypad_Comma
    # USB#070086 is used on AS/400 keyboards. Standard Keypad_= is USB#070067.
    #0x0086: (0x0000, 0x0000, 0xffff),  # Keypad_=
    # USB#070087 is used for Brazilian /? and Japanese _ 'ro'.
    0x0087: (0x0000, 0x0000, 0x005e),  # International1
    # USB#070088 is used as Japanese Hiragana/Katakana key.
    0x0088: (0x0065, 0x0000, 0x0068),  # International2
    # USB#070089 is used as Japanese Yen key.
    0x0089: (0x0000, 0x007d, 0x005d),  # International3
    # USB#07008a is used as Japanese Henkan (Convert) key.
    0x008a: (0x0064, 0x0000, 0xffff),  # International4
    # USB#07008b is used as Japanese Muhenkan (No-convert) key.
    0x008b: (0x0066, 0x0000, 0xffff),  # International5
    0x008c: (0x0000, 0x0000, 0xffff),  # International6
    0x008d: (0x0000, 0x0000, 0xffff),  # International7
    0x008e: (0x0000, 0x0000, 0xffff),  # International8
    0x008f: (0x0000, 0x0000, 0xffff),  # International9
    # USB#070090 is used as Korean Hangul/English toggle key.
    0x0090: (0x0082, 0x0000, 0xffff),  # LANG1
    # USB#070091 is used as Korean Hanja conversion key.
    0x0091: (0x0083, 0x0000, 0xffff),  # LANG2
    # USB#070092 is used as Japanese Katakana key.
    0x0092: (0x0062, 0x0000, 0xffff),  # LANG3
    # USB#070093 is used as Japanese Hiragana key.
    0x0093: (0x0063, 0x0000, 0xffff),  # LANG4
    # USB#070094 is used as Japanese Zenkaku/Hankaku (Fullwidth/halfwidth) key.
    0x0094: (0x0000, 0x0000, 0xffff),  # LANG5
    0x0095: (0x0000, 0x0000, 0xffff),  # LANG6
    0x0096: (0x0000, 0x0000, 0xffff),  # LANG7
    0x0097: (0x0000, 0x0000, 0xffff),  # LANG8
    0x0098: (0x0000, 0x0000, 0xffff),  # LANG9
    0x0099: (0x0000, 0x0000, 0xffff),  # AlternateErase
    0x009a: (0x0000, 0x0000, 0xffff),  # SysReq/Attention
    0x009b: (0x0088, 0x0000, 0xffff),  # Cancel
    0x009c: (0x0000, 0x0000, 0xffff),  # Clear
    0x009d: (0x0000, 0x0000, 0xffff),  # Prior
    0x009e: (0x0000, 0x0000, 0xffff),  # Return
    0x009f: (0x0000, 0x0000, 0xffff),  # Separator
    0x00a0: (0x0000, 0x0000, 0xffff),  # Out
    0x00a1: (0x0000, 0x0000, 0xffff),  # Oper
    0x00a2: (0x0000, 0x0000, 0xffff),  # Clear/Again
    0x00a3: (0x0000, 0x0000, 0xffff),  # CrSel/Props
    0x00a4: (0x0000, 0x0000, 0xffff),  # ExSel
    #0x00b0: (0x0000, 0x0000, 0xffff),  # Keypad_00
    #0x00b1: (0x0000, 0x0000, 0xffff),  # Keypad_000
    #0x00b2: (0x0000, 0x0000, 0xffff),  # ThousandsSeparator
    #0x00b3: (0x0000, 0x0000, 0xffff),  # DecimalSeparator
    #0x00b4: (0x0000, 0x0000, 0xffff),  # CurrencyUnit
    #0x00b5: (0x0000, 0x0000, 0xffff),  # CurrencySubunit
    0x00b6: (0x00bb, 0x0000, 0xffff),  # Keypad_(
    0x00b7: (0x00bc, 0x0000, 0xffff),  # Keypad_)
    #0x00b8: (0x0000, 0x0000, 0xffff),  # Keypad_{
    #0x00b9: (0x0000, 0x0000, 0xffff),  # Keypad_}
    #0x00ba: (0x0000, 0x0000, 0xffff),  # Keypad_Tab
    #0x00bb: (0x0000, 0x0000, 0xffff),  # Keypad_Backspace
    #0x00bc: (0x0000, 0x0000, 0xffff),  # Keypad_A
    #0x00bd: (0x0000, 0x0000, 0xffff),  # Keypad_B
    #0x00be: (0x0000, 0x0000, 0xffff),  # Keypad_C
    #0x00bf: (0x0000, 0x0000, 0xffff),  # Keypad_D
    #0x00c0: (0x0000, 0x0000, 0xffff),  # Keypad_E
    #0x00c1: (0x0000, 0x0000, 0xffff),  # Keypad_F
    #0x00c2: (0x0000, 0x0000, 0xffff),  # Keypad_Xor
    #0x00c3: (0x0000, 0x0000, 0xffff),  # Keypad_^
    #0x00c4: (0x0000, 0x0000, 0xffff),  # Keypad_%
    #0x00c5: (0x0000, 0x0000, 0xffff),  # Keypad_<
    #0x00c6: (0x0000, 0x0000, 0xffff),  # Keypad_>
    #0x00c7: (0x0000, 0x0000, 0xffff),  # Keypad_&
    #0x00c8: (0x0000, 0x0000, 0xffff),  # Keypad_&&
    #0x00c9: (0x0000, 0x0000, 0xffff),  # Keypad_|
    #0x00ca: (0x0000, 0x0000, 0xffff),  # Keypad_||
    #0x00cb: (0x0000, 0x0000, 0xffff),  # Keypad_:
    #0x00cc: (0x0000, 0x0000, 0xffff),  # Keypad_#
    #0x00cd: (0x0000, 0x0000, 0xffff),  # Keypad_Space
    #0x00ce: (0x0000, 0x0000, 0xffff),  # Keypad_@
    #0x00cf: (0x0000, 0x0000, 0xffff),  # Keypad_!
    #0x00d0: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryStore
    #0x00d1: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryRecall
    #0x00d2: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryClear
    #0x00d3: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryAdd
    #0x00d4: (0x0000, 0x0000, 0xffff),  # Keypad_MemorySubtract
    #0x00d5: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryMultiply
    #0x00d6: (0x0000, 0x0000, 0xffff),  # Keypad_MemoryDivide
    0x00d7: (0x007e, 0x0000, 0xffff),  # Keypad_+/-
    #0x00d8: (0x0000, 0x0000, 0xffff),  # Keypad_Clear
    #0x00d9: (0x0000, 0x0000, 0xffff),  # Keypad_ClearEntry
    #0x00da: (0x0000, 0x0000, 0xffff),  # Keypad_Binary
    #0x00db: (0x0000, 0x0000, 0xffff),  # Keypad_Octal
    0x00dc: (0x0081, 0x0000, 0xffff),  # Keypad_Decimal
    #0x00dd: (0x0000, 0x0000, 0xffff),  # Keypad_Hexadecimal
    # USB#0700de - #0700df are reserved.
    0x00e0: (0x0025, 0x001d, 0x003b),  # LeftControl
    0x00e1: (0x0032, 0x002a, 0x0038),  # LeftShift
    0x00e2: (0x0040, 0x0038, 0x003a),  # LeftAlt/Option
    0x00e3: (0x0085, 0x005b, 0x0037),  # LeftGUI/Super/Win/Cmd
    0x00e4: (0x0069, 0xe01d, 0x003e),  # RightControl
    0x00e5: (0x003e, 0x0036, 0x003c),  # RightShift
    0x00e6: (0x006c, 0xe038, 0x003d),  # RightAlt/Option
    0x00e7: (0x0086, 0x005c, 0x0036),  # RightGUI/Super/Win/Cmd
}

usb_keymap_platform = {
    'Linux': dict([(v[0], k) for (k, v) in usb_keymap.items()]),
    'Windows': dict([(v[1], k) for (k, v) in usb_keymap.items()]),
    'Darwin': dict([(v[2], k) for (k, v) in usb_keymap.items()]),
}

def usb_hid_to_keycode(keycode):
    i = 0
    if platform.system() == 'Linux':
        i = 0
    elif platform.system() == 'Windows':
        i = 1
    else:
        i = 2
    try:
        return usb_keymap[keycode][i]
    except:
        return 0

def keycode_to_usb_hid(keycode):
    try:
        return usb_keymap_platform[platform.system()][keycode]
    except:
        return 0

class SerialConnection:
    def __init__(self, port):
        self.s = None
        if port is not None:
            self.s = serial.Serial(port, timeout=0, writeTimeout=0.5)
        self.state = False

    def start(self):
        if self.s is not None:
            self.s.writelines(['t'.encode('utf-8')])

    def report_event(self, ev):
        #print('> {} {} {} {} {} {}'.format(*ev).encode('utf-8'))
        if not self.state:
            return
        # type
        # posx
        # posy
        # value1
        # value2
        # value3
        try:
            self.s.writelines(['{} {} {} {} {} {}\n'.format(*ev).encode('utf-8')])
        except:
            pass

    def report_mouse_position(self, pos, res):
        ev = ('m', pos[0], pos[1], res[0], res[1], 0)
        self.report_event(ev)

    def report_mouse_click(self, pos, button):
        ev = ('c', pos[0], pos[1], button, 0, 0)
        self.report_event(ev)

    def report_mouse_wheel(self, value):
        ev = ('w', 0, 0, value)
        self.report_event(ev)

    def report_key(self, code, mod, char):
        if char == '':
            char = 256
        else:
            char = ord(char)
        ev = ('k', 0, 0, code, mod, char)
        self.report_event(ev)

    def handle(self):
        l = b''
        if self.s is not None:
            l = self.s.readline()
        if l == b'':
            return None
        l = l.strip()
        print(l.decode('utf-8'))
        if l == b'Running tests':
            self.state = True
        elif l.startswith(b'Test summary:'):
            (p, f, s, t) = re.search(b'^Test summary: (\d+) passed, (\d+) failed, and (\d+) skipped, out of (\d+) test\(s\).$', l).groups()
            self.state = False
            status = True
            text = l
            if (int(p) + int(s)) == int(t):
                status = True
            else:
                status = False
            return (status, l)

class MainWindowBase:
    def __init__(self, ser):
        self.s = ser
        self.exit = 0

    def handle(self):
        pass

    def finish(self, status, text):
        pass

    def draw_text(self, text):
        pass

if platform.system() == 'Darwin' or platform.system() == 'Windows':
    class MainWindowPyGame(MainWindowBase):
        modifier_map = {
            pygame.KMOD_NONE:   0xe000,
            pygame.KMOD_LSHIFT: 0xe002,
            pygame.KMOD_RSHIFT: 0xe020,
            pygame.KMOD_SHIFT:  0xe002,
            pygame.KMOD_LCTRL:  0xe001,
            pygame.KMOD_RCTRL:  0xe010,
            pygame.KMOD_CTRL:   0xe001,
            pygame.KMOD_LALT:   0xe004,
            pygame.KMOD_RALT:   0xe040,
            pygame.KMOD_ALT:    0xe004,
            pygame.KMOD_LMETA:  0xe008,
            pygame.KMOD_RMETA:  0xe080,
            pygame.KMOD_META:   0xe008,
            pygame.KMOD_NUM:    0x0000,
            pygame.KMOD_CAPS:   0x0000,
            pygame.KMOD_MODE:   0xe000
        }

        patch_scancodes = {
            pygame.K_LCTRL:  usb_hid_to_keycode(0x00e0),
            pygame.K_LSHIFT: usb_hid_to_keycode(0x00e1),
            pygame.K_LALT:   usb_hid_to_keycode(0x00e2),
            pygame.K_LMETA:  usb_hid_to_keycode(0x00e3),
            pygame.K_RCTRL:  usb_hid_to_keycode(0x00e4),
            pygame.K_RSHIFT: usb_hid_to_keycode(0x00e5),
            pygame.K_RALT:   usb_hid_to_keycode(0x00e6),
            pygame.K_RMETA:  usb_hid_to_keycode(0x00e7)
        }

        def modifier_normalize(self, val):
            out = 0x0000
            for (k, v) in self.modifier_map.items():
                if val & k:
                    out |= v
            return out

        def __init__(self, ser):
            super().__init__(ser)
            pygame.init()
            self.s = ser
            self.info = pygame.display.Info()
            self.screen = pygame.display.set_mode((self.info.current_w, self.info.current_h), pygame.FULLSCREEN)
            self.mouse = pygame.mouse.get_pos()
            self.textpos = (5, 0)
            self.screen.fill((255, 255, 255))

            self.font = pygame.font.SysFont("monospace", 8)
            self.font1 = pygame.font.SysFont("monospace", 24)
            self.handle()

            self.exit_info()

        def handle(self):
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    sys.exit()
                elif event.type == pygame.KEYDOWN:
                    self.on_key(event)
                elif event.type == pygame.MOUSEMOTION:
                    self.on_mouse_motion(event)
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    self.on_mouse_click(event)
            pygame.display.flip()

        def draw_text(self, text):
            label = self.font.render(text, 1, (0x20, 0x20, 0x20))
            self.screen.blit(label, self.textpos)
            self.textpos = (self.textpos[0], self.textpos[1] + 10)
            if (self.textpos[1] + 10) >= self.info.current_h:
                self.textpos = (self.textpos[0] + 150, 0)

        def on_key(self, event):
            chhex = ord(event.unicode) if len(event.unicode) > 0 else 0
            # Normalize modifiers
            mod = self.modifier_normalize(event.mod)
            if event.key in self.patch_scancodes:
                event.scancode = self.patch_scancodes[event.key]
            self.draw_text('Key {}/{:03x} {} ({:02x})'.format(keycode_to_usb_hid(event.scancode), mod & 0x0fff, json.dumps(event.unicode), chhex))
            if keycode_to_usb_hid(event.scancode) == 0x0029:
                self.exit += 1
            else:
                self.exit = 0

            #pprint.pprint(event)
            self.s.report_key(keycode_to_usb_hid(event.scancode), mod, event.unicode)

        def on_mouse_motion(self, event):
            pygame.draw.line(self.screen, (0,0,0), self.mouse, event.pos)
            self.mouse = event.pos
            #print('========================= Motion ========================')
            #pprint.pprint(event)
            self.s.report_mouse_position(self.mouse, (self.info.current_w, self.info.current_h))
        def on_mouse_click(self, event):
            # print('========================= Click ========================')
            if event.button > 3:
                self.on_mouse_wheel(event)
                return
            fill=None
            if event.button == 1:
                fill = 'red'
            elif event.button == 2:
                fill = 'green'
            elif event.button == 3:
                fill = 'blue'

            pygame.draw.circle(self.screen, pygame.Color(fill),
                               event.pos, 10)
            # pprint.pprint(vars(event))
            # self.ok()
            self.s.report_mouse_click(event.pos, event.button)
        def on_mouse_wheel(self, event):
            #print('========================= Wheel ========================')
            #pprint.pprint(vars(event))
            val = 0
            if event.button == 4:
                val = -1
            elif event.button == 5:
                val = 1
            if val != 0:
                self.s.report_mouse_wheel(val)

        def exit_info(self):
            fill = 'blue'
            label = self.font1.render('Press ESC 10 times to exit', 1, pygame.Color(fill))
            pos = label.get_rect()
            pos.centerx = self.screen.get_rect().centerx
            pos.centery = 100
            self.screen.blit(label, pos)

        def finish(self, status, text):
            fill = 'green' if status == True else 'red'
            label = self.font1.render(text, 1, pygame.Color(fill))
            pos = label.get_rect()
            pos.centerx = self.screen.get_rect().centerx
            pos.centery = self.screen.get_rect().centery
            self.screen.blit(label, pos)
    MainWindow = MainWindowPyGame
else:
    class MainWindowTkinter(MainWindowBase):
        modifier_map = {
            0x0001:  0xe002,
            0x0004:  0xe001,
            0x0008:  0xe004,
            0x0080:  0xe040,
            0x20000: 0xe004
        }

        def modifier_normalize(self, val):
            out = 0x0000
            for (k, v) in self.modifier_map.items():
                if val & k:
                    out |= v
            return out

        def __init__(self, ser):
            super().__init__(ser)
            self.tk = Tk()
            self.tk.config(menu=None)
            self.tk.overrideredirect(False)
            try:
                self.tk.attributes("-toolwindow", 1)
            except:
                pass
            try:
                self.tk.attributes('-zoomed', True)
            except:
                pass
            self.tk.attributes('-fullscreen', True)
            self.tk.wm_attributes('-topmost', True)
            self.tk.lift()

            self.canvas = Canvas(self.tk, bg='white', width=self.tk.winfo_screenwidth(), height=self.tk.winfo_screenheight())
            self.canvas.pack()
            self.tk.bind('<Motion>', self.on_mouse_motion)
            self.tk.bind('<Key>', self.on_key)
            self.tk.bind('<Button-1>', self.on_mouse_click)
            self.tk.bind('<Button-2>', self.on_mouse_click)
            self.tk.bind('<Button-3>', self.on_mouse_click)
            if platform.system() == 'Linux':
                self.tk.bind('<Button-4>', self.on_mouse_wheel)
                self.tk.bind('<Button-5>', self.on_mouse_wheel)
            else:
                self.tk.bind('<MouseWheel>', self.on_mouse_wheel)

            self.mouse = (self.tk.winfo_pointerx(), self.tk.winfo_pointery())
            self.textpos = (5, 0)
            self.exit = 0

            self.exit_info()

        def draw_text(self, text):
            self.canvas.create_text(self.textpos[0], self.textpos[1], text=text, font=('Monospace', 8), anchor=NW, fill='gray')
            self.textpos = (self.textpos[0], self.textpos[1] + 10)
            if (self.textpos[1] + 10) >= self.tk.winfo_screenheight():
                self.textpos = (self.textpos[0] + 150, 0)

        def on_key(self, event):
            #print('========================= Key ========================')
            chhex = ord(event.char) if len(event.char) > 0 else 0
            mod = self.modifier_normalize(event.state)
            self.draw_text('Key {}/{:03x} {} ({:02x})'.format(keycode_to_usb_hid(event.keycode), mod & 0x0fff, json.dumps(event.char), chhex))
            if keycode_to_usb_hid(event.keycode) == 0x0029:
                self.exit += 1
            else:
                self.exit = 0

            #pprint.pprint(vars(event))
            self.s.report_key(keycode_to_usb_hid(event.keycode), mod, event.char)
        def on_mouse_motion(self, event):
            self.canvas.create_line(self.mouse[0], self.mouse[1], event.x, event.y)
            self.mouse = (event.x, event.y)
            #print('========================= Motion ========================')
            #pprint.pprint(vars(event))
            self.s.report_mouse_position(self.mouse, (self.tk.winfo_screenwidth(), self.tk.winfo_screenheight()))
        def on_mouse_click(self, event):
            # print('========================= Click ========================')
            fill=None
            if event.num == 1:
                fill = 'red'
            elif event.num == 2:
                fill = 'green'
            elif event.num == 3:
                fill = 'blue'
            self.canvas.create_oval(event.x - 10,
                                    event.y - 10,
                                    event.x + 10,
                                    event.y + 10, fill=fill)
            # pprint.pprint(vars(event))
            # self.ok()
            # self.fail()
            self.s.report_mouse_click((event.x, event.y), event.num)
        def on_mouse_wheel(self, event):
            #print('========================= Wheel ========================')
            #pprint.pprint(vars(event))
            val = 0
            if platform.system() == 'Linux':
                if event.num == 4:
                    val = -1
                elif event.num == 5:
                    val = 1
            else:
                val = event.delta

            if val != 0:
                self.s.report_mouse_wheel(val)

        def exit_info(self):
            fill = 'blue'
            self.canvas.create_text(self.tk.winfo_screenwidth() / 2, 100, text='Press ESC 10 times to exit', font=('Monospace', 24), fill=fill)

        def finish(self, status, text):
            fill = 'green' if status == True else 'red'
            self.canvas.create_text(self.tk.winfo_screenwidth() / 2, self.tk.winfo_screenheight() / 2, text=text, font=('Monospace', 24), fill=fill)

        def handle(self):
            self.tk.update_idletasks()
            self.tk.update()

    MainWindow = MainWindowTkinter

def usage():
    print('Usage: {} <tty>'.format(sys.argv[0]))
    sys.exit(0)

if __name__ == '__main__':
    sport = None
    if len(sys.argv) >= 2:
        sport = sys.argv[1]
    else:
        usage()
    s = SerialConnection(sport)
    w = MainWindow(s)
    s.start()
    while True:
        r = s.handle()
        if r is not None:
            w.finish(r[0], r[1])
        # w.tk.update_idletasks()
        # w.tk.update()
        w.handle()
        if w.exit >= 10:
            break
