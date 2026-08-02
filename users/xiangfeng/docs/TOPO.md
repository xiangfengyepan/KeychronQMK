# Topo (arcade game)

Whack-a-mole on the whole keyboard. Part of the on-keyboard arcade — open with **Fn + H**, turn the
knob to select **TOPO**, press the knob to start. Code lives in `src/arcade.c` (the `topo_*` functions).

## How it plays
A key lights up (a "mole"). **Press that key** before it fades. Each hit scores 1. It's **sudden
death**:

- **Timeout** — a mole you don't hit within its window → game over.
- **Wrong key** — pressing *any* key that isn't a live mole → game over.

The only safe way out is **holding the knob** (~0.5 s) to quit — the knob is never a mole (it has no
LED) and never counts as a miss.

## Field
The **whole board**: all **82 backlit keys** (`cR/cC`, built from `g_led_config.matrix_co`). The knob
is excluded (no LED). Moles never overlap.

## Difficulty — fixed window, exponential spawn
Each mole's **window is a fixed 3 s** (`MOLE_LIFE`). Difficulty comes from moles **spawning faster and
piling up**, ramping **on every hit** (not in steps):

```
spawn gap  =  230 + 1170 · e^(−hits / 25)   ms      (1400 ms → ~230 ms)
max live   =  1 + hits/10   (capped at 8)
```

| hits | spawn gap |
|---|---|
| 0 | 1400 ms |
| 10 | 1014 ms |
| 20 | 756 ms |
| 40 | 466 ms |
| 80 | 278 ms |

Big early relief, then it closes in fast and never quite bottoms out; with the 3 s window and sudden
death, surviving means keeping an increasingly crowded board clear.

## Rendering & score
- Each mole is drawn brightest when fresh and **fades yellow → red** as its 3 s runs out
  (`topo_render`) — the color is your countdown.
- **Score = hits.** On game-over the board does the arcade **fill sweep** with a **saturating
  exponential** so the last keys are brutal: `frac = 1 − e^(−hits / 32)` (`game_over(topo_score, 1)`),
  colored by tier (bronze / cyan / gold). Full board ≈ 130+ hits.

## Tuning (in `src/arcade.c`)
- `MOLE_LIFE` (3000) — the per-mole window.
- `230 + 1170·expf(-hits/25)` in `topo_tick` — the spawn ramp (lower `/25` = harsher).
- `1 + topo_score/10`, cap `TOPO_MAX` (8) — max simultaneous moles.
- `1 − expf(-score/32)` in `game_over` — the score-fill curve.

See **[TETRIS.md](TETRIS.md)** for the other game and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for
the arcade shell.
