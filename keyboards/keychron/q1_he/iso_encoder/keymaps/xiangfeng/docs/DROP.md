# Drop-Merge (arcade game)

A **color-2048** dropper rendered on the RGB grid. A single colored box falls; land it on a box of the
**same color** and the two **merge** into the next-tier color, chaining upward. Part of the on-keyboard
arcade — open with **Fn + H**, turn the knob to select **DROP**, press the knob to start. Code lives in
`src/arcade.c` (the `drop_*` functions).

## The well — a 4 × 13 grid on rows 1–4
The board reuses Tetris's fall geometry but is **4 wide** and shifted **down one row**, so the **F-row
(matrix row 0) stays free for the tier legend**. A cell `(across a, fall f)` maps to matrix key
`(row drow(a), dcol(a, f))` where

```
drow(a)    = a + 1                         // across a(0..3) -> matrix rows 1..4
dcol(a, f) = (a == 3 && f >= 11) ? f+1 : f // row 4 steps over the one keyless spot (col 11)
```

- **Width = 4** (across): matrix **rows 1–4**. This is the direction you *move* the box (PgUp/PgDn).
- **Length = 13** (fall): the direction the box *falls* (left → right; "down" held vertical). For rows
  1–3, fall index `f` is matrix col `f` (cols 0–12). Row 4 (the ZXCV row) jumps its keyless col-11 spot
  exactly like Tetris, so all 13 fall positions land on a real key.

Boxes always **spawn in across-lane `a = 1`** (the number row) at the top of the fall axis and drop under
gravity to sit on top of that lane's stack.

## Tier colors (9 tiers)
Lowest → highest, straight from the palette (`DTIER[]`):

```
0 RED · 1 ORANGE · 2 GOLD · 3 GREEN · 4 CYAN · 5 BLUE · 6 PURPLE · 7 MAGENTA · 8 WHITE
```

## The F-row legend
Matrix row 0 lights **F1…F9 with the 9 tier colors** (F1 = lowest tier). Tiers currently in the spawn
pool are at **full brightness**; locked higher tiers are **dimmed to ~22 %**. Drawn every frame.

## Controls
| Input | Action |
|---|---|
| **PgUp / PgDn** | move the falling box across the well (toward row 1 / row 4) |
| **Home** | hard-drop |
| **Knob press** | *nothing* |
| **Knob hold (~0.5 s)** | quit to the lobby |

(Same three keys Tetris uses.)

## Mechanics
- **Merge (vertical only):** when the box lands, if the top two boxes of that column share a color (and
  are below tier 8) they collapse into one box of the **next** tier; this repeats upward while the new
  top matches the box below it (`drop_merge`).
- **Growing spawn pool:** starts at tiers **0–2** (F1–F3). A cumulative `dmade[tier]` counts every box of
  each tier ever **created by a merge**; once a tier ≥ 3 reaches **3 created**, it permanently joins the
  pool (`dpool`) — so 3× tier-3 boxes unlock tier 3, etc. Each spawn picks **uniformly** across the
  current pool `rnd() % (dpool + 1)`.
- **Gravity:** `dgrav_ms` = 480 ms per fall step.
- **Game over:** a column reaches the top of the 13-long axis (on a land or at spawn) → the **score
  screen**.

## Rendering & score
- Each frame draws the legend, every stacked box in its tier color, and the falling box on top
  (`drop_render`).
- **Score = highest tier reached.** On game-over the board does the arcade **fill sweep**; **tier 8 fills
  the whole board** (`game_over(dmaxtier, 6)` → `frac = tier / 8`), colored by tier
  (bronze < 40 % < cyan < 80 % < gold).

## Tuning (in `src/arcade.c`)
- `DWID` / `DLEN` — well size; `drow(a)` / `dcol(a, f)` — the cell → matrix mapping.
- `DTIER[]` — the 9 tier colors (palette constants).
- `dpool` start value (2) in `drop_start`, and the `dmade[nt] >= 3` unlock threshold in `drop_merge`.
- `dgrav_ms` (480) — fall speed.
- The legend dim factor (`* 22 / 100`) in `drop_render`.
- Fill target `tier / 8` (case 6) in `game_over`.

See **[PONG.md](PONG.md)** for the other new game and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the
arcade shell (lobby / countdown / score).
