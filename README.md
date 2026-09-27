# uConsole Keyboard Firmware by Alexey Cluster

This is a clean port of the official uConsole keyboard firmware from [ClockworkPi's repository](https://github.com/clockworkpi/uConsole/tree/master/Code/uconsole_keyboard), rewritten to use **pure STM32 HAL** instead of the messy Arduino/STM32duino framework.

The same firmware can also be built for the **ClockworkPi DevTerm** keyboard, see [DevTerm keyboard](#devterm-keyboard).


## Why This Exists

The original firmware relies on STM32duino and various Arduino libraries with complex dependencies. This makes it difficult to:
- Edit and compile the firmware directly on the uConsole itself
- Customize key mappings and functionality
- Understand what's actually happening under the hood
- Maintain and debug the code

This port completely eliminates all that Arduino nonsense and uses clean STM32 HAL code. Now you can easily edit, compile, and flash the keyboard firmware right on your uConsole, remapping keys and customizing functionality however you want.


## What's Included

**Fully ported features:**
- Full keyboard scanning
- Trackball handling
- Backlight brightness control via PWM with auto-dimming
- USB HID keyboard, mouse, and gamepad support
- USB Consumer controls (volume, brightness)

**What's changed/added compared to the original firmware:**
- **Layer system** - up to 10 configurable keyboard layouts, switchable via `LeftCtrl`+`RightCtrl`+`Number`
- **Horizontal scrolling** - Fn + Trackball scrolls horizontally as well as vertically
- **Keyboard backlight dimming after inactivity** - configurable in `config.h`
- **Trackball acceleration curve** - improved mouse control
- **Fn Lock** - toggle to permanently invert Fn for F1–F12 keys (press `Fn`+`Esc` to toggle; backlight blinks to confirm)


## Configuration

### Layers

The firmware supports up to **10 keyboard layers**. A layer is a complete set of key bindings and trackball settings. Think of it as separate keyboard profiles that you can switch between on the fly.

**Why is this useful?** You can have one layer for everyday typing, another optimized for gaming with gamepad button mappings and faster trackball, a third for a specific application, and so on — all without reflashing the firmware. Each layer can override any key, Fn combination, and trackball speed/acceleration independently.

**How to switch layers:** press `LeftCtrl`+`RightCtrl`+`Number` (e.g. `LeftCtrl`+`RightCtrl`+`1` for layer 1, `LeftCtrl`+`RightCtrl`+`2` for layer 2, `LeftCtrl`+`RightCtrl`+`3` for layer 3, etc.). The keyboard backlight will blink to indicate the layer number.

**Inheritance:** you only need to define what is *different* on each layer. Any key or setting not explicitly set on a layer is automatically inherited from layer 1. This means layer 2 can override just a handful of buttons and everything else will work exactly as on layer 1.

#### Default bindings

The firmware ships with three pre-configured layers. Layer 1 is the main layer with a standard QWERTY layout. Most keys are mapped as you would expect (Q→Q, W→W, etc.), so only the non-obvious bindings are listed below.

##### Layer 1 — non-standard key assignments

| Key | Action | Notes |
|-----|--------|-------|
| Select | Keypad − | Handy for file managers (MC, far2l) |
| Start | Keypad + | Handy for file managers (MC, far2l) |
| Volume | Volume Down | |
| Shift + Volume | Volume Up | |
| Trackball click | Mouse Middle | |
| GP A | Enter | |
| GP B | Left Meta/Cmd | Convenient for Meta+key shortcuts |
| GP X | Mouse Right Click | Use the mouse with one hand |
| GP Y | Mouse Left Click | Use the mouse with one hand |

##### Layer 1 — Fn combinations

| Fn + Key | Action | Notes |
|----------|--------|-------|
| Fn + Volume | Mute | |
| Fn + Esc | Fn Lock | Toggle F1–F12 without holding Fn |
| Fn + Tab | Caps Lock | |
| Fn + 1–0 | F1–F10 | |
| Fn + − | F11 | |
| Fn + = | F12 | |
| Fn + U | Page Up | |
| Fn + I | Insert | |
| Fn + H | Home | |
| Fn + J | End | |
| Fn + K | Page Down | |
| Fn + Backspace | Delete | |
| Fn + Space | Toggle backlight | |
| Fn + , | Brightness Down | |
| Fn + . | Brightness Up | |
| Fn + ↑ | Page Up | |
| Fn + ↓ | Page Down | |
| Fn + ← | Home | |
| Fn + → | End | |
| Fn + Select | Print Screen | |
| Fn + Start | Pause | |
| Fn + Left Alt | Application/Menu | |
| Fn + GP X | Mouse Forward | Useful in browsers and IDEs |
| Fn + GP Y | Mouse Back | Useful in browsers and IDEs |
| Fn + GP A | Keypad * | Handy for file managers (MC, far2l) |
| Fn + GP B | Keypad / | Handy for file managers (MC, far2l) |

##### Layer 2 — Game layer

Trackball speed is increased and **acceleration is disabled** on this layer (linear cursor movement). The four face buttons send **numeric keypad keys** in a numpad-like cluster (7 / 9 on the top row, 1 / 3 on the bottom), which pairs well with games or tools that expect keypad input. Fn combinations for these keys are **not** redefined here; they stay the same as on layer 1 unless you add `BIND_FN` overrides in `layers.h`.

| Key | Action |
|-----|--------|
| GP A | Keypad 9 |
| GP B | Keypad 7 |
| GP X | Keypad 3 |
| GP Y | Keypad 1 |

All other keys on layer 2 are inherited from layer 1.

##### Layer 3 — Gamepad layer

Same trackball tuning as layer 2 (higher speed, no acceleration). The following buttons are remapped to **virtual gamepad** controls; Fn combinations for these keys are disabled on this layer. The advantage of a virtual gamepad over keyboard keys is that there is **no limit on simultaneous button presses** — all gamepad buttons can be held at the same time, which is critical for gaming.

| Key | Action |
|-----|--------|
| Select | Gamepad Button 5 |
| Start | Gamepad Button 6 |
| ↑ ↓ ← → | Gamepad D-Pad |
| GP A | Gamepad Button 1 |
| GP B | Gamepad Button 2 |
| GP X | Gamepad Button 3 |
| GP Y | Gamepad Button 4 |

Switch to this layer with `LeftCtrl`+`RightCtrl`+`3`. All other keys on layer 3 are inherited from layer 1.

#### Customizing layers

All layer configuration lives in a single file: `layers.h` (`layers_devterm.h` for the DevTerm keyboard). To change bindings or add new layers, edit this file and rebuild the firmware.

**Basic syntax:**

Each layer is opened with **`LAYER(number, "Name")`**: the number is the layer index (1–10), and **`"Name"`** is a short human-readable label (for example `"Main"`, `"Game"`, `"Gamepad"`).

```c
LAYER(1, "Main");                       // Start defining layer 1

BIND(BUTTON_Q, KEY_Q);                  // Q key produces 'Q'
BIND(BUTTON_GAMEPAD_A, MOUSE_LEFT);     // GP A button produces mouse left click
BIND_FN(BUTTON_1, KEY_F1);              // Fn+1 produces F1
BIND_FN(BUTTON_ESC, SK_KEYBOARD_LOCK);  // Fn+Esc toggles Fn Lock
```

**Trackball settings (per layer):**

```c
TRACKBALL_SPEED(50);                          // Cursor speed divisor (higher = faster)
TRACKBALL_ACCELERATION(0.3f);                 // Acceleration exponent (0 = linear, higher = more exponential)
TRACKBALL_SCROLL_VERTICAL_SPEED(100);         // Vertical scroll speed
TRACKBALL_SCROLL_VERTICAL_ACCELERATION(0.3f); // Vertical scroll acceleration
TRACKBALL_SCROLL_HORIZONTAL_SPEED(100);       // Horizontal scroll speed
TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION(0.3f); // Horizontal scroll acceleration
```

**Adding a new layer** — just add a `LAYER(N, "Name")` block at the end of the file. You only need to specify the keys and settings that differ from layer 1:

```c
LAYER(4, "Custom");
TRACKBALL_SPEED(200);                   // Faster cursor on this layer
BIND(BUTTON_SPACE, KEY_ENTER);          // Space produces Enter
BIND_FN(BUTTON_SPACE, KEY_NONE);        // Disable Fn+Space on this layer
```

**Available key codes:** see `Core/Inc/hid_keyboard.h` for keyboard keys, `Core/Inc/hid_gamepad.h` for gamepad buttons, and `Core/Inc/keymaps.h` for button names. Mouse buttons: `MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`, `MOUSE_BACK`, `MOUSE_FORWARD`. Consumer keys: `CONSUMER_VOLUME_UP`, `CONSUMER_VOLUME_DOWN`, `CONSUMER_MUTE`, `CONSUMER_BRIGHTNESS_UP`, `CONSUMER_BRIGHTNESS_DOWN`. Special keys: `SK_FN_KEY`, `SK_KEYBOARD_LOCK` (Fn Lock), `SK_KEYBOARD_LIGHT` (toggle backlight). Use `KEY_NONE` to explicitly disable a key.

**Fn combination fallback:** if an Fn combination is not defined for a key, pressing Fn+key will simply produce the normal (non-Fn) action of that key.


##### Keyboard hardware layout

Keep in mind that the uConsole keyboard has two types of keys:

**Matrix keys (8x8 = 64 keys):** all letter, number, and symbol keys (Q, W, E, ..., 1, 2, 3, ..., Esc, Tab, Space, Enter, Backspace, etc.), as well as Select, Start, Volume, Fn Left, and Fn Right. These are wired in a matrix — the MCU scans rows and columns to detect which key is pressed. This is an efficient way to connect many keys with fewer GPIO pins, but it has a downside: when three or more matrix keys in certain combinations are pressed simultaneously, the controller may register a "phantom" key press that wasn't actually pressed. This is known as **ghosting**. In practice, this is rarely a problem during normal typing.

**Non-matrix keys (17 keys):** these are wired directly to individual GPIO pins on the MCU — each key has its own dedicated pin. This means they can all be pressed simultaneously without any ghosting or conflicts. The non-matrix keys are:

| Key | Notes |
|-----|-------|
| Shift Left, Shift Right | Modifier keys |
| Ctrl Left, Ctrl Right | Modifier keys |
| Alt Left, Alt Right | Modifier keys |
| ↑ ↓ ← → | Arrow keys / D-pad |
| GP A, GP B, GP X, GP Y | Gamepad buttons (top right side of the device) |
| Trackball click | Pressing the trackball |
| Mouse L, Mouse R | Mouse buttons (near the D-pad) |

This hardware design ensures that modifier keys (Shift, Ctrl, Alt) and gaming controls (D-pad, GP buttons) always register correctly, no matter how many of them are held at the same time.


### Other hardware and behavior settings: `config.h`

| Setting | Description |
|---------|-------------|
| `backlight_vals` | PWM values of the backlight levels that `SK_KEYBOARD_LIGHT` cycles through (maximum 2000) |
| `KEYBOARD_INITIAL_BACKLIGHT_VALUE_ID` | Backlight level after power-on (index into `backlight_vals`) |
| `KEYBOARD_BACKLIGHT_OFF_TIME` | Seconds of inactivity before backlight dims (0 = never) |
| `KEYBOARD_BACKLIGHT_DIM_OUT_DURATION` | Dim-out animation duration in ms |
| `KEYBOARD_BACKLIGHT_DIMMED_OUT_VALUE` | Backlight PWM value after dimming out |
| `KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL` | Resume backlight on trackball movement (0/1) |
| `REPLACE_DOUBLE_SEMICOLON_WITH_APOSTROPHE` | Double press of `;` is replaced with `'`, and `::` with `"` (0/1) |
| `DOUBLE_PRESS_TIME_MS` | Maximum time between two presses to count as a double press |
| `VERTICAL_SCROLL_INVERTED` | 0 = traditional scrolling, 1 = natural scrolling |
| `HORIZONTAL_SCROLL_INVERTED` | 0 = traditional scrolling, 1 = natural scrolling |


## DevTerm keyboard

The DevTerm keyboard uses the same MCU, the same pins, the same bootloader and the same USB IDs as the uConsole keyboard, so the firmware only needs a different key layout and trackball orientation. On a DevTerm the right firmware is built automatically, see [Building](#building).

> **Note:** DevTerm support is new and has only been tested briefly on a real DevTerm. Keep a way to recover at hand, see [Unbricking and recovery](#unbricking-and-recovery).

Everything in this document applies to the DevTerm as well, with these differences:

| Topic | uConsole | DevTerm |
|-------|----------|---------|
| Layer file | `layers.h` | `layers_devterm.h` |
| Arrow keys | Non-matrix keys, they are also the D-pad | Matrix keys `BUTTON_UP` etc. |
| D-pad | Same as the arrow keys | Separate non-matrix keys `BUTTON_GAMEPAD_UP`, `BUTTON_GAMEPAD_DOWN`, `BUTTON_GAMEPAD_LEFT`, `BUTTON_GAMEPAD_RIGHT` |
| Fn | Two matrix keys `BUTTON_FN_LEFT`, `BUTTON_FN_RIGHT` | One non-matrix key `BUTTON_FN` |
| Right Shift, Ctrl, Alt | Non-matrix keys | Matrix keys |
| Cmd key | None | `BUTTON_CMD`, bound to Right Meta |
| Mouse buttons | Left, right | Left, middle (`BUTTON_MOUSE_M`), right |
| Trackball click | Mouse Middle | Mouse Left |
| Pointer movement | Every pulse of the trackball moves the pointer at once | The pointer glides, see [Trackball of the DevTerm](#trackball-of-the-devterm) |
| Scroll speed | `100`, acceleration `0.5f` vertical and `0.3f` horizontal | `50`, acceleration `0.2f` |
| Keyboard backlight | Yes | No, `SK_KEYBOARD_LIGHT` and the backlight settings in `config.h` have no effect |
| USB product name | `uConsole` | `DevTerm` |

All other default bindings are the same as on the uConsole, including the Fn combinations, the game layer and the gamepad layer. On the gamepad layer the D-pad sends the gamepad directions and the arrow keys stay arrow keys.

### Trackball of the DevTerm

The trackball of the DevTerm produces only 1 to 6 pulses per swipe. To get an even movement out of that, the pointer glides like in the stock firmware: every pulse sets a speed, and the pointer moves on smoothly for a moment.

The default values are the behavior of the stock firmware. They are set per layer in `layers_devterm.h`:

```c
TRACKBALL_SPEED(100);           // Pointer speed in percent of the stock firmware
TRACKBALL_ACCELERATION(0.0f);   // 0 = the pointer follows the speed of the ball, higher = fast rolling is rewarded
TRACKBALL_SMOOTHNESS(100);      // How long the pointer glides after a pulse, in percent of the stock firmware
TRACKBALL_MEASUREMENT(TRACKBALL_MEASURE_AVERAGE);   // Or TRACKBALL_MEASURE_FAST
```

`TRACKBALL_MEASUREMENT` selects how the speed of the ball is measured. `TRACKBALL_MEASURE_AVERAGE` is the running average of the stock firmware. It needs about ten pulses to follow the speed, so a short flick is never recognized as fast. `TRACKBALL_MEASURE_FAST` knows the speed from the second pulse on. A fast flick goes much further with it, so use it with a speed of about 50.

**Live tuning.** You can try values without flashing:

```bash
sudo python3 tools/trackball_tune.py                        # Show the values in use
sudo python3 tools/trackball_tune.py speed=200              # Twice as fast as the stock firmware
sudo python3 tools/trackball_tune.py speed=50 measure=fast
sudo python3 tools/trackball_tune.py preset stock           # Presets: stock, faster, flick
sudo python3 tools/trackball_tune.py reload                 # Back to the values of the layer
```

The values take effect at once and are lost when you change the layer or restart the keyboard. The tool prints the lines for `layers_devterm.h`, put them there and run `make flash` to keep them.

Scrolling with Fn does not glide. It uses the `TRACKBALL_SCROLL_*` values like on the uConsole.

### Differences to the stock DevTerm firmware


- Scrolling is done with Fn + trackball, not with the middle mouse button + trackball.
- The switch on the back of the keyboard does not select between joystick and keyboard mode any more, use the layers instead.
- A layer change or Fn Lock is confirmed by blinking the keyboard backlight. The DevTerm has no backlight, so there is no visible confirmation.


## Building

### Prerequisites

On **Debian**, **Ubuntu**, and most derivatives, install everything in one go like this:

```bash
sudo apt update
sudo apt install build-essential gcc-arm-none-eabi dfu-util
```

### Build and flash

```bash
# Build the firmware
make all

# Clean build files
make clean

# First install from the factory keyboard firmware (run once)
make first_flash

# Later firmware updates
make flash
```

Use **`make first_flash`** the **first** time you install this firmware on a keyboard that still runs the original uConsole firmware. After that, use **`make flash`** for updates.

### uConsole or DevTerm

The commands above build the firmware for the keyboard of the device they run on. The keyboard is identified by its USB product name, `uConsole` or `DevTerm`, which works with the original firmware and with this one.

The uConsole firmware is written to `build/uconsole_keyboard.bin` and the DevTerm firmware to `build/devterm/devterm_keyboard.bin`.

Add `KEYBOARD=uconsole` or `KEYBOARD=devterm` to any command to choose the firmware yourself:

```bash
make all KEYBOARD=devterm
make flash KEYBOARD=devterm
```

You have to do this in two cases:

- **The keyboard can't be identified**, because it is already in bootloader mode, because no keyboard is connected, or because you are not on Linux. `make all` builds the uConsole firmware then. `make flash` and `make first_flash` stop and ask for `KEYBOARD`.
- **The keyboard runs the firmware of the other keyboard.** It reports the wrong product name then, so `make flash` stops to protect you. Add `FORCE=1` to replace the firmware anyway:

```bash
make flash KEYBOARD=devterm FORCE=1
```

The wrong firmware is not expected to do damage, but the keys will be mixed up until you flash the right one.


## Unbricking and recovery

For any of the steps below, you need a way to run shell commands on the host while the uConsole keyboard module may be unusable (DFU mode, reset, or other failure). Besides plugging in an **external USB keyboard**, you can use **SSH** from another machine on the network if the uConsole is already reachable and you can log in.

1. **Emergency software reset to bootloader** — If **this firmware is already running** but you need the bootloader (for example the host cannot drive the normal HID reboot path), hold **Left Ctrl** and **Right Ctrl**, then **press the trackball** (trackball click). The firmware handles this before any layer mapping in `non_matrix_action()` and calls `jump_to_bootloader()`, which arms the independent watchdog with a very short timeout and resets the MCU into the bootloader. This is intentionally hard to trigger by accident (both Ctrl keys plus a dedicated non-matrix key).

2. **Hardware switch** — If the MCU is not running this firmware, or the software method above does not help, use the switch on the **back of the keyboard module** to force **update / bootloader (DFU)** mode so the host sees the Maple/STM32 DFU device. Then run `make flash KEYBOARD=uconsole` or `make flash KEYBOARD=devterm` (or invoke `dfu-util` with the same options as in the Makefile). `KEYBOARD` is required here, because a keyboard in bootloader mode can't be identified. This works even when application firmware is missing or broken, as long as the bootloader is intact.


## License

Based on the original uConsole keyboard firmware by ClockworkPi.


## Support the Developer and the Project

* [GitHub Sponsors](https://github.com/sponsors/ClusterM)
* [Buy Me A Coffee](https://www.buymeacoffee.com/cluster)
* [Sber](https://messenger.online.sberbank.ru/sl/Lnb2OLE4JsyiEhQgC)
* [Donation Alerts](https://www.donationalerts.com/r/clustermeerkat)
* [Boosty](https://boosty.to/cluster)
