# Keychron Q1 HE (ISO) — Custom Firmware

Custom build on top of the SRGBmods/Keychron QMK fork.
**Board:** `keychron/q1_he/iso_encoder` · **Keymap:** `xiangfeng` (VIA-enabled) · **device_version:** 1.2.1

The **entire custom build lives in this one keymap folder**
(`keyboards/keychron/q1_he/iso_encoder/keymaps/xiangfeng/`) — keymap, config, custom RGB
effects, the IME dictionary and these docs. Only three shared files keep small in-place patches
(`quantum/mousekey.c`, `common/analog_matrix/profile.c`, `common/keychron_raw_hid.c`; see *Files* below).

Build:

```bash
qmk compile -kb keychron/q1_he/iso_encoder -km xiangfeng
# output: .build/keychron_q1_he_iso_encoder_xiangfeng.bin
```

Flash the `.bin` with **QMK Toolbox** on Windows (DFU: switch to *Cable*, hold the reset button under the space bar or **Esc** while plugging in; WinUSB driver via Zadig once).

## Documentation
- **[KEYMAP.md](KEYMAP.md)** — layers, custom keys, mouse speed / shape movers, RGB adjust keys, the pinyin IME, and the custom-keycode table.
- **[IME.md](IME.md)** — the Fn+I pinyin input method: how it works end-to-end, the dictionary, and the code path.
- **[TETRIS.md](TETRIS.md)** / **[TOPO.md](TOPO.md)** / **[FLAPPY.md](FLAPPY.md)** / **[DINO.md](DINO.md)** / **[MEMORY_GAME.md](MEMORY_GAME.md)** / **[REACTION.md](REACTION.md)** — the six on-keyboard arcade games (Fn+H).
- **[LIGHTING_EFFECTS.md](LIGHTING_EFFECTS.md)** — the full RGB effect list and cycle order (including the custom effects 25–31).
- **[MEMORY.md](MEMORY.md)** — flash / RAM / EEPROM storage map and the firmware size breakdown.

This file is the overview of everything; the others go deeper.

---

## 1. Typed-letter RGB effects
Two RGB matrix effects that render what you type using a 3×5 pixel font on the LED grid.
Source: `src/letters.c` (registered as **USER** effects in `rgb_matrix_user.inc`).

