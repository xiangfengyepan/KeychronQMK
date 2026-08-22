/* Piano (Keylight): a Hall-effect "graphic EQ piano". The board is dark until
 * you press. Keys are binned into vertical COLUMNS by their physical x-center;
 * each column is a note whose HUE runs by horizontal order — left/low = red,
 * right/high = violet (0 -> 300deg, ~C3 -> C6). Every frame each column takes
 * the MAX analog travel among its keys (analog_matrix_get_travel) -> a target
 * bar height 0..1: press deeper -> the bar climbs higher. The bar rises fast
 * toward the pressed depth and, on release, FALLS SLOWLY (EQ decay, ~700 ms).
 *
 * Render: light a column's keys from the bottom row up to the current height in
 * the column's hue, with the leading (top) tip brightest and the body graded
 * dimmer; unlit keys stay dark. Everything is scaled by the global brightness
 * (rgb_matrix_config.hsv.v -> gv) so Fn+W/S dims it like any other effect. */

#include "quantum.h"
#include "rgb_matrix.h"
#include <math.h>
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

extern uint8_t analog_matrix_get_travel(uint8_t row, uint8_t col); // live per-key travel (0..~240)

#define PN_FULL_TRAVEL 240.0f  // travel at a full 4.0 mm press
#define PN_NCOLS       15      // vertical columns the board is binned into (~one key wide)
#define PN_HUE_SPAN    213     // 300deg in QMK's 0..255 hue (300/360*256): red -> violet
#define PN_RISE_MS     140.0f  // full-scale rise time while pressed (fast climb)
#define PN_DECAY_MS    700.0f  // full-scale fall time on release (slow EQ decay)
#define PN_BANDH       0.20f   // height (in 0..1) of one key row's band, for the tip
#define PN_DEADZONE    0.04f   // travel below this reads as "not pressed"
#define PN_ROWH        0.12f   // ~half a key-row in normalized height: lets the bar reach the top (F) row (whose hfrac is exactly 1.0) on a firm press

static float    pn_level[PN_NCOLS]; // current bar height per column, 0..1
static uint32_t pn_last  = 0;
static bool     pn_ready = false;

static bool  pn_bounds_ready = false;
static float pn_minx, pn_maxx, pn_miny, pn_maxy;

static void pn_bounds(void) {
    pn_minx = 255; pn_maxx = 0; pn_miny = 255; pn_maxy = 0;
    for (uint16_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        float x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        if (x < pn_minx) pn_minx = x;
        if (x > pn_maxx) pn_maxx = x;
        if (y < pn_miny) pn_miny = y;
        if (y > pn_maxy) pn_maxy = y;
    }
    pn_bounds_ready = true;
}

// column index (0..PN_NCOLS-1) for a physical x, by horizontal order
static inline uint8_t pn_col_of(float x) {
    float spanx = (pn_maxx > pn_minx) ? (pn_maxx - pn_minx) : 1.0f;
    int   c     = (int)((x - pn_minx) / spanx * PN_NCOLS);
    if (c < 0) c = 0;
    if (c >= PN_NCOLS) c = PN_NCOLS - 1;
    return (uint8_t)c;
}

bool piano_effect(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!pn_bounds_ready) pn_bounds();
    uint8_t  gv  = rgb_matrix_config.hsv.v;
    uint32_t now = timer_read32();
    if (!pn_ready) { for (uint8_t c = 0; c < PN_NCOLS; c++) pn_level[c] = 0; pn_last = now; pn_ready = true; }

    if (led_min == 0) { // advance the bars once per frame
        float dt = (float)(now - pn_last); if (dt > 100.0f) dt = 100.0f; pn_last = now;

        // per-column target height = max analog travel among the column's keys
        float target[PN_NCOLS];
        for (uint8_t c = 0; c < PN_NCOLS; c++) target[c] = 0.0f;
        for (uint8_t r = 0; r < MATRIX_ROWS; r++)
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                uint8_t led = g_led_config.matrix_co[r][c];
                if (led == NO_LED) continue;
                float d = analog_matrix_get_travel(r, c) / PN_FULL_TRAVEL;
                if (d <= PN_DEADZONE) continue;
                if (d > 1.0f) d = 1.0f;
                uint8_t col = pn_col_of(g_led_config.point[led].x);
                if (d > target[col]) target[col] = d;
            }

        for (uint8_t c = 0; c < PN_NCOLS; c++) {
            if (target[c] > pn_level[c]) {                    // rise fast toward the pressed depth
                pn_level[c] += dt / PN_RISE_MS;
                if (pn_level[c] > target[c]) pn_level[c] = target[c];
            } else {                                          // fall slowly (EQ decay)
                pn_level[c] -= dt / PN_DECAY_MS;
                if (pn_level[c] < target[c]) pn_level[c] = target[c];
                if (pn_level[c] < 0) pn_level[c] = 0;
            }
        }
    }

    float spany = (pn_maxy > pn_miny) ? (pn_maxy - pn_miny) : 1.0f;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        float   lx = g_led_config.point[i].x, ly = g_led_config.point[i].y;
        uint8_t col   = pn_col_of(lx);
        float   level = pn_level[col];
        float   hfrac = (pn_maxy - ly) / spany; // 0 at the bottom row .. 1 at the top row

        uint8_t rr = 0, gg = 0, bb = 0;
        if (level > hfrac - PN_ROWH) {                      // bar has reached this key's row (top/F row reachable near full press)
            float fill = (level - hfrac) / PN_BANDH;         // how far the bar reaches past this key's row
            if (fill > 1.0f) fill = 1.0f;
            if (fill < 0.0f) fill = 0.0f;
            bool  tip    = fill < 0.985f;                    // partially-filled top row = leading edge
            float bright = tip ? (0.55f + 0.45f * fill)      // tip pops brightest
                               : (0.42f + 0.22f * hfrac);    // body grades brighter upward
            uint8_t hue = (uint8_t)(col * PN_HUE_SPAN / (PN_NCOLS - 1));
            HSV hsv = { hue, 255, (uint8_t)(bright * 255.0f) };
            RGB rgb = hsv_to_rgb(hsv);
            rr = rgb.r; gg = rgb.g; bb = rgb.b;
        }
        rgb_matrix_set_color(i, scale8(rr, gv), scale8(gg, gv), scale8(bb, gv)); // honors Fn+W/S
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
