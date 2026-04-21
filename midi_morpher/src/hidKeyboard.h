#pragma once
#include <USB.h>
#include <USBHIDKeyboard.h>

extern "C" bool tud_mounted(void);
extern USBHIDKeyboard hidKeyboard;

// Modifier bits stored in fsChannel when isKeyboard = true
#define KEY_MOD_CTRL  0x01
#define KEY_MOD_SHIFT 0x02
#define KEY_MOD_ALT   0x04
#define KEY_MOD_GUI   0x08

struct HIDKeyEntry {
    uint8_t     key;   // value passed to keyboard.press()
    const char *name;  // short display name
};

inline constexpr HIDKeyEntry hidKeys[] = {
    // Navigation (14)
    { KEY_RETURN,      "Enter"  },
    { KEY_BACKSPACE,   "Bksp"   },
    { ' ',             "Space"  },
    { KEY_TAB,         "Tab"    },
    { KEY_ESC,         "Esc"    },
    { KEY_DELETE,      "Del"    },
    { KEY_HOME,        "Home"   },
    { KEY_END,         "End"    },
    { KEY_PAGE_UP,     "PgUp"   },
    { KEY_PAGE_DOWN,   "PgDn"   },
    { KEY_UP_ARROW,    "Up"     },
    { KEY_DOWN_ARROW,  "Down"   },
    { KEY_LEFT_ARROW,  "Left"   },
    { KEY_RIGHT_ARROW, "Right"  },
    // Function keys (12)
    { KEY_F1,  "F1"  }, { KEY_F2,  "F2"  }, { KEY_F3,  "F3"  }, { KEY_F4,  "F4"  },
    { KEY_F5,  "F5"  }, { KEY_F6,  "F6"  }, { KEY_F7,  "F7"  }, { KEY_F8,  "F8"  },
    { KEY_F9,  "F9"  }, { KEY_F10, "F10" }, { KEY_F11, "F11" }, { KEY_F12, "F12" },
    // Letters (26)
    { 'a',"A" },{ 'b',"B" },{ 'c',"C" },{ 'd',"D" },{ 'e',"E" },{ 'f',"F" },
    { 'g',"G" },{ 'h',"H" },{ 'i',"I" },{ 'j',"J" },{ 'k',"K" },{ 'l',"L" },
    { 'm',"M" },{ 'n',"N" },{ 'o',"O" },{ 'p',"P" },{ 'q',"Q" },{ 'r',"R" },
    { 's',"S" },{ 't',"T" },{ 'u',"U" },{ 'v',"V" },{ 'w',"W" },{ 'x',"X" },
    { 'y',"Y" },{ 'z',"Z" },
    // Digits (10)
    { '0',"0" },{ '1',"1" },{ '2',"2" },{ '3',"3" },{ '4',"4" },
    { '5',"5" },{ '6',"6" },{ '7',"7" },{ '8',"8" },{ '9',"9" },
};
inline constexpr uint8_t NUM_HID_KEYS = sizeof(hidKeys) / sizeof(hidKeys[0]); // 62

inline void sendKeyDown(uint8_t keyIdx, uint8_t modifiers) {
    if (!tud_mounted() || keyIdx >= NUM_HID_KEYS) return;
    if (modifiers & KEY_MOD_CTRL)  hidKeyboard.press(KEY_LEFT_CTRL);
    if (modifiers & KEY_MOD_SHIFT) hidKeyboard.press(KEY_LEFT_SHIFT);
    if (modifiers & KEY_MOD_ALT)   hidKeyboard.press(KEY_LEFT_ALT);
    if (modifiers & KEY_MOD_GUI)   hidKeyboard.press(KEY_LEFT_GUI);
    hidKeyboard.press(hidKeys[keyIdx].key);
}

inline void sendKeyUp() {
    if (!tud_mounted()) return;
    hidKeyboard.releaseAll();
}
