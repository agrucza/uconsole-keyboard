#ifndef HID_RAW_H
#define HID_RAW_H

#include <stdint.h>
#include "usbd_custom_hid_if.h"

#define KEYBOARD_OPTION_FN_LOCK                     0
#define KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT      7

/*
 * Live tuning of the trackball (VARIANT_TRACKBALL_GLIDE only), used by
 * tools/trackball_tune.py. The 3 data bytes of report 5 are:
 *
 *   write: HID_VENDOR_TUNE, parameter, value
 *   read:  HID_VENDOR_TUNE, parameter | HID_VENDOR_TUNE_READ, 0
 *
 * The parameters are TRACKBALL_TUNE_* in trackball.h. The keyboard answers
 * with HID_VENDOR_TUNE, parameter, value, or with HID_VENDOR_TUNE,
 * HID_VENDOR_TUNE_ERROR, parameter if it does not know the parameter.
 * The values are lost when the layer changes or the keyboard restarts.
 */
#define HID_VENDOR_TUNE         0xFD
#define HID_VENDOR_TUNE_READ    0x80
#define HID_VENDOR_TUNE_ERROR   0xFF

void hid_vendor_on_recv(uint8_t* data);
void hid_vendor_schedule_send(void);
void hid_vendor_task(void);

#endif
