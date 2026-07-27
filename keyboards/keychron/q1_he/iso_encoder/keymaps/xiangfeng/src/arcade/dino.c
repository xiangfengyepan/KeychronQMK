/* Dino — ground runner. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"

/* ================= DINO — ground runner =================
 * Dino at col 2. Bottom obstacles (row 4): jump (knob). Top obstacles (row 3): duck (hold a key). */
#define D_COL 2
static float    d_bottom, d_vy, d_speed;
static bool     d_duck, d_space_held;
static uint16_t d_score;
static int32_t  d_spawn;
static struct { float col; uint8_t row; bool passed; } d_obs[8];
static uint8_t  d_no;
void dino_start(void) {
    d_bottom = 4; d_vy = 0; d_no = 0; d_score = 0; d_speed = 0.06f; d_spawn = 700;
    d_duck = false; d_space_held = false; phys_last = timer_read32(); st = A_DINO;
}
void dino_jump(void) { if (d_bottom >= 3.99f) d_vy = -0.35f; } // impulse: tap clears ~2 rows
void dino_tick(void) {
    uint32_t now = timer_read32(); float dt = (float)(now - phys_last); phys_last = now;
    float s = dt / 16.67f;
    float g = (d_vy > 0 && d_space_held) ? 0.0035f : 0.03f; // tap = ~2-row jump; HOLD while falling = strong float (~2× reach); release → instantly back to 0.03
    d_vy += g * s; d_bottom += d_vy * s;
    if (d_bottom >= 4) { d_bottom = 4; d_vy = 0; }
    if (d_bottom < 1) { d_bottom = 1; if (d_vy < 0) d_vy = 0; }
    bool grounded = d_bottom >= 3.99f, ducking = d_duck && grounded;
    d_speed += 0.000015f * dt; d_spawn -= (int32_t)dt;
    if (d_spawn <= 0 && d_no < 8) { bool top = (rnd() % 5) < 2; d_obs[d_no].col = 14; d_obs[d_no].row = top ? 3 : 4; d_obs[d_no].passed = false; d_no++; d_spawn = 850 + rnd() % 450; }
    for (uint8_t i = 0; i < d_no; i++) d_obs[i].col -= d_speed * s;
    for (uint8_t i = 0; i < d_no; i++) if (!d_obs[i].passed && d_obs[i].col < D_COL) { d_obs[i].passed = true; d_score++; }
    uint8_t w = 0; for (uint8_t i = 0; i < d_no; i++) if (d_obs[i].col > -1.0f) d_obs[w++] = d_obs[i]; d_no = w;
    int8_t rb = iround(d_bottom), c0 = ducking ? 4 : rb - 1, c1 = ducking ? 4 : rb; // occupied cells
    for (uint8_t i = 0; i < d_no; i++)
        if (iround(d_obs[i].col) == D_COL && (d_obs[i].row == c0 || d_obs[i].row == c1)) { game_over(d_score, 3); return; }
}
void dino_render(void) {
    for (uint8_t c = 0; c < 14; c++) px(4, c, 16, 16, 20);      // ground line
    for (uint8_t i = 0; i < d_no; i++) {
        int8_t c = iround(d_obs[i].col); if (c < 0 || c > 13) continue;
        if (d_obs[i].row == 3) px(3, c, 235, 120, 40); else px(4, c, 230, 80, 40);
    }
    bool grounded = d_bottom >= 3.99f, ducking = d_duck && grounded;
    if (ducking) px(4, D_COL, 110, 230, 140);
    else { int8_t rb = iround(d_bottom); for (int8_t r = rb - 1; r <= rb; r++) if (r >= 0 && r < 5) px(r, D_COL, 110, 230, 140); }
}
/* Space = jump (tap ~2 rows; hold = float down slower); Ctrl = duck. Called from arcade_key while A_DINO. */
void dino_key(uint8_t row, uint8_t col, bool pressed) {
    if (row == 5 && col == 6) { d_space_held = pressed; if (pressed) dino_jump(); }
    else if (row == 5 && (col == 0 || col == 11)) d_duck = pressed;
}
