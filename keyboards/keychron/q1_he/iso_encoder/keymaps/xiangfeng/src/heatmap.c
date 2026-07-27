/* Pressure Heatmap: the board is dark until you press. Each key lights by its
 * live analog Hall-effect travel (how far it's pressed) and bleeds heat into its
 * neighbors with a soft falloff — a deeper press spreads farther. Released keys
 * cool back to black over ~a second, like a thermal camera.
 *
 * depth = analog_matrix_get_travel() / full-travel (240 = 4.0 mm).
 * heat[led] rises instantly to the spatial press field and decays slowly.
 * Color is a cool→hot thermal ramp; the whole thing is scaled by the global
 * brightness (Fn+W/S), so it dims like any other effect. */

#include "quantum.h"
#include "rgb_matrix.h"
#include <math.h>
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

extern uint8_t analog_matrix_get_travel(uint8_t row, uint8_t col); // live per-key travel (0..~240)

#define HM_FULL_TRAVEL 240.0f // travel at a full 4.0 mm press (FULL_TRAVEL_UNIT*TRAVEL_SCALE)
#define HM_SBASE       9.0f   // base glow radius (LED px)
#define HM_SGROW       20.0f  // extra radius at full depth -> deeper press spreads farther
#define HM_COOL_MIN    0.4f   // cool-down at RGB speed 0   (slow fade, long heat trails)
#define HM_COOL_MAX    10.0f  // cool-down at RGB speed 255 (fast fade) — tune live with Fn+T/G
#define HM_SRC_MAX     32     // max simultaneous pressed keys tracked

static float    hm_heat[RGB_MATRIX_LED_COUNT];
static uint32_t hm_last  = 0;
static bool     hm_ready = false;

// cool → hot thermal ramp (blue/green → yellow → red → white)
static void hm_thermal(float t, uint8_t *r, uint8_t *g, uint8_t *b) {
    static const float   ts[8] = {0.0f, 0.07f, 0.22f, 0.40f, 0.55f, 0.70f, 0.85f, 1.0f};
    static const uint8_t cr[8] = {0, 8,   0,   0,   0,   225, 240, 255};
    static const uint8_t cg[8] = {0, 2,   70,  190, 200, 200, 55,  255};
    static const uint8_t cb[8] = {0, 70,  205, 190, 45,  0,   0,   255};
    if (t <= 0) { *r = *g = *b = 0;   return; }
    if (t >= 1) { *r = *g = *b = 255; return; }
    for (uint8_t i = 1; i < 8; i++) {
        if (t <= ts[i]) {
            float f = (t - ts[i - 1]) / (ts[i] - ts[i - 1]);
            *r = (uint8_t)(cr[i - 1] + (cr[i] - cr[i - 1]) * f);
            *g = (uint8_t)(cg[i - 1] + (cg[i] - cg[i - 1]) * f);
            *b = (uint8_t)(cb[i - 1] + (cb[i] - cb[i - 1]) * f);
            return;
        }
    }
    *r = *g = *b = 255;
}

bool heatmap_effect(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t  gv  = rgb_matrix_config.hsv.v;
    uint32_t now = timer_read32();
    if (!hm_ready) { for (uint16_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) hm_heat[i] = 0; hm_last = now; hm_ready = true; }

    if (led_min == 0) { // advance the heat field once per frame
        float dt = (now - hm_last) / 1000.0f; if (dt > 0.1f) dt = 0.1f; hm_last = now;
        float cool  = HM_COOL_MIN + (rgb_matrix_config.speed / 255.0f) * (HM_COOL_MAX - HM_COOL_MIN); // RGB speed (Fn+T/G)
        float decay = expf(-dt * cool);

        float   sx[HM_SRC_MAX], sy[HM_SRC_MAX], sd[HM_SRC_MAX]; uint8_t ns = 0; // pressed keys this frame
        for (uint8_t r = 0; r < MATRIX_ROWS && ns < HM_SRC_MAX; r++)
            for (uint8_t c = 0; c < MATRIX_COLS && ns < HM_SRC_MAX; c++) {
                uint8_t led = g_led_config.matrix_co[r][c];
                if (led == NO_LED) continue;
                float d = analog_matrix_get_travel(r, c) / HM_FULL_TRAVEL;
                if (d > 0.04f) { if (d > 1) d = 1; sx[ns] = g_led_config.point[led].x; sy[ns] = g_led_config.point[led].y; sd[ns] = d; ns++; }
            }

        for (uint16_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
            float lx = g_led_config.point[i].x, ly = g_led_config.point[i].y, target = 0;
            for (uint8_t s = 0; s < ns; s++) {
                float sig = HM_SBASE + sd[s] * HM_SGROW;
                float dx = lx - sx[s], dy = ly - sy[s];
                target += sd[s] * expf(-(dx * dx + dy * dy) / (2.0f * sig * sig)); // farther -> smaller
            }
            if (target > 1) target = 1;
            float h = hm_heat[i] * decay;
            hm_heat[i] = target > h ? target : h; // rise instantly, cool slowly
        }
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint8_t r, g, b; hm_thermal(hm_heat[i], &r, &g, &b);
        rgb_matrix_set_color(i, scale8(r, gv), scale8(g, gv), scale8(b, gv)); // honors Fn+W/S brightness
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
