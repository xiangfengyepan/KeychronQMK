# Pinyin IME (Fn + I) — implementation

A tiny **pinyin input method baked into the keyboard**: type a syllable, cycle candidates shown on
the RGB LEDs, then confirm and the keyboard **draws the character with the mouse** in a paint app.
Everything is in this folder — `src/ime/ime.c` (logic, behind `include/ime.h`) + `src/ime/hanzi_data.c` + `include/hanzi_data.h` (dictionary). `keymap.c` keeps only thin call-outs (`ime_active` / `ime_process_record` / `ime_render` / `ime_toggle`) plus the shared mouse-drawing engine.

---

## 1. What it does (user flow)

1. **Fn + I** (`IME_TOGG`) turns the IME on — the board enters *compose mode*.
2. Type a toneless **pinyin** syllable (e.g. `feng`). Candidates load live as you type (prefix match).
3. The **current candidate is animated stroke-by-stroke across the LEDs** so you can recognise it, and
   the **number row shows how many pinyin letters you've typed** — 1 letter → key `1`, 2 → `1`+`2`, …
   (green; dark when the buffer is empty).
4. Cycle / pick the one you want, then **confirm** — the cursor **draws it** (have a paint app focused).
5. The IME stays on and the drawing **carriage advances right**, so you can write a whole sentence.
6. **Esc** or **Fn + I** exits.

### Controls (while composing)

