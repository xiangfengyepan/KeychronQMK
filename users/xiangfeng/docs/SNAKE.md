# Snake (arcade game)

Classic Snake played across the whole keyboard. Part of the on-keyboard arcade — open with **Fn + H**,
turn the knob to select **SNAKE**, press the knob to start. Code lives in `src/arcade/snake.c` (the
`snake_*` functions).

Once launched the arcade owns every key; Snake listens to the **arrow keys** for steering. **Knob-hold
(~0.5 s) exits to the lobby.**

## Controls
| Key | Action |
|---|---|
| **↑ / ↓ / ← / →** | steer the snake (absolute directions; no direct reverse into your own neck) |
| **Knob hold (~0.5 s)** | quit to the lobby |

## The board / grid
The snake plays on a **per-key adjacency grid** built at `snake_start()` from the arcade hub's cell
table (`cR/cC/nC`) plus each key's physical position (`g_led_config.point`):
- keys are binned into rows by y (6 rows on the Q1 HE), sorted by x within each row;
- **← / →** step to the previous / next key in the **same row** (wrapping within the row) — so
  horizontal movement is dead straight;
- **↑ / ↓** step to the **nearest-center-x key in the adjacent row** (wrapping top↔bottom) — so vertical
  movement lands on the visually-aligned key.

Every backlit key is a cell, so the whole board is in play. Snake segments, the apple, and collisions
are all keyed by cell index (layout-independent — the same code runs on the M1 V5).

## Mechanics
- One **apple** at a time on a random free cell (pulsing red). Eating it grows the snake by **+1** and
  **speeds it up** — the step interval starts at **180 ms** and drops **6 ms per apple** down to a
  **70 ms** floor.
- **Edges wrap** — leaving one edge reappears on the opposite side.
- **Game over** when the head runs into the snake's own body (the tail cell it's about to vacate is
  ignored unless you're eating that step).
- The snake is drawn **green**, head brightest and fading toward the tail.

## Score on game over
`snake_die` calls the shared hub screen `game_over(applesEaten, 9)`, so the arcade **score-fill** screen
animates your apple count key-by-key (bronze → cyan → gold); **40 apples fills the whole board**. Knob-tap
or 10 s idle returns to the lobby.

## Tuning (in `src/arcade/snake.c`)
- Start / floor / step-down pace (`180` / `70` / `6` ms) — the difficulty ramp.
- `SNK_MAX` — maximum snake length.
- The `game_over(…, 9)` divisor lives in `arcade.c` (`case 9: score / 40`).

See **[RUBIK.md](RUBIK.md)**, **[PONG.md](PONG.md)** and **[Q1_HE_CUSTOM_FIRMWARE.md](Q1_HE_CUSTOM_FIRMWARE.md)**
for the arcade shell (lobby / countdown / score).
