# Tetris (arcade game)

Tetris rendered on the RGB grid, played with the board held **vertically**. Part of the on-keyboard
arcade — open with **Fn + H**, turn the knob to select **TETRIS**, press the knob to start. Code lives
in `arcade.c` (the `tetris_*` / `t*` functions).

## The well — a true 5 × 13 grid
Every one of the 65 grid cells maps to a real key, so pieces are always solid and fall to a solid floor.
The mapping is `(fall f, across a)` → matrix key `(row a, mcol(a, f))` where

```
mcol(a, f) = (a == 4 && f >= 11) ? f + 1 : f
```

- **Width = 5** (across): matrix **rows 0–4**. This is the direction you *move* the piece.
- **Length = 13** (fall): the direction pieces *fall* (left → right; "down" when the board is held
  vertical). For rows 0–3 fall index `f` is just matrix col `f` (cols 0–12). The floor is fall 12.

The full grid:

```
 fall→  f0   f1  f2  f3  f4  f5  f6  f7  f8  f9  f10  f11  f12
 a0 :  Esc   F1  F2  F3  F4  F5  F6  F7  F8  F9  F10  F11  F12
 a1 :   ~    1   2   3   4   5   6   7   8   9   0    -    =
 a2 :  Tab   Q   W   E   R   T   Y   U   I   O   P    [    ]
 a3 :  Caps  A   S   D   F   G   H   J   K   L   ;    '    \  (ISO NUHS key)
 a4 :  LSh   \   Z   X   C   V   B   N   M   ,   .    /   RSh
```

(`a3·f12` is the ISO **Non-US #/~** key, `KC_NUHS` — printed `\` on these keycaps; the dedicated
Non-US `\`/`|` key, `KC_NUBS`, is the separate one at `a4·f1`, beside Left Shift.)

> **The one keyless spot.** The board has exactly one hole: **(row 4, col 11)** — the ZXCV row jumps
> `.` → `/`. Only that row is short a key, so **only row 4** steps over it: its last two cells (`f11`,
> `f12`) map to matrix cols **12 (`/`)** and **13 (`RShift`)** instead of 11/12. Rows 0–3 keep their real
> col-11 key (F11 / - / [ / '), so **no piece ever shows a dark hole**, every one of the 13 columns is a
> full 5 tall (lines clear), and there is no wall to trap a piece. Physically `/` and `RShift` sit right
> under `'` / `#`, so the step is invisible.
>
> (Earlier attempts skipped col 11 on *every* row — which punched a dark hole through any piece crossing
> the seam — or walled the keyless cell, which trapped pieces mid-field. Per-row `mcol` avoids both.)

A **line clears** when a full cross-section fills — here, all 5 across-cells at one fall position.
Cleared lines shift the stack toward the floor and **gravity speeds up** (−18 ms per line, floor 140 ms).

## Controls
| Input | Action |
|---|---|
| **Knob turn CW / CCW** | rotate the piece clockwise / counter-clockwise (with wall-kicks) |
| **PgUp / PgDn** | move the piece across the well (toward row 0 / row 4) |
| **Home** | hard-drop |
| **Knob press** | *nothing* (rotation is on the turn) |
| **Knob hold (~0.5 s)** | quit to the lobby |

## Mechanics
- **Pieces:** all 7 tetrominoes (`PIECES[7][4][4]`), drawn from a shuffled **7-bag** (`next_piece`) so
  you get a fair spread. Standard Tetris colors (`PC[]`).
- **Rotation:** simple CW/CCW with a small wall-kick set `{0, −1, +1, −2, +2}` (`trot`).
- **Gravity:** `grav_ms` starts 650 ms and drops 18 ms per cleared line (min 140 ms).
- **Game over:** a freshly spawned piece that can't fit → the **score screen**.

## Rendering & score
- The well is drawn each frame: filled cells in their piece color, empty cells a **dim blue** so the
  playfield outline is visible; the active piece on top (`tetris_render`).
- **Score = lines cleared.** On game-over the board does the arcade **fill sweep**; **15 lines fills
  the whole board** (`game_over(tlines, 0)` → `frac = lines / 15`), colored by tier
  (bronze < 40 % < cyan < 80 % < gold).

## Tuning (in `arcade.c`)
- `TLEN` / `TWID` — well size; `mcol(a, f)` — the cell → matrix-column mapping.
- `grav_ms` start (650) and the `-= 18` speed-up.
- `KICK[]` — wall-kick offsets.
- Fill target `score / 15` in `game_over`.

See **[TOPO.md](TOPO.md)** for the other game and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the
arcade shell (lobby / countdown / score).
