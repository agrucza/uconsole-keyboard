#ifndef KEYBOARD_VARIANT_H
#define KEYBOARD_VARIANT_H

/*
 * Hardware variants supported by this firmware.
 *
 * The variant is selected at build time: `make KEYBOARD=uconsole` (default)
 * or `make KEYBOARD=devterm`. The Makefile passes it as -DKEYBOARD_VARIANT=...
 *
 * Both keyboards use the same MCU, the same pins for the 8x8 matrix, the 17
 * directly wired keys and the trackball, the same bootloader and the same USB
 * IDs. They differ in which key sits at which position, in the orientation of
 * the trackball, and in the backlight (uConsole only).
 */
#define VARIANT_UCONSOLE 1
#define VARIANT_DEVTERM  2

#ifndef KEYBOARD_VARIANT
#define KEYBOARD_VARIANT VARIANT_UCONSOLE
#endif

/*
 * Every variant defines:
 *
 * VARIANT_USB_PRODUCT_STRING  USB product string reported to the host
 * VARIANT_LAYERS_FILE         file with the key bindings, see load_config()
 * VARIANT_HAS_BACKLIGHT       1 if PA8 (BL_CTRL) drives a keyboard backlight
 * TRACKBALL_HOn_IRQ()         movement reported by hall sensor HOn. The sensor
 *                             pins are HO1 = PC8, HO2 = PC9, HO3 = PC10 and
 *                             HO4 = PC11. The pin labels in main.h (HO_UP_Pin,
 *                             HO_RIGHT_Pin, ...) come from the .ioc file and
 *                             describe the uConsole orientation only.
 *
 * The button IDs of each variant are in keymaps.h.
 */
#if KEYBOARD_VARIANT == VARIANT_UCONSOLE

#define VARIANT_USB_PRODUCT_STRING  "uConsole"
#define VARIANT_LAYERS_FILE         "layers.h"
#define VARIANT_HAS_BACKLIGHT       1

#define TRACKBALL_HO1_IRQ()         trackball_interrupt_y_neg() /* up */
#define TRACKBALL_HO2_IRQ()         trackball_interrupt_x_pos() /* right */
#define TRACKBALL_HO3_IRQ()         trackball_interrupt_y_pos() /* down */
#define TRACKBALL_HO4_IRQ()         trackball_interrupt_x_neg() /* left */

#elif KEYBOARD_VARIANT == VARIANT_DEVTERM

#define VARIANT_USB_PRODUCT_STRING  "DevTerm"
#define VARIANT_LAYERS_FILE         "layers_devterm.h"
#define VARIANT_HAS_BACKLIGHT       0

#define TRACKBALL_HO1_IRQ()         trackball_interrupt_x_neg() /* left */
#define TRACKBALL_HO2_IRQ()         trackball_interrupt_y_neg() /* up */
#define TRACKBALL_HO3_IRQ()         trackball_interrupt_x_pos() /* right */
#define TRACKBALL_HO4_IRQ()         trackball_interrupt_y_pos() /* down */

#else
#error "Unknown KEYBOARD_VARIANT, use VARIANT_UCONSOLE or VARIANT_DEVTERM"
#endif

#endif
