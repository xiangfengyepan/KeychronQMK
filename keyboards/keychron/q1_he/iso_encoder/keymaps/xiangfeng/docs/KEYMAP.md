# Q1 HE (ISO) — Keymap & keys

Board `keychron/q1_he/iso_encoder`, keymap `keychron` (VIA). Notation throughout: **layer X · key Y**
(hold **Fn** to reach the Fn layer). Lighting effects are documented separately in
[LIGHTING_EFFECTS.md](LIGHTING_EFFECTS.md).

## Layers
| # | Name | Reached by |
|---|------|-----------|
| 0 | Mac base | default (Mac) |
| 1 | Mac Fn | hold Fn (Mac) |
| 2 | Win base | default (Windows) |
| 3 | Win Fn | hold Fn (Windows) |

The base layout is **baked from the user's Keychron Launcher export** (raw keycodes written into the
matrix), so a fresh flash boots with the configured layout; VIA/Launcher can still edit it live.

> **Mac vs Windows:** the custom mouse/shape keys below live on the **Mac Fn layer (1)**. In Windows
> mode Fn → layer 3, where those keys are stock RGB/profile keys — so they only fire in **Mac** mode.
> The marquee-clear is the exception (on **Win Fn**). Ask to mirror the cluster to layer 3 if needed.

## Mouse cursor speed — 4 levels (persistent)
`MK_3_SPEED`, tap to lock a speed. **layer 1 · F1–F4**, ascending: **0.1× / 0.4× / 1× / 2×**.
F1 / F2 are QMK built-ins (`KC_MS_ACCEL0` / `KC_MS_ACCEL2`); F3 / F4 are custom (`MS_ACC4` / `MS_ACC5`).
Scroll-wheel scales the same way. Speeds set in this folder's `config.h` (`MK_C_OFFSET_*`).
(The 0.2× level still exists internally but no key selects it; F5 is unmapped.)

## Auto mouse-shape mover — 10 shapes on the number row
One shape per **number key** on **layer 1 (Mac Fn)**: **tap to start**, **tap the same key again to
stop**. Only one runs at a time; starting another switches to it. Traversal speed follows the mouse
accel level (layer 1 · F1–F4); size is fixed. Paths live in `shp_pos()`; speed from
`mousekey_get_offset()`.

| Key | Shape | | Key | Shape |
|---|---|---|---|---|
| **layer 1 · 1** | ∞ infinity (horizontal) | | **layer 1 · 6** | ★ star (5-point) |
| **layer 1 · 2** | ● circle | | **layer 1 · 7** | ♥ heart |
| **layer 1 · 3** | ▲ triangle | | **layer 1 · 8** | spirograph |
| **layer 1 · 4** | ■ square | | **layer 1 · 9** | spiral |
| **layer 1 · 5** | ⬡ hexagon | | **layer 1 · 0** | lissajous (3:2) |

> **Why normal keys, not DKS?** Keychron's official DKS can only emit real keystrokes/modifiers
> (`report_action` in `action_okmc.c` only handles basic/modifier keycodes) — it can't trigger a
> firmware routine like a shape mover, and custom firmware can't appear in the Launcher. So the
> shapes are plain custom keycodes on the number row instead.
>
> **Dropped** (were redundant/weakest): vertical figure-8, wave, pentagon, rose.

## Full-screen DVD bounce — layer 1 · F9 (`MS_DVD`)
Tap **F9** to launch the classic bouncing-logo path; tap again to stop. Unlike the number-row
shapes (which move the cursor **relatively**), this uses the **absolute digitizer report**
(`digitizer_set_position`, x/y as screen fractions 0–1), so it bounces off the **real screen edges
at any resolution** — no need to know the pixel size. Pace follows the mouse-accel level (F1–F4).

> Requires `DIGITIZER_ENABLE`/`DIGITIZER_SHARED_EP` (in `rules.mk`); the board exposes an extra
> absolute-pointer HID interface. It maps to the primary display. Starting a number-row shape or an
> IME character draw stops the bounce, and vice-versa; **layer 3 · Space** (`MS_STOP`) stops it too.

