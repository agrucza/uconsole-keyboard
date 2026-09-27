#include <stdint.h>
#include "hid_vendor.h"
#include "keymaps.h"
#include "trackball.h"
#include "usbd_custom_hid_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t sending_scheduled = 0;

#if VARIANT_TRACKBALL_GLIDE
static uint8_t tune_reply[4];
static uint8_t tune_reply_scheduled = 0;

static void hid_vendor_tune(uint8_t* data) {
    const uint8_t parameter = data[1] & (uint8_t)~HID_VENDOR_TUNE_READ;
    uint8_t value = 0;
    bool ok = true;

    if (!(data[1] & HID_VENDOR_TUNE_READ)) {
        ok = trackball_tune_set(parameter, data[2]);
    }
    if (ok && parameter != TRACKBALL_TUNE_RELOAD) {
        ok = trackball_tune_get(parameter, &value);
    }

    tune_reply[0] = 0x05;
    tune_reply[1] = HID_VENDOR_TUNE;
    tune_reply[2] = ok ? parameter : HID_VENDOR_TUNE_ERROR;
    tune_reply[3] = ok ? value : parameter;
    tune_reply_scheduled = 1;
}
#endif

void hid_vendor_on_recv(uint8_t* data) {
#if VARIANT_TRACKBALL_GLIDE
    if (data[0] == HID_VENDOR_TUNE) {
        hid_vendor_tune(data);
        return;
    }
#endif

    if (data[0] == 0xFE && data[1] == 0x55 && data[2] == 0xAA) {
        // Bootloader magic
        jump_to_bootloader();
    }

    if ((data[0] & 0x0F) != 0x0F) {
        layer_set(data[0] & 0x0F);
    }

    for (int i = 0; i <= 8; i++) {
        if (data[2] & (1 << i)) {
            uint8_t value = (data[1] & (1 << i)) ? 1 : 0;
            switch (i) {
                case KEYBOARD_OPTION_FN_LOCK:
                    fn_lock_set(value);
                    break;                    
                case KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT:
                    keyboard_state.double_p_to_brace_left = value;
                    break;
            }
        }
    }

    sending_scheduled = 1;
}

static USBD_StatusTypeDef hid_vendor_send_state(void) {
    uint8_t data[4] = {
        0x05,
        layer_get(),
        keyboard_state.fn_lock ? (1 << KEYBOARD_OPTION_FN_LOCK) : 0,
        keyboard_state.double_p_to_brace_left ? (1 << KEYBOARD_OPTION_DOUBLE_P_TO_BRACE_LEFT) : 0
    };

    return hid_send_report(&hUsbDeviceFS, data, sizeof(data), 1);
}

void hid_vendor_schedule_send(void) {
    sending_scheduled = 1;
}

void hid_vendor_task(void) {
#if VARIANT_TRACKBALL_GLIDE
    if (tune_reply_scheduled) {
        if (hid_send_report(&hUsbDeviceFS, tune_reply, sizeof(tune_reply), 1) == USBD_OK) {
            tune_reply_scheduled = 0;
        }
        return;
    }
#endif
    if (sending_scheduled) {
        USBD_StatusTypeDef result = hid_vendor_send_state();
        if (result == USBD_OK) {
            sending_scheduled = 0;
        }
    }
}
