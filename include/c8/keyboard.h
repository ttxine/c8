#ifndef C8_KEYBOARD_H
#define C8_KEYBOARD_H

#include <stdbool.h>

typedef enum c8_key {
    C8_KEY_0 = 0,
    C8_KEY_1,
    C8_KEY_2,
    C8_KEY_3,
    C8_KEY_4,
    C8_KEY_5,
    C8_KEY_6,
    C8_KEY_7,
    C8_KEY_8,
    C8_KEY_9,
    C8_KEY_A,
    C8_KEY_B,
    C8_KEY_C,
    C8_KEY_D,
    C8_KEY_E,
    C8_KEY_F,
    C8_KEY_NUM
} C8Key;

typedef struct c8_keyboard C8Keyboard;

C8Keyboard *c8_keyboard_new(void);
void c8_keyboard_press_key(C8Keyboard *keyboard, C8Key key);
void c8_keyboard_release_key(C8Keyboard *keyboard, C8Key key);
bool c8_keyboard_is_key_pressed(C8Keyboard *keyboard, C8Key key);
C8Key c8_keyboard_wait_for_press(C8Keyboard *keyboard);

#endif
