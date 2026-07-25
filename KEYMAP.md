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

## Mouse cursor speed — 5 levels (persistent)
`MK_3_SPEED`, tap to lock a speed. **layer 1 · F1–F5**, ascending: **0.1× / 0.2× / 0.4× / 1× / 2×**.
F1–F3 are QMK built-ins (`KC_MS_ACCEL0/1/2`); F4/F5 are custom (`MS_ACC4/MS_ACC5`). Scroll-wheel
scales the same way. Speeds set in `q1_he/config.h` (`MK_C_OFFSET_*`).

## Auto mouse-shape mover — 10 shapes on the number row
One shape per **number key** on **layer 1 (Mac Fn)**: **tap to start**, **tap the same key again to
stop**. Only one runs at a time; starting another switches to it. Traversal speed follows the mouse
accel level (layer 1 · F1–F5); size is fixed. Paths live in `shp_pos()`; speed from
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
at any resolution** — no need to know the pixel size. Pace follows the mouse-accel level (F1–F5).

> Requires `DIGITIZER_ENABLE`/`DIGITIZER_SHARED_EP` (in `rules.mk`); the board exposes an extra
> absolute-pointer HID interface. It maps to the primary display. Starting a number-row shape or the
> name-drawing stops the bounce, and vice-versa.

## Draw 祥沣
**layer 1 · F10** (`MS_DRAW`) — tap to draw the two characters with the cursor, one stroke at a time.
It **holds the left mouse button during a stroke** and lifts between strokes, so run it inside a
**paint app**. Stroke order and shapes are hand-authored approximations. Tap again to stop.

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

## Custom keycodes
| Keycode | Location | Action |
|---|---|---|
| `MS_ACC4` | layer 1 · F4 | mouse speed 1.0× |
| `MS_ACC5` | layer 1 · F5 | mouse speed 2.0× |
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
| `MS_DRAW` | layer 1 · F10 | draw 祥沣 in a paint app (pen up/down per stroke) |
| `LT_CLEAR` | layer 3 · Backspace | clear the letter/marquee buffer |

Custom keycodes show as **"Unknown"** in VIA — don't remap those keys there or you lose the feature.

## Layer-3 notes (Win Fn)
- **Esc = `DF(1)`** (from the Launcher export; sets default layer → 1). *Not* the Keychron default
  (`_______`). Kept per user request.
- **`~` = `_______`** (transparent) — restored to the Keychron default.

## Build & flash
See [CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md).
