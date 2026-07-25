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
 * Keymap baked from the user's Keychron Launcher export
 * (Keymap-Q1 HE ISO Knob-24-20-59.json). Keycodes are written as their raw
 * 16-bit QMK values straight into the matrix, so this is an exact copy of the
 * configured layout (layers + encoder), independent of EEPROM.
 */

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "digitizer.h" // absolute pointer, for the full-screen DVD bounce
#include <math.h>

// Capture typed keys for the LETTERS_MARQUEE / LETTERS_BIG RGB effects.
extern void letters_process_record(uint16_t keycode, keyrecord_t *record);
extern void letters_clear(void); // wipe the marquee / letter buffer

// Extra persistent mouse-speed levels (beyond built-in ACCEL0/1/2).
extern void    mousekey_set_accel_level(uint8_t level);
extern uint8_t mousekey_get_offset(void);
enum custom_keycodes { MS_ACC4 = SAFE_RANGE, MS_ACC5, LT_CLEAR, MS_DRAW, MS_DVD,
                       MS_SH1, MS_SH2, MS_SH3, MS_SH4, MS_SH5,
                       MS_SH6, MS_SH7, MS_SH8, MS_SH9, MS_SH0 };

// Auto mouse-shape mover. One shape per number key (layer 1 · 1..0):
//   tap = start that shape, tap the same key again = stop.
// Fixed size; traversal speed follows the mouse-accel level (F1..F5).
//   1 ∞infinity  2 circle  3 triangle  4 square  5 hexagon
//   6 star       7 heart   8 spirograph 9 spiral 0 lissajous
// Separately, layer 1 · F9 = full-screen DVD bounce (absolute, see dvd_*).
enum { SHP_OFF = 0, SHP_INF, SHP_INFH, SHP_WAVE, SHP_SPIRAL,
       SHP_CIRCLE, SHP_TRI, SHP_SQUARE, SHP_PENTA, SHP_HEX,
       SHP_STAR, SHP_HEART, SHP_ROSE, SHP_LISS, SHP_SPIRO };
#define SHP_INTERVAL 12 // ms per step
static uint8_t  shp_active = SHP_OFF;
static uint16_t shp_timer  = 0;
static float    shp_theta  = 0;
static float    shp_ax = 0, shp_ay = 0; // fractional movement accumulators
static bool     draw_on = false;        // "draw my name" (祥沣) active

// Full-screen DVD bounce (layer 1 · F9). Uses the absolute digitizer report:
// x/y are screen fractions [0,1], so it bounces off the REAL screen edges at any
// resolution (the firmware can't read the pixel size, but 0..1 spans the screen).
static bool     dvd_on = false;
static uint16_t dvd_timer = 0;
static float    dvd_px = 0.10f, dvd_py = 0.10f;   // position, screen fraction
static float    dvd_vx = 0.0060f, dvd_vy = 0.0043f; // velocity per tick

static void shp_poly_v(uint8_t n, float r, uint8_t k, float *vx, float *vy) {
    float a = -1.5708f + (6.28318f / n) * k; // top vertex at start (0,0), centre (0,r)
    *vx = r * cosf(a);
    *vy = r + r * sinf(a);
}