## Pinyin IME — type a character, draw it with the mouse
**Fn + I** (`IME_TOGG`, on both Mac Fn layer 1 and Win Fn layer 3) toggles a baked **pinyin input
method**. While it's on, the keyboard is in *compose mode*:

| Key | Action |
|---|---|
| letters | type toneless pinyin (e.g. `feng`) — candidates load as you type (prefix match) |
| **← / →** (or **Tab**) | previous / next candidate |
| **F1–F12** | jump to that candidate — the F-row is a live candidate strip (see below) |
| **Space / Enter** | confirm — draws the current character with the mouse |
| **Backspace** | delete the last pinyin letter |
| **Esc** / **Fn + I** | exit IME |

The current candidate is **animated stroke-by-stroke across the RGB LEDs** (green trail, bright head);
a faint blue glow means IME is on but no match yet. Two on-key meters help you compose:
- **Number row = pinyin buffer length** — 1 letter lights `1`, 2 letters light `1`+`2`, … in green
  (up to `1–0` = 10), dark when the buffer is empty.
- **F-row = candidate strip** — the matched candidates light **F1…F12** (cyan), and the **currently
  selected** one is **magenta**. Press an **F-key to jump to that candidate** (updates the preview);
  Space/Enter still confirms. Matched by key *position*, so it works even though the Mac top row sends
  media keys. On **confirm**, the character is drawn with the
mouse (same engine as F10) — so **have a paint app focused**. IME stays on after a confirm so you can
type the next character; Esc or Fn+I leaves.

