# Reaction (arcade game)

A reflex test on the whole board. Part of the on-keyboard arcade — open with **Fn + H**, turn the knob
to select **REACT**, press the knob to start. Code lives in `src/arcade.c` (the `react_*` / `rx_*` functions).

## How it plays
Three rounds. Each round:
1. The board waits on a **dim red** for a random `900–3300 ms` — *don't press yet*.
2. It flashes **bright green** — the `GO`. A hidden timer starts.
3. **Hit any key** as fast as you can; your reaction (ms) is recorded.

Press **on the red** (before the green) and it's a **false start** — the board flashes brighter red and
that round **restarts** (no time recorded). After 3 clean rounds it averages your three times.

Any key counts — every one of the 82 keys, plus the knob, registers a hit.

## Field
The **whole board**: all 82 backlit keys light at once (`cR/cC`). Red = wait, green = go.

## Controls
| Input | Action |
|---|---|
| **Any key / knob** | hit (records your reaction once the board is green) |
| **Knob hold (~0.5 s)** | quit to the lobby |

## Scoring — average, exactly 1 key per ms
`score = average of the 3 reaction times (ms)`. On game-over the board does the arcade **fill sweep**;
the fill maps your average onto the 82 keys at **1 key per millisecond**:

```
keys = 182 − avg_ms      (clamped 0…82)     →     frac = (182 − avg) / 82
```

So **≤ 100 ms ⇒ the complete board (82/82)**, and every ms slower drops exactly one key:
**181 ms ⇒ 1 key**, **182 ms (or slower) ⇒ a true 0** (dark board — reaction skips the usual minimum
sliver). Some points: 120 ms → 62, 150 ms → 32, 170 ms → 12. Colored by the usual tier
(bronze / cyan / gold); the score screen shows `X / 82 keys`.

> 100 ms is essentially the human floor, so 82/82 is meant to be nearly unreachable. Widen the window
> by lowering the `182` (later zero) in `game_over` case 5.

## Tuning (in `src/arcade.c`)
- `900 + rnd()%2400` in `rx_newround` — the random pre-`GO` wait.
- `(182 − avg)/82` in `game_over` case 5 — the ms → fill curve (raise `182` to be more forgiving; keep
  the divisor at `82` for exactly 1 key per ms).
- Round count `3` in `react_press`; green/red colors in `react_render`.

See **[TETRIS.md](TETRIS.md)**, **[TOPO.md](TOPO.md)**, **[FLAPPY.md](FLAPPY.md)**, **[DINO.md](DINO.md)**,
**[MEMORY_GAME.md](MEMORY_GAME.md)** and **[CUSTOM_FIRMWARE.md](CUSTOM_FIRMWARE.md)** for the arcade shell.