static void shp_pos(uint8_t s, float th, float *ox, float *oy) {
    if (s == SHP_INF) { // vertical figure-8
        *ox = 30.0f * sinf(2.0f * th);
        *oy = 95.0f * sinf(th);
        return;
    }
    if (s == SHP_INFH) { // horizontal infinity
        *ox = 95.0f * sinf(th);
        *oy = 30.0f * sinf(2.0f * th);
        return;
    }
    if (s == SHP_CIRCLE) { // circle looping downward from start
        *ox = 55.0f * sinf(th);
        *oy = 55.0f * (1.0f - cosf(th));
        return;
    }
    if (s == SHP_WAVE) {
        *ox = 90.0f * sinf(th);
        *oy = 45.0f * sinf(4.0f * th);
        return;
    }
    if (s == SHP_SPIRAL) { // grows outward over one loop
        float r = 60.0f * th / 6.28318f;
        float a = 3.0f * th;
        *ox = r * cosf(a);
        *oy = r * sinf(a);
        return;
    }
    if (s == SHP_STAR) { // 5-point star (10 alternating vertices)
        float   seg = th / (6.28318f / 10);
        uint8_t k   = (uint8_t)seg;
        float   f   = seg - (float)k;
        float   a0 = -1.5708f + 0.628318f * k, a1 = -1.5708f + 0.628318f * (k + 1);
        float   r0 = (k & 1) ? 28.0f : 70.0f, r1 = ((k + 1) & 1) ? 28.0f : 70.0f;
        float   x0 = r0 * cosf(a0), y0 = r0 * sinf(a0), x1 = r1 * cosf(a1), y1 = r1 * sinf(a1);
        *ox = x0 + (x1 - x0) * f;
        *oy = y0 + (y1 - y0) * f;
        return;
    }
    if (s == SHP_HEART) {
        float sx = sinf(th);
        *ox = 5.0f * (16.0f * sx * sx * sx);
        *oy = -5.0f * (13.0f * cosf(th) - 5.0f * cosf(2 * th) - 2.0f * cosf(3 * th) - cosf(4 * th));
        return;
    }
    if (s == SHP_ROSE) { // 4-petal rose
        float r = 60.0f * cosf(2.0f * th);
        *ox = r * cosf(th);
        *oy = r * sinf(th);
        return;
    }
    if (s == SHP_LISS) {
        *ox = 80.0f * sinf(3.0f * th);
        *oy = 80.0f * sinf(2.0f * th);
        return;
    }
    if (s == SHP_SPIRO) {
        *ox = 42.0f * cosf(th) + 25.0f * cosf(7.0f * th);
        *oy = 42.0f * sinf(th) - 25.0f * sinf(7.0f * th);
        return;
    }
    if (s == SHP_SQUARE) { // axis-aligned square, corner at start
        const float S = 95.0f;
        const float X[4] = {0, S, S, 0}, Y[4] = {0, 0, S, S};
        float       seg = th / (6.28318f / 4);
        uint8_t     k   = (uint8_t)seg;
        float       f   = seg - (float)k;
        *ox = X[k & 3] + (X[(k + 1) & 3] - X[k & 3]) * f;
        *oy = Y[k & 3] + (Y[(k + 1) & 3] - Y[k & 3]) * f;
        return;
    }
    // regular polygon: triangle / pentagon / hexagon
    uint8_t n   = (s == SHP_TRI) ? 3 : (s == SHP_PENTA) ? 5 : 6;
    float   r   = (s == SHP_TRI) ? 70.0f : 58.0f;
    float   seg = th / (6.28318f / n);
    uint8_t k   = (uint8_t)seg;
    float   f   = seg - (float)k;
    float   x0, y0, x1, y1;
    shp_poly_v(n, r, k % n, &x0, &y0);
    shp_poly_v(n, r, (k + 1) % n, &x1, &y1);
    *ox = x0 + (x1 - x0) * f;
    *oy = y0 + (y1 - y0) * f;
}

static void dvd_stop(void) {
    if (dvd_on) {
        dvd_on = false;
        digitizer_in_range_off(); // lift the absolute pointer
    }
}

static void shp_start(uint8_t s) {
    shp_active = s;
    shp_theta  = 0;
    shp_ax = shp_ay = 0;
    shp_timer = timer_read();
    draw_on   = false; // shapes and name-drawing are mutually exclusive
    dvd_stop();        // ...and the full-screen bounce
}

// One shape per key: tap starts it, tap the same key again stops it.
static void shp_toggle(uint8_t s) {
    if (shp_active == s)
        shp_active = SHP_OFF;
    else
        shp_start(s);
}

// Full-screen DVD bounce toggle (layer 1 · F9).
static void dvd_toggle(void) {
    if (dvd_on) {
        dvd_stop();
    } else {
        shp_active = SHP_OFF; // one auto-mover at a time
        draw_on    = false;
        dvd_on     = true;
        dvd_timer  = timer_read();
        digitizer_in_range_on();
    }
}

