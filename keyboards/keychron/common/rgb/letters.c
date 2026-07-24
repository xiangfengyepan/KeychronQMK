/* Copyright 2024 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Typed-letter RGB effects, using a small ring buffer of recently pressed keys
 * (the "memory"), rendered with a 3x5 pixel font onto the physical LED grid.
 *   - LETTERS_MARQUEE : typed text scrolls right -> left across the board
 *   - LETTERS_BIG     : the last key you pressed is drawn big, then fades
 * Note: the board has one LED per (staggered) key, so letters are necessarily
 * coarse -- this is as legible as the hardware allows.
 */

#include "quantum.h"
#include "rgb_matrix.h"
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

#define LT_BUF 20
#define GCOLS 15
#define GROWS 6
#define FONT_ROWS 5

// glyph indices: 0..25 = A-Z, 26..35 = 0-9, 36 = space
// each row value is 3 bits, bit2 = left column, bit0 = right column
static const uint8_t FONT[37][FONT_ROWS] = {
    {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7}, {7, 4, 6, 4, 4}, // A-F
    {3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2}, {5, 6, 4, 6, 5}, {4, 4, 4, 4, 7}, // G-L
    {5, 7, 7, 5, 5}, {5, 7, 7, 7, 5}, {2, 5, 5, 5, 2}, {6, 5, 6, 4, 4}, {2, 5, 5, 6, 3}, {6, 5, 6, 5, 5}, // M-R
    {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5}, // S-X
    {5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},                                                                     // Y,Z
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {6, 1, 2, 4, 7}, {7, 1, 3, 1, 7}, {5, 5, 7, 1, 1},                  // 0-4
    {7, 4, 7, 1, 7}, {3, 4, 7, 5, 7}, {7, 1, 2, 4, 4}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 6},                  // 5-9
    {0, 0, 0, 0, 0}                                                                                       // space
};

static uint8_t  lt_glyph[LT_BUF];
static uint16_t lt_time[LT_BUF];
static uint8_t  lt_head = 0, lt_count = 0;

static uint8_t kc_to_glyph(uint16_t kc) {
    if (kc >= KC_A && kc <= KC_Z) return kc - KC_A;         // A-Z -> 0..25
    if (kc >= KC_1 && kc <= KC_9) return 27 + (kc - KC_1);  // '1'..'9' -> 27..35
    if (kc == KC_0) return 26;                              // '0' -> 26
    if (kc == KC_SPACE) return 36;
    return 0xFF;
}

void letters_process_record(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return;
    uint8_t g = kc_to_glyph(keycode);
    if (g == 0xFF) return;
    lt_glyph[lt_head] = g;
    lt_time[lt_head]  = timer_read();
    lt_head           = (lt_head + 1) % LT_BUF;
    if (lt_count < LT_BUF) lt_count++;
}

// physical LED -> coarse (col,row) grid, computed once from g_led_config
static uint8_t gX[RGB_MATRIX_LED_COUNT], gY[RGB_MATRIX_LED_COUNT];
static bool    grid_ready = false;

static void build_grid(void) {
    uint8_t minx = 255, maxx = 0, miny = 255, maxy = 0;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        uint8_t x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    uint8_t rx = (maxx > minx) ? (maxx - minx) : 1;
    uint8_t ry = (maxy > miny) ? (maxy - miny) : 1;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        uint8_t x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        gX[i]     = ((uint16_t)(x - minx) * (GCOLS - 1) + rx / 2) / rx; // 0..14
        gY[i]     = ((uint16_t)(y - miny) * (GROWS - 1) + ry / 2) / ry; // 0..5
    }
    grid_ready = true;
}

static uint8_t glyph_col_bits(uint8_t g, uint8_t fx) {
    uint8_t bits = 0;
    for (uint8_t r = 0; r < FONT_ROWS; r++)
        if (FONT[g][r] & (1 << (2 - fx))) bits |= (1 << r);
    return bits;
}

bool letters_marquee(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!grid_ready) build_grid();
    uint8_t gv = rgb_matrix_config.hsv.v;

    static uint8_t colbits[LT_BUF * 4];
    static uint8_t colhue[LT_BUF * 4];
    uint16_t       ncols = 0;
    uint8_t        start = (lt_head - lt_count + LT_BUF) % LT_BUF;
    for (uint8_t n = 0; n < lt_count; n++) {
        uint8_t g   = lt_glyph[(start + n) % LT_BUF];
        uint8_t hue = (uint8_t)(n * 28);
        for (uint8_t fx = 0; fx < 3; fx++) {
            colbits[ncols] = glyph_col_bits(g, fx);
            colhue[ncols]  = hue;
            ncols++;
        }
        colbits[ncols] = 0; // 1-column gap between letters
        colhue[ncols]  = hue;
        ncols++;
    }

    uint16_t span   = ncols + GCOLS;
    uint16_t ms     = 300 - rgb_matrix_config.speed;
    if (ms < 40) ms = 40;
    uint16_t scroll = ncols ? (uint16_t)((g_rgb_timer / ms) % span) : 0;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint8_t r = 0, g = 0, b = 0;
        if (ncols && gY[i] < FONT_ROWS) {
            int16_t src = (int16_t)gX[i] + scroll - GCOLS; // text enters from the right
            if (src >= 0 && src < (int16_t)ncols && (colbits[src] & (1 << gY[i]))) {
                HSV hsv = {colhue[src], 255, gv};
                RGB c   = hsv_to_rgb(hsv);
                r = c.r; g = c.g; b = c.b;
            }
        }
        rgb_matrix_set_color(i, r, g, b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

bool letters_big(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!grid_ready) build_grid();
    uint8_t gv = rgb_matrix_config.hsv.v;

    uint8_t bright = 0, glyph = 36;
    if (lt_count) {
        uint8_t newest = (lt_head - 1 + LT_BUF) % LT_BUF;
        glyph          = lt_glyph[newest];
        uint16_t el    = timer_elapsed(lt_time[newest]);
        if (el < 900)
            bright = 255; // hold
        else if (el < 1600)
            bright = 255 - (uint8_t)(((uint32_t)(el - 900) * 255) / 700); // fade
        else
            bright = 0;
    }
    uint8_t hue = (uint8_t)(glyph * 10);

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint8_t r = 0, g = 0, b = 0;
        if (bright && gY[i] < FONT_ROWS && gX[i] >= 5 && gX[i] <= 10) {
            uint8_t fx = (gX[i] - 5) / 2; // 6 board columns -> 3 font columns
            if (fx < 3 && (FONT[glyph][gY[i]] & (1 << (2 - fx)))) {
                HSV hsv = {hue, 255, scale8(bright, gv)};
                RGB c   = hsv_to_rgb(hsv);
                r = c.r; g = c.g; b = c.b;
            }
        }
        rgb_matrix_set_color(i, r, g, b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
