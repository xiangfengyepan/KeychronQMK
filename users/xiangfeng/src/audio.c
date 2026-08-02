/* Audio visualizer: a spectrum-analyzer effect driven by the PC companion app
 * (~/audio-keyboard) over Raw HID. The app streams per-band levels; this renders
 * them as vertical EQ bars rising from the bottom of the board — green at the
 * bottom, through yellow, to red at the top (classic VU look). If no packets
 * arrive for a moment the bars fall to black. Scaled by the global brightness
 * so Fn+W/S dims it.
 *
 * Raw HID packet (command 0xAC, 32 bytes, routed here by kc_custom_hid_rx):
 *   [0]=0xAC  [1]=version(0x01)  [2]=band count  [3]=overall RMS  [4..]=band levels 0-255 */

#include "quantum.h"
#include "rgb_matrix.h"
#include <math.h>
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

#define AU_MAXB 28
static volatile uint8_t  au_band[AU_MAXB];
static volatile uint8_t  au_nb  = 0;
static volatile uint8_t  au_rms = 0;
static volatile uint32_t au_rx  = 0; // timer of the last received packet

// Strong override of the weak hook in common/keychron_raw_hid.c (command 0xAC).
void kc_custom_hid_rx(uint8_t *data, uint8_t length) {
    if (length < 4 || data[1] != 0x01) return;            // check version byte
    uint8_t n = data[2]; if (n > AU_MAXB) n = AU_MAXB;
    for (uint8_t i = 0; i < n && (uint8_t)(4 + i) < length; i++) au_band[i] = data[4 + i];
    au_nb  = n;
    au_rms = data[3];
    au_rx  = timer_read32();
}

static float au_disp[AU_MAXB]; // smoothed display level per band, 0..1
static bool  au_bounds_ready = false;
static float au_minx, au_maxx, au_miny, au_maxy;

static void au_bounds(void) {
    au_minx = 255; au_maxx = 0; au_miny = 255; au_maxy = 0;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        float x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        if (x < au_minx) au_minx = x;
        if (x > au_maxx) au_maxx = x;
        if (y < au_miny) au_miny = y;
        if (y > au_maxy) au_maxy = y;
    }
    au_bounds_ready = true;
}

// VU color by bar height h (0 bottom .. 1 top): green -> yellow -> red
static void au_color(float h, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (h < 0.55f) { float f = h / 0.55f;              *r = (uint8_t)(255 * f); *g = 255;                     *b = 0; }
    else           { float f = (h - 0.55f) / 0.45f; if (f > 1) f = 1; *r = 255; *g = (uint8_t)(255 * (1 - f)); *b = 0; }
}

bool audio_effect(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!au_bounds_ready) au_bounds();
    uint8_t gv = rgb_matrix_config.hsv.v;

    if (led_min == 0) { // advance the smoothed bars once per frame
        bool stale = (timer_read32() - au_rx) > 500; // no companion app -> bars fall to black
        for (uint8_t i = 0; i < AU_MAXB; i++) {
            float target = (stale || i >= au_nb) ? 0.0f : au_band[i] / 255.0f;
            if (target > au_disp[i]) au_disp[i] = target;             // rise instantly
            else                     au_disp[i] += (target - au_disp[i]) * 0.28f; // fall smoothly
        }
    }

    uint8_t n     = au_nb ? au_nb : 1;
    float   spanx = (au_maxx > au_minx) ? (au_maxx - au_minx) : 1.0f;
    float   spany = (au_maxy > au_miny) ? (au_maxy - au_miny) : 1.0f;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        float   lx = g_led_config.point[i].x, ly = g_led_config.point[i].y;
        uint8_t band = (uint8_t)((lx - au_minx) / spanx * n);
        if (band >= n) band = n - 1;
        float   level = au_disp[band];
        float   hfrac = (au_maxy - ly) / spany; // 0 at the bottom row .. 1 at the top row
        uint8_t r = 0, g = 0, b = 0;
        // lit only where the bar actually reaches this key. Strict '>' plus a small
        // floor so the bottom row (hfrac~0) is DARK in silence instead of always on.
        if (level > 0.04f && level > hfrac) au_color(hfrac, &r, &g, &b);
        rgb_matrix_set_color(i, scale8(r, gv), scale8(g, gv), scale8(b, gv)); // honors Fn+W/S
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
