# Memory (arcade game)

Simon-says on the whole keyboard. Part of the on-keyboard arcade — open with **Fn + H**, turn the knob
to select **MEMORY**, press the knob to start. Code lives in `src/arcade.c` (the `memory_*` functions).

> File is `MEMORY_GAME.md` (not `MEMORY.md`) so it doesn't collide with the memory-map doc `MEMORY.md`.

## How it plays
The board **flashes a sequence** of keys (blue), one at a time. When it finishes, **you repeat it** by
pressing those keys in the same order. Get the whole sequence right and the board flashes **green** —
then the next round replays the sequence with **one more random key appended**.

- **Right key** → a short green flash on that key; keep going.
- **Wrong key** → game over.
- **Score = rounds completed** (the sequence length you reached before slipping).

It uses **all 82 backlit keys** (`cR/cC`); any key can appear in the sequence, and keys can repeat.

## The three phases (`mstage`)
| Stage | What's on screen |
|---|---|
| **0 — show** | each step lights blue for `MEM_ON` ms, then `MEM_GAP` ms dark; advances automatically |
| **1 — input** | faint wash over the board (your turn); correct presses flash green |
| **2 — round clear** | whole board green ~0.5 s, then back to show with the longer sequence |

Presses are ignored except during stage 1, so mashing keys while the sequence plays back can't hurt you.

## Controls
| Input | Action |
|---|---|
| **Press the keys** | repeat the shown sequence, in order (stage 1 only) |
| **Knob hold (~0.5 s)** | quit to the lobby |

## Rendering & score
- Playback / feedback colors in `memory_render`; the faint input wash makes "your turn" obvious.
- On game-over the board does the arcade **fill sweep** with a **saturating exponential** so early
  rounds show real progress: `frac = 1 − e^(−rounds / 6)` (`game_over(mlen−1, 4)`), colored by tier
  (bronze / cyan / gold).

## Tuning (in `src/arcade.c`)
- `MEM_ON` (420) / `MEM_GAP` (200) — playback speed.
- `MEM_MAX` (64) — longest sequence held.
- round-clear hold `500` ms in `memory_tick`; feedback flash `180` ms in `memory_render`.
- `1 − expf(-rounds/6)` in `game_over` — the score-fill curve.

See **[TETRIS.md](TETRIS.md)**, **[TOPO.md](TOPO.md)**, **[FLAPPY.md](FLAPPY.md)**, **[DINO.md](DINO.md)**
and **[Q1_HE_CUSTOM_FIRMWARE.md](Q1_HE_CUSTOM_FIRMWARE.md)** for the arcade shell.
