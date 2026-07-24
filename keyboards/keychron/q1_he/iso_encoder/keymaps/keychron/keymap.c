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

// Capture typed keys for the LETTERS_MARQUEE / LETTERS_BIG RGB effects.
extern void letters_process_record(uint16_t keycode, keyrecord_t *record);
extern void letters_clear(void); // wipe the marquee / letter buffer

// Extra persistent mouse-speed levels (beyond built-in ACCEL0/1/2).
extern void mousekey_set_accel_level(uint8_t level);
enum custom_keycodes { MS_ACC4 = SAFE_RANGE, MS_ACC5, LT_CLEAR };

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
        { 0x0000, 0x00DD, 0x00DE, 0x00DF, MS_ACC4, MS_ACC5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00D3 },
        { 0x5242, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, LT_CLEAR, 0x00D9 },
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
        { 0x5241, 0x7E0B, 0x7E0C, 0x7E0D, 0x7E0E, 0x7700, 0x7701, 0x7702, 0x7703, 0x7704, 0x7705, 0x7706, 0x7707, 0x0001, 0x0049 },
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
    switch (keycode) {
        case MS_ACC4: // layer 1, F4: 1.0x -> speed level index 4 (mkspd_3)
            if (record->event.pressed) mousekey_set_accel_level(4);
            return false;
        case MS_ACC5: // layer 1, F5: 2.0x -> speed level index 5 (mkspd_4)
            if (record->event.pressed) mousekey_set_accel_level(5);
            return false;
        case LT_CLEAR: // layer 1, Backspace: reset the marquee / letter buffer
            if (record->event.pressed) letters_clear();
            return false;
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