- **Dictionary:** 276 baked characters (`hanzi_data.c`) — the 12 most-common characters for each of the
  23 usable pinyin initials (all letters except i/u/v). Built from two open datasets: **stroke medians**
  from Make Me a Hanzi (<https://github.com/skishore/makemeahanzi>) and **frequency + pinyin** from
  hanziDB (<https://github.com/ruddfawcett/hanziDB.csv>). Full regeneration steps in [IME.md](IME.md).
- Your names are the **first candidate** for their pinyin: `feng`→沣, `pan`→潘, `ye`→叶, `xiang`→祥.
- ⚠️ **Low-res preview:** the LED grid is ~87 keys, so a complex character is a rough trace, not crisp
  — you'll rely partly on knowing the cycle order. Coverage is the 276 most-common characters (12 per pinyin initial), not a
  full IME. To add/adjust characters, regenerate `hanzi_data.c`.

## RGB adjust keys (on layer 1)
Step = **1** for all four (fine control); **hold to auto-repeat**; the board flashes **red** when a
setting hits min/max (hue wraps, so it never flashes).

| Action | Keys | | Action | Keys |
|---|---|---|---|---|
| Cycle effect next/prev | Q / A | | Brightness ± | W / S |
| Hue ± | E / D | | Speed ± | T / G |
| Saturation ± | R / F | | Toggle RGB | Tab |

See [LIGHTING_EFFECTS.md](LIGHTING_EFFECTS.md) for the full effect list and order.

## Clear the letter/marquee buffer
**layer 3 · Backspace** (`LT_CLEAR`) — wipes the buffer for the Letters Marquee / Big-letter effects.
Nothing clears automatically.

## Layer-change indicator
When the **default layer** changes (Fn + Esc → `DF()`), the whole board flashes **red for 1 s**, and
**during that second F1–F4 light green** as a layer meter — the count of green keys = the new layer:
Mac base = F1, Mac Fn = F1–F2, Win base = F1–F3, Win Fn = F1–F4. Both clear after the second (the
green is **not** persistent). **Fn + L** (`LAY_SHOW`) fires the same flash any time to *peek* the
layer without switching — tap and release Fn to read your base (Mac = 1 key, Win = 3). Separate from the short 140 ms **red** flash used for RGB min/max limits.

## Block / lock mode — Fn + Z
**layer 3 · Z** (`BLK_TOGG`) toggles a mode where **every keypress is swallowed** — nothing reaches the
PC (handy for wiping the board down). **Fn still works**, so **Fn + Z** again exits. While locked the
board shows a **dim amber wash** so the state is obvious.

## Arcade — Fn + H
**layer 1 · H / layer 3 · H** (`ARCADE`) opens a tiny arcade on the RGB grid. While open it **owns the
whole board and swallows all keys**; the **knob** is the shared control (turn = browse/rotate, tap =
start/action, **hold ~0.5 s = quit / exit**). Six games:

| Game | Play | Controls (beyond knob-hold = quit) | Doc |
|---|---|---|---|
| **Tetris** | fit falling tetrominoes | knob turn = rotate · PgUp/PgDn = move · Home = drop | [TETRIS.md](TETRIS.md) |
| **Topo** | whack-a-mole, sudden death | press the lit key | [TOPO.md](TOPO.md) |
| **Flappy** | fly through pipe gaps | knob tap **or any key** = flap | [FLAPPY.md](FLAPPY.md) |
| **Dino** | jump/duck past obstacles | **Space** = jump (tap 2 / hold 3) · **Ctrl** = duck · knob = jump | [DINO.md](DINO.md) |
| **Memory** | Simon on all keys | repeat the flashed sequence by pressing the keys | [MEMORY_GAME.md](MEMORY_GAME.md) |
| **Reaction** | reflex test | wait for green, hit **any key** fast; 3 rounds, avg out of 82 | [REACTION.md](REACTION.md) |

## Custom keycodes
| Keycode | Location | Action |
|---|---|---|
| `MS_ACC4` | layer 1 · F3 | mouse speed 1.0× |
| `MS_ACC5` | layer 1 · F4 | mouse speed 2.0× |
| `MS_SH1` | layer 1 · 1 | shape mover: ∞ infinity |
| `MS_SH2` | layer 1 · 2 | shape mover: circle |
| `MS_SH3` | layer 1 · 3 | shape mover: triangle |
| `MS_SH4` | layer 1 · 4 | shape mover: square |
| `MS_SH5` | layer 1 · 5 | shape mover: hexagon |
| `MS_SH6` | layer 1 · 6 | shape mover: star |
| `MS_SH7` | layer 1 · 7 | shape mover: heart |
| `MS_SH8` | layer 1 · 8 | shape mover: spirograph |
| `MS_SH9` | layer 1 · 9 | shape mover: spiral |
| `MS_SH0` | layer 1 · 0 | shape mover: lissajous |
| `MS_DVD` | layer 1 · F9 | full-screen DVD bounce (absolute digitizer) |
| `IME_TOGG` | layer 1 · I / layer 3 · I | toggle the pinyin IME (Fn+I) |
| `MS_STOP` | layer 3 · Space | stop any running mouse animation (shape / DVD bounce / IME draw) |
| `MS_BOOST` | layer 3 · LShift | **hold** to boost mouse speed to 1.0×; restores your speed on release |
| `BLK_TOGG` | layer 3 · Z | block/lock mode on/off — swallow all keys (nothing reaches the PC); **persists across power-off** |
| `LAY_SHOW` | layer 1 · L / layer 3 · L | peek the layer meter (1 s red flash + green F1–F4) **without** changing the layer |
| `ARCADE` | layer 1 · H / layer 3 · H | open the on-keyboard arcade (lobby → Tetris / Topo) |
| `LT_CLEAR` | layer 3 · Backspace | clear the letter/marquee buffer |

Custom keycodes show as **"Unknown"** in VIA — don't remap those keys there or you lose the feature.

## Layer-3 notes (Win Fn)
- **Ctrl / Alt = mouse buttons 4 / 5** (`KC_MS_BTN4` / `KC_MS_BTN5`, back / forward) — mirrors the Mac Fn layer.
- **Space = `MS_STOP`** (stop mouse animation) · **LShift = `MS_BOOST`** (hold-boost speed) · **I = `IME_TOGG`**.
- **Esc = `DF(1)`** (from the Launcher export; sets default layer → 1). *Not* the Keychron default
  (`_______`). Kept per user request.
- **`~` = `_______`** (transparent) — restored to the Keychron default.

## Build & flash
See [CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md).
