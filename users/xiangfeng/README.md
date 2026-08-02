# Reusable QMK custom-firmware modules

A QMK **userspace** of drop-in modules — custom RGB effects, an on-keyboard arcade, a pinyin
IME, RGB-adjust feedback, a mouse-animation engine, and small helpers — that work on **any
keyboard with `rgb_matrix` + an encoder**. The same code runs on two boards:

| Keyboard | Keymap / userspace | Board-specific doc |
|---|---|---|
| Keychron **Q1 HE** (ISO) | `xiangfeng` | [docs/Q1_HE_CUSTOM_FIRMWARE.md](docs/Q1_HE_CUSTOM_FIRMWARE.md) |
| Monsgeek **M1 V5** (ISO, wireless) | `xiangjun` | `users/xiangjun/docs/M1_V5_CUSTOM_FIRMWARE.md` |

This README covers the **reusable** parts. Everything board-specific (baked keymap, HE profiles,
wireless keys, power timeouts, flashing) lives in each board's own doc.

## The modules (in `src/` + `include/`)
| Module | What it is | Portable? |
|---|---|---|
| **rgbfx** (`src/rgbfx`) | RGB-adjust feedback: hold-to-repeat on the hue/sat/val/speed keys, blink at min/max, blackout on hue-wrap-through-0, the **binary value readout** on the number row, and the Palette knob "type H,S,V" gesture | ✅ any rgb_matrix board |
| **palette** (`src/palette.c`) | Calibration RGB effect: fills the board with one named `COL_*` swatch at a time; knob = next/prev, tap = reset, hold = type the H,S,V over USB | ✅ |
| **arcade** (`src/arcade.c` + `src/arcade/`) | 8 games on the LED grid (Tetris, Topo, Flappy, Dino, Memory, Reaction, Drop-Merge, Pong); knob is the dial | ✅ (positions map through `g_led_config`) |
| **ime** (`src/ime/`) | Baked pinyin IME (compose mode): type pinyin → pick a candidate → the character is *drawn with the mouse*. 276-char dictionary | ✅ (needs **mouse** for the draw engine) |
| **mouse** (`src/mouse/`) | Mouse-animation engine: shape movers, full-screen DVD bounce (absolute digitizer), and the shared "draw a glyph with the cursor" tracer used by the IME | ✅ (DVD bounce needs `DIGITIZER_ENABLE`) |
| **effects** (`src/letters.c`, `spider_mask.c`, `crab.c`, `cat_mask.c`) | Hand-painted / typed-letter RGB effects (positional `f(x,y,t)→RGB` fields) | ✅ (art is tuned to a 224×64 canvas; retune per board) |
| **utils** (`src/utils`) | `pal_rgb()` (HSV→RGB at a brightness) + `pal_put_u8()` (uint8→decimal string) | ✅ |
| **heatmap** (`src/heatmap.c`) | Pressure heatmap from live key travel | ❌ **Hall-effect only** (`analog_matrix_get_travel`) |
| **audio** (`src/audio.c`) | PC audio-spectrum visualizer over Raw HID | ❌ needs a board raw-HID hook + the companion app |

Named colors live in `include/palette.h` (15 `COL_*` HSV constants) — the single source of color
for the effects, keyboard indicators, and arcade palettes.

## Reusing this on a new keyboard
1. **Copy** `users/<name>/` into the target repo's `users/` (it's a plain copy, so boards can diverge).
2. **Name the keymap `<name>`** (or set `USER_NAME = <name>` in the keymap's `rules.mk`) — QMK then
   auto-includes the userspace (adds it to `VPATH` + the `-I` path, so `#include "include/foo.h"` resolves).
3. In the keymap's **`rules.mk`**, enable only the modules you want. The userspace `rules.mk` is
   **per-module opt-in** — e.g. `XIANGJUN_ARCADE = yes`, `XIANGJUN_IME = yes`. (The M1 V5 keymap
   shows the full pattern; the Q1 HE includes them all.) Dependencies: **ime needs mouse**; the
   custom RGB effects need `RGB_MATRIX_CUSTOM_USER = yes`.
4. Provide the keymap-level glue QMK requires in the keymap folder:
   - **`rgb_matrix_user.inc`** — `RGB_MATRIX_EFFECT(...)` + a one-line `extern` wrapper per custom effect.
   - **`config.h`** — define `KEYCHRON_RGB_ENABLE` (a misnomer meaning "this board has rgb_matrix";
     it's the guard the effect files use) and any `RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL` rgbfx needs.
   - **The three QMK hooks** in `keymap.c` calling the module APIs: `process_record_user`
     (arcade / IME / palette-knob / rgbfx-adjust dispatch), `housekeeping_task_user` *or* the RGB
     indicator hook (per-tick tasks), and `rgb_matrix_indicators_advanced_user` (render). Copy the
     structure from either board's `keymap.c`.

## Portability caveats
- **heatmap** requires Hall-effect analog switches; **audio** requires a per-board raw-HID hook
  (`kc_custom_hid_rx` on the Keychron) + the `~/audio-keyboard` companion app — leave both OFF on
  boards without that hardware.
- The custom effects' **art is tuned to a 224×64 LED canvas**; on a different board the
  arcade/IME/palette/binary-readout positions land on different keys and want re-mapping.
- Some boards misuse keymap-level hook names (`process_record_user`, the indicator hook) at the
  *keyboard* level — those need a small in-place board-file patch before a keymap can hook in
  (see the M1 V5 doc).
