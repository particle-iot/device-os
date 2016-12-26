#ifndef __SPARK_WIRING_USBKEYBOARDSCANCODE_H
#define __SPARK_WIRING_USBKEYBOARDSCANCODE_H

// http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
typedef enum {
  // input.h compatible key names
  KEY_RESERVED           = 0x00,     // Reserved (no event indicated)
  KEY_A                  = 0x04,     // Keyboard a and A
  KEY_B                  = 0x05,     // Keyboard b and B
  KEY_C                  = 0x06,     // Keyboard c and C
  KEY_D                  = 0x07,     // Keyboard d and D
  KEY_E                  = 0x08,     // Keyboard e and E
  KEY_F                  = 0x09,     // Keyboard f and F
  KEY_G                  = 0x0A,     // Keyboard g and G
  KEY_H                  = 0x0B,     // Keyboard h and H
  KEY_I                  = 0x0C,     // Keyboard i and I
  KEY_J                  = 0x0D,     // Keyboard j and J
  KEY_K                  = 0x0E,     // Keyboard k and K
  KEY_L                  = 0x0F,     // Keyboard l and L
  KEY_M                  = 0x10,     // Keyboard m and M
  KEY_N                  = 0x11,     // Keyboard n and N
  KEY_O                  = 0x12,     // Keyboard o and O
  KEY_P                  = 0x13,     // Keyboard p and P
  KEY_Q                  = 0x14,     // Keyboard q and Q
  KEY_R                  = 0x15,     // Keyboard r and R
  KEY_S                  = 0x16,     // Keyboard s and S
  KEY_T                  = 0x17,     // Keyboard t and T
  KEY_U                  = 0x18,     // Keyboard u and U
  KEY_V                  = 0x19,     // Keyboard v and V
  KEY_W                  = 0x1A,     // Keyboard w and W
  KEY_X                  = 0x1B,     // Keyboard x and X
  KEY_Y                  = 0x1C,     // Keyboard y and Y
  KEY_Z                  = 0x1D,     // Keyboard z and Z
  KEY_1                  = 0x1E,     // Keyboard 1 and !
  KEY_2                  = 0x1F,     // Keyboard 2 and @
  KEY_3                  = 0x20,     // Keyboard 3 and #
  KEY_4                  = 0x21,     // Keyboard 4 and $
  KEY_5                  = 0x22,     // Keyboard 5 and %
  KEY_6                  = 0x23,     // Keyboard 6 and ^
  KEY_7                  = 0x24,     // Keyboard 7 and &
  KEY_8                  = 0x25,     // Keyboard 8 and *
  KEY_9                  = 0x26,     // Keyboard 9 and (
  KEY_0                  = 0x27,     // Keyboard 0 and )
  KEY_ENTER              = 0x28,     // Keyboard Return (ENTER)
  KEY_ESC                = 0x29,     // Keyboard ESCAPE
  KEY_BACKSPACE          = 0x2A,     // Keyboard DELETE (Backspace)
  KEY_TAB                = 0x2B,     // Keyboard Tab
  KEY_SPACE              = 0x2C,     // Keyboard Spacebar
  KEY_MINUS              = 0x2D,     // Keyboard - and (underscore)
  KEY_EQUAL              = 0x2E,     // Keyboard = and +
  KEY_LEFTBRACE          = 0x2F,     // Keyboard [ and {
  KEY_RIGHTBRACE         = 0x30,     // Keyboard ] and }
  KEY_BACKSLASH          = 0x31,     // Keyboard \ and |
  KEY_HASH               = 0x32,     // Keyboard Non-US # and ~
  KEY_SEMICOLON          = 0x33,     // Keyboard ; and :
  KEY_APOSTROPHE         = 0x34,     // Keyboard ‘ and “
  KEY_GRAVE              = 0x35,     // Keyboard Grave Accent and Tilde
  KEY_COMMA              = 0x36,     // Keyboard , and <
  KEY_DOT                = 0x37,     // Keyboard . and >
  KEY_SLASH              = 0x38,     // Keyboard / and ?
  KEY_CAPSLOCK           = 0x39,     // Keyboard Caps Lock
  KEY_F1                 = 0x3A,     // Keyboard F1
  KEY_F2                 = 0x3B,     // Keyboard F2
  KEY_F3                 = 0x3C,     // Keyboard F3
  KEY_F4                 = 0x3D,     // Keyboard F4
  KEY_F5                 = 0x3E,     // Keyboard F5
  KEY_F6                 = 0x3F,     // Keyboard F6
  KEY_F7                 = 0x40,     // Keyboard F7
  KEY_F8                 = 0x41,     // Keyboard F8
  KEY_F9                 = 0x42,     // Keyboard F9
  KEY_F10                = 0x43,     // Keyboard F10
  KEY_F11                = 0x44,     // Keyboard F11
  KEY_F12                = 0x45,     // Keyboard F12
  KEY_PRINT              = 0x46,     // Keyboard PrintScreen
  KEY_SCROLLLOCK         = 0x47,     // Keyboard Scroll Lock
  KEY_PAUSE              = 0x48,     // Keyboard Pause
  KEY_INSERT             = 0x49,     // Keyboard Insert
  KEY_HOME               = 0x4A,     // Keyboard Home
  KEY_PAGEUP             = 0x4B,     // Keyboard PageUp
  KEY_DELETE             = 0x4C,     // Keyboard Delete Forward
  KEY_END                = 0x4D,     // Keyboard End
  KEY_PAGEDOWN           = 0x4E,     // Keyboard PageDown
  KEY_RIGHT              = 0x4F,     // Keyboard RightArrow
  KEY_LEFT               = 0x50,     // Keyboard LeftArrow
  KEY_DOWN               = 0x51,     // Keyboard DownArrow
  KEY_UP                 = 0x52,     // Keyboard UpArrow
  KEY_NUMLOCK            = 0x53,     // Keypad Num Lock and Clear
  KEY_KPSLASH            = 0x54,     // Keypad /
  KEY_KPASTERISK         = 0x55,     // Keypad *
  KEY_KPMINUS            = 0x56,     // Keypad -
  KEY_KPPLUS             = 0x57,     // Keypad +
  KEY_KPENTER            = 0x58,     // Keypad ENTER
  KEY_KP1                = 0x59,     // Keypad 1 and End
  KEY_KP2                = 0x5A,     // Keypad 2 and Down Arrow
  KEY_KP3                = 0x5B,     // Keypad 3 and PageDn
  KEY_KP4                = 0x5C,     // Keypad 4 and Left Arrow
  KEY_KP5                = 0x5D,     // Keypad 5
  KEY_KP6                = 0x5E,     // Keypad 6 and Right Arrow
  KEY_KP7                = 0x5F,     // Keypad 7 and Home
  KEY_KP8                = 0x60,     // Keypad 8 and Up Arrow
  KEY_KP9                = 0x61,     // Keypad 9 and PageUp
  KEY_KP0                = 0x62,     // Keypad 0 and Insert
  KEY_KPDOT              = 0x63,     // Keypad . and Delete
  KEY_102ND              = 0x64,     // Keyboard Non-US \ and |
  KEY_COMPOSE            = 0x65,     // Keyboard Application
                                     // LOGICAL_MAXIMUM
  KEY_POWER              = 0x66,     // Keyboard Power
  KEY_KPEQUAL            = 0x67,     // Keypad =
  KEY_F13                = 0x68,     // Keyboard F13
  KEY_F14                = 0x69,     // Keyboard F14
  KEY_F15                = 0x6A,     // Keyboard F15
  KEY_F16                = 0x6B,     // Keyboard F16
  KEY_F17                = 0x6C,     // Keyboard F17
  KEY_F18                = 0x6D,     // Keyboard F18
  KEY_F19                = 0x6E,     // Keyboard F19
  KEY_F20                = 0x6F,     // Keyboard F20
  KEY_F21                = 0x70,     // Keyboard F21
  KEY_F22                = 0x71,     // Keyboard F22
  KEY_F23                = 0x72,     // Keyboard F23
  KEY_F24                = 0x73,     // Keyboard F24
  KEY_OPEN               = 0x74,     // Keyboard Execute
  KEY_HELP               = 0x75,     // Keyboard Help
  KEY_MENU               = 0x76,     // Keyboard Menu
  KEY_SELECT             = 0x77,     // Keyboard Select
  KEY_STOP               = 0x78,     // Keyboard Stop
  KEY_AGAIN              = 0x79,     // Keyboard Again
  KEY_UNDO               = 0x7A,     // Keyboard Undo
  KEY_CUT                = 0x7B,     // Keyboard Cut
  KEY_COPY               = 0x7C,     // Keyboard Copy
  KEY_PASTE              = 0x7D,     // Keyboard Paste
  KEY_FIND               = 0x7E,     // Keyboard Find
  KEY_MUTE               = 0x7F,     // Keyboard Mute
  KEY_VOLUMEUP           = 0x80,     // Keyboard Volume Up
  KEY_VOLUMEDOWN         = 0x81,     // Keyboard Volume Down
  KEY_LOCKCAPSLOCK       = 0x82,     // Keyboard Locking Caps Lock
  KEY_LOCKNUMLOCK        = 0x83,     // Keyboard Locking Num Lock
  KEY_LOCKSCROLLOCK      = 0x84,     // Keyboard Locking Scroll Lock
  KEY_KPCOMMA            = 0x85,     // Keypad Comma
  KEY_KPEQUALSIGN        = 0x86,     // Keypad Equal Sign
  KEY_RO                 = 0x87,     // Keyboard International1
  KEY_KATAKANAHIRAGANA   = 0x88,     // Keyboard International2
  KEY_YEN                = 0x89,     // Keyboard International3
  KEY_HENKAN             = 0x8A,     // Keyboard International4
  KEY_MUHENKAN           = 0x8B,     // Keyboard International5
  KEY_KPJPCOMMA          = 0x8C,     // Keyboard International6
  KEY_INTL7              = 0x8D,     // Keyboard International7
  KEY_INTL8              = 0x8E,     // Keyboard International8
  KEY_INTL9              = 0x8F,     // Keyboard International9
  KEY_HANGEUL            = 0x90,     // Keyboard LANG1
  KEY_HANJA              = 0x91,     // Keyboard LANG2
  KEY_KATAKANA           = 0x92,     // Keyboard LANG3
  KEY_HIRAGANA           = 0x93,     // Keyboard LANG4
  KEY_ZENKAKUHENKAKU     = 0x94,     // Keyboard LANG5
  KEY_LANG6              = 0x95,     // Keyboard LANG6
  KEY_LANG7              = 0x96,     // Keyboard LANG7
  KEY_LANG8              = 0x97,     // Keyboard LANG8
  KEY_LANG9              = 0x98,     // Keyboard LANG9
  KEY_ALTERASE           = 0x99,     // Keyboard Alternate Erase
  KEY_SYSRQ              = 0x9A,     // Keyboard SysReq/Attention
  KEY_CANCEL             = 0x9B,     // Keyboard Cancel
  KEY_CLEAR              = 0x9C,     // Keyboard Clear
  KEY_PRIOR              = 0x9D,     // Keyboard Prior
  KEY_ENTER2             = 0x9E,     // Keyboard Return
  KEY_SEPARATOR          = 0x9F,     // Keyboard Separator
  KEY_OUT                = 0xA0,     // Keyboard Out
  KEY_OPER               = 0xA1,     // Keyboard Oper
  KEY_CLRAGAIN           = 0xA2,     // Keyboard Clear/Again
  KEY_CRSEL              = 0xA3,     // Keyboard CrSel/Props
  KEY_EXSEL              = 0xA4,     // Keyboard ExSel
                                     // Keys 0xA5 to 0xAF reserved
  KEY_KP00               = 0xB0,     // Keypad 00
  KEY_KP000              = 0xB1,     // Keypad 000
  KEY_THOUSANDSEP        = 0xB2,     // Thousands Separator
  KEY_DECIMALSEP         = 0xB3,     // Decimal Separator
  KEY_CURRENCY           = 0xB4,     // Currency Unit
  KEY_CURRENCYSUB        = 0xB5,     // Currency Sub-unit
  KEY_KPLEFTPAREN        = 0xB6,     // Keypad (
  KEY_KPRIGHTPAREN       = 0xB7,     // Keypad )
  KEY_KPLEFTBRACE        = 0xB8,     // Keypad {
  KEY_KPRIGHTBRACE       = 0xB9,     // Keypad }
  KEY_KPTAB              = 0xBA,     // Keypad Tab
  KEY_KPBACKSPACE        = 0xBB,     // Keypad Backspace
  KEY_KPA                = 0xBC,     // Keypad A
  KEY_KPB                = 0xBD,     // Keypad B
  KEY_KPC                = 0xBE,     // Keypad C
  KEY_KPD                = 0xBF,     // Keypad D
  KEY_KPE                = 0xC0,     // Keypad E
  KEY_KPF                = 0xC1,     // Keypad F
  KEY_KPXOR              = 0xC2,     // Keypad XOR
  KEY_KPPOWER            = 0xC3,     // Keypad ^
  KEY_KPPERCENT          = 0xC4,     // Keypad %
  KEY_KPLT               = 0xC5,     // Keypad <
  KEY_KPGT               = 0xC6,     // Keypad >
  KEY_KPAMP              = 0xC7,     // Keypad &
  KEY_KPAMPAMP           = 0xC8,     // Keypad &&
  KEY_KPBAR              = 0xC9,     // Keypad |
  KEY_KPBARBAR           = 0xCA,     // Keypad ||
  KEY_KPCOLON            = 0xCB,     // Keypad :
  KEY_KPHASH             = 0xCC,     // Keypad #
  KEY_KPSPACE            = 0xCD,     // Keypad Space
  KEY_KPAT               = 0xCE,     // Keypad @
  KEY_KPEXCLAM           = 0xCF,     // Keypad !
  KEY_KPMEMSTORE         = 0xD0,     // Keypad Memory Store
  KEY_KPMEMRECALL        = 0xD1,     // Keypad Memory Recall
  KEY_KPMEMCLEAR         = 0xD2,     // Keypad Memory Clear
  KEY_KPMEMADD           = 0xD3,     // Keypad Memory Add
  KEY_KPMEMSUB           = 0xD4,     // Keypad Memory Subtract
  KEY_KPMEMMULT          = 0xD5,     // Keypad Memory Multiply
  KEY_KPMEMDIV           = 0xD6,     // Keypad Memory Divide
  KEY_KPPLUSMINUS        = 0xD7,     // Keypad +/-
  KEY_KPCLEAR            = 0xD8,     // Keypad Clear
  KEY_KPCLEARENT         = 0xD9,     // Keypad Clear Entry
  KEY_KPBINARY           = 0xDA,     // Keypad Binary
  KEY_KPOCTAL            = 0xDB,     // Keypad Octal
  KEY_KPDECIMAL          = 0xDC,     // Keypad Decimal
  KEY_KPHEX              = 0xDD,     // Keypad Hexadecimal
                                     // Keys 0xDE to 0xDF reserved
                                     // These are sent as modifiers
  KEY_LEFTCTRL           = 0xE0,     // Keyboard LeftControl
  KEY_LEFTSHIFT          = 0xE1,     // Keyboard LeftShift
  KEY_LEFTALT            = 0xE2,     // Keyboard LeftAlt
  KEY_LEFTGUI            = 0xE3,     // Keyboard Left GUI
  KEY_RIGHTCTRL          = 0xE4,     // Keyboard RightControl
  KEY_RIGHTSHIFT         = 0xE5,     // Keyboard RightShift
  KEY_RIGHTALT           = 0xE6,     // Keyboard RightAlt
  KEY_RIGHTGUI           = 0xE7,     // Keyboard Right GUI
                                     // End of modifiers
  
  // Platform-specific names
  KEY_LEFT_WINDOWS       = 0xE3,     // Left Windows Key (⊞)
  KEY_RIGHT_WINDOWS      = 0xE7,     // Right Windows Key (⊞)
  KEY_LEFT_COMMAND       = 0xE3,     // Left Mac Command Key (⌘)
  KEY_RIGHT_COMMAND      = 0xE7,     // Right Mac Command Key (⌘)
  KEY_LEFT_META          = 0xE3,     // Left *nix Meta Key (◆)
  KEY_RIGHT_META         = 0xE7,     // Right *nix Meta Key (◆)
  
  
  // Additional alternative names
  KEY_INTL1              = 0x87,     // Keyboard International1
  KEY_INTL2              = 0x88,     // Keyboard International2
  KEY_INTL3              = 0x89,     // Keyboard International3
  KEY_INTL4              = 0x8A,     // Keyboard International4
  KEY_INTL5              = 0x8B,     // Keyboard International5
  KEY_INTL6              = 0x8C,     // Keyboard International6
  KEY_LANG1              = 0x90,     // Keyboard LANG1
  KEY_LANG2              = 0x91,     // Keyboard LANG2
  KEY_LANG3              = 0x92,     // Keyboard LANG3
  KEY_LANG4              = 0x93,     // Keyboard LANG4
  KEY_LANG5              = 0x94,     // Keyboard LANG5


  // SDL2-like key names
  KEY_UNKNOWN            = 0x00,     // Reserved (no event indicated)
  KEY_ERROR_ROLLOVER     = 0x01,     // Keyboard ErrorRollOver
  KEY_POSTFAIL           = 0x02,     // Keyboard POSTFail
  KEY_ERROR_UNDEFINED    = 0x03,     // Keyboard ErrorUndefined

  KEY_RETURN             = 0x28,     // Keyboard Return (ENTER)
  KEY_ESCAPE             = 0x29,     // Keyboard ESCAPE

  KEY_EQUALS             = 0x2E,     // Keyboard = and +
  KEY_LEFTBRACKET        = 0x2F,     // Keyboard [ and {
  KEY_RIGHTBRACKET       = 0x30,     // Keyboard ] and }

  KEY_NONUSHASH          = 0x32,     // Keyboard Non-US # and ~

  KEY_PERIOD             = 0x37,     // Keyboard . and >

  KEY_PRINTSCREEN        = 0x46,     // Keyboard PrintScreen

  KEY_NUMLOCKCLEAR       = 0x53,     // Keypad Num Lock and Clear
  KEY_KP_DIVIDE          = 0x54,     // Keypad /
  KEY_KP_MULTIPLY        = 0x55,     // Keypad *
  KEY_KP_MINUS           = 0x56,     // Keypad -
  KEY_KP_PLUS            = 0x57,     // Keypad +
  KEY_KP_ENTER           = 0x58,     // Keypad ENTER
  KEY_KP_1               = 0x59,     // Keypad 1 and End
  KEY_KP_2               = 0x5A,     // Keypad 2 and Down Arrow
  KEY_KP_3               = 0x5B,     // Keypad 3 and PageDn
  KEY_KP_4               = 0x5C,     // Keypad 4 and Left Arrow
  KEY_KP_5               = 0x5D,     // Keypad 5
  KEY_KP_6               = 0x5E,     // Keypad 6 and Right Arrow
  KEY_KP_7               = 0x5F,     // Keypad 7 and Home
  KEY_KP_8               = 0x60,     // Keypad 8 and Up Arrow
  KEY_KP_9               = 0x61,     // Keypad 9 and PageUp
  KEY_KP_0               = 0x62,     // Keypad 0 and Insert
  KEY_KP_PERIOD          = 0x63,     // Keypad . and Delete
  KEY_NONUSBACKSLASH     = 0x64,     // Keyboard Non-US \ and |
  KEY_APPLICATION        = 0x65,     // Keyboard Application
                                     // LOGICAL_MAXIMUM

  KEY_KP_EQUALS          = 0x67,     // Keypad =

  KEY_EXECUTE            = 0x74,     // Keyboard Execute

  KEY_LOCKINGCAPSLOCK    = 0x82,     // Keyboard Locking Caps Lock
  KEY_LOCKINGNUMLOCK     = 0x83,     // Keyboard Locking Num Lock
  KEY_LOCKINGSCROLLOCK   = 0x84,     // Keyboard Locking Scroll Lock
  KEY_KP_COMMA           = 0x85,     // Keypad Comma
  KEY_KP_EQUALSAS400     = 0x86,     // Keypad Equal Sign
  KEY_INTERNATIONAL1     = 0x87,     // Keyboard International1
  KEY_INTERNATIONAL2     = 0x88,     // Keyboard International2
  KEY_INTERNATIONAL3     = 0x89,     // Keyboard International3
  KEY_INTERNATIONAL4     = 0x8A,     // Keyboard International4
  KEY_INTERNATIONAL5     = 0x8B,     // Keyboard International5
  KEY_INTERNATIONAL6     = 0x8C,     // Keyboard International6
  KEY_INTERNATIONAL7     = 0x8D,     // Keyboard International7
  KEY_INTERNATIONAL8     = 0x8E,     // Keyboard International8
  KEY_INTERNATIONAL9     = 0x8F,     // Keyboard International9

  KEY_SYSREQ             = 0x9A,     // Keyboard SysReq/Attention

  KEY_RETURN2            = 0x9E,     // Keyboard Return
  
  KEY_CLEARAGAIN         = 0xA2,     // Keyboard Clear/Again
                                     // Keys 0xA5 to 0xAF reserved
  KEY_KP_00              = 0xB0,     // Keypad 00
  KEY_KP_000             = 0xB1,     // Keypad 000
  KEY_THOUSANDSSEPARATOR = 0xB2,     // Thousands Separator
  KEY_DECIMALSEPARATOR   = 0xB3,     // Decimal Separator
  KEY_CURRENCYUNIT       = 0xB4,     // Currency Unit
  KEY_CURRENCYSUBUNIT    = 0xB5,     // Currency Sub-unit
  KEY_KP_LEFTPAREN       = 0xB6,     // Keypad (
  KEY_KP_RIGHTPAREN      = 0xB7,     // Keypad )
  KEY_KP_LEFTBRACE       = 0xB8,     // Keypad {
  KEY_KP_RIGHTBRACE      = 0xB9,     // Keypad }
  KEY_KP_TAB             = 0xBA,     // Keypad Tab
  KEY_KP_BACKSPACE       = 0xBB,     // Keypad Backspace
  KEY_KP_A               = 0xBC,     // Keypad A
  KEY_KP_B               = 0xBD,     // Keypad B
  KEY_KP_C               = 0xBE,     // Keypad C
  KEY_KP_D               = 0xBF,     // Keypad D
  KEY_KP_E               = 0xC0,     // Keypad E
  KEY_KP_F               = 0xC1,     // Keypad F
  KEY_KP_XOR             = 0xC2,     // Keypad XOR
  KEY_KP_POWER           = 0xC3,     // Keypad ^
  KEY_KP_PERCENT         = 0xC4,     // Keypad %
  KEY_KP_LESS            = 0xC5,     // Keypad <
  KEY_KP_GREATER         = 0xC6,     // Keypad >
  KEY_KP_AMPERSAND       = 0xC7,     // Keypad &
  KEY_KP_DBLAMPERSAND    = 0xC8,     // Keypad &&
  KEY_KP_VERTICALBAR     = 0xC9,     // Keypad |
  KEY_KP_DBLVERTICALBAR  = 0xCA,     // Keypad ||
  KEY_KP_COLON           = 0xCB,     // Keypad :
  KEY_KP_HASH            = 0xCC,     // Keypad #
  KEY_KP_SPACE           = 0xCD,     // Keypad Space
  KEY_KP_AT              = 0xCE,     // Keypad @
  KEY_KP_EXCLAM          = 0xCF,     // Keypad !
  KEY_KP_MEMSTORE        = 0xD0,     // Keypad Memory Store
  KEY_KP_MEMRECALL       = 0xD1,     // Keypad Memory Recall
  KEY_KP_MEMCLEAR        = 0xD2,     // Keypad Memory Clear
  KEY_KP_MEMADD          = 0xD3,     // Keypad Memory Add
  KEY_KP_MEMSUBTRACT     = 0xD4,     // Keypad Memory Subtract
  KEY_KP_MEMMULTIPLY     = 0xD5,     // Keypad Memory Multiply
  KEY_KP_MEMDIVIDE       = 0xD6,     // Keypad Memory Divide
  KEY_KP_PLUSMINUS       = 0xD7,     // Keypad +/-
  KEY_KP_CLEAR           = 0xD8,     // Keypad Clear
  KEY_KP_CLEARENTRY      = 0xD9,     // Keypad Clear Entry
  KEY_KP_BINARY          = 0xDA,     // Keypad Binary
  KEY_KP_OCTAL           = 0xDB,     // Keypad Octal
  KEY_KP_DECIMAL         = 0xDC,     // Keypad Decimal
  KEY_KP_HEXADECIMAL     = 0xDD,     // Keypad Hexadecimal
                                     // Keys 0xDE to 0xDF reserved
                                     // These are sent as modifiers
  KEY_LCTRL              = 0xE0,     // Keyboard LeftControl
  KEY_LSHIFT             = 0xE1,     // Keyboard LeftShift
  KEY_LALT               = 0xE2,     // Keyboard LeftAlt
  KEY_LGUI               = 0xE3,     // Keyboard Left GUI
  KEY_RCTRL              = 0xE4,     // Keyboard RightControl
  KEY_RSHIFT             = 0xE5,     // Keyboard RightShift
  KEY_RALT               = 0xE6,     // Keyboard RightAlt
  KEY_RGUI               = 0xE7,     // Keyboard Right GUI
                                     // End of modifiers

  KEY_MODE               = 0x101,
  KEY_AUDIONEXT          = 0x102,
  KEY_AUDIOPREV          = 0x103,
  KEY_AUDIOSTOP          = 0x104,
  KEY_AUDIOPLAY          = 0x105,
  KEY_AUDIOMUTE          = 0x106,
  KEY_MEDIASELECT        = 0x107,
  KEY_WWW                = 0x108,
  KEY_MAIL               = 0x109,
  KEY_CALCULATOR         = 0x10A,
  KEY_COMPUTER           = 0x10B,
  KEY_AC_SEARCH          = 0x10C,
  KEY_AC_HOME            = 0x10D,
  KEY_AC_BACK            = 0x10E,
  KEY_AC_FORWARD         = 0x10F,
  KEY_AC_STOP            = 0x110,
  KEY_AC_REFRESH         = 0x111,
  KEY_AC_BOOKMARKS       = 0x112,
  KEY_BRIGHTNESSDOWN     = 0x113,
  KEY_BRIGHTNESSUP       = 0x114,
  KEY_DISPLAYSWITCH      = 0x115,
  KEY_KBDILLUMTOGGLE     = 0x116,
  KEY_KBDILLUMDOWN       = 0x117,
  KEY_KBDILLUMUP         = 0x118,
  KEY_EJECT              = 0x119,
  KEY_SLEEP              = 0x11A,
  KEY_APP1               = 0x11B,
  KEY_APP2               = 0x11C
} UsbKeyboardScanCode;

