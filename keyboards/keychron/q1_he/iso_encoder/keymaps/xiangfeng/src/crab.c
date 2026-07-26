/* Claude Code crab: a little clay-orange crab (hand-painted sprite) scuttles the
 * dark board. Its two eyes are gaps in the shell; the legs wiggle as it walks.
 * Press a key on its shell and it STOPS, hops, flashes gold and throws expanding
 * rings, then skitters off. Background stays dark — only the crab glows.
 *
 * The sprite is the exact set of keys painted in the key-painter tool, mapped to
 * the physical LED positions (g_led_config.point) and stored as offsets from the
 * crab's centre, so it moves as one rigid sprite. */

#include "quantum.h"
#include "rgb_matrix.h"
#include "include/palette.h" // named color constants (COL_ORANGE / COL_RED)
#include <math.h>
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

// painted shell cells (matrix row,col); the eyes (1,6) & (1,9) are intentionally absent -> dark gaps
static const uint8_t CRAB_CELLS[][2] = {
    {0,5},{0,6},{0,7},{0,8},{0,9},
    {1,5},{1,7},{1,8},{1,10},
    {2,3},{2,4},{2,5},{2,6},{2,7},{2,8},{2,9},{2,10},{2,11},
    {3,4},{3,5},{3,6},{3,7},{3,8},{3,9},{3,10},
    {4,4},{4,6},{4,8},{4,10},
};
#define CRAB_N (sizeof(CRAB_CELLS) / sizeof(CRAB_CELLS[0]))
static const uint8_t CRAB_EYES[][2] = {{1,6},{1,9}};       // the two eyes -> red
#define CRAB_EYE_N (sizeof(CRAB_EYES) / sizeof(CRAB_EYES[0]))
#define CRAB_MAX (CRAB_N + CRAB_EYE_N)

static struct { float dx, dy; bool leg, eye; } cr_part[CRAB_MAX]; // offset from centre (px)
static float   cr_px[CRAB_MAX], cr_py[CRAB_MAX];                  // this-frame render position of each part
static uint8_t cr_body_r, cr_body_g, cr_body_b;                  // shell, from COL_ORANGE
static uint8_t cr_eye_r, cr_eye_g, cr_eye_b;                     // eyes,  from COL_RED
static uint8_t cr_np = 0;
static bool    cr_ready = false;
static float   cr_x, cr_y, cr_tx, cr_ty, cr_walk;
static uint32_t cr_cel = 0, cr_last = 0;
static float   cr_minx, cr_maxx, cr_miny, cr_maxy, cr_hw, cr_hh;
static uint16_t cr_seed = 1;
static uint16_t cr_rnd(void) { cr_seed = cr_seed * 25173u + 13849u; return cr_seed; }
static float cr_frand(void) { return (cr_rnd() % 1000) / 1000.0f; }

static void cr_pick(void) {
    cr_tx = cr_minx + cr_hw + cr_frand() * fmaxf(0.1f, cr_maxx - cr_minx - 2 * cr_hw);
    cr_ty = cr_miny + cr_hh + cr_frand() * fmaxf(0.1f, cr_maxy - cr_miny - 2 * cr_hh);
}
static void cr_build(void) {
    cr_minx = 255; cr_maxx = 0; cr_miny = 255; cr_maxy = 0;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        float x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        if (x < cr_minx) cr_minx = x;
        if (x > cr_maxx) cr_maxx = x;
        if (y < cr_miny) cr_miny = y;
        if (y > cr_maxy) cr_maxy = y;
    }
    float px[CRAB_MAX], py[CRAB_MAX]; bool leg[CRAB_MAX], eyf[CRAB_MAX]; float sx = 0, sy = 0; cr_np = 0;
    uint8_t nbody = 0;
    for (uint16_t j = 0; j < CRAB_N; j++) {                 // shell cells (centroid uses these)
        uint8_t led = g_led_config.matrix_co[CRAB_CELLS[j][0]][CRAB_CELLS[j][1]];
        if (led == NO_LED) continue;
        px[cr_np] = g_led_config.point[led].x; py[cr_np] = g_led_config.point[led].y;
        leg[cr_np] = (CRAB_CELLS[j][0] == 4); eyf[cr_np] = false;
        sx += px[cr_np]; sy += py[cr_np]; cr_np++; nbody++;
    }
    for (uint16_t j = 0; j < CRAB_EYE_N; j++) {             // eyes
        uint8_t led = g_led_config.matrix_co[CRAB_EYES[j][0]][CRAB_EYES[j][1]];
        if (led == NO_LED) continue;
        px[cr_np] = g_led_config.point[led].x; py[cr_np] = g_led_config.point[led].y;
        leg[cr_np] = false; eyf[cr_np] = true; cr_np++;
    }
    float cx = sx / nbody, cy = sy / nbody; cr_hw = 0; cr_hh = 0;
    for (uint8_t j = 0; j < cr_np; j++) {
        cr_part[j].dx = px[j] - cx; cr_part[j].dy = py[j] - cy; cr_part[j].leg = leg[j]; cr_part[j].eye = eyf[j];
        if (fabsf(cr_part[j].dx) > cr_hw) cr_hw = fabsf(cr_part[j].dx);
        if (fabsf(cr_part[j].dy) > cr_hh) cr_hh = fabsf(cr_part[j].dy);
    }
    cr_hw += 6; cr_hh += 6;
    cr_x = (cr_minx + cr_maxx) / 2; cr_y = (cr_miny + cr_maxy) / 2;
    RGB o = hsv_to_rgb((HSV)COL_ORANGE); cr_body_r = o.r; cr_body_g = o.g; cr_body_b = o.b; // shell
    RGB e = hsv_to_rgb((HSV)COL_RED);    cr_eye_r  = e.r; cr_eye_g  = e.g; cr_eye_b  = e.b; // eyes
    cr_seed ^= (uint16_t)timer_read32(); cr_pick();
    cr_last = timer_read32(); cr_ready = true;
}

