# Pong (arcade game)

Two-player Pong rendered on the RGB grid. Two **1 × 2** paddles at the far-left and far-right columns
knock a **1 × 1** ball back and forth; first to **3** wins. Part of the on-keyboard arcade — open with
**Fn + H**, turn the knob to select **PONG**, press the knob to start. Code lives in `src/arcade.c` (the
`pong_*` functions).

## The court
- **Play area = matrix rows 0–4** (everything except the bottom space-bar row), full width.
- **Left paddle** occupies matrix **col 0** (`P_LCOL`); **right paddle** occupies matrix **col 12**
  (`P_RCOL`) — a clean straight column, not the scattered col-13 keys.
- Each paddle is **2 rows tall** (`PAD_H`); the ball is a single key.
- **Score pips** live on the free **space-bar row (matrix row 5):** the left score lights up to the **3
  leftmost** row-5 keys (cols 0, 1, 2) in **cyan**; the right score lights up to the **3 rightmost** keys
  (cols 14, 13, 12) in **magenta**.

## Controls (2 players, all held keys)
| Player | Up | Down |
|---|---|---|
| **Left** (cyan) | **`<`** — the ISO key left of Z `(row 4, col 1)` | **Left Win / GUI** `(row 5, col 1)` |
| **Right** (magenta) | **↑** arrow `(row 4, col 14)` | **↓** arrow `(row 5, col 13)` |

Paddles move **vertically only** and keep moving while a key is held (tracked as `p_lu / p_ld / p_ru /
p_rd`, stepped in `pong_tick`). **Knob hold (~0.5 s) quits to the lobby.**

## Mechanics
- **Ball physics** (`pong_tick`, dt-scaled): bounces off the top/bottom walls; on a paddle hit the ball
  reflects and gains a little **angle based on where it struck the paddle** (`pVy += (by − paddleCenter)
  * 0.04`) plus a **1.03× speed-up** each hit.
- **Serve freeze:** after each point (and at the very start) the ball recenters and **freezes for ~3 s**
  (`P_SERVE_WAIT`), **blinking** as a countdown, then serves **toward the side that was just scored on**.
  Paddles stay movable during the freeze.
- **Scoring:** a ball that leaves past a paddle scores for the **other** side. First to **`P_WIN` = 3**
  wins — the match-winning point **skips the serve freeze** and goes straight to the winner overlay.

## Rendering & winner screen
- Each frame draws both paddles, the ball (unless mid-blink during a freeze), and the score pips
  (`pong_render`).
- On a win the arcade **score-fill screen** fills the whole board in the **winner's color** — cyan for
  Left, magenta for Right (`pong_end` sets `sc_frac = 1` and the fill color, reusing `sc_render`).
  Knob-tap or 10 s idle returns to the lobby.

## Tuning (in `src/arcade.c`)
- `P_LCOL` / `P_RCOL` — paddle columns; `PAD_H` — paddle height; `P_ROWS` — court height.
- `P_WIN` (3) — points to win; `P_SERVE_WAIT` (3000 ms) — the serve freeze.
- Serve speed `0.085`, angle add `0.04`, and per-hit speed-up `1.03` in `pong_tick`.
- Paddle speed `pv = 0.14 * s` in `pong_tick`.
- `LPIP[] / RPIP[]` — which row-5 keys the score pips use.

See **[DROP.md](DROP.md)** for the other new game and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the
arcade shell (lobby / countdown / score).