typedef enum {
  MOD_RESERVED           = 0xE000,
  MOD_LEFTCTRL           = 0xE001,     // Keyboard LeftControl
  MOD_LEFTSHIFT          = 0xE002,     // Keyboard LeftShift
  MOD_LEFTALT            = 0xE004,     // Keyboard LeftAlt
  MOD_LEFTGUI            = 0xE008,     // Keyboard Left GUI
  MOD_RIGHTCTRL          = 0xE010,     // Keyboard RightControl
  MOD_RIGHTSHIFT         = 0xE020,     // Keyboard RightShift
  MOD_RIGHTALT           = 0xE040,     // Keyboard RightAlt
  MOD_RIGHTGUI           = 0xE080,     // Keyboard Right GUI
  MOD_LCTRL              = 0xE001,     // Keyboard LeftControl
  MOD_LSHIFT             = 0xE002,     // Keyboard LeftShift
  MOD_LALT               = 0xE004,     // Keyboard LeftAlt
  MOD_LGUI               = 0xE008,     // Keyboard Left GUI
  MOD_RCTRL              = 0xE010,     // Keyboard RightControl
  MOD_RSHIFT             = 0xE020,     // Keyboard RightShift
  MOD_RALT               = 0xE040,     // Keyboard RightAlt
  MOD_RGUI               = 0xE080,     // Keyboard Right GUI
  
  // Platform-specific names
  MOD_LEFT_WINDOWS       = 0xE008,     // Left Windows Key (⊞)
  MOD_RIGHT_WINDOWS      = 0xE080,     // Right Windows Key (⊞)
  MOD_LEFT_COMMAND       = 0xE008,     // Left Mac Command Key (⌘)
  MOD_RIGHT_COMMAND      = 0xE080,     // Right Mac Command Key (⌘)
  MOD_LEFT_META          = 0xE008,     // Left *nix Meta Key (◆)
  MOD_RIGHT_META         = 0xE080,     // Right *nix Meta Key (◆)
} UsbKeyboardModifier;

#endif // __SPARK_WIRING_USBKEYBOARDSCANCODE_H