// --- "Draw my name" 祥沣 with the mouse, one stroke at a time (pen up/down) ---
// The routine holds the left mouse button while tracing a stroke and lifts it
// between strokes, so it draws in a paint app. Toggle with MS_DRAW (layer 1 · F10).
// Coordinates are real stroke medians from Make Me a Hanzi (skishore/makemeahanzi),
// in canonical stroke order, mapped into the drawing grid (x -> right, y -> down)
// and scaled to ~115 units tall. 祥 = 礻(4) + 羊(6),  沣 = 氵(3) + 丰(4) = 17 strokes.
static const int16_t DRAW_X[] = {
    -95,-86,-84,  -117,-114,-109,-97,-91,-84,-87,-102,-111,-123,  -96,-94,-94,-96,-95,  -88,-79,-76,   // 礻
    -66,-57,-55,  -34,-31,-46,  -67,-64,-59,-33,-25,  -66,-59,-43,-34,-28,  -78,-74,-70,-55,-20,-10,  -52,-48,-49,   // 羊
    22,34,37,  10,21,24,  18,17,17,21,37,   // 氵
    51,54,61,80,93,96,100,  51,54,61,80,95,100,  40,44,50,68,102,108,115,  67,71,76,75,74};              // 丰
static const int16_t DRAW_Y[] = {
    -53,-46,-42,  -20,-19,-19,-23,-25,-25,-17,4,15,24,  2,9,30,41,47,  -3,1,5,   // 礻
    -50,-43,-38,  -57,-53,-35,  -25,-24,-25,-30,-31,  -7,-6,-9,-11,-11,  13,14,13,10,6,8,  -22,-19,57,   // 羊
    -47,-39,-34,  -21,-14,-10,  48,43,36,29,0,   // 氵
    -26,-25,-25,-28,-32,-32,-31,  -6,-5,-5,-8,-11,-10,  16,17,17,13,10,10,12,  -58,-57,-52,-35,58};       // 丰
static const uint8_t DRAW_LEN[] = {3,10,5,3, 3,3,5,5,6,3, 3,3,5, 7,6,7,5};
#define DRAW_NSTROKE (sizeof(DRAW_LEN))

static uint8_t  draw_s = 0, draw_p = 0, draw_phase = 0; // phase 0=pen-up move, 1=drawing, 2=final release
static uint16_t draw_base = 0, draw_timer = 0;
static float    draw_cx = 0, draw_cy = 0, draw_fx = 0, draw_fy = 0;

static void draw_start(void) {
    shp_active = SHP_OFF; // don't run a shape at the same time
    dvd_stop();
    draw_on = !draw_on;   // toggle
    draw_s = draw_p = draw_phase = 0;
    draw_base = 0;
    draw_cx = draw_cy = draw_fx = draw_fy = 0;
    draw_timer = timer_read();
}

// Hold-to-repeat for the RGB adjust keys (step is 1, so a hold ramps smoothly).
#define RGB_HOLD_INTERVAL 28 // ms between repeats while a key is held
static uint16_t rgb_hold_kc    = 0;
static uint16_t rgb_hold_timer = 0;
static void rgb_hold_apply(uint16_t kc) {
    switch (kc) {
        case UG_HUEU: rgb_matrix_increase_hue_noeeprom(); break;
        case UG_HUED: rgb_matrix_decrease_hue_noeeprom(); break;
        case UG_SATU: rgb_matrix_increase_sat_noeeprom(); break;
        case UG_SATD: rgb_matrix_decrease_sat_noeeprom(); break;
        case UG_VALU: rgb_matrix_increase_val_noeeprom(); break;
        case UG_VALD: rgb_matrix_decrease_val_noeeprom(); break;
        case UG_SPDU: rgb_matrix_increase_speed_noeeprom(); break;
        case UG_SPDD: rgb_matrix_decrease_speed_noeeprom(); break;
    }
}

