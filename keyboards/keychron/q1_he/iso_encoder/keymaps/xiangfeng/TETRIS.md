# Tetris (arcade game)

Tetris rendered on the RGB grid, played with the board held **vertically**. Part of the on-keyboard
arcade — open with **Fn + H**, turn the knob to select **TETRIS**, press the knob to start. Code lives
in `arcade.c` (the `tetris_*` / `t*` functions).

## The well — 5 × 13
- **Width = 5** (across): matrix **rows 0–4**. This is the direction you *move* the piece.
- **Length = 13** (fall): matrix **cols 0–13, skipping col 11**. This is the direction pieces *fall*
  (left → right in the normal keyboard frame; "down" when you hold the board vertical).

> **Why skip col 11?** Matrix cell **(row 4, col 11) has no key**, so that column can't be a full
> 5-tall line and could never clear. Skipping it keeps every column a clean 5 tall — at the cost of a
> **thin dark column** at physical col 11 (the `F11 / - / [ / '` stack). `FALLCOL[]` maps fall index →
> matrix column; a cell `(fall f, across a)` lights matrix key `(row a, col FALLCOL[f])`.

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
- `TLEN` / `TWID` / `FALLCOL[]` — well size & column mapping.
- `grav_ms` start (650) and the `-= 18` speed-up.
- `KICK[]` — wall-kick offsets.
- Fill target `score / 15` in `game_over`.

See **[TOPO.md](TOPO.md)** for the other game and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the
arcade shell (lobby / countdown / score).