bool crab_effect(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!cr_ready) cr_build();
    uint8_t  gv  = rgb_matrix_config.hsv.v;
    uint32_t now = timer_read32();

    if (led_min == 0) {                                        // advance the sim once per frame
        float dt = (now - cr_last) / 1000.0f; if (dt > 0.05f) dt = 0.05f; cr_last = now;
        bool  cel = now < cr_cel;
        static bool was_cel = false;
        if (was_cel && !cel) cr_pick();                        // celebration over -> new spot
        was_cel = cel;
        if (!cel) {
            float dx = cr_tx - cr_x, dy = cr_ty - cr_y, dd = sqrtf(dx * dx + dy * dy);
            if (dd < 6) cr_pick();
            else { float v = 14.0f * dt / dd; cr_x += dx * v; cr_y += dy * v; }  // ~1 key/sec
            cr_walk += dt * 4.5f;
            for (uint8_t j = 0; j < g_last_hit_tracker.count; j++) {             // fresh press on the shell?
                if (g_last_hit_tracker.tick[j] >= 20) continue;
                float hx = g_last_hit_tracker.x[j], hy = g_last_hit_tracker.y[j], best = 1e9f;
                for (uint8_t p = 0; p < cr_np; p++) {
                    float ex = hx - (cr_x + cr_part[p].dx), ey = hy - (cr_y + cr_part[p].dy);
                    float d = ex * ex + ey * ey; if (d < best) best = d;
                }
                if (best < 12 * 12) { cr_cel = now + 1200; break; }             // -> celebrate
            }
        } else cr_walk += dt * 1.6f;

        float bob = 1.5f * sinf(cr_walk * 2), sway = 2.0f * sinf(cr_walk);
        for (uint8_t p = 0; p < cr_np; p++) {
            float legb = 0;
            if (cr_part[p].leg) { legb = 2.0f * sinf(cr_walk * 3 + cr_part[p].dx * 0.09f); if (legb < 0) legb = 0; legb *= 1.8f; }
            cr_px[p] = cr_x + cr_part[p].dx + sway;
            cr_py[p] = cr_y + cr_part[p].dy + bob + legb;
        }
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        float lx = g_led_config.point[i].x, ly = g_led_config.point[i].y;
        float bd = 1e9f; uint8_t bp = 255;
        for (uint8_t p = 0; p < cr_np; p++) { float ex = lx - cr_px[p], ey = ly - cr_py[p]; float d = ex * ex + ey * ey; if (d < bd) { bd = d; bp = p; } }
        uint8_t r = 0, g = 0, b = 0;
        const float TH = 10.0f;
        if (bp != 255 && bd < TH * TH) {                       // covered by the crab
            float c = 1.0f - sqrtf(bd) / TH; if (c < 0) c = 0; float cc = c * c;
            if (cr_part[bp].eye) { r = (uint8_t)(cc * cr_eye_r);  g = (uint8_t)(cc * cr_eye_g);  b = (uint8_t)(cc * cr_eye_b); }  // red  HSV(0,255,255)
            else                 { r = (uint8_t)(cc * cr_body_r); g = (uint8_t)(cc * cr_body_g); b = (uint8_t)(cc * cr_body_b); } // orange HSV(10,255,255)
        }
        rgb_matrix_set_color(i, scale8(r, gv), scale8(g, gv), scale8(b, gv));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
