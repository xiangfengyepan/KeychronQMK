/* Flappy — 5 rows tall, scrolls right->left. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"

/* ================= FLAPPY — 5 rows tall, scrolls right->left =================
 * Bird fixed at col 2; pipes have a 3-tall gap. Ported 1:1 from arcade.html. */
#define FB_COL 2
static float    f_bt, f_vy, f_speed;
static bool     f_started;
static uint16_t f_score;
static int32_t  f_spawn;
static struct { float col; uint8_t gap; bool passed; } f_pipe[6];
static uint8_t  f_np;
void flappy_start(void) {
    f_bt = 1.5f; f_vy = 0; f_np = 0; f_score = 0; f_speed = 0.05f; f_spawn = 1400;
    f_started = false; phys_last = timer_read32(); st = A_FLAPPY;
}
void flappy_flap(void) { f_started = true; f_vy = -0.066f; } // ~1-block lift
void flappy_tick(void) {
    uint32_t now = timer_read32(); float dt = (float)(now - phys_last); phys_last = now;
    float s = dt / 16.67f;
    if (!f_started) return;
    f_vy += 0.0022f * s; if (f_vy > 0.13f) f_vy = 0.13f; f_bt += f_vy * s;
    if (f_bt < 0) { f_bt = 0; f_vy = 0; }                       // bonk the ceiling, don't die
    if (f_bt > 5) { game_over(f_score, 2); return; }            // hit the floor (space row); ZXCV row survivable
    f_speed += 0.000012f * dt; f_spawn -= (int32_t)dt;
    if (f_spawn <= 0 && f_np < 6) { f_pipe[f_np].col = 14; f_pipe[f_np].gap = rnd() % 3; f_pipe[f_np].passed = false; f_np++; f_spawn = 1600; }
    for (uint8_t i = 0; i < f_np; i++) f_pipe[i].col -= f_speed * s;
    for (uint8_t i = 0; i < f_np; i++) if (!f_pipe[i].passed && f_pipe[i].col < FB_COL) { f_pipe[i].passed = true; f_score++; }
    uint8_t w = 0; for (uint8_t i = 0; i < f_np; i++) if (f_pipe[i].col > -1.0f) f_pipe[w++] = f_pipe[i]; f_np = w;
    int8_t br = iround(f_bt);
    for (uint8_t i = 0; i < f_np; i++)
        if (iround(f_pipe[i].col) == FB_COL && (br < f_pipe[i].gap || br > f_pipe[i].gap + 2)) { game_over(f_score, 2); return; }
}
void flappy_render(void) {
    for (uint8_t i = 0; i < f_np; i++) {
        int8_t c = iround(f_pipe[i].col); if (c < 0 || c > 13) continue;
        for (uint8_t r = 0; r < 5; r++) if (r < f_pipe[i].gap || r > f_pipe[i].gap + 2) px(r, c, 40, 190, 60);
    }
    int8_t br = iround(f_bt); if (br < 0) br = 0; if (br > 5) br = 5;
    px(br, FB_COL, 255, 220, 40);
}