| Key | Action |
|---|---|
| letters (A–Z) | append to the pinyin buffer (max 7) → refresh candidates |
| **← / →** or **Tab** | previous / next candidate |
| **F1 – F12** | jump to that candidate (the F-row is a live candidate strip; selected = magenta) |
| **Space / Enter** | confirm the current candidate → draw it |
| **Backspace** | delete the last pinyin letter |
| **Esc** / **Fn + I** | exit the IME |
| **1 – 0** | *(no action — they're the buffer-length meter; presses are swallowed)* |
| modifiers / Fn | pass through (so Fn+I can toggle off) |

`IME_TOGG` lives on **layer 1 · I** (Mac Fn) and **layer 3 · I** (Win Fn), so it works in either OS mode.

---

## 2. The dictionary — `src/ime/hanzi_data.c` / `include/hanzi_data.h`

- **276 characters** — the 12 most-common per usable pinyin initial (23 letters, no i/u/v). Your names
  come first for their syllable (`feng`→沣, `pan`→潘, `ye`→叶, `xiang`→祥 are the
  first candidate for their pinyin), plus common homophones and a broad common set.
- Each entry is real **stroke-median** data from **Make Me a Hanzi** (`skishore/makemeahanzi`), in
  canonical stroke order, mapped into the drawing grid (x → right, y → down), mapped with a fixed em-box transform (`x=S·(mx−512)`, `y=S·(388−my)`, `S≈0.125`) so all characters
  share one centre and scale — no resampling; the raw medians are used as-is.
- Pinyin is **toneless** (`hǎo` → `hao`; `ü` → `v`); the pinyin **and the character frequency** (used to
  keep each letter's most-common characters) come from **hanziDB** — see Sources below.
- Cost: ~56 KB of flash (the dictionary is by far the biggest custom item — see `MEMORY.md`).

```c
typedef struct {
    const int16_t *x;      // stroke points, x (concatenated over all strokes)
    const int16_t *y;      // stroke points, y
    const uint8_t *len;    // points per stroke
    uint8_t  nstroke;      // number of strokes
    const char *py;        // toneless pinyin, e.g. "feng"
} hanzi_t;
extern const hanzi_t hanzi_table[];   // ordered; name chars first
extern const uint16_t hanzi_count;    // 276
```

### Sources (open data, fetched from GitHub raw)

| Data | What it provides | GitHub |
|---|---|---|
| **Make Me a Hanzi** — `graphics.txt` | per-character **stroke medians** (the drawing) | <https://github.com/skishore/makemeahanzi><br>raw: `https://raw.githubusercontent.com/skishore/makemeahanzi/master/graphics.txt` |
| **hanziDB** — `data/hanziDB.csv` | **frequency rank** + **pinyin** (which characters, and their order) | <https://github.com/ruddfawcett/hanziDB.csv><br>raw: `https://raw.githubusercontent.com/ruddfawcett/hanziDB.csv/master/data/hanziDB.csv` |

### Regenerating `src/ime/hanzi_data.c`
For each usable pinyin initial (all letters **except i / u / v**, which have no syllables), take the
**12 most-common characters** (by hanziDB `frequency_rank`) whose toneless pinyin starts with that
letter and that have medians — forcing the name characters (沣/潘/叶/祥) first so they stay candidate 1.
Transform every median point with the fixed em-box map

```
x = round(S · (mx − 512)),   y = round(S · (388 − my)),   S ≈ 0.125
```

(fit against the existing name characters; Y is flipped, and the **raw medians are used as-is — no
resampling**). Emit the `x[]/y[]/l[]` arrays + a `hanzi_table[]` row, and set `hanzi_count`. Budget:
~0.2 KB per character; ~31 KB of flash headroom (≈ another ~150 characters).

---

## 3. Code path (`src/ime/ime.c`)

### State
```c
#define PY_MAX 7            // longest pinyin buffer
#define IME_CAND_MAX 12     // candidate list cap = the F-row (F1..F12)
#define IME_LED_MAX 220     // LED path cap
static bool     ime_on;                 // compose mode active
static char     py_buf[PY_MAX+1]; static uint8_t py_len;
static uint16_t cand[IME_CAND_MAX]; static uint8_t cand_n, cand_i;   // indices into hanzi_table
static uint8_t  ime_leds[IME_LED_MAX]; static uint16_t ime_led_n, ime_led_pos, ime_led_timer;
```

### Input — `process_record_user()`
When `ime_on`, a `switch` at the **top of the function** intercepts keys *before* normal handling:
letters build `py_buf` and call `ime_update()`; arrows/Tab move `cand_i`; the **F-row (matched by
position, row 0 cols 1–12) sets `cand_i`** and reloads the preview; **digits `1–0` are swallowed** (the
number row is just a length meter now); Space/Enter call `ime_confirm()`; Backspace trims a letter; Esc
exits. Anything else (`break`) falls
through so modifiers and Fn still work. `IME_TOGG` in the main switch flips `ime_on` and resets state.

### Candidate lookup — `ime_update()`
Prefix match over the whole table, capped at `IME_CAND_MAX`:
```c
for (i = 0; i < hanzi_count && cand_n < IME_CAND_MAX; i++)
    if (strncmp(hanzi_table[i].py, py_buf, py_len) == 0) cand[cand_n++] = i;
```
Because the table is ordered (names first, then frequency), candidates come out in a useful order.
Then it calls `ime_led_load()` to build the preview for `cand[cand_i]`.

### LED preview — `ime_led_load()` + `rgb_matrix_indicators_advanced_user()`
- `ime_led_load()` **rasterises** the current candidate's strokes into an ordered list of LED indices
  (`ime_leds[]`): each stroke point is mapped to the **nearest physical LED** (`nearest_led()` over
  `g_led_config.point[]`), scaling the centred glyph onto the key grid (`IME_LED_SCALE 0.46`,
  centred at 112,32). Consecutive duplicates are dropped. This runs once per candidate change.
- The indicator callback (when `ime_on`) clears the board, then **reveals the path progressively**
  (`ime_led_pos` advances ~every 55 ms and loops): a green trail with a bright green head. No match
  yet → a faint blue "listening" glow. On top, the **number row lights `1…min(py_len,10)` green**
  (`matrix_co[1][1..10]` = keys `1`–`9`,`0`) as a pinyin-buffer-length meter — dark when `py_len == 0`.
  The **F-row lights the candidate strip** (`matrix_co[0][1..12]` = F1–F12): available candidates cyan,
  the selected `cand_i` magenta. It `return false`s to hide the normal effect + kb indicators.
- ⚠️ The LED grid is only ~87 keys, so a complex character is a rough trace, not crisp — you rely
  partly on cycle order.

### Confirm → draw — `ime_confirm()` + the draw engine
```c
static void ime_confirm(void) {
    const hanzi_t *h = &hanzi_table[cand[cand_i]];
    draw_begin(h->x, h->y, h->len, h->nstroke);   // hand the glyph to the mouse-draw engine
    ime_reset();                                  // clear buffer, keep IME on for the next char
}
```
The **draw engine** (`draw_begin()` + the `draw_on` block in `housekeeping_task_user`) traces each
stroke as a polyline with **relative** mouse moves (`host_mouse_send`), **holding the left button
during a stroke** and lifting between strokes — so it draws in a paint app. Because it's the standard
mouse report, drawing works **wired or wireless** (unlike the DVD bounce, which needs the USB-only
digitizer).

### Sentences — the carriage
Each glyph is centred on its own origin, so to write left-to-right the engine places that origin at
`carriage_x` and steps it right by `CHAR_ADVANCE` (130) after each confirmed character. `draw_cx/cy`
stay continuous across characters, so the pen just travels to the next cell. Toggling the IME **on**
resets the carriage (`carriage_x = 0`, pen origin = current cursor) — i.e. starts a fresh line where
the cursor is. To start another line, move the mouse and toggle Fn+I off/on.
- `MS_STOP` (**layer 3 · Space**) aborts an in-progress draw and releases the mouse button.
- Screen-edge limit: relative moves can't cross the real screen edge, so a very long line eventually
  piles at the edge — start a new line before then.

---

## 4. Why an IME on the keyboard at all?
Keychron's official DKS can only emit real keystrokes, and custom firmware can't appear in the
Launcher — so a "draw my name" feature has to be firmware-side. Baking a small dictionary + a mouse
draw-engine lets the board write characters no host software is involved in. It's a *personal*
dictionary (flash-limited), not a full IME, and the LED preview is deliberately coarse.
