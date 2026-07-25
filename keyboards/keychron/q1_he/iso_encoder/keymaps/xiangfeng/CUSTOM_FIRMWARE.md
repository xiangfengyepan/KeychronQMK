# Keychron Q1 HE (ISO) — Custom Firmware

Custom build on top of the SRGBmods/Keychron QMK fork.
**Board:** `keychron/q1_he/iso_encoder` · **Keymap:** `keychron` (VIA-enabled) · **device_version:** 1.2.1

Build:

```bash
qmk compile -kb keychron/q1_he/iso_encoder -km keychron
# output: .build/keychron_q1_he_iso_encoder_keychron.bin
```

Flash the `.bin` with **QMK Toolbox** on Windows (DFU: switch to *Cable*, hold the reset button under the space bar or **Esc** while plugging in; WinUSB driver via Zadig once).

## Documentation
- **[KEYMAP.md](KEYMAP.md)** — layers, custom keys, mouse speed / shape movers / name-drawing, RGB adjust keys, and the custom-keycode table.
- **[LIGHTING_EFFECTS.md](LIGHTING_EFFECTS.md)** — the full RGB effect list and cycle order (including the custom effects 25–27).

This file is the overview of everything; the two above go deeper on keymap and lighting.

---

## 1. Typed-letter RGB effects
Two RGB matrix effects that render what you type using a 3×5 pixel font on the LED grid.
Source: `keyboards/keychron/common/rgb/letters.c` (registered in `common/rgb/rgb_matrix_kb.inc`).

| Effect | ID | Behaviour |
|--------|----|-----------|
| **Letters Marquee** | 25 | Your typed text scrolls right → left. Remembers the last **20** characters. |
| **Letters Big** | 26 | The last key you pressed is drawn large in the center, then fades. |
| **Spider-Man** | 27 | Red mask with two white angular eyes (occasional blink); each keypress fires a quick white web-burst. **Power-on default** (`RGB_MATRIX_DEFAULT_MODE`). Source: `common/rgb/spider_mask.c`. |

