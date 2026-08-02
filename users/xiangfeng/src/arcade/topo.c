/* Topo (whack-a-mole) — whole board. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"
#include <math.h>

/* ================= TOPO — whole board ================= */
#define TOPO_MAX 8
#define MOLE_LIFE 3000
static struct { uint8_t r, c; uint32_t born; } mole[TOPO_MAX];
static uint8_t  nmole = 0;
static uint16_t topo_score = 0;
static uint32_t spawn_t = 0;
void topo_start(void) { nmole = 0; topo_score = 0; spawn_t = timer_read32(); st = A_TOPO; }
void topo_tick(void) {
    uint32_t now = timer_read32();
    for (uint8_t i = 0; i < nmole; i++) if (timer_elapsed32(mole[i].born) > MOLE_LIFE) { game_over(topo_score, 1); return; }
    float   interval = 230.0f + 1170.0f * expf(-(float)topo_score / 25.0f); // exp per-hit spawn ramp
    uint8_t maxc     = 1 + topo_score / 10; if (maxc > TOPO_MAX) maxc = TOPO_MAX;
    if (timer_elapsed32(spawn_t) >= (uint32_t)interval && nmole < maxc && nC) {
        for (uint8_t tr = 0; tr < 24; tr++) {
            uint16_t idx = rnd() % nC; uint8_t r = cR[idx], c = cC[idx];
            bool used = false;
            for (uint8_t i = 0; i < nmole; i++) if (mole[i].r == r && mole[i].c == c) { used = true; break; }
            if (!used) { mole[nmole].r = r; mole[nmole].c = c; mole[nmole].born = now; nmole++; break; }
        }
        spawn_t = now;
    }
}
void topo_render(void) {
    for (uint8_t i = 0; i < nmole; i++) {
        uint32_t age = timer_elapsed32(mole[i].born); if (age > MOLE_LIFE) age = MOLE_LIFE;
        uint8_t g = 40 + (uint8_t)((215u * (MOLE_LIFE - age)) / MOLE_LIFE); // fresh=yellow -> old=red
        px(mole[i].r, mole[i].c, 255, g, 40);
    }
}
void topo_hit(uint8_t r, uint8_t c) {
    for (uint8_t i = 0; i < nmole; i++) if (mole[i].r == r && mole[i].c == c) { mole[i] = mole[--nmole]; topo_score++; return; }
    game_over(topo_score, 1); // wrong key = miss = over
}
