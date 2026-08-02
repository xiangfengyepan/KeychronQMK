/* Baked pinyin IME — compose mode driven by Fn+I (see keymap.c for the toggle).
 *
 * While on: type pinyin (letters) -> candidates load from the baked dictionary
 * (src/ime/hanzi_data.c); Left/Right (or Tab) cycle; the F-row (F1..F12) jumps to
 * a candidate; Space/Enter confirm and hand the glyph to the mouse-drawing engine
 * in keymap.c (draw_begin); Backspace deletes a letter; Esc cancels. The current
 * candidate is animated stroke-by-stroke across the RGB LEDs.
 *
 * This file owns ONLY the IME state and logic. The "draw with the mouse" engine
 * (draw_begin, the draw_* state, the carriage) is shared with the mouse shape /
 * "draw my name" features and stays in keymap.c; ime.c reaches it through ime.h. */

#include "quantum.h"
#include "rgb_matrix.h"
#include "include/ime.h"
#include "include/mouse.h"      // mouse character-drawing engine (draw_begin + carriage)
#include "include/hanzi_data.h" // baked pinyin -> stroke-median dictionary
#include "include/palette.h"    // named-color HSV constants (COL_*)
#include "include/utils.h"      // pal_rgb()
#include <math.h>
#include <string.h>

// Fn+I toggles IME mode. While on: type pinyin (letters) -> candidates load;
// Left/Right (or Tab) cycle; 1-9 jump to a candidate; Space/Enter confirm and
// draw the character with the mouse; Backspace deletes a letter; Esc cancels.
// The current candidate is animated stroke-by-stroke across the RGB LEDs.
#define PY_MAX 7
#define IME_CAND_MAX 12   // cap candidates to the F-row (F1..F12)
#define IME_LED_MAX 220
#define IME_LED_SCALE 0.46f // glyph units -> LED grid
static bool     ime_on = false;
static char     py_buf[PY_MAX + 1];
static uint8_t  py_len = 0;
static uint16_t cand[IME_CAND_MAX];
static uint8_t  cand_n = 0, cand_i = 0;
static uint8_t  ime_leds[IME_LED_MAX];
static uint16_t ime_led_n = 0, ime_led_pos = 0, ime_led_timer = 0;

