/* Palette calibration effect.
 *
 * Fills the whole board with ONE named color at a time so you can eyeball it.
 *   - turn the knob   -> next / previous swatch (palette_step)
 *   - press the knob  -> reset the current swatch to its default (palette_reset)
 *   - HSV adjust keys  -> fine-tune the live color (they edit rgb_matrix_config.hsv)
 *
 * The live color IS rgb_matrix_config.hsv, so the existing HSV macro keys tune it
 * directly (step 1 per tap). Each swatch's default lives in include/palette.h; the
 * knob loads that default into the live HSV, then you nudge and read off the delta. */

#include "quantum.h"
#include "rgb_matrix.h"
#include "include/palette.h"

#if defined(KEYCHRON_RGB_ENABLE)

// ordered list the knob steps through — baked defaults, one per named color in palette.h
// ordered as a walk around the color wheel so visually-close swatches sit next
// to each other on the knob; the near-neutrals (lavender, white) sit at the end.
static const HSV pal_def[] = {
    COL_RED, COL_CORAL, COL_ORANGE, COL_GOLD, COL_LIME,
    COL_GREEN, COL_GREEN_LIGHT, COL_CYAN, COL_SKY, COL_BLUE,
    COL_PURPLE, COL_MAGENTA, COL_ROSE, COL_PINK, COL_WHITE,
};
#define PAL_N (sizeof(pal_def) / sizeof(pal_def[0]))

static HSV     pal_live[PAL_N]; // your in-RAM tuning, seeded from the defaults
static uint8_t pal_i    = 0;
static bool    pal_init = false;

static void pal_init_once(void) {
    if (pal_init) return;
    for (uint8_t i = 0; i < PAL_N; i++) pal_live[i] = pal_def[i];
    pal_init = true;
}
static void pal_load(void)  { rgb_matrix_sethsv_noeeprom(pal_live[pal_i].h, pal_live[pal_i].s, pal_live[pal_i].v); }
static void pal_stash(void) {                        // remember the live edits for the current swatch
    pal_live[pal_i].h = rgb_matrix_get_hue();
    pal_live[pal_i].s = rgb_matrix_get_sat();
    pal_live[pal_i].v = rgb_matrix_get_val();
}

void palette_step(bool forward) {                    // knob turn
    pal_init_once();
    pal_stash();                                     // keep this swatch's tuning before moving on
    pal_i = forward ? (pal_i + 1) % PAL_N : (uint8_t)((pal_i + PAL_N - 1) % PAL_N);
    pal_load();
}
void palette_reset(void) {                           // knob tap -> back to the baked default
    pal_init_once();
    pal_live[pal_i] = pal_def[pal_i];
    pal_load();
}

bool palette_effect(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!pal_init) { pal_init_once(); pal_load(); }  // first entry -> show a defined color
    RGB c = hsv_to_rgb(rgb_matrix_config.hsv);
    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
