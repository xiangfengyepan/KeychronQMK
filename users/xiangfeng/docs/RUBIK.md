# Rubik (arcade game)

A speed-cubing companion on the RGB grid: a hold-to-start timer, a random scramble, and the resulting
cube colours — all driven by the **Space bar**. Part of the on-keyboard arcade — open with **Fn + H**,
turn the knob to select **RUBIK**, press the knob to start. Code lives in
`src/arcade/rubik.c` (the `rubik_*` functions + the cube engine).

Once launched the arcade owns every key; the game itself only listens to **Space** (`rubik_key` ignores
any other key). **Knob-hold (~0.5 s) exits to the lobby.**

## Three phases
The phase machine (`enum { RB_IDLE, RB_HOLD, RB_TIMER, RB_STOP, RB_SCRAMBLE, RB_COLORS }`) is driven
entirely by Space press/release:

### A. Timer  (hold Space ≥ 3 s → release)
- **Hold** Space: the number row **fills amber left→right** as a progress bar (`e * 10 / HOLD_MS` keys
  lit), so you can see the arming build up.
- Once you pass **`HOLD_MS` = 3000 ms** the whole number row **breathes green** ("armed — release any
  time"), so you never have to guess when to let go.
- **Release** (after arming) starts a **count-up clock**, shown in **binary on the number row**: key
  **`1` = bit 9 (512)** … **`0` = bit 0 (1)**, so 0–1023 s. Set bits are amber, clear bits very dim.
- Press **Space again** to **stop**; the time freezes and stays lit, **recoloured green** to mark it
  stopped (`RB_STOP`; the live count-up is amber).
- All timing is `uint32_t` (`timer_read32` / `timer_elapsed32`) — no 16-bit wrap.

### B. Scramble  (quick Space tap, < 3 s)
- A tap generates a **20-move WCA scramble** (`NMOVES = 20`), no two consecutive moves on the same face.
- Moves play **one every 1 s** (`MOVE_MS = 1000`): the **face key** (`R L U D B F`) lights **amber**,
  plus **`2`** (blue) for a double and the **`'`** key (pink — the **number-row `'`**, `KC_MINS`, not the
  `´` key by Enter) for a prime — **up to three keys at once**. Standard WCA scrambles only ever mark
  one of `'`/`2`, so in practice you'll see 1–2 keys.
- After the last move it flows straight into the colours; press **Space during the scramble** to stop it
  early and jump straight to the colour readout.

### C. Colours  (the scrambled cube)
Each **keyboard row is one cube face**, its 9 stickers on 9 keys (3×3 read left→right, top→bottom).
Order top→bottom is **F U D L R B** (`ROW_FACE[]`):

| Row | Keys | Face | Colour |
|---|---|---|---|
| F-row | F1…F9 | F | green |
| number | 1…9 | U | white |
| QWERTY | Q…O | D | yellow |
| home | A…L | L | orange |
| shift | Z … `.` | R | red |
| bottom | the 9 non-Space keys | B | blue |

## The cube engine
A real cube runs underneath (ported verbatim from the preview artifact), so the colours are a genuine
reachable scramble, not random noise. `cube[6][9]` holds a face index (0–5) per sticker. One CW
quarter-turn (`qturn`) rotates the face's own stickers by the permutation **`ROT = {6,3,0,7,4,1,8,5,2}`**
and cycles the four adjacent 3-sticker strips (`CYC[6][4]`); prime = 3 quarters, double = 2.
`gen_scramble` builds the move list with the arcade's `rnd()`, then applies it to a solved cube.

## Key positions (resolved at start)
`build_positions` (called from `rubik_start`) resolves each named key's physical `(row,col)` from the
keymap via `pos_of` → `keymap_key_to_keycode` + `g_led_config`, so the game follows the board layout
with no hard-coded LED indices. **Q1 HE note:** `pos_of` scans the **first 3 layers**, because the Q1 HE
keeps `F1…F12` on layer 2 (layer 0 has media keys there) — the physical position is the same wherever a
keycode is mapped, so the first match wins. The **B face** (bottom row) is the 9 non-Space keys on the
Space matrix row, left→right, skipping the wide-spacebar `KC_NO` gaps.

## Tuning (in `src/arcade/rubik.c`)
- `HOLD_MS` (3000) — Space hold to arm the timer; `NMOVES` (20) — scramble length; `MOVE_MS` (1000) —
  per-move display time.
- `COL[6][3]` — the six sticker colours; `ROW_FACE[6]` — which face each physical row shows.
- Timer bit order in `show_binary` (`1`=MSB … `0`=LSB); the amber/blue/pink move colours in `rubik_render`.

See **[PONG.md](PONG.md)**, **[DROP.md](DROP.md)**, **[REACTION.md](REACTION.md)** and
**[Q1_HE_CUSTOM_FIRMWARE.md](Q1_HE_CUSTOM_FIRMWARE.md)** for the arcade shell (lobby / countdown / score).