static uint8_t nearest_led(float lx, float ly) {
    uint8_t best = 0; float bd = 1e9f;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        float dx = (float)g_led_config.point[i].x - lx;
        float dy = (float)g_led_config.point[i].y - ly;
        float d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static void ime_led_add(float gx, float gy) { // map centred glyph point -> LED, append to path
    uint8_t li = nearest_led(112.0f + gx * IME_LED_SCALE, 32.0f + gy * IME_LED_SCALE);
    if (ime_led_n < IME_LED_MAX && (ime_led_n == 0 || ime_leds[ime_led_n - 1] != li))
        ime_leds[ime_led_n++] = li;
}
static void ime_led_load(void) { // rasterise the current candidate into an ordered LED path
    ime_led_n = 0; ime_led_pos = 0; ime_led_timer = timer_read();
    if (!cand_n) return;
    const hanzi_t *h = &hanzi_table[cand[cand_i]];
    uint16_t base = 0;
    for (uint8_t s = 0; s < h->nstroke; s++) {
        uint8_t L = h->len[s];
        ime_led_add(h->x[base], h->y[base]);
        for (uint8_t p = 1; p < L; p++) {
            float x1 = h->x[base + p - 1], y1 = h->y[base + p - 1];
            float x0 = h->x[base + p],     y0 = h->y[base + p];
            float dx = x0 - x1, dy = y0 - y1;
            float dist = sqrtf(dx * dx + dy * dy);
            uint8_t steps = (uint8_t)(dist / 6.0f) + 1;
            for (uint8_t k = 1; k <= steps; k++) ime_led_add(x1 + dx * (float)k / steps, y1 + dy * (float)k / steps);
        }
        base += L;
    }
}
static void ime_update(void) { // rebuild candidate list for the current pinyin prefix
    cand_n = 0; cand_i = 0;
    if (py_len)
        for (uint16_t i = 0; i < hanzi_count && cand_n < IME_CAND_MAX; i++)
            if (strncmp(hanzi_table[i].py, py_buf, py_len) == 0) cand[cand_n++] = (uint16_t)i;
    ime_led_load();
}
static void ime_reset(void) { py_len = 0; py_buf[0] = 0; cand_n = 0; cand_i = 0; ime_led_n = 0; ime_led_pos = 0; }
static void ime_confirm(void) {
    if (!cand_n) return;
    const hanzi_t *h = &hanzi_table[cand[cand_i]];
    draw_begin(h->x, h->y, h->len, h->nstroke); // draw it with the mouse (engine lives in keymap.c)
    ime_reset();                                // ready for the next character
}

// ---- public API ----
bool ime_active(void) { return ime_on; }

void ime_toggle(void) { // Fn+I
    ime_on = !ime_on;
    ime_reset();
    ime_led_timer = timer_read();
    if (ime_on) { carriage_x = 0; draw_cx = draw_cy = 0; } // start a fresh line at the cursor
}

// compose mode: intercept typing, cycling and confirm. Returns true if the key
// was consumed (keymap.c then returns false); false lets it pass through (so Fn+I
// can toggle off, and modifiers / Fn / layer keys still work).
bool ime_process_record(uint16_t keycode, keyrecord_t *record) {
    // F-row (matrix row 0, cols 1..12 = F1..F12) = jump to that candidate. Matched by POSITION,
    // because on Mac base the top row sends media keys, not KC_F1. Navigates (doesn't confirm).
    if (record->event.key.row == 0 && record->event.key.col >= 1 && record->event.key.col <= 12) {
        if (record->event.pressed) { uint8_t k = record->event.key.col - 1; if (k < cand_n) { cand_i = k; ime_led_load(); } }
        return true;
    }
    switch (keycode) {
        case KC_ESC:  if (record->event.pressed) { ime_on = false; ime_reset(); } return true; // exit IME
        case KC_BSPC: if (record->event.pressed && py_len) { py_buf[--py_len] = 0; ime_update(); } return true; // delete a letter
        case KC_SPC:
        case KC_ENT:  if (record->event.pressed) ime_confirm(); return true;                    // Space/Enter = confirm -> draw
        case KC_TAB:
        case KC_RGHT: if (record->event.pressed && cand_n) { cand_i = (cand_i + 1) % cand_n; ime_led_load(); } return true;         // next
        case KC_LEFT: if (record->event.pressed && cand_n) { cand_i = (cand_i + cand_n - 1) % cand_n; ime_led_load(); } return true; // previous
        default:
            if (keycode >= KC_1 && keycode <= KC_0) return true; // number row is the length meter now — swallow; candidates are picked on the F-row
            if (keycode >= KC_A && keycode <= KC_Z) { // build the pinyin buffer
                if (record->event.pressed && py_len < PY_MAX) { py_buf[py_len++] = 'a' + (keycode - KC_A); py_buf[py_len] = 0; ime_update(); }
                return true;
            }
            return false; // IME_TOGG / modifiers / Fn / layer keys pass through (so Fn+I can toggle off)
    }
}

// draw the candidate stroke-by-stroke over the whole board, plus the number-row
// length meter and the F-row candidate strip. keymap.c calls this then returns false.
void ime_render(uint8_t led_min, uint8_t led_max) {
    for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, 0, 0, 0);
    if (cand_n) {
        if (led_min == 0 && timer_elapsed(ime_led_timer) > 55) { // advance once per frame
            ime_led_timer = timer_read();
            if (++ime_led_pos > ime_led_n + 6) ime_led_pos = 0; // loop with a short pause
        }
        RGB trail = pal_rgb((HSV)COL_CYAN, 170), head = pal_rgb((HSV)COL_GREEN_LIGHT, 255);
        for (uint16_t i = 0; i < ime_led_n && i <= ime_led_pos; i++) rgb_matrix_set_color(ime_leds[i], trail.r, trail.g, trail.b);
        if (ime_led_pos < ime_led_n) rgb_matrix_set_color(ime_leds[ime_led_pos], head.r, head.g, head.b); // bright head
    } else { // IME on, no match yet -> faint "listening" glow (COL_SKY, very dim)
        RGB c = pal_rgb((HSV)COL_SKY, 12);
        for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    // number row = pinyin buffer length: 1 letter -> key '1', 2 -> '1'+'2', … (green); dark when empty
    RGB buf = pal_rgb((HSV)COL_GREEN, 210);
    for (uint8_t j = 1; j <= py_len && j <= 10; j++) {
        uint8_t led = g_led_config.matrix_co[1][j]; // number row: col 1='1' … col 9='9', col 10='0'
        if (led != NO_LED) rgb_matrix_set_color(led, buf.r, buf.g, buf.b);
    }
    // F1..F12 = candidate list; press a key to jump to it, the selected one is highlighted
    RGB sel = pal_rgb((HSV)COL_PINK, 255), avail = pal_rgb((HSV)COL_CYAN, 200);
    for (uint8_t j = 0; j < cand_n && j < 12; j++) {
        uint8_t led = g_led_config.matrix_co[0][1 + j]; // F1=(0,1) … F12=(0,12)
        if (led == NO_LED) continue;
        if (j == cand_i) rgb_matrix_set_color(led, sel.r, sel.g, sel.b);   // selected candidate (COL_PINK)
        else             rgb_matrix_set_color(led, avail.r, avail.g, avail.b); // available candidate (COL_CYAN)
    }
}
