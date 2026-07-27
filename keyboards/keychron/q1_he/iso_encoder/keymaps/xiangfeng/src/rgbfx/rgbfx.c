/* rgbfx.c — RGB-adjust feedback + palette-knob helpers (see include/rgbfx.h). */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/rgbfx.h"
#include "include/palette.h" // named color constants (COL_*) + palette_reset()
#include "include/utils.h"   // pal_rgb() / pal_put_u8()

// Hold-to-repeat for the RGB adjust keys (step is 1, so a hold ramps smoothly).
#define RGB_HOLD_INTERVAL 28  // ms between repeats once auto-repeat is running
#define RGB_HOLD_DELAY    350 // ms you must hold before auto-repeat starts (so a tap = exactly 1 step)
static uint16_t rgb_hold_kc      = 0;
static uint16_t rgb_hold_timer   = 0;
static bool     rgb_hold_started = false; // has auto-repeat kicked in yet?

// Palette-effect knob: turn = next/prev swatch, tap = reset, hold = type the live H,S,V.
#define PAL_KNOB_HOLD_MS 500
static bool     pal_knob_down    = false;
static uint16_t pal_knob_timer   = 0;
static bool     pal_knob_typed   = false; // did this hold already type the value?
static void pal_type_hsv(void) { // type "H,S,V" of the live color over USB (no trailing Enter)
    char b[16]; uint8_t n = 0;
    n += pal_put_u8(b + n, rgb_matrix_get_hue()); b[n++] = ',';
    n += pal_put_u8(b + n, rgb_matrix_get_sat()); b[n++] = ',';
    n += pal_put_u8(b + n, rgb_matrix_get_val());
    b[n] = 0;
    send_string(b);
}
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

// Feedback while adjusting RGB:
//   - min/max of saturation/brightness/speed -> BLINKS red (not a solid hold)
//   - hue is cyclic -> board BLANKS once each time it passes through 0
//     (hue 0 is red, so a red flash there wouldn't be visible)
//   - default-layer change -> solid 1 s red flash (with the green layer meter)
#define LAYER_FLASH_MS 1000 // solid red on layer change
#define BLINK_HALF_MS  110  // min/max blink: red 110 ms on / 110 ms off
#define LIMIT_FLASH_MS 220  // min/max window = one blink period (a tap still shows one pulse)
#define HUE_FLASH_MS   140  // single blackout when hue wraps past 0
enum flash_kind { FLASH_OFF, FLASH_LIMIT, FLASH_LAYER, FLASH_HUE };
static enum flash_kind flash_mode  = FLASH_OFF;
static uint16_t        flash_timer = 0;
static uint16_t        flash_ms    = 0;
static uint16_t        adj_check_kc = 0;
static uint8_t         hue_before   = 0; // hue captured before an adjust, to detect a 0-wrap

// Binary value display: while adjusting an RGB setting, show its 0-255 value as 8 bits
// on the number row (key '1' = MSB (bit7) … key '8' = LSB (bit0)), color-coded per setting.
#define BIN_SHOW_MS 2000                 // keep it lit this long after the last adjust
static uint16_t bin_kc    = 0;           // which UG_* setting is being shown (0 = none)
static uint16_t bin_timer = 0;
static void flash_start(enum flash_kind k, uint16_t ms) { flash_mode = k; flash_timer = timer_read(); flash_ms = ms; }
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
    if (rgb_at_limit(kc)) flash_start(FLASH_LIMIT, LIMIT_FLASH_MS);
}
static void hue_wrap_check(uint16_t kc, uint8_t before) { // cyclic hue crossed 0 -> one flash
    uint8_t now = rgb_matrix_get_hue();
    if ((kc == UG_HUEU && now < before) || (kc == UG_HUED && now > before)) flash_start(FLASH_HUE, HUE_FLASH_MS);
}

// solid red flash + green layer meter (default-layer change or LAY_SHOW)
void rgbfx_flash_layer(void) { flash_start(FLASH_LAYER, LAYER_FLASH_MS); }

void rgbfx_adjust_press(uint16_t keycode) {
    hue_before       = rgb_matrix_get_hue(); // remember, to spot a 0-wrap after the tap
    rgb_hold_kc      = keycode; // arm auto-repeat while held
    rgb_hold_timer   = timer_read();
    rgb_hold_started = false;    // wait RGB_HOLD_DELAY before the first repeat
    adj_check_kc     = keycode;  // check min/max after the step applies
    bin_kc = keycode; bin_timer = timer_read(); // show this setting's value in binary
}

void rgbfx_adjust_release(uint16_t keycode) {
    if (rgb_hold_kc == keycode) rgb_hold_kc = 0;
    // persist whatever value the hold reached
    rgb_matrix_sethsv(rgb_matrix_get_hue(), rgb_matrix_get_sat(), rgb_matrix_get_val());
    rgb_matrix_set_speed(rgb_matrix_get_speed());
}

