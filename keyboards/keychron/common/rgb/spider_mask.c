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
 * Spider-Man mask: a red mask filling the board with two big white angular
 * eyes and faint web strands. The mask breathes, the eyes blink occasionally,
 * and each keypress fires a quick white web-burst from that key.
 *
 * The mask geometry (eye ellipses + web darkening) is computed once into a
 * per-LED colour table; the per-frame loop only does cheap integer work plus
 * the keypress burst, so it stays light on the MCU.
 */

#include "quantum.h"
#include "rgb_matrix.h"
#include <math.h>
#include <lib/lib8tion/lib8tion.h>

#if defined(KEYCHRON_RGB_ENABLE)

// unit-space board size + eye geometry (matches the preview)
#define SM_UW 16.25f
#define SM_UH 6.3f
#define SM_LCX 5.1f  // left-eye centre x (right eye mirrors)
#define SM_ECY 2.15f // eye centre y
#define SM_ERX 2.9f  // eye radius x
#define SM_ERY 1.25f // eye radius y
#define SM_EA 0.34f  // eye tilt (rad)

static uint8_t sm_type[RGB_MATRIX_LED_COUNT]; // 0 = mask, 1 = eye outline, 2 = eye
static uint8_t sm_r[RGB_MATRIX_LED_COUNT], sm_g[RGB_MATRIX_LED_COUNT], sm_b[RGB_MATRIX_LED_COUNT];
static bool    sm_ready = false;

static float sm_eye_d2(float x, float y, float cx, float a) {
    float dx = x - cx, dy = y - SM_ECY;
    float u = dx * cosf(a) + dy * sinf(a);
    float v = -dx * sinf(a) + dy * cosf(a);
    return (u / SM_ERX) * (u / SM_ERX) + (v / SM_ERY) * (v / SM_ERY);
}

// Build the static mask colour table once from the physical LED layout.
static void sm_build(void) {
    uint8_t minx = 255, maxx = 0, miny = 255, maxy = 0;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        uint8_t x = g_led_config.point[i].x, y = g_led_config.point[i].y;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    float rangex = (maxx > minx) ? (float)(maxx - minx) : 1.0f;
    float rangey = (maxy > miny) ? (float)(maxy - miny) : 1.0f;

    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        float x  = (g_led_config.point[i].x - minx) / rangex * SM_UW;
        float y  = (g_led_config.point[i].y - miny) / rangey * SM_UH;
        float dL = sm_eye_d2(x, y, SM_LCX, SM_EA);
        float dR = sm_eye_d2(x, y, SM_UW - SM_LCX, -SM_EA);
        float de = dL < dR ? dL : dR;

        if (de < 1.0f) { // white eye (slight inner shading)
            float sh   = 0.78f + 0.22f * (1.0f - de);
            sm_type[i] = 2;
            sm_r[i]    = (uint8_t)(228 * sh);
            sm_g[i]    = (uint8_t)(238 * sh);
            sm_b[i]    = (uint8_t)(255 * sh);
        } else if (de < 1.5f) { // black eye outline
            sm_type[i] = 1;
            sm_r[i] = 6; sm_g[i] = 4; sm_b[i] = 8;
        } else { // red mask + faint web strands
            float ang  = atan2f(y - 2.6f, x - SM_UW * 0.5f);
            float sp   = powf(fabsf(cosf(ang * 3.5f)), 18);
            float dist = sqrtf((x - SM_UW * 0.5f) * (x - SM_UW * 0.5f) + (y - 2.6f) * (y - 2.6f));
            float rr   = powf(0.5f + 0.5f * cosf(dist * 2.1f), 16);
            float w    = sp > (rr * 0.8f) ? sp : (rr * 0.8f);
            if (w > 1.0f) w = 1.0f;
            w *= 0.55f;
            sm_type[i] = 0;
            sm_r[i]    = (uint8_t)(205 * (1 - w) + 64 * w);
            sm_g[i]    = (uint8_t)(22 * (1 - w) + 6 * w);
            sm_b[i]    = (uint8_t)(28 * (1 - w) + 10 * w);
        }
    }
    sm_ready = true;
}

bool spider_mask(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (!sm_ready) sm_build();

    uint8_t gv    = rgb_matrix_config.hsv.v;
    uint8_t br    = 190 + scale8(sin8((uint8_t)(g_rgb_timer >> 5)), 65); // breathe 190..255
    bool    blink = (g_rgb_timer % 4000) < 130;                          // brief blink every 4s
    uint8_t count = g_last_hit_tracker.count;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint8_t r = sm_r[i], g = sm_g[i], b = sm_b[i];

        if (sm_type[i] == 0) { // mask breathes
            r = scale8(r, br);
            g = scale8(g, br);
            b = scale8(b, br);
        } else if (sm_type[i] == 2 && blink) { // eyes shut -> dim red
            r = 90; g = 10; b = 14;
        }

        // quick white web-burst from recent key presses
        if (count) {
            uint8_t  px = g_led_config.point[i].x, py = g_led_config.point[i].y;
            uint16_t aw = 0;
            for (uint8_t j = 0; j < count; j++) {
                uint16_t tk = g_last_hit_tracker.tick[j];
                if (tk > 320) continue;
                int16_t dx   = (int16_t)px - g_last_hit_tracker.x[j];
                int16_t dy   = (int16_t)py - g_last_hit_tracker.y[j];
                uint8_t dist = sqrt16(dx * dx + dy * dy);
                int16_t R    = tk / 4; // expanding ring radius (px)
                int16_t diff = dist - R;
                if (diff < 0) diff = -diff;
                if (diff < 18) {
                    uint8_t env  = 255 - (uint8_t)((uint32_t)tk * 255 / 320);
                    uint8_t band = 255 - (uint8_t)((uint16_t)diff * 255 / 18);
                    aw += scale8(scale8(env, band), 200);
                }
            }
            if (aw) {
                if (aw > 255) aw = 255;
                r = qadd8(r, (uint8_t)aw);
                g = qadd8(g, (uint8_t)aw);
                b = qadd8(b, (uint8_t)aw);
            }
        }

        rgb_matrix_set_color(i, scale8(r, gv), scale8(g, gv), scale8(b, gv));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif
