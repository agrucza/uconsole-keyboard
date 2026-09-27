#!/usr/bin/env python3
"""
Live tuning of the DevTerm trackball, without flashing.

The values take effect at once. They are lost when the layer is changed or the
keyboard restarts, so put the values you like into layers_devterm.h and flash.

Examples:
  trackball_tune.py                       show the values in use
  trackball_tune.py speed=200             twice as fast as the stock firmware
  trackball_tune.py speed=50 measure=fast
  trackball_tune.py preset stock          presets: stock, faster, flick
  trackball_tune.py reload                back to the values of the layer

Values:
  speed=N     pointer speed in percent of the stock firmware, 5 to 1275 in steps of 5
  accel=N     0 = the pointer speed follows the speed of the ball, up to 2.55.
              Higher values reward fast rolling
  smooth=N    how long the pointer glides after a pulse, in percent of the stock
              firmware, 0 to 1275 in steps of 5
  measure=M   average: as the stock firmware
              fast: a fast flick is recognized at once and goes much further

Needs access to the hidraw device: run it with sudo, or install
tools/70-uconsole-keyboard.rules. The protocol is described in Core/Inc/hid_vendor.h.
"""

from __future__ import annotations

import argparse
import glob
import os
import select
import sys
import time

DEVICE_VID = "1eaf"
DEVICE_PID = "0024"
PRODUCT = "DevTerm"

REPORT_ID = 0x05
TUNE = 0xFD
TUNE_READ = 0x80
TUNE_ERROR = 0xFF

SPEED, ACCELERATION, SMOOTHNESS, MEASUREMENT, RELOAD = 1, 2, 3, 4, 5
MEASUREMENTS = {"average": 0, "fast": 1}

PRESETS = {
    "stock": {"speed": 100, "accel": 0.0, "smooth": 100, "measure": "average"},
    "faster": {"speed": 200, "accel": 0.0, "smooth": 100, "measure": "average"},
    "flick": {"speed": 50, "accel": 0.0, "smooth": 100, "measure": "fast"},
}


def encode(name: str, text: str) -> tuple[int, int]:
    """Returns the parameter and the byte to send for a value given by the user."""
    try:
        if name == "speed":
            value = round(float(text) / 5)
            if not 1 <= value <= 255:
                raise ValueError("speed must be between 5 and 1275")
            return SPEED, value
        if name == "accel":
            value = round(float(text) * 100)
            if not 0 <= value <= 255:
                raise ValueError("accel must be between 0 and 2.55")
            return ACCELERATION, value
        if name == "smooth":
            value = round(float(text) / 5)
            if not 0 <= value <= 255:
                raise ValueError("smooth must be between 0 and 1275")
            return SMOOTHNESS, value
        if name == "measure":
            if text not in MEASUREMENTS:
                raise ValueError("measure must be average or fast")
            return MEASUREMENT, MEASUREMENTS[text]
    except ValueError as e:
        raise ValueError(str(e) if "must be" in str(e) else f"{name}={text}: not a number") from None
    raise ValueError(f"unknown value {name}, use speed, accel, smooth or measure")


def find_keyboard(vid: str, pid: str) -> tuple[str | None, str | None]:
    """Returns the hidraw device and the USB product name of the keyboard."""
    for hid_sys in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        path = os.path.realpath(os.path.join(hid_sys, "device"))
        while path and path != "/":
            id_vendor = os.path.join(path, "idVendor")
            if os.path.isfile(id_vendor):
                try:
                    with open(id_vendor) as f:
                        ven = f.read().strip().lower()
                    with open(os.path.join(path, "idProduct")) as f:
                        prod = f.read().strip().lower()
                    product = None
                    if os.path.isfile(os.path.join(path, "product")):
                        with open(os.path.join(path, "product")) as f:
                            product = f.read().strip()
                except OSError:
                    break
                if ven == vid and prod == pid:
                    return "/dev/" + os.path.basename(hid_sys), product
                break
            path = os.path.dirname(path)
    return None, None


class Keyboard:
    def __init__(self, device: str):
        self.fd = os.open(device, os.O_RDWR)

    def close(self) -> None:
        os.close(self.fd)

    def request(self, parameter: int, value: int = 0, timeout: float = 1.0) -> int | None:
        """Sends a tuning command and returns the value of the answer."""
        os.write(self.fd, bytes([REPORT_ID, TUNE, parameter, value]))
        wanted = parameter & ~TUNE_READ
        end = time.monotonic() + timeout
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise TimeoutError
            if not select.select([self.fd], [], [], remaining)[0]:
                raise TimeoutError
            data = os.read(self.fd, 64)
            # Other reports arrive on the same device, e.g. when the trackball is moved
            if len(data) < 4 or data[0] != REPORT_ID or data[1] != TUNE:
                continue
            if data[2] == TUNE_ERROR:
                raise ValueError(f"the keyboard does not know parameter {data[3]}")
            if data[2] == wanted:
                return data[3]

    def get(self, parameter: int) -> int:
        return self.request(parameter | TUNE_READ)

    def values(self) -> dict:
        measurement = self.get(MEASUREMENT)
        return {
            "speed": self.get(SPEED) * 5,
            "accel": self.get(ACCELERATION) / 100,
            "smooth": self.get(SMOOTHNESS) * 5,
            "measure": "fast" if measurement else "average",
        }


def show(values: dict) -> None:
    print(f"speed={values['speed']} accel={values['accel']:g} smooth={values['smooth']} measure={values['measure']}")
    print()
    print("To keep these values, put them into the first layer of layers_devterm.h and run make flash:")
    print()
    print(f"  TRACKBALL_SPEED({values['speed']});")
    print(f"  TRACKBALL_ACCELERATION({values['accel']:.2f}f);")
    print(f"  TRACKBALL_SMOOTHNESS({values['smooth']});")
    print(f"  TRACKBALL_MEASUREMENT(TRACKBALL_MEASURE_{values['measure'].upper()});")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("settings", nargs="*", metavar="SETTING",
                    help="name=value, 'preset NAME' or 'reload'")
    ap.add_argument("--device", "-d", metavar="PATH", help="hidraw device (default: auto-detect)")
    ap.add_argument("--force", action="store_true",
                    help="do not check that the keyboard is a DevTerm")
    args = ap.parse_args()

    # Check the command line before anything is sent
    commands: list[tuple[int, int]] = []
    words = list(args.settings)
    try:
        while words:
            word = words.pop(0)
            if word == "reload":
                commands.append((RELOAD, 0))
            elif word == "preset":
                if not words or words[0] not in PRESETS:
                    raise ValueError("preset must be followed by one of: " + ", ".join(PRESETS))
                for name, value in PRESETS[words.pop(0)].items():
                    commands.append(encode(name, str(value)))
            elif "=" in word:
                name, text = word.split("=", 1)
                commands.append(encode(name, text))
            else:
                raise ValueError(f"don't understand '{word}', see --help")
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 2

    device = args.device
    if not device:
        device, product = find_keyboard(DEVICE_VID, DEVICE_PID)
        if not device:
            print(f"Error: keyboard not found ({DEVICE_VID}:{DEVICE_PID})", file=sys.stderr)
            return 1
        if product != PRODUCT and not args.force:
            print(f"Error: the keyboard reports \"{product}\". The live tuning is only "
                  f"available on the {PRODUCT} keyboard.", file=sys.stderr)
            return 1

    try:
        keyboard = Keyboard(device)
    except PermissionError:
        print(f"Error: no access to {device}. Run this with sudo, or install "
              "tools/70-uconsole-keyboard.rules.", file=sys.stderr)
        return 1
    except OSError as e:
        print(f"Error: {device}: {e}", file=sys.stderr)
        return 1

    try:
        for parameter, value in commands:
            keyboard.request(parameter, value)
        show(keyboard.values())
    except TimeoutError:
        print("Error: no answer from the keyboard. Its firmware has no live tuning, "
              "flash the current firmware first.", file=sys.stderr)
        return 1
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    finally:
        keyboard.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
