/* Tetris — 5 across x 13 fall. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/palette.h"
#include "include/arcade_internal.h"
#include <string.h>

/* ================= TETRIS — 5 across x 13 fall ================= */
#define TWID 5
#define TLEN 13
// 5x13 well: every cell maps to a real key. mcol(a,f) = matrix col. Only the ZXCV row (a=4) steps over
// the one keyless spot (matrix col 11) by sliding its last two cells to matrix cols 12 & 13.
static inline uint8_t mcol(uint8_t a, uint8_t f) { return (a == 4 && f >= 11) ? f + 1 : f; }
static uint8_t  tb[TLEN][TWID];
static int8_t   pty, prot, pf, pa;
static bool     plive;
static uint32_t grav_t;
static uint16_t grav_ms, tlines;
static uint8_t  bag[7], bagi = 7;
static const int8_t PIECES[7][4][4][2] = {
 {{{1,0},{1,1},{1,2},{1,3}},{{0,2},{1,2},{2,2},{3,2}},{{2,0},{2,1},{2,2},{2,3}},{{0,1},{1,1},{2,1},{3,1}}}, // I
 {{{0,1},{0,2},{1,1},{1,2}},{{0,1},{0,2},{1,1},{1,2}},{{0,1},{0,2},{1,1},{1,2}},{{0,1},{0,2},{1,1},{1,2}}}, // O
 {{{0,1},{1,0},{1,1},{1,2}},{{0,1},{1,1},{1,2},{2,1}},{{1,0},{1,1},{1,2},{2,1}},{{0,1},{1,0},{1,1},{2,1}}}, // T
 {{{0,1},{0,2},{1,0},{1,1}},{{0,1},{1,1},{1,2},{2,2}},{{1,1},{1,2},{2,0},{2,1}},{{0,0},{1,0},{1,1},{2,1}}}, // S
 {{{0,0},{0,1},{1,1},{1,2}},{{0,2},{1,1},{1,2},{2,1}},{{1,0},{1,1},{2,1},{2,2}},{{0,1},{1,0},{1,1},{2,0}}}, // Z
 {{{0,0},{1,0},{1,1},{1,2}},{{0,1},{0,2},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,0},{2,1}}}, // J
 {{{0,2},{1,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{1,2},{2,0}},{{0,0},{0,1},{1,1},{2,1}}}, // L
};
// tetromino colors = nearest palette constant (index 0 = empty/dark)
static const HSV PC[8] = {{0,0,0}, COL_CYAN, COL_GOLD, COL_PURPLE, COL_GREEN, COL_RED, COL_BLUE, COL_ORANGE};
static uint8_t next_piece(void) {
    if (bagi >= 7) {
        for (uint8_t i = 0; i < 7; i++) bag[i] = i;
        for (uint8_t i = 6; i > 0; i--) { uint8_t j = rnd() % (i + 1), t = bag[i]; bag[i] = bag[j]; bag[j] = t; }
        bagi = 0;
    }
    return bag[bagi++];
}
static bool tfits(uint8_t ty, uint8_t rot, int8_t f, int8_t a) {
    for (uint8_t k = 0; k < 4; k++) {
        int8_t A = a + PIECES[ty][rot][k][0], F = f + PIECES[ty][rot][k][1];
        if (A < 0 || A >= TWID || F < 0 || F >= TLEN || tb[F][A]) return false;
    }
    return true;
}
static void tspawn(void) {
    pty = next_piece(); prot = 0; pf = 0; pa = 1;
    if (!tfits(pty, 0, 0, 1)) { plive = false; game_over(tlines, 0); } else plive = true;
}
static void tlock(void) {
    for (uint8_t k = 0; k < 4; k++) {
        int8_t A = pa + PIECES[pty][prot][k][0], F = pf + PIECES[pty][prot][k][1];
        if (A >= 0 && A < TWID && F >= 0 && F < TLEN) tb[F][A] = pty + 1;
    }
    for (int8_t f = TLEN - 1; f >= 0; f--) {
        bool full = true;
        for (uint8_t a = 0; a < TWID; a++) if (!tb[f][a]) { full = false; break; }
        if (full) {
            tlines++;
            for (int8_t g = f; g > 0; g--) memcpy(tb[g], tb[g - 1], TWID);
            memset(tb[0], 0, TWID);
            f++;
            if (grav_ms > 140) grav_ms -= 18;
        }
    }
    tspawn();
}
void tetris_start(void) { memset(tb, 0, sizeof(tb)); tlines = 0; grav_ms = 650; bagi = 7; grav_t = timer_read32(); st = A_TETRIS; tspawn(); }
void tetris_tick(void) {
    if (!plive) return;
    if (timer_elapsed32(grav_t) > grav_ms) { grav_t = timer_read32(); if (tfits(pty, prot, pf + 1, pa)) pf++; else tlock(); }
}
void tetris_render(void) {
    for (uint8_t f = 0; f < TLEN; f++)
        for (uint8_t a = 0; a < TWID; a++)
            if (tb[f][a]) { RGB c = hsv_to_rgb(PC[tb[f][a]]); px(a, mcol(a, f), c.r, c.g, c.b); } // empty cells stay dark
    if (plive)
        for (uint8_t k = 0; k < 4; k++) {
            int8_t A = pa + PIECES[pty][prot][k][0], F = pf + PIECES[pty][prot][k][1];
            if (A >= 0 && A < TWID && F >= 0 && F < TLEN) { RGB c = hsv_to_rgb(PC[pty + 1]); px(A, mcol(A, F), c.r, c.g, c.b); }
        }
}
void tmove(int8_t d) { if (plive && tfits(pty, prot, pf, pa + d)) pa += d; }
static const int8_t KICK[5] = {0, -1, 1, -2, 2};
void trot(int8_t dir) {
    if (!plive) return;
    uint8_t nr = (prot + (dir < 0 ? 3 : 1)) & 3;
    for (uint8_t i = 0; i < 5; i++) if (tfits(pty, nr, pf, pa + KICK[i])) { prot = nr; pa += KICK[i]; return; }
}
void tdrop(void) { if (!plive) return; while (tfits(pty, prot, pf + 1, pa)) pf++; tlock(); grav_t = timer_read32(); }
