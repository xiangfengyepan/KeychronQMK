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

## Auto mouse-shape mover — DKS (press-depth)
Press a key to a **depth** and release; how deep you pressed picks the shape (**light → deep = 1st →
last**). Press to the active shape's depth again to stop. Uses the Hall-Effect travel reading
(`analog_matrix_get_travel`); traversal speed follows the accel level (F1–F5); size fixed.

| Key | light … deep |
|---|---|
| **layer 1 · F6** | vertical-8 · horizontal-∞ · wave · spiral |
| **layer 1 · F7** | circle · triangle · square · pentagon |
| **layer 1 · F8** | star · heart · rose · lissajous |
| **layer 1 · F9** | hexagon · DVD-bounce · spirograph |

> Depth bands read from **actuation → bottom**, so all four are easiest to reach on a
> **shallow-actuation** profile (e.g. the gaming / rapid-trigger one). On a deep 2.6 mm profile
> you'd only reach the last ~2. Paths live in `shp_pos()`; speed from `mousekey_get_offset()`.

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
| `MS_DK6` | layer 1 · F6 | DKS press-depth shapes: 8 / ∞ / wave / spiral |
| `MS_DK7` | layer 1 · F7 | DKS press-depth shapes: circle / triangle / square / pentagon |
| `MS_DK8` | layer 1 · F8 | DKS press-depth shapes: star / heart / rose / lissajous |
| `MS_DK9` | layer 1 · F9 | DKS press-depth shapes: hexagon / DVD / spirograph |
| `MS_DRAW` | layer 1 · F10 | draw 祥沣 in a paint app (pen up/down per stroke) |
| `LT_CLEAR` | layer 3 · Backspace | clear the letter/marquee buffer |

Custom keycodes show as **"Unknown"** in VIA — don't remap those keys there or you lose the feature.

## Layer-3 notes (Win Fn)
- **Esc = `DF(1)`** (from the Launcher export; sets default layer → 1). *Not* the Keychron default
  (`_______`). Kept per user request.
- **`~` = `_______`** (transparent) — restored to the Keychron default.

## Build & flash
See [CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md).
