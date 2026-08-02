# Flappy (arcade game)

Flappy Bird on the RGB grid. Part of the on-keyboard arcade — open with **Fn + H**, turn the knob to
select **FLAPPY**, press the knob to start. Code lives in `src/arcade.c` (the `flappy_*` / `f_*` functions).

## How it plays
A **1×1 bird** (gold) sits at a fixed column (**col 2**) while green **pipes** scroll in from the
right. Each pipe has a **3-tall gap**; steer the bird through it. **+1** per pipe passed.

The bird **hovers in place until your first flap** (classic Flappy), so you get a moment to react. Each
flap gives an upward nudge; gravity pulls it back down.

- **Ceiling** — bonks harmlessly (the bird just can't go above row 0).
- **Floor / pipe** — game over. The floor is the **space row**: the bird can dip all the way to the
  **ZXCV row (row 4)** and survive; it only dies once it falls past row 5 (`f_bt > 5`).

## Field — scrolling
- **Rows**: pipes fill matrix **rows 0–4** (top = row 0); the bird's height `f_bt` is a float rounded to
  the lit row and may drop to **row 5** (space row) before the floor kills it.
- **Scroll**: pipes spawn off-screen at logical col 14 and move left; only cols **0–13** are drawn.
- Collision is purely logical (at col 2), so the missing key at (row 4, col 11) never matters.

## Controls
| Input | Action |
|---|---|
| **Knob press** | flap |
| **Any key** | also flaps |
| **Knob hold (~0.5 s)** | quit to the lobby |

## Physics (ported 1:1 from the prototype)
Frame-rate independent — each tick scales by real elapsed time `s = dt / 16.67`:
- gravity `f_vy += 0.0022·s`, terminal-capped at `0.13`; flap sets `f_vy = −0.066` (a **~1-block** lift).
- pipe speed starts `0.05` col/frame and creeps up (`+0.000012·dt`); a new pipe every **1600 ms** with
  a random gap position (`rnd()%3`, so gap spans rows `gap … gap+2`).

A gentle fall (~0.6 s floor-to-floor) that a tap keeps aloft.

## Rendering & score
- Pipes green, bird gold (`flappy_render`).
- **Score = pipes passed.** On game-over the board does the arcade **fill sweep**; **20 pipes fills the
  board** (`game_over(f_score, 2)` → `frac = pipes / 20`), colored by tier (bronze / cyan / gold).

## Tuning (in `src/arcade.c`)
- `0.0022f` gravity, `0.13f` terminal cap, `−0.066f` flap impulse (~1-block lift) — the feel.
- `1600` respawn ms, `0.05f` start speed, `0.000012f` ramp — pacing.
- Fill target `score / 20` in `game_over`.

See **[DINO.md](DINO.md)** and **[Q1_HE_CUSTOM_FIRMWARE.md](Q1_HE_CUSTOM_FIRMWARE.md)** for the arcade shell.