// Flash the board red when an RGB setting hits its min/max while adjusting.
#define LIMIT_FLASH_MS 140
static bool     limit_flash       = false;
static uint16_t limit_flash_timer = 0;
static uint16_t adj_check_kc      = 0;
static bool rgb_at_limit(uint16_t kc) {
    switch (kc) {
        case UG_SATU: return rgb_matrix_get_sat() >= 255;
        case UG_SATD: return rgb_matrix_get_sat() == 0;
        case UG_VALU: return rgb_matrix_get_val() >= 255;
        case UG_VALD: return rgb_matrix_get_val() <= RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL + RGB_MATRIX_VAL_STEP;
        case UG_SPDU: return rgb_matrix_get_speed() >= 255;
        case UG_SPDD: return rgb_matrix_get_speed() == 0;
        default:      return false; // hue wraps -> no min/max
    }
}
static void limit_check(uint16_t kc) {
    if (rgb_at_limit(kc)) { limit_flash = true; limit_flash_timer = timer_read(); }
}

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {
        { 0x0029, 0x00BE, 0x00BD, 0x7E04, 0x7E05, 0x7828, 0x7827, 0x00AC, 0x00AE, 0x00AB, 0x00A8, 0x00AA, 0x00A9, 0x004C, 0x00A8 },
        { 0x0035, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x002D, 0x002E, 0x002A, 0x004B },
        { 0x002B, 0x0014, 0x001A, 0x0008, 0x0015, 0x0017, 0x001C, 0x0018, 0x000C, 0x0012, 0x0013, 0x002F, 0x0030, 0x0028, 0x004E },
        { 0x0039, 0x0004, 0x0016, 0x0007, 0x0009, 0x000A, 0x000B, 0x000D, 0x000E, 0x000F, 0x0033, 0x0034, 0x0032, 0x004A, 0x0000 },
        { 0x00E1, 0x0064, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0011, 0x0010, 0x0036, 0x0037, 0x0000, 0x0038, 0x00E5, 0x0052 },
        { 0x00E0, 0x7E00, 0x7E02, 0x0000, 0x0000, 0x0000, 0x002C, 0x0000, 0x0000, 0x7E03, 0x5221, 0x00E4, 0x0050, 0x0051, 0x004F },
    },
    [1] = {
        { 0x5242, 0x00DD, 0x00DE, 0x00DF, MS_ACC4, MS_ACC5, 0x0000, 0x0000, 0x0000, MS_DVD, MS_DRAW, 0x0000, 0x0000, 0x0000, 0x00D3 },
        { 0x5242, MS_SH1, MS_SH2, MS_SH3, MS_SH4, MS_SH5, MS_SH6, MS_SH7, MS_SH8, MS_SH9, MS_SH0, 0x0000, 0x0000, 0x0000, 0x00D9 },
        { 0x0000, 0x0000, 0x00CD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00D1, 0x00DA },
        { 0x0000, 0x00CF, 0x00CE, 0x00D0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00CD },
        { 0x00D4, 0x0000, 0x00D5, 0x0000, 0x0000, 0x0000, 0x00D1, 0x0000, 0x0000, 0x00D2, 0x0000, 0x0000, 0x00CF, 0x00CE, 0x00D0 },
    },
    [2] = {
        { 0x0029, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x004C, 0x00AE },
        { 0x0035, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x002D, 0x002E, 0x002A, 0x004B },
        { 0x002B, 0x0014, 0x001A, 0x0008, 0x0015, 0x0017, 0x001C, 0x0018, 0x000C, 0x0012, 0x0013, 0x002F, 0x0030, 0x0028, 0x004E },
        { 0x0039, 0x0004, 0x0016, 0x0007, 0x0009, 0x000A, 0x000B, 0x000D, 0x000E, 0x000F, 0x0033, 0x0034, 0x0032, 0x004A, 0x0000 },
        { 0x00E1, 0x0064, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0011, 0x0010, 0x0036, 0x0037, 0x0000, 0x0038, 0x00E5, 0x0052 },
        { 0x00E0, 0x00E3, 0x00E2, 0x0000, 0x0000, 0x0000, 0x002C, 0x0000, 0x0000, 0x00E6, 0x5223, 0x00E4, 0x0050, 0x0051, 0x004F },
    },
    [3] = {
        { 0x5241, 0x00BE, 0x00BD, 0x7E06, 0x7E07, 0x7828, 0x7827, 0x00AC, 0x00AE, 0x00AB, 0x00A8, 0x00AA, 0x00A9, 0x0046, 0x00D3 },
        { 0x0001, 0x7E0B, 0x7E0C, 0x7E0D, 0x7E0E, 0x7700, 0x7701, 0x7702, 0x7703, 0x7704, 0x7705, 0x7706, 0x7707, LT_CLEAR, 0x0049 },
        { 0x7820, 0x7821, 0x7827, 0x7823, 0x7825, 0x7829, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x00D1, 0x0001 },
        { 0x0001, 0x7822, 0x7828, 0x7824, 0x7826, 0x782A, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x004D, 0x0000 },
        { 0x0001, 0x0001, 0x7E10, 0x7E11, 0x7E12, 0x0001, 0x7E0F, 0x7013, 0x0001, 0x0001, 0x0001, 0x0000, 0x0001, 0x0001, 0x00CD },
        { 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x00D2, 0x0001, 0x0001, 0x00CF, 0x00CE, 0x00D0 },
    },
};
// clang-format on

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [1] = {ENCODER_CCW_CW(MS_WHLL, MS_WHLR)},
    [2] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [3] = {ENCODER_CCW_CW(MS_WHLU, MS_WHLD)},
};
#endif // ENCODER_MAP_ENABLE

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    letters_process_record(keycode, record);
    // Red flash (same as the RGB min/max feedback) when the default layer changes.
    if (record->event.pressed && keycode >= QK_DEF_LAYER && keycode <= QK_DEF_LAYER_MAX) {
        limit_flash       = true;
        limit_flash_timer = timer_read();
    }
    switch (keycode) {
        case MS_ACC4: // layer 1, F4: 1.0x -> speed level index 4 (mkspd_3)
            if (record->event.pressed) mousekey_set_accel_level(4);
            return false;
        case MS_ACC5: // layer 1, F5: 2.0x -> speed level index 5 (mkspd_4)
            if (record->event.pressed) mousekey_set_accel_level(5);
            return false;
        case LT_CLEAR: // Win Fn (layer 3), Backspace: reset the marquee / letter buffer
            if (record->event.pressed) letters_clear();
            return false;
        case MS_SH1: if (record->event.pressed) shp_toggle(SHP_INFH);   return false; // 1  ∞ infinity
        case MS_SH2: if (record->event.pressed) shp_toggle(SHP_CIRCLE); return false; // 2  circle
        case MS_SH3: if (record->event.pressed) shp_toggle(SHP_TRI);    return false; // 3  triangle
        case MS_SH4: if (record->event.pressed) shp_toggle(SHP_SQUARE); return false; // 4  square
        case MS_SH5: if (record->event.pressed) shp_toggle(SHP_HEX);    return false; // 5  hexagon
        case MS_SH6: if (record->event.pressed) shp_toggle(SHP_STAR);   return false; // 6  star
        case MS_SH7: if (record->event.pressed) shp_toggle(SHP_HEART);  return false; // 7  heart
        case MS_SH8: if (record->event.pressed) shp_toggle(SHP_SPIRO);  return false; // 8  spirograph
        case MS_SH9: if (record->event.pressed) shp_toggle(SHP_SPIRAL); return false; // 9  spiral
        case MS_SH0: if (record->event.pressed) shp_toggle(SHP_LISS);   return false; // 0  lissajous
        case MS_DVD:  if (record->event.pressed) dvd_toggle();  return false; // F9: full-screen DVD bounce
        case MS_DRAW: if (record->event.pressed) draw_start();  return false; // F10: draw 祥沣
        case UG_HUEU: case UG_HUED: case UG_SATU: case UG_SATD:
        case UG_VALU: case UG_VALD: case UG_SPDU: case UG_SPDD:
            if (record->event.pressed) {
                rgb_hold_kc    = keycode; // begin auto-repeat while held
                rgb_hold_timer = timer_read();
                adj_check_kc   = keycode; // check min/max after the step applies
            } else {
                if (rgb_hold_kc == keycode) rgb_hold_kc = 0;
                // persist whatever value the hold reached
                rgb_matrix_sethsv(rgb_matrix_get_hue(), rgb_matrix_get_sat(), rgb_matrix_get_val());
                rgb_matrix_set_speed(rgb_matrix_get_speed());
            }
            return true; // let the normal handler apply the first (tap) step
    }
    return true;
}