void rgbfx_pal_knob(bool pressed) {
    if (pressed) { pal_knob_down = true; pal_knob_timer = timer_read(); pal_knob_typed = false; }
    else { pal_knob_down = false; if (!pal_knob_typed) palette_reset(); } // tap = reset (hold already typed)
}

void rgbfx_task(void) {
    if (rgb_hold_kc && timer_elapsed(rgb_hold_timer) > (rgb_hold_started ? RGB_HOLD_INTERVAL : RGB_HOLD_DELAY)) {
        uint8_t h0 = rgb_matrix_get_hue();
        rgb_hold_apply(rgb_hold_kc);
        rgb_hold_timer   = timer_read();
        rgb_hold_started = true;      // now repeat fast
        limit_check(rgb_hold_kc);     // pinned at a boundary re-arms the blink window
        hue_wrap_check(rgb_hold_kc, h0); // one flash each time a held hue passes 0
        bin_timer = timer_read();     // keep the binary readout alive while holding
    }
    if (adj_check_kc) { // one-shot check after a tap (value already applied)
        limit_check(adj_check_kc);
        hue_wrap_check(adj_check_kc, hue_before);
        adj_check_kc = 0;
    }
    if (pal_knob_down && !pal_knob_typed && timer_elapsed(pal_knob_timer) > PAL_KNOB_HOLD_MS) {
        pal_type_hsv();      // knob held long enough -> type the live H,S,V once
        pal_knob_typed = true;
    }
}

void rgbfx_render(uint8_t led_min, uint8_t led_max) {
    if (flash_mode != FLASH_OFF) {
        if (timer_elapsed(flash_timer) < flash_ms) {
            // min/max blinks (free-running clock, so it toggles even while re-armed on hold);
            // layer flash stays solid red; hue-wrap BLANKS the board (hue 0 is red, so a red
            // flash would be invisible — blacking out is what reads).
            bool on = (flash_mode != FLASH_LIMIT) || (timer_read() % (2 * BLINK_HALF_MS)) < BLINK_HALF_MS;
            if (on) {
                RGB fl = pal_rgb((HSV)COL_RED, 255); // min/max + layer flash color
                uint8_t fr = fl.r, fg = fl.g, fb = fl.b;
                if (flash_mode == FLASH_HUE) { fr = fg = fb = 0; } // hue 0 = red -> blank instead
                for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, fr, fg, fb); // whole board
                if (flash_mode == FLASH_LAYER) { // layer change: green layer meter on top, ONLY during the 1 s flash
                    RGB lm = pal_rgb((HSV)COL_GREEN, 220);
                    uint8_t cur = get_highest_layer(layer_state | default_layer_state);
                    for (uint8_t i = 0; i <= cur && i < 4; i++) {
                        uint8_t led = g_led_config.matrix_co[0][1 + i]; // F1..F4 = matrix (0,1)..(0,4)
                        if (led != NO_LED) rgb_matrix_set_color(led, lm.r, lm.g, lm.b);
                    }
                }
            }
        } else {
            flash_mode = FLASH_OFF; // flash done -> back to the normal RGB effect
        }
    }
    // Binary readout of the setting you're adjusting, on the number row.
    // Key '1' = bit 7 (MSB) … key '8' = bit 0 (LSB); lit bit = setting color, clear bit = dim.
    if (bin_kc && timer_elapsed(bin_timer) < BIN_SHOW_MS) {
        uint8_t v = 0, cr = 0, cg = 0, cb = 0;
        switch (bin_kc) {
            case UG_HUEU: case UG_HUED: v = rgb_matrix_get_hue();   cr = 0;   cg = 200; cb = 0;   break; // Hue   → green
            case UG_SATU: case UG_SATD: v = rgb_matrix_get_sat();   cr = 255; cg = 90;  cb = 0;   break; // Sat   → orange
            case UG_VALU: case UG_VALD: v = rgb_matrix_get_val();   cr = 220; cg = 220; cb = 220; break; // Value → white
            case UG_SPDU: case UG_SPDD: v = rgb_matrix_get_speed(); cr = 0;   cg = 180; cb = 200; break; // Speed → cyan
        }
        for (uint8_t b = 0; b < 8; b++) {
            uint8_t led = g_led_config.matrix_co[1][1 + b]; // keys '1'..'8'
            if (led == NO_LED) continue;
            if ((v >> (7 - b)) & 1) rgb_matrix_set_color(led, cr, cg, cb); // 1 bit → setting color
            else                    rgb_matrix_set_color(led, 16, 16, 16); // 0 bit → dim (so all 8 slots show)
        }
    }
}