| Effect | ID | Behaviour |
|--------|----|-----------|
| **Letters Marquee** | 25 | Your typed text scrolls right → left. Remembers the last **20** characters. |
| **Letters Big** | 26 | The last key you pressed is drawn large in the center, then fades. |
| **Spider-Man** | 27 | Red mask with two white angular eyes (occasional blink); each keypress fires a quick white web-burst. **Power-on default** (`RGB_MATRIX_DEFAULT_MODE`). Source: `src/spider_mask.c`. |
| **Claude crab** | 28 | A hand-painted crab (from the key-painter tool) scuttles the dark board — **orange** shell (`COL_ORANGE`), **red** eyes (`COL_RED`), wiggling legs; press a key on its shell and it **stops** for a beat, then skitters off. Source: `src/crab.c`. |
| **Palette** | 29 | Calibration tool: fills the board with one named color at a time. Turn the **knob** for next/prev swatch, HSV adjust keys to fine-tune (kept in RAM per swatch), **tap the knob** to reset a swatch, **hold the knob** to type its `H,S,V` out over USB. Colors live in `include/palette.h`. Source: `src/palette.c`. |
| **Pressure Heatmap** | 30 | Analog effect: dark board; each key glows by its **live Hall-effect travel** (how far it's pressed), spreading heat to neighbors with falloff (deeper = spreads farther), cooling back to black on release. Cool→hot thermal ramp; dims with Fn+W/S; **cool-down rate = RGB speed (Fn+T/G)**. Reads `analog_matrix_get_travel()`. Source: `src/heatmap.c`. |
| **Audio** | 31 | PC audio-spectrum visualizer: vertical EQ bars (green→yellow→red) driven by the `~/audio-keyboard` companion app over Raw HID (command `0xAC`, dispatched by Keychron's `kc_raw_hid_rx` → `kc_custom_hid_rx` weak hook). Bars fall to black with no app; dims with Fn+W/S. Source: `src/audio.c`. |

- Select by cycling RGB modes (they're the last seven effects) or from the VIA Effect dropdown.
- Speed (marquee scroll) follows the global RGB speed (layer 1 · T / G).
- **Reset the buffer:** **layer 3 · Backspace** (`LT_CLEAR`, Windows Fn). Nothing clears automatically — the text stays until you clear it.
- Letters are intentionally coarse (one LED per staggered key).

## 2. Baked keymap
The `xiangfeng` keymap (`keymap.c`) is your exact **Keychron Launcher export** (`launcher-export.json`) — 4 layers + encoder — written as raw keycodes into the matrix, so a fresh flash boots with your layout as the default. VIA/Launcher can still edit it live.

## 3. Mouse keys — 4 persistent speed levels
`MK_3_SPEED` (`quantum/mousekey.c`), **tap to lock** a speed (no holding).
On **layer 1, keys F1–F4** (hold Fn to reach layer 1), ascending:

| Key | Level | Speed |
|-----|-------|-------|
| layer 1 · F1 | acc0 | 0.1× |
| layer 1 · F2 | acc2 | 0.4× |
| layer 1 · F3 | acc4 (`MS_ACC4`) | 1.0× |
| layer 1 · F4 | acc5 (`MS_ACC5`) | 2.0× |
| power-on default | — | 1.0× |

F1 / F2 are QMK's built-in `KC_MS_ACCEL0` / `KC_MS_ACCEL2`; F3 / F4 are custom keycodes. Scroll-wheel speed scales to the same ratios. Values in this folder's `config.h` (`MK_C_OFFSET_*`). (The 0.2× level still exists in `config.h` but no key selects it; F5 is unmapped.)

- **Auto mouse-shape mover** — 10 shapes on the **number row** (layer 1 · **1**–**0**): **tap to start**, tap the same key again to stop. One at a time. Speed follows the accel level (F1–F4); size fixed.
  - **1** ∞ infinity · **2** circle · **3** triangle · **4** square · **5** hexagon
  - **6** star · **7** heart · **8** spirograph · **9** spiral · **0** lissajous
  - Plain custom keycodes (not DKS): Keychron's official DKS can only emit real keystrokes, so it can't drive a firmware routine and custom firmware never appears in the Launcher. Dropped from the old set: vertical-8, wave, pentagon, rose. Relative movement in `housekeeping_task_user` (`host_mouse_send`); speed from `mousekey_get_offset()`.
- **Full-screen DVD bounce** — **layer 1 · F9** (`MS_DVD`): tap to launch a bouncing-logo path, tap to stop. Uses the **absolute digitizer** (`digitizer_set_position`, screen fractions 0–1) so it bounces off the **real screen edges at any resolution**. Needs `DIGITIZER_ENABLE`/`DIGITIZER_SHARED_EP`.
- **Stop all mouse animation** — **Fn + Space** (`MS_STOP`, on **both** layer 1 Mac-Fn and layer 3 Win-Fn): one press halts whatever mouse routine is running — a shape mover, the DVD bounce, or an in-progress IME character draw — and releases the left button if a stroke was mid-draw.
- **Momentary speed boost** — **layer 3 · LShift** (`MS_BOOST`, Windows Fn + LShift): **hold** to jump the mouse speed to 1.0×, release to restore whatever speed you had. Affects the cursor and every mouse animation (shapes / DVD / IME draw). Uses `mousekey_get_accel_level()` (a small getter added to the `quantum/mousekey.c` patch) to save/restore.
- **Mouse buttons 4 / 5** — **layer 3 · Ctrl / Alt** now send `KC_MS_BTN4` / `KC_MS_BTN5` (back / forward), mirroring the same keys on the Mac Fn layer (layer 1).

## 4. RGB adjust — fine step + hold-to-repeat
- **Step = 1** for Hue / Saturation / Brightness / Speed (finest control). Defined in this folder's `config.h`.
- **Hold to repeat:** a **tap = exactly 1 step**; holding **past ~350 ms** starts auto-repeat (~28 ms/step) for a smooth sweep, saved on release. The 350 ms delay (`RGB_HOLD_DELAY`) stops a normal tap from firing several steps. (`process_record_user` + `housekeeping_task_user` in `keymap.c`.)
- **Min/max feedback:** the board **blinks red** (~110 ms on / off) while **saturation / brightness / speed** is pinned at its limit — a blink, not a solid hold, so it's obvious when held. Hue is cyclic, so instead the board **blanks (goes dark) once each time it passes through 0** — hue 0 is red, so a red flash there wouldn't show. (`rgb_matrix_indicators_advanced_user` in `keymap.c`, driven by a small `flash_kind` state machine.)
- **Layer-change indicator:** changing the default layer (Fn+Esc) flashes the board **red for 1 s**, and during that second **F1–F4 glow green** as a meter of the new layer (1–4 keys = layer 0–3). Both clear after 1 s — no persistent light.

| Setting | Keys (layer 1) | Range | Step | Default |
|---------|---------|-------|------|---------|
| Hue | E / D | 0–255 (wraps) | 1 | 0 |
| Saturation | R / F | 0–255 | 1 | 255 |
| Brightness | W / S | 0–255 (off ≤ 48) | 1 | 255 |
| Speed | T / G | 0–255 | 1 | 127 |

## 5. Hall-Effect profiles (baked defaults)
Per-profile defaults set in `common/analog_matrix/profile.c` (`profile_reset` — a **kept in-place patch**),
gated by the `*_PROFILE_INDEX` defines in this folder's `config.h`.
Actuation/sensitivity units = 0.1 mm; travel range 0.5–4.0 mm.

| Profile (Launcher) | Index | Mode | Actuation | Rapid-Trigger sens. | Use |
|--------------------|-------|------|-----------|----------------------|-----|
| Profile 1 | 0 | Regular | 2.6 mm | — | Typing / programming |
| Profile 2 | 1 | Rapid Trigger | 1.2 mm | 0.2 mm | Valorant / gaming |
| Profile 3 | 2 | (unchanged) | — | — | Xbox gamepad mapping |

> These apply on an EEPROM/profile reset. **Quick-switch keys** on layer 3 (Windows Fn): **Z** = `PROF1` (Profile 1, Regular), **X** = `PROF2` (Profile 2, Rapid Trigger), **C** = `PROF3` (Profile 3). **SOCD** (last-input wins on **A↔D** and **W↔S**) is baked into the gaming profile for clean counter-strafing. ⚠️ Snap-Tap-style SOCD is **banned in some titles** (CS2); Valorant hasn't explicitly banned it but Vanguard could treat it as a ToS violation — **account risk**. Per-key HE calibration stays per-unit (auto-calibrates).

## 6. Power-management timeouts (baked defaults)
In this folder's `config.h` (seconds):

| Setting | Default | Stock |
|---------|---------|-------|
| Auto-sleep (`CONNECTED_IDLE_TIME`) | 600 (10 min) | 7200 (2 h) |
| Auto backlight-off (`CONNECTED_BACKLIGHT_DISABLE_TIMEOUT`) | 60 (1 min) | 600 (10 min) |

## 7. VIA definition
This folder's **`usevia-definition.json`** adds **Letters Marquee (25)**, **Letters Big (26)** and **Spider-Man (27)** to the lighting Effect dropdown. (The keyboard's own `via_json` is back to stock, so use this copy.)

- The **Keychron Launcher has no Design tab** — load this definition in **[usevia.app](https://usevia.app)** → Settings → *Show Design tab* → Design → drop the JSON.
- Custom keycodes (`MS_ACC4/5`, `MS_SH*`, `MS_DVD`, `IME_TOGG`, `MS_STOP`, `MS_BOOST`, `BLK_TOGG`, `LAY_SHOW`, `ARCADE`, `LT_CLEAR`) show as **Unknown** in VIA — don't remap those keys or you'll lose the feature.

## 8. On-keyboard arcade — Fn + H
A tiny arcade rendered on the RGB grid (`src/arcade.c`). **Fn + H** opens the **lobby**; while it's open
the arcade owns the whole board and **swallows all keys**. The **knob is the dial**:

| Knob | Action |
|---|---|
| **turn** | browse games (lobby) / rotate the piece CW·CCW (Tetris) |
| **tap** | start the selected game / return to lobby from the score screen / **flap (Flappy), jump (Dino)** |
| **hold ~0.5 s** | quit a game → lobby; hold in the lobby → **exit the arcade** |

In-game the swallowed keys become controls too: **any key = flap** (Flappy), **Space = jump / Ctrl =
duck** (Dino), press the **lit key** (Topo), **repeat the flashed keys** (Memory), or **hit any key**
(Reaction).

Flow: **lobby** (game name animates letter-by-letter, a 5×5 LED font) → **3× red countdown** → game →
**score fill** (lights the board top-left → down; bronze / cyan / gold by score; max 82 keys) → knob-tap
or **10 s idle** returns to the lobby. Six
games:

| Game | One-liner | Doc |
|---|---|---|
| **Tetris** | 5×13 well, knob rotates, PgUp/PgDn move, Home drops | [TETRIS.md](TETRIS.md) |
| **Topo** | whack-a-mole, sudden death, exponential spawn ramp | [TOPO.md](TOPO.md) |
| **Flappy** | fly through pipe gaps; knob tap or any key = flap | [FLAPPY.md](FLAPPY.md) |
| **Dino** | Space = jump (tap 2 / hold 3), Ctrl = duck | [DINO.md](DINO.md) |
| **Memory** | Simon on all 82 keys; repeat the flashed sequence | [MEMORY_GAME.md](MEMORY_GAME.md) |
| **Reaction** | wait for green, hit any key fast; 3 rounds, avg / 82 | [REACTION.md](REACTION.md) |

---

## Custom keycodes
| Keycode | Location | Action |
|---------|----------|--------|
| `MS_ACC4` | layer 1 · F3 | mouse speed 1.0× |
| `MS_ACC5` | layer 1 · F4 | mouse speed 2.0× |
| `MS_SH1`…`MS_SH0` | layer 1 · 1–0 | shape movers: ∞ / circle / triangle / square / hexagon / star / heart / spirograph / spiral / lissajous |
| `MS_DVD` | layer 1 · F9 | full-screen DVD bounce (absolute digitizer) |
| `IME_TOGG` | layer 1/3 · I | toggle pinyin IME — type pinyin, cycle candidates (←/→ or Tab, or 1–9), Space/Enter confirms → draws the character with the mouse. 276-char baked dictionary — 12 common chars per pinyin initial (`src/hanzi_data.c`). |
| `MS_STOP` | layer 1 · Space **and** layer 3 · Space | stop any running mouse animation (shape mover / DVD bounce / IME draw); releases the button if mid-stroke |
| `MS_BOOST` | layer 3 · LShift | **hold** to boost mouse speed to 1.0×; restores the prior speed on release |
| `BLK_TOGG` | layer 3 · < (ISO key left of Z) | block/lock mode — swallow all keys; Fn+< again exits; dim amber wash; **persists across power-off** |
| `LAY_SHOW` | layer 1 · L / layer 3 · L | peek the layer meter (1 s flash + green F1–F4) without changing the layer |
| `ARCADE` | layer 1/3 · H | open the on-keyboard arcade (lobby → Tetris / Topo / Flappy / Dino / Memory / Reaction); knob-hold to exit |
| `LT_CLEAR` | layer 3 · Backspace | clear the letter/marquee buffer |

## Files
Everything lives in **`keymaps/xiangfeng/`**. The custom C sources sit in a **`src/`** subfolder and the headers in **`include/`** (only `keymap.c` and `rgb_matrix_user.inc` must stay in the keymap root):
- `keymap.c` — baked keymap, mouse-speed levels, shape movers, DVD bounce, RGB hold-repeat + min/max & layer flash, the pinyin IME, `MS_STOP`, block/lock mode, arcade hooks
- `config.h` — timeouts, RGB steps, mouse speeds, HE-profile defines, default effect
- `rules.mk` — VIA + digitizer + custom **USER** RGB effects + `SRC` list (points at `src/*.c`)
- `rgb_matrix_user.inc` — registers the custom USER RGB effects
- **`src/`** — custom C sources:
  - `src/hanzi_data.c` — the 276-char pinyin → stroke-median dictionary (12 common chars per initial); built from [makemeahanzi](https://github.com/skishore/makemeahanzi) (strokes) + [hanziDB.csv](https://github.com/ruddfawcett/hanziDB.csv) (frequency/pinyin) — see [IME.md](IME.md)
  - `src/arcade.c` — the on-keyboard arcade (lobby, countdown, Tetris, Topo, Flappy, Dino, Memory, Reaction, score)
  - `src/letters.c`, `src/spider_mask.c`, `src/crab.c`, `src/palette.c`, `src/heatmap.c`, `src/audio.c` — custom RGB effects (`src/audio.c` also overrides the `kc_custom_hid_rx` hook to receive the audio companion app's Raw HID stream)
- **`include/`** — headers: `palette.h` (15 named `COL_*` color constants, the single source of color for the effects/indicators/game palettes), `arcade.h`, `hanzi_data.h`, `tetris.h`
- `usevia-definition.json`, `launcher-export.json` — VIA / Launcher references
- docs: this file, `KEYMAP.md`, `IME.md`, `TETRIS.md`, `TOPO.md`, `FLAPPY.md`, `DINO.md`, `MEMORY_GAME.md`, `REACTION.md`, `LIGHTING_EFFECTS.md`, `MEMORY.md`

Three shared files keep **small in-place patches** (they can't live in a keymap folder):
- `quantum/mousekey.c` — 3 → 5 speed levels + `mousekey_set_accel_level()` / `mousekey_get_offset()` / `mousekey_get_accel_level()`
- `keyboards/keychron/common/analog_matrix/profile.c` — baked HE profiles + SOCD (`profile_reset`)
- `keyboards/keychron/common/keychron_raw_hid.c` — one `case 0xAC` + a weak `kc_custom_hid_rx()` hook so the keymap can receive a custom Raw HID stream (the audio visualizer) without touching VIA/Launcher

## Activation note
Firmware code (letter effects, mouse mode, hold-repeat) works immediately after flashing. EEPROM-backed defaults (keymap, timeouts, HE profiles) load on an **EEPROM reset** — do one factory reset after flashing if they don't appear.

## Dropped
- **Aurora Bloom** effect (removed; didn't look good on hardware).
