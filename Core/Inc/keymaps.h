#ifndef KEYMAPS_H
#define KEYMAPS_H

#include "main.h"
#include <stdint.h>
#include "keyboard_matrix.h"
#include "keyboard_non_matrix.h"
#include "keyboard_state.h"

/* Special keys and virtual keys */
enum SKEYS {
  SK_FN_KEY = SPECIAL_KEY_FLAG,
  SK_KEYBOARD_LOCK,
  SK_KEYBOARD_LIGHT
};

#define KEY_NONE                0xFFFF

/*
 * Button IDs. Matrix keys are row * 8 + column, the directly wired keys are
 * MATRIX_KEYS + index of the KEYn pin. Both keyboards share most of the
 * positions, the differences are selected by the keyboard variant.
 */
#define BUTTON_SELECT           0x00
#define BUTTON_START            0x01
#define BUTTON_VOLUME           0x02
#define BUTTON_GRAVE            0x03
#define BUTTON_BRACE_LEFT       0x04
#define BUTTON_BRACE_RIGHT      0x05
#define BUTTON_MINUS            0x06
#define BUTTON_EQUAL            0x07
#define BUTTON_1                0x08
#define BUTTON_2                0x09
#define BUTTON_3                0x0A
#define BUTTON_4                0x0B
#define BUTTON_5                0x0C
#define BUTTON_6                0x0D
#define BUTTON_7                0x0E
#define BUTTON_8                0x0F
#define BUTTON_9                0x10
#define BUTTON_0                0x11
#define BUTTON_ESC              0x12
#define BUTTON_TAB              0x13
#if KEYBOARD_VARIANT == VARIANT_DEVTERM
/* Arrow keys of the DevTerm are part of the matrix, the D-pad is BUTTON_GAMEPAD_UP etc. */
#define BUTTON_UP               0x14
#define BUTTON_DOWN             0x15
#define BUTTON_LEFT             0x16
#define BUTTON_RIGHT            0x17
#endif
#define BUTTON_Q                0x18
#define BUTTON_W                0x19
#define BUTTON_E                0x1A
#define BUTTON_R                0x1B
#define BUTTON_T                0x1C
#define BUTTON_Y                0x1D
#define BUTTON_U                0x1E
#define BUTTON_I                0x1F
#define BUTTON_O                0x20
#define BUTTON_P                0x21
#define BUTTON_A                0x22
#define BUTTON_S                0x23
#define BUTTON_D                0x24
#define BUTTON_F                0x25
#define BUTTON_G                0x26
#define BUTTON_H                0x27
#define BUTTON_J                0x28
#define BUTTON_K                0x29
#define BUTTON_L                0x2A
#define BUTTON_Z                0x2B
#define BUTTON_X                0x2C
#define BUTTON_C                0x2D
#define BUTTON_V                0x2E
#define BUTTON_B                0x2F
#define BUTTON_N                0x30
#define BUTTON_M                0x31
#define BUTTON_COMMA            0x32
#define BUTTON_DOT              0x33
#define BUTTON_SLASH            0x34
#define BUTTON_BACKSLASH        0x35
#define BUTTON_SEMICOLON        0x36
#define BUTTON_APOSTROPHE       0x37
#define BUTTON_BACKSPACE        0x38
#define BUTTON_ENTER            0x39

#if KEYBOARD_VARIANT == VARIANT_UCONSOLE

#define BUTTON_FN_LEFT          0x3A
#define BUTTON_FN_RIGHT         0x3B
#define BUTTON_SPACE            0x3C

#define BUTTON_TRACKBALL        (MATRIX_KEYS + 0x00)
#define BUTTON_UP               (MATRIX_KEYS + 0x01)
#define BUTTON_DOWN             (MATRIX_KEYS + 0x02)
#define BUTTON_LEFT             (MATRIX_KEYS + 0x03)
#define BUTTON_RIGHT            (MATRIX_KEYS + 0x04)
#define BUTTON_GAMEPAD_A        (MATRIX_KEYS + 0x05)
#define BUTTON_GAMEPAD_B        (MATRIX_KEYS + 0x06)
#define BUTTON_GAMEPAD_X        (MATRIX_KEYS + 0x07)
#define BUTTON_GAMEPAD_Y        (MATRIX_KEYS + 0x08)
#define BUTTON_SHIFT_LEFT       (MATRIX_KEYS + 0x09)
#define BUTTON_SHIFT_RIGHT      (MATRIX_KEYS + 0x0A)
#define BUTTON_CTRL_LEFT        (MATRIX_KEYS + 0x0B)
#define BUTTON_CTRL_RIGHT       (MATRIX_KEYS + 0x0C)
#define BUTTON_ALT_LEFT         (MATRIX_KEYS + 0x0D)
#define BUTTON_MOUSE_L          (MATRIX_KEYS + 0x0E)
#define BUTTON_ALT_RIGHT        (MATRIX_KEYS + 0x0F)
#define BUTTON_MOUSE_R          (MATRIX_KEYS + 0x10)

#elif KEYBOARD_VARIANT == VARIANT_DEVTERM

#define BUTTON_ALT_RIGHT        0x3A
#define BUTTON_CTRL_RIGHT       0x3B
#define BUTTON_SHIFT_RIGHT      0x3C
#define BUTTON_SPACE            0x3D

#define BUTTON_TRACKBALL        (MATRIX_KEYS + 0x00)
#define BUTTON_GAMEPAD_UP       (MATRIX_KEYS + 0x01)
#define BUTTON_GAMEPAD_DOWN     (MATRIX_KEYS + 0x02)
#define BUTTON_GAMEPAD_LEFT     (MATRIX_KEYS + 0x03)
#define BUTTON_GAMEPAD_RIGHT    (MATRIX_KEYS + 0x04)
#define BUTTON_GAMEPAD_A        (MATRIX_KEYS + 0x05)
#define BUTTON_GAMEPAD_B        (MATRIX_KEYS + 0x06)
#define BUTTON_GAMEPAD_X        (MATRIX_KEYS + 0x07)
#define BUTTON_GAMEPAD_Y        (MATRIX_KEYS + 0x08)
#define BUTTON_SHIFT_LEFT       (MATRIX_KEYS + 0x09)
#define BUTTON_FN               (MATRIX_KEYS + 0x0A)
#define BUTTON_CTRL_LEFT        (MATRIX_KEYS + 0x0B)
#define BUTTON_CMD              (MATRIX_KEYS + 0x0C)
#define BUTTON_ALT_LEFT         (MATRIX_KEYS + 0x0D)
#define BUTTON_MOUSE_L          (MATRIX_KEYS + 0x0E)
#define BUTTON_MOUSE_M          (MATRIX_KEYS + 0x0F)
#define BUTTON_MOUSE_R          (MATRIX_KEYS + 0x10)

#endif

extern KEYBOARD_STATE keyboard_state;

void matrix_action(uint8_t row, uint8_t col, uint8_t mode);
void non_matrix_action(uint8_t col, uint8_t mode);
void bind_button(uint8_t layer, uint8_t fn, uint16_t button, uint16_t key);
void layer_set(uint8_t layer);
uint8_t layer_get(void);
void fn_lock_set(uint8_t fn_lock);

#endif