- Select by cycling RGB modes (they're the last two effects) or from the VIA Effect dropdown.
- Speed (marquee scroll) follows the global RGB speed (layer 1 · T / G).
- **Reset the buffer:** **layer 3 · Backspace** (`LT_CLEAR`, Windows Fn). Nothing clears automatically — the text stays until you clear it.
- Letters are intentionally coarse (one LED per staggered key).

## 2. Baked keymap
The `keychron` keymap (`.../keymaps/keychron/keymap.c`) is your exact **Keychron Launcher export** — 4 layers + encoder — written as raw keycodes into the matrix, so a fresh flash boots with your layout as the default. VIA/Launcher can still edit it live.

## 3. Mouse keys — 5 persistent speed levels
`MK_3_SPEED` extended from 3 → 5 levels (`quantum/mousekey.c`), **tap to lock** a speed (no holding).
On **layer 1, keys F1–F5** (hold Fn to reach layer 1), ascending:

| Key | Level | Speed |
|-----|-------|-------|
| layer 1 · F1 | acc0 | 0.1× |
| layer 1 · F2 | acc1 | 0.2× |
| layer 1 · F3 | acc2 | 0.4× |
| layer 1 · F4 | acc4 (`MS_ACC4`) | 1.0× |
| layer 1 · F5 | acc5 (`MS_ACC5`) | 2.0× |
| power-on default | — | 1.0× |

F1–F3 are QMK's built-in `KC_MS_ACCEL0/1/2`; F4/F5 are custom keycodes. Scroll-wheel speed scales to the same ratios. Values in `q1_he/config.h` (`MK_C_OFFSET_*`).

- **Auto mouse-shape mover** — 10 shapes on the **number row** (layer 1 · **1**–**0**): **tap to start**, tap the same key again to stop. One at a time. Speed follows the accel level (F1–F5); size fixed.
  - **1** ∞ infinity · **2** circle · **3** triangle · **4** square · **5** hexagon
  - **6** star · **7** heart · **8** spirograph · **9** spiral · **0** lissajous
  - Plain custom keycodes (not DKS): Keychron's official DKS can only emit real keystrokes, so it can't drive a firmware routine and custom firmware never appears in the Launcher. Dropped from the old set: vertical-8, wave, pentagon, rose. Relative movement in `housekeeping_task_user` (`host_mouse_send`); speed from `mousekey_get_offset()`.
- **Full-screen DVD bounce** — **layer 1 · F9** (`MS_DVD`): tap to launch a bouncing-logo path, tap to stop. Uses the **absolute digitizer** (`digitizer_set_position`, screen fractions 0–1) so it bounces off the **real screen edges at any resolution**. Needs `DIGITIZER_ENABLE`/`DIGITIZER_SHARED_EP`.

## 4. RGB adjust — fine step + hold-to-repeat
- **Step = 1** for Hue / Saturation / Brightness / Speed (finest control). Defined in `q1_he/config.h`.
- **Hold to repeat:** holding an adjust key ramps continuously (~28 ms/step) and saves on release. Tap = 1 nudge, hold = sweep. (`process_record_user` + `housekeeping_task_user` in `keymap.c`.)
- **Min/max feedback:** the board flashes **red** when **saturation / brightness / speed** reaches its limit. Hue wraps around, so it has no min/max and never flashes. (`rgb_matrix_indicators_advanced_user` in `keymap.c`.)

| Setting | Keys (layer 1) | Range | Step | Default |
|---------|---------|-------|------|---------|
| Hue | E / D | 0–255 (wraps) | 1 | 0 |
| Saturation | R / F | 0–255 | 1 | 255 |
| Brightness | W / S | 0–255 (off ≤ 48) | 1 | 255 |
| Speed | T / G | 0–255 | 1 | 127 |

## 5. Hall-Effect profiles (baked defaults)
Per-profile defaults set in `common/analog_matrix/profile.c` (`profile_reset`), keyed by `q1_he/config.h`.
Actuation/sensitivity units = 0.1 mm; travel range 0.5–4.0 mm.

| Profile (Launcher) | Index | Mode | Actuation | Rapid-Trigger sens. | Use |
|--------------------|-------|------|-----------|----------------------|-----|
| Profile 1 | 0 | Regular | 2.6 mm | — | Typing / programming |
| Profile 2 | 1 | Rapid Trigger | 1.2 mm | 0.2 mm | Valorant / gaming |
| Profile 3 | 2 | (unchanged) | — | — | Xbox gamepad mapping |

> These apply on an EEPROM/profile reset. **SOCD** (last-input wins on **A↔D** and **W↔S**) is baked into the gaming profile for clean counter-strafing. ⚠️ Snap-Tap-style SOCD is **banned in some titles** (CS2); Valorant hasn't explicitly banned it but Vanguard could treat it as a ToS violation — **account risk**. Per-key HE calibration stays per-unit (auto-calibrates).

## 6. Power-management timeouts (baked defaults)
In `q1_he/config.h` (seconds):

| Setting | Default | Stock |
|---------|---------|-------|
| Auto-sleep (`CONNECTED_IDLE_TIME`) | 600 (10 min) | 7200 (2 h) |
| Auto backlight-off (`CONNECTED_BACKLIGHT_DISABLE_TIMEOUT`) | 60 (1 min) | 600 (10 min) |

## 7. VIA definition
`keyboards/keychron/q1_he/via_json/q1_he_iso_encoder.json` adds **Letters Marquee (25)**, **Letters Big (26)** and **Spider-Man (27)** to the lighting Effect dropdown.

- The **Keychron Launcher has no Design tab** — load this definition in **[usevia.app](https://usevia.app)** → Settings → *Show Design tab* → Design → drop the JSON.
- Custom keycodes (`MS_ACC4/5`, `LT_CLEAR`) show as **Unknown** in VIA — don't remap those keys or you'll lose the feature.

---

## Custom keycodes
| Keycode | Location | Action |
|---------|----------|--------|
| `MS_ACC4` | layer 1 · F4 | mouse speed 1.0× |
| `MS_ACC5` | layer 1 · F5 | mouse speed 2.0× |
| `MS_SH1`…`MS_SH0` | layer 1 · 1–0 | shape movers: ∞ / circle / triangle / square / hexagon / star / heart / spirograph / spiral / lissajous |
| `MS_DVD` | layer 1 · F9 | full-screen DVD bounce (absolute digitizer) |
| `IME_TOGG` | layer 1/3 · I | toggle pinyin IME — type pinyin, cycle candidates (←/→ or Tab, or 1–9), Space/Enter confirms → draws the character with the mouse. 209-char baked dictionary (`hanzi_data.c`). |
| `LT_CLEAR` | layer 3 · Backspace | clear the letter/marquee buffer |

## Files changed
- `keyboards/keychron/common/rgb/letters.c`, `spider_mask.c` *(new)* + `rgb_matrix_kb.inc`, `rgb.mk` — letter + Spider-Man effects
- `keyboards/keychron/q1_he/iso_encoder/keymaps/keychron/keymap.c` — baked keymap, mouse levels, RGB hold-repeat, min/max red flash, LT_CLEAR
- `keyboards/keychron/q1_he/config.h` — timeouts, RGB steps, mouse speeds, HE profile defines
- `keyboards/keychron/common/analog_matrix/profile.c` — per-profile HE defaults
- `quantum/mousekey.c` — 3→5 speed levels + `mousekey_set_accel_level()`
- `keyboards/keychron/q1_he/via_json/q1_he_iso_encoder.json` — VIA effect list

## Activation note
Firmware code (letter effects, mouse mode, hold-repeat) works immediately after flashing. EEPROM-backed defaults (keymap, timeouts, HE profiles) load on an **EEPROM reset** — do one factory reset after flashing if they don't appear.

## Dropped
- **Aurora Bloom** effect (removed; didn't look good on hardware).
