#!/bin/bash
#
# Identifies the connected keyboard by its USB product name.
#
# The uConsole and the DevTerm keyboard have the same USB IDs, they can only be
# told apart by the USB product name. This applies to the stock firmware and to
# this firmware alike.
#
# Usage:
#   check_keyboard_variant.sh --detect
#       Used by the Makefile when KEYBOARD is not given. Prints the KEYBOARD
#       value for the connected keyboard ("uconsole" or "devterm"). Prints
#       nothing if there is no keyboard, if it is in bootloader mode, or if
#       both kinds are connected.
#
#   check_keyboard_variant.sh <expected product name>
#       Safeguard for "make flash" and "make first_flash": refuses to flash a
#       firmware that was built for the other keyboard.
#       Exit code 0: the connected keyboard matches, or it could not be
#                    identified (not connected, bootloader mode, not Linux).
#       Exit code 1: the connected keyboard is the other one.
#       Set FORCE=1 to flash anyway. This is needed to recover a keyboard that
#       runs the wrong firmware, because it reports the wrong product name then.

# Product names of all keyboards supported by this firmware (see Core/Inc/keyboard_variant.h).
# The KEYBOARD value of the Makefile is the product name in lower case.
KNOWN_PRODUCTS="uConsole DevTerm"
KEYBOARD_VID="1eaf"
# The bootloader (DFU mode) has the same vendor ID, it says nothing about the keyboard
DFU_PID="0003"
USB_DEVICES_DIR="${USB_DEVICES_DIR:-/sys/bus/usb/devices}"

# Prints the product name of every connected keyboard that is supported, one per line
connected_keyboards() {
    local d ven prod name known
    for d in "$USB_DEVICES_DIR"/*; do
        [ -f "$d/idVendor" ] && [ -f "$d/idProduct" ] || continue
        ven=$(tr 'A-F' 'a-f' < "$d/idVendor" 2>/dev/null)
        prod=$(tr 'A-F' 'a-f' < "$d/idProduct" 2>/dev/null)
        [ "$ven" = "$KEYBOARD_VID" ] || continue
        [ "$prod" = "$DFU_PID" ] && continue
        name=$(cat "$d/product" 2>/dev/null)
        for known in $KNOWN_PRODUCTS; do
            [ "$name" = "$known" ] && echo "$name"
        done
    done
}

if [ -z "$1" ]; then
    echo "Usage: $0 --detect | <expected product name>" >&2
    exit 2
fi

if [ "$1" = "--detect" ]; then
    [ -d "$USB_DEVICES_DIR" ] || exit 0
    kinds=$(connected_keyboards | sort -u)
    # Exactly one kind of keyboard, otherwise the choice is up to the user
    if [ -n "$kinds" ] && [ "$(echo "$kinds" | wc -l)" -eq 1 ]; then
        echo "$kinds" | tr 'A-Z' 'a-z'
    fi
    exit 0
fi

EXPECTED="$1"

if [ ! -d "$USB_DEVICES_DIR" ]; then
    echo "Note: can't check which keyboard is connected on this platform, make sure that the firmware is for the $EXPECTED keyboard"
    exit 0
fi

matching=0
mismatch=""
while read -r name; do
    [ -n "$name" ] || continue
    if [ "$name" = "$EXPECTED" ]; then
        matching=$((matching + 1))
    else
        mismatch="$name"
    fi
done < <(connected_keyboards)

if [ -n "$mismatch" ]; then
    if [ "$FORCE" = "1" ]; then
        echo "Warning: the connected keyboard reports \"$mismatch\", but the firmware is for the $EXPECTED keyboard. Flashing anyway because of FORCE=1."
        exit 0
    fi
    echo "Error: the connected keyboard reports \"$mismatch\", but the firmware is for the $EXPECTED keyboard." >&2
    echo "Build and flash the firmware for your keyboard with KEYBOARD=$(echo "$mismatch" | tr 'A-Z' 'a-z')." >&2
    echo "If the keyboard runs the wrong firmware and you want to replace it, add FORCE=1." >&2
    exit 1
fi

if [ "$matching" -eq 0 ]; then
    echo "Note: can't check which keyboard is connected (not found or already in bootloader mode), make sure that the firmware is for the $EXPECTED keyboard"
    exit 0
fi

echo "Found the $EXPECTED keyboard"
exit 0
