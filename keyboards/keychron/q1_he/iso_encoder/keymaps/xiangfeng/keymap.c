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
 *
 * This file is thin glue: the layout tables, the custom_keycodes enum, the
 * block/lock mode, and the three QMK entry points that dispatch into the
 * feature modules — the mouse-animation engine (src/mouse), the RGB-adjust
 * feedback (src/rgbfx), the pinyin IME (src/ime) and the arcade (src/arcade).
 */

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "include/ime.h"     // baked pinyin IME (compose mode, Fn+I) — logic in src/ime/ime.c
#include "include/arcade.h"  // on-keyboard arcade (Fn+H): lobby, Tetris, Topo
#include "include/palette.h" // named-color calibration RGB effect (knob = next/prev/reset)
#include "include/mouse.h"   // mouse-animation engine (shapes / DVD bounce / draw) — src/mouse
#include "include/rgbfx.h"   // RGB-adjust feedback + palette-knob helpers — src/rgbfx
#include "include/utils.h"   // pal_rgb() for the lock-mode wash

// Capture typed keys for the LETTERS_MARQUEE / LETTERS_BIG RGB effects.
extern void letters_process_record(uint16_t keycode, keyrecord_t *record);
extern void letters_clear(void); // wipe the marquee / letter buffer

// Extra persistent mouse-speed levels (beyond built-in ACCEL0/1/2).
extern void    mousekey_set_accel_level(uint8_t level);
extern uint8_t mousekey_get_accel_level(void);
enum custom_keycodes { MS_ACC4 = SAFE_RANGE, MS_ACC5, LT_CLEAR, MS_DVD,
                       MS_SH1, MS_SH2, MS_SH3, MS_SH4, MS_SH5,
                       MS_SH6, MS_SH7, MS_SH8, MS_SH9, MS_SH0,
                       IME_TOGG, // pinyin IME on/off (Fn+I)
                       MS_STOP,  // stop any running mouse animation (Win Fn + Space)
                       MS_BOOST, // hold to boost mouse speed to F4/1.0x (Win Fn + LShift)
                       BLK_TOGG, // block/lock mode on/off (Win Fn + <, the ISO key left of Z) — swallow all keys
                       LAY_SHOW, // flash the layer meter without changing layer (Win Fn + L)
                       ARCADE }; // open the on-keyboard arcade (Fn + H)

static bool keys_locked = false; // block mode: keypresses don't reach the PC

