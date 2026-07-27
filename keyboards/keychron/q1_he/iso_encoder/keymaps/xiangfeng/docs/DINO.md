# Dino (arcade game)

The Chrome offline dinosaur on the RGB grid. Part of the on-keyboard arcade — open with **Fn + H**,
turn the knob to select **DINO**, press the knob to start. Code lives in `src/arcade.c` (the `dino_*` /
`d_*` functions).

## How it plays
A **2-tall dino** (green) runs in place at a fixed column (**col 2**) along the **ground line**
(row 4). Obstacles scroll in from the right; clear them. **+1** per obstacle passed. One hit ends it.

Two obstacle kinds:
- **Bottom** (on the ground, row 4) → **jump** over it.
- **Top** (floating, row 3) → **duck** under it (the dino shrinks to a single ground cell).

~40 % of obstacles are top ones (`rnd()%5 < 2`).

## Field
- **5 rows**: matrix **rows 0–4**, ground at **row 4**. The dino's foot `d_bottom` is a float in
  `[1, 4]`; standing it occupies two rows, ducking just the ground row.
- **Scroll**: obstacles spawn off-screen at logical col 14 and move left; cols **0–13** are drawn.

## Controls
| Input | Action |
|---|---|
| **Space** | jump — tap clears **~2 rows**, **hold** (while rising) reaches **~3 rows** |
| **Ctrl** (either) | duck (while held, and only while grounded) |
| **Knob press** | jump too (tap height only — the knob can't hold-for-higher) |
| **Knob hold (~0.5 s)** | quit to the lobby |

> Jump over the **bottom** obstacles, **duck** under the **top** ones. **Tap Space** for a fixed ~2-row
> jump; **hold Space while falling** to **float down slower** (hang time to clear a wider gap), not to
> jump higher. Space = `matrix (5,6)`, Ctrl = `matrix (5,0)`/`(5,11)` — the arcade owns the board, so
> these don't type.

## Physics (ported from the prototype)
Frame-rate independent (`s = dt / 16.67`):
- gravity `d_vy += g·s` where `g = 0.03` normally, or **`0.0035` while *descending* (`d_vy > 0`) with Space
  held** — a strong float that roughly **doubles the air time / horizontal reach**. Rising always uses the
  full `0.03`, so the jump apex is fixed by the impulse `d_vy = −0.35` (grounded only) at **~2 rows**
  regardless of holding. **Release mid-float → gravity snaps back to `0.03`** (re-evaluated every frame), so
  you drop immediately when you let go.
- obstacle speed starts `0.06` col/frame and creeps up (`+0.000015·dt`); next obstacle in
  `850 + rnd()%450` ms.

## Rendering & score
- Ground line dim, bottom obstacles red, top obstacles orange, dino green (`dino_render`).
- **Score = obstacles passed.** On game-over the board does the arcade **fill sweep**; **25 obstacles
  fills the board** (`game_over(d_score, 3)` → `frac = passed / 25`), colored by tier.

## Tuning (in `src/arcade.c`)
- `0.03f` gravity, `−0.32f` jump impulse — the feel (lower gravity = more hang time).
- `rnd()%5 < 2` top-obstacle chance; `850 + rnd()%450` respawn ms; `0.06f` start speed.
- Fill target `score / 25` in `game_over`.

See **[FLAPPY.md](FLAPPY.md)** and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the arcade shell.
