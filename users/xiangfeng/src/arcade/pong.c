/* Pong — 2-player, rows 0-4. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/palette.h"
#include "include/arcade_internal.h"
#include <math.h>

/* ================= PONG — 2-player, rows 0-4 =================
 * Ported 1:1 from arcade.html (pong* functions). 1x2 paddles at col 0 (left) and col 12
 * (right), 1x1 ball. Left paddle: '<' up / Left-Win down. Right paddle: Up / Down arrows.
 * First to 3 wins. 3 s serve freeze (blinks) after each point; paddles stay movable. */
#define P_ROWS  5
#define P_LCOL  0
#define P_RCOL  12
#define PAD_H   2
#define P_WIN   3
#define P_SERVE_WAIT 3000
static float    pL, pR, pBx, pBy, pVx, pVy;
static uint8_t  pScL, pScR;
static bool     pWait, p_lu, p_ld, p_ru, p_rd;
static int8_t   pServeDir; static uint32_t pServeT;
static void pong_serve(int8_t dir) {                // recenter + freeze; launches after P_SERVE_WAIT
    pBx = (P_LCOL + P_RCOL) / 2.0f; pBy = 2; pVx = 0; pVy = 0; pWait = true; pServeDir = dir; pServeT = timer_read32();
}
static void pong_launch(void) {
    pWait = false; pVx = pServeDir * 0.085f;
    pVy = (((float)(rnd() % 1000) / 1000.0f) * 0.9f - 0.45f) * 0.085f;
}
void pong_start(void) {
    pL = 1.5f; pR = 1.5f; pScL = 0; pScR = 0; p_lu = p_ld = p_ru = p_rd = false;
    pong_serve((rnd() & 1) ? 1 : -1); phys_last = timer_read32(); st = A_PONG;
}
static const HSV PONG_CY = COL_CYAN, PONG_MG = COL_MAGENTA;
static void pong_end(bool left_won) {               // winner overlay reuses the score fill screen
    RGB c = hsv_to_rgb(left_won ? PONG_CY : PONG_MG);
    sc_frac = 1.0f; sR = c.r; sG = c.g; sB = c.b; sc_t = timer_read32(); st = A_SCORE;
}
void pong_tick(void) {
    uint32_t now = timer_read32(); float dt = (float)(now - phys_last); phys_last = now;
    float s = dt / 16.67f; if (s > 3) s = 3;
    float pv = 0.14f * s, lim = P_ROWS - PAD_H;
    if (p_lu) pL -= pv;                             // paddles move even while frozen
    if (p_ld) pL += pv;
    if (p_ru) pR -= pv;
    if (p_rd) pR += pv;
    if (pL < 0)   pL = 0;
    if (pL > lim) pL = lim;
    if (pR < 0)   pR = 0;
    if (pR > lim) pR = lim;
    if (pWait) {                                    // ball held at center; launch after the freeze
        pBx = (P_LCOL + P_RCOL) / 2.0f; pBy = 2;
        if (timer_elapsed32(pServeT) >= P_SERVE_WAIT) pong_launch(); else return;
    }
    pBx += pVx * s; pBy += pVy * s;
    if (pBy < 0) { pBy = 0; pVy = fabsf(pVy); }
    if (pBy > P_ROWS - 1) { pBy = P_ROWS - 1; pVy = -fabsf(pVy); }
    int8_t lr = iround(pL), rr = iround(pR), by = iround(pBy);
    if (pBx <= P_LCOL && pVx < 0 && (by == lr || by == lr + 1)) { pVx = fabsf(pVx); pBx = P_LCOL; pVy += (pBy - (pL + PAD_H / 2.0f)) * 0.04f; pVx *= 1.03f; }
    if (pBx >= P_RCOL && pVx > 0 && (by == rr || by == rr + 1)) { pVx = -fabsf(pVx); pBx = P_RCOL; pVy += (pBy - (pR + PAD_H / 2.0f)) * 0.04f; pVx *= 1.03f; }
    if (pBx < P_LCOL - 0.5f)      { if (++pScR >= P_WIN) { pong_end(false); return; } pong_serve(-1); } // scored on left -> serve toward left
    else if (pBx > P_RCOL + 0.5f) { if (++pScL >= P_WIN) { pong_end(true);  return; } pong_serve(1);  }
}
void pong_render(void) {
    RGB cy = hsv_to_rgb(PONG_CY), mg = hsv_to_rgb(PONG_MG);
    int8_t lr = iround(pL), rr = iround(pR);
    for (uint8_t i = 0; i < PAD_H; i++) { px(lr + i, P_LCOL, cy.r, cy.g, cy.b); px(rr + i, P_RCOL, mg.r, mg.g, mg.b); }
    bool blink = !pWait || ((P_SERVE_WAIT - timer_elapsed32(pServeT)) / 300) % 2 == 0; // freeze = blinking countdown
    int8_t bx = iround(pBx), by = iround(pBy);
    if (blink && bx >= 0 && bx <= P_RCOL && by >= 0 && by < P_ROWS) px(by, bx, 255, 230, 120);
    static const uint8_t LPIP[3] = {0, 1, 2}, RPIP[3] = {14, 13, 12};   // score pips on the free space row (row 5)
    for (uint8_t i = 0; i < pScL && i < 3; i++) px(5, LPIP[i], cy.r, cy.g, cy.b);
    for (uint8_t i = 0; i < pScR && i < 3; i++) px(5, RPIP[i], mg.r, mg.g, mg.b);
}
/* paddles are held keys (track press + release). Called from arcade_key while A_PONG. */
void pong_key(uint8_t row, uint8_t col, bool pressed) {
    if      (row == 4 && col == 1)  p_lu = pressed; // '<'  = left paddle up
    else if (row == 5 && col == 1)  p_ld = pressed; // LWin = left paddle down
    else if (row == 4 && col == 14) p_ru = pressed; // Up   = right paddle up
    else if (row == 5 && col == 13) p_rd = pressed; // Down = right paddle down
}