// Persist block/lock mode in EEPROM so it survives a power cycle.
static void set_locked(bool v) { keys_locked = v; eeconfig_update_user((uint32_t)(v ? 1 : 0)); }
void eeconfig_init_user(void)      { eeconfig_update_user(0); }              // default: unlocked
void keyboard_post_init_user(void) { keys_locked = eeconfig_read_user() & 1u; } // restore on boot

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
        { 0x5242, 0x00DD, 0x00DF, MS_ACC4, MS_ACC5, 0x0000, 0x0000, 0x0000, 0x0000, MS_DVD, 0x0000, 0x0000, 0x0000, 0x0000, 0x00D3 },
        { 0x5242, MS_SH1, MS_SH2, MS_SH3, MS_SH4, MS_SH5, MS_SH6, MS_SH7, MS_SH8, MS_SH9, MS_SH0, 0x0000, 0x0000, 0x0000, 0x00D9 },
        { 0x0000, 0x0000, 0x00CD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, IME_TOGG, 0x0000, 0x0000, 0x0000, 0x0000, 0x00D1, 0x00DA },
        { 0x0000, 0x00CF, 0x00CE, 0x00D0, 0x0000, 0x0000, ARCADE, 0x0000, 0x0000, LAY_SHOW, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00CD },
        { 0x00D4, 0x0000, 0x00D5, 0x0000, 0x0000, 0x0000, MS_STOP, 0x0000, 0x0000, 0x00D2, 0x0000, 0x0000, 0x00CF, 0x00CE, 0x00D0 },
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
        { 0x7820, 0x7821, 0x7827, 0x7823, 0x7825, 0x7829, 0x0001, 0x0001, IME_TOGG, 0x0001, 0x0001, 0x0001, 0x0001, 0x00D1, 0x0001 },
        { 0x0001, 0x7822, 0x7828, 0x7824, 0x7826, 0x782A, ARCADE, 0x0001, 0x0001, LAY_SHOW, 0x0001, 0x0001, 0x0001, 0x004D, 0x0000 },
        { MS_BOOST, BLK_TOGG, PROF1, PROF2, PROF3, 0x0001, 0x7E0F, 0x7013, 0x0001, 0x0001, 0x0001, 0x0000, 0x0001, 0x0001, 0x00CD },
        { 0x00D4, 0x0001, 0x00D5, 0x0000, 0x0000, 0x0000, MS_STOP, 0x0000, 0x0000, 0x00D2, 0x0001, 0x0001, 0x00CF, 0x00CE, 0x00D0 },
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
// NB: with ENCODER_MAP_ENABLE, knob turns arrive as ENCODER_CW/CCW_EVENT through
// process_record_user (handled there for the arcade), NOT via encoder_update_user.

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (arcade_active()) { // arcade takes over the whole keyboard
        if (IS_ENCODEREVENT(record->event)) { // knob turn (arrives via the encoder map)
            if (record->event.pressed) arcade_encoder(record->event.type == ENCODER_CW_EVENT);
            return false;
        }
        if (IS_QK_MOMENTARY(keycode)) return true; // let Fn release process so the layer doesn't latch on
        arcade_key(record->event.key.row, record->event.key.col, record->event.pressed);
        return false;
    }
    if (keys_locked) { // block/lock mode: nothing reaches the PC (e.g. to clean the board)
        if (keycode == BLK_TOGG) { if (record->event.pressed) set_locked(false); return false; }
        if (IS_QK_MOMENTARY(keycode)) return true; // let Fn switch layers so Fn+Z can unlock
        return false;                              // swallow every real key
    }
    if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_PALETTE) { // palette calibration: knob owns colors
        if (IS_ENCODEREVENT(record->event)) {                  // knob turn -> next / previous swatch
            if (record->event.pressed) palette_step(record->event.type == ENCODER_CW_EVENT);
            return false;
        }
        if (record->event.key.row == 0 && record->event.key.col == 14) { // knob push (any layer)
            rgbfx_pal_knob(record->event.pressed);                        // hold = type H,S,V; tap = reset
            return false;
        }
    }
    if (ime_active()) { // compose mode: intercept typing, cycling and confirm (src/ime/ime.c)
        if (ime_process_record(keycode, record)) return false; // key consumed by the IME
        // else fall through so Fn+I (IME_TOGG) can toggle off and modifiers still work
    }
    letters_process_record(keycode, record);
    // Longer red flash when the default layer changes.
    if (record->event.pressed && keycode >= QK_DEF_LAYER && keycode <= QK_DEF_LAYER_MAX) {
        rgbfx_flash_layer();
    }
    switch (keycode) {
        case MS_ACC4: // layer 1, F3: 1.0x -> speed level index 4 (mkspd_3)
            if (record->event.pressed) mousekey_set_accel_level(4);
            return false;
        case MS_ACC5: // layer 1, F4: 2.0x -> speed level index 5 (mkspd_4)
            if (record->event.pressed) mousekey_set_accel_level(5);
            return false;
        case LT_CLEAR: // Win Fn (layer 3), Backspace: reset the marquee / letter buffer
            if (record->event.pressed) letters_clear();
            return false;
        case MS_SH1: if (record->event.pressed) mouse_shape_toggle(SHP_INFH);   return false; // 1  ∞ infinity
        case MS_SH2: if (record->event.pressed) mouse_shape_toggle(SHP_CIRCLE); return false; // 2  circle
        case MS_SH3: if (record->event.pressed) mouse_shape_toggle(SHP_TRI);    return false; // 3  triangle
        case MS_SH4: if (record->event.pressed) mouse_shape_toggle(SHP_SQUARE); return false; // 4  square
        case MS_SH5: if (record->event.pressed) mouse_shape_toggle(SHP_HEX);    return false; // 5  hexagon
        case MS_SH6: if (record->event.pressed) mouse_shape_toggle(SHP_STAR);   return false; // 6  star
        case MS_SH7: if (record->event.pressed) mouse_shape_toggle(SHP_HEART);  return false; // 7  heart
        case MS_SH8: if (record->event.pressed) mouse_shape_toggle(SHP_SPIRO);  return false; // 8  spirograph
        case MS_SH9: if (record->event.pressed) mouse_shape_toggle(SHP_SPIRAL); return false; // 9  spiral
        case MS_SH0: if (record->event.pressed) mouse_shape_toggle(SHP_LISS);   return false; // 0  lissajous
        case MS_DVD:  if (record->event.pressed) mouse_dvd_toggle();  return false; // F9: full-screen DVD bounce
        case BLK_TOGG: // Win Fn (layer 3) < (ISO key left of Z): enter block/lock mode (Fn+< again exits); persists across power-off
            if (record->event.pressed) set_locked(true);
            return false;
        case LAY_SHOW: // Win Fn (layer 3) L: flash the layer meter WITHOUT changing the layer
            if (record->event.pressed) rgbfx_flash_layer();
            return false;
        case ARCADE: // Fn + H: open the on-keyboard arcade (knob-hold to exit)
            if (record->event.pressed) arcade_open(timer_read32());
            return false;
        case MS_STOP: // Win Fn (layer 3) Space: stop any running mouse animation (shape / DVD / draw)
            if (record->event.pressed) mouse_stop();
            return false;
        case MS_BOOST: { // Win Fn (layer 3) LShift: HOLD to boost speed to F4 (1.0x); restore on release
            static uint8_t boost_prev = 4;
            if (record->event.pressed) { boost_prev = mousekey_get_accel_level(); mousekey_set_accel_level(4); }
            else                       { mousekey_set_accel_level(boost_prev); }
            return false;
        }
        case IME_TOGG: // Fn+I: toggle the pinyin IME (resets buffer + draw carriage in ime.c)
            if (record->event.pressed) ime_toggle();
            return false;
        case UG_HUEU: case UG_HUED: case UG_SATU: case UG_SATD:
        case UG_VALU: case UG_VALD: case UG_SPDU: case UG_SPDD:
            if (record->event.pressed) rgbfx_adjust_press(keycode);
            else                       rgbfx_adjust_release(keycode);
            return true; // let the normal handler apply the first (tap) step
    }
    return true;
}

void housekeeping_task_user(void) {
    if (arcade_active()) { arcade_tick(); return; } // arcade runs alone
    rgbfx_task(); // RGB adjust auto-repeat + min/max checks + palette knob-hold typing
    mouse_task(); // DVD bounce / shape mover / character drawing
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (arcade_active()) { arcade_render(led_min, led_max); return false; } // arcade owns the board
    if (keys_locked) { // block/lock mode -> dim wash so it's obvious (COL_GOLD, dim)
        RGB c = pal_rgb((HSV)COL_GOLD, 70);
        for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, c.r, c.g, c.b);
        return false;
    }
    if (ime_active()) { ime_render(led_min, led_max); return false; } // IME owns the board (src/ime/ime.c)
    rgbfx_render(led_min, led_max); // flash feedback + binary value readout, over the normal effect
    return true;
}
