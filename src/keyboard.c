#include "c8/keyboard.h"

#include <stdio.h>
#include <stdlib.h>

struct c8_keyboard {
    bool keys[C8_KEY_NUM];
};

C8Keyboard *c8_keyboard_new(void)
{
    C8Keyboard *keyboard = calloc(sizeof(C8Keyboard), 1);

    if (keyboard == NULL) {
        fprintf(stderr, "keyboard: can't allocate keyboard\n");
        return NULL;
    }

    return keyboard;
}

void c8_keyboard_press_key(C8Keyboard *keyboard, C8Key key)
{
    if (key < C8_KEY_NUM) {
        keyboard->keys[key] = true;
    }
}

void c8_keyboard_release_key(C8Keyboard *keyboard, C8Key key)
{
    if (key < C8_KEY_NUM) {
        keyboard->keys[key] = false;
    }
}

bool c8_keyboard_is_key_pressed(C8Keyboard *keyboard, C8Key key)
{
    if (key < C8_KEY_NUM) {
        return keyboard->keys[key];
    }
}

C8Key c8_keyboard_wait_for_press(C8Keyboard *keyboard)
{
    for (C8Key key = 0; key < C8_KEY_NUM; key++) {
        if (c8_keyboard_is_key_pressed(keyboard, key)) {
            return key;
        }
    }

    return C8_KEY_NUM;
}