void housekeeping_task_user(void) {
    if (rgb_hold_kc && timer_elapsed(rgb_hold_timer) > RGB_HOLD_INTERVAL) {
        rgb_hold_apply(rgb_hold_kc);
        rgb_hold_timer = timer_read();
        limit_check(rgb_hold_kc); // held at a boundary keeps the flash lit
    }
    if (adj_check_kc) { // one-shot check after a tap (value already applied)
        limit_check(adj_check_kc);
        adj_check_kc = 0;
    }

    if (dvd_on && timer_elapsed(dvd_timer) > SHP_INTERVAL) { // full-screen bounce
        dvd_timer   = timer_read();
        uint8_t off = mousekey_get_offset();
        if (off == 0) off = 1;
        float spd = 0.5f + off * 0.05f; // pace follows the mouse-accel level (F1..F5)
        dvd_px += dvd_vx * spd;
        dvd_py += dvd_vy * spd;
        if (dvd_px <= 0.0f)      { dvd_px = 0.0f; dvd_vx = -dvd_vx; }
        else if (dvd_px >= 1.0f) { dvd_px = 1.0f; dvd_vx = -dvd_vx; }
        if (dvd_py <= 0.0f)      { dvd_py = 0.0f; dvd_vy = -dvd_vy; }
        else if (dvd_py >= 1.0f) { dvd_py = 1.0f; dvd_vy = -dvd_vy; }
        digitizer_set_position(dvd_px, dvd_py); // absolute -> bounces off real edges
    }

    if (shp_active && timer_elapsed(shp_timer) > SHP_INTERVAL) {
        shp_timer   = timer_read();
        uint8_t off = mousekey_get_offset();
        if (off == 0) off = 1;
        float dth = 0.02f + off * 0.004f; // step size scales with accel level
        float t0 = shp_theta, t1 = t0 + dth;
        float x0, y0, x1, y1;
        shp_pos(shp_active, t0, &x0, &y0);
        shp_pos(shp_active, t1, &x1, &y1); // sinf/mod are periodic, so t1 > 2pi is fine
        shp_ax += (x1 - x0);
        shp_ay += (y1 - y0);
        shp_theta = (t1 > 6.28318f) ? t1 - 6.28318f : t1;
        int8_t mx = (int8_t)shp_ax, my = (int8_t)shp_ay;
        shp_ax -= mx;
        shp_ay -= my;
        if (mx || my) {
            report_mouse_t rep = {0};
            rep.x = mx;
            rep.y = my;
            host_mouse_send(&rep);
        }
    }

    if (draw_on && timer_elapsed(draw_timer) > SHP_INTERVAL) {
        draw_timer = timer_read();
        if (draw_phase == 2) { // finished: release the button
            report_mouse_t rel = {0};
            host_mouse_send(&rel);
            draw_on = false;
        } else {
            uint8_t off = mousekey_get_offset();
            if (off == 0) off = 1;
            float   step = 2.0f + off * 0.4f;
            uint8_t btn  = (draw_phase == 1) ? 0x01 : 0x00; // left button while drawing a stroke
            uint8_t len  = DRAW_LEN[draw_s];
            float   tx = DRAW_X[draw_base + draw_p], ty = DRAW_Y[draw_base + draw_p];
            float   ex = tx - draw_cx, ey = ty - draw_cy;
            float   dist = sqrtf(ex * ex + ey * ey);
            float   mvx, mvy;
            if (dist <= step + 0.01f) { // reached this point
                mvx = ex; mvy = ey;
                draw_cx = tx; draw_cy = ty;
                if (draw_phase == 0) { // at stroke start -> pen down
                    draw_phase = 1;
                    draw_p     = 1;
                } else {
                    draw_p++;
                    if (draw_p >= len) { // stroke finished -> lift, next stroke
                        draw_phase = 0;
                        draw_base += len;
                        draw_s++;
                        draw_p = 0;
                        if (draw_s >= DRAW_NSTROKE) draw_phase = 2; // whole name done
                    }
                }
            } else {
                mvx = ex / dist * step;
                mvy = ey / dist * step;
                draw_cx += mvx;
                draw_cy += mvy;
            }
            draw_fx += mvx;
            draw_fy += mvy;
            int8_t rx = (int8_t)draw_fx, ry = (int8_t)draw_fy;
            draw_fx -= rx;
            draw_fy -= ry;
            report_mouse_t rep = {0};
            rep.x       = rx;
            rep.y       = ry;
            rep.buttons = btn;
            host_mouse_send(&rep);
        }
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (limit_flash) {
        if (timer_elapsed(limit_flash_timer) < LIMIT_FLASH_MS) {
            for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, 255, 0, 0);
        } else {
            limit_flash = false;
        }
    }
    return true;
}
