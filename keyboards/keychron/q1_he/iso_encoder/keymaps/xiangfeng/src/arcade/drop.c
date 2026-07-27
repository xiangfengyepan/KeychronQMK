/* Drop-merge (color 2048) — 4x13 well. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/palette.h"
#include "include/arcade_internal.h"
#include <string.h>

/* ================= DROP-MERGE — color 2048 on a 4x13 well =================
 * Ported 1:1 from arcade.html (drop* functions). A colored box falls under gravity
 * down keyboard rows 1-4 (row 0 / F-row is a static tier legend). Merges: a landed box
 * merges VERTICALLY (same-color box below) and HORIZONTALLY (same-color box at the same
 * height in an adjacent lane, pulled into the dropped lane; the neighbor's gap collapses
 * with gravity), promoting to the next tier and chaining fully both ways. Spawn pool
 * grows: tiers 0-2 always spawn; a tier >=3 joins the pool once 3 of it have been made. */
#define DWID     4
#define DLEN     13
#define DSPAWN   1                 // spawn lane (across index a=1)
#define DTIER_MAX 8
static inline uint8_t drow(uint8_t a) { return a + 1; }                        // across a(0..3) -> keyboard row 1..4
static inline uint8_t dcol(uint8_t a, uint8_t f) { return (a == 3 && f >= 11) ? f + 1 : f; } // row 4 steps over keyless col 11
// tier colors: 9 well-separated palette constants, tier 0 (lowest) -> 8 (highest)
static const HSV DTIER[9] = { COL_RED, COL_ORANGE, COL_GOLD, COL_GREEN, COL_CYAN, COL_BLUE, COL_PURPLE, COL_MAGENTA, COL_WHITE };
static uint8_t  dstack[DWID][DLEN], dcount[DWID];   // dstack[a][i] = tier, bottom(0)->up
static int8_t   dbox_a, dbox_f; static uint8_t dbox_color; static bool dbox_live;
static uint32_t dgrav_t; static uint16_t dgrav_ms, dmerges; static uint8_t dmaxtier, dpool, dmade[9];
static void drop_spawn(void) {
    if (dcount[DSPAWN] >= DLEN) { dbox_live = false; game_over(dmaxtier, 6); return; }
    dbox_color = rnd() % (dpool + 1); dbox_a = DSPAWN; dbox_f = 0; dbox_live = true; dgrav_t = timer_read32();
}
static void drop_bookkeep(uint8_t nt) {             // record one promotion to tier nt
    dmerges++;
    if (nt > dmaxtier) dmaxtier = nt;
    if (nt < 9) dmade[nt]++;
    if (nt >= 3 && dmade[nt] >= 3 && nt > dpool) dpool = nt; // 3 made of a tier (>=3) unlocks it into the pool
}
// Resolve merges around the active box (the top of lane `a`, i.e. the piece just dropped):
//  - VERTICAL: same-color box directly below in the same lane -> promote it, active drops onto it.
//  - HORIZONTAL: same-color box at the SAME height in an adjacent lane -> promote the active (it
//    stays in lane `a`) and PULL the neighbor's box out; boxes above the gap fall down (gravity).
// Re-checks after every merge and chains fully (both directions) until nothing else matches.
static void drop_resolve(uint8_t a) {
    bool merged = true;
    while (merged && dcount[a] > 0) {
        merged = false;
        uint8_t i = dcount[a] - 1, t = dstack[a][i]; // active box
        if (t >= DTIER_MAX) break;                   // white can't promote further
        if (i >= 1 && dstack[a][i - 1] == t) {       // vertical: box below matches
            dcount[a]--;
            dstack[a][i - 1] = t + 1;
            drop_bookkeep(t + 1);
            merged = true;
            continue;
        }
        for (int8_t dir = -1; dir <= 1 && !merged; dir += 2) { // horizontal: same-height neighbor
            int8_t na = (int8_t)a + dir;
            if (na < 0 || na >= DWID) continue;
            if (i < dcount[na] && dstack[na][i] == t) {
                dstack[a][i] = t + 1;                            // promote active (stays in lane a)
                drop_bookkeep(t + 1);
                for (uint8_t k = i; k + 1 < dcount[na]; k++) dstack[na][k] = dstack[na][k + 1]; // gravity
                dcount[na]--;
                merged = true;
            }
        }
    }
}
static void drop_land(void) {
    uint8_t a = dbox_a; dstack[a][dcount[a]++] = dbox_color; drop_resolve(a); dbox_live = false;
    if (dcount[a] >= DLEN) { game_over(dmaxtier, 6); return; }   // column overflowed the top edge
    drop_spawn();
}
void drop_start(void) {
    memset(dstack, 0, sizeof(dstack)); memset(dcount, 0, sizeof(dcount)); memset(dmade, 0, sizeof(dmade));
    dmerges = 0; dmaxtier = 0; dpool = 2; dgrav_ms = 480; st = A_DROP; drop_spawn();
}
void drop_move(int8_t d) {
    if (!dbox_live) return;
    int8_t na = dbox_a + d;
    if (na < 0 || na >= DWID) return;
    int8_t rest = DLEN - 1 - dcount[na];
    if (dbox_f <= rest) dbox_a = na;
}
void drop_hard(void) {
    if (!dbox_live) return;
    int8_t rest = DLEN - 1 - dcount[dbox_a];
    dbox_f = rest < 0 ? 0 : rest;
    drop_land();
}
void drop_tick(void) {
    if (!dbox_live) return;
    if (timer_elapsed32(dgrav_t) > dgrav_ms) {
        dgrav_t = timer_read32(); int8_t rest = DLEN - 1 - dcount[dbox_a];
        if (dbox_f < rest) dbox_f++; else drop_land();
    }
}
void drop_render(void) {
    for (uint8_t i = 0; i < 9; i++) {               // F-row legend: F1..F9, spawnable bright / locked ~22%
        RGB c = hsv_to_rgb(DTIER[i]);
        if (i > dpool) { c.r = c.r * 22 / 100; c.g = c.g * 22 / 100; c.b = c.b * 22 / 100; }
        px(0, i + 1, c.r, c.g, c.b);
    }
    for (uint8_t a = 0; a < DWID; a++)
        for (uint8_t i = 0; i < dcount[a]; i++) { uint8_t f = DLEN - 1 - i; RGB c = hsv_to_rgb(DTIER[dstack[a][i]]); px(drow(a), dcol(a, f), c.r, c.g, c.b); }
    if (dbox_live) { RGB c = hsv_to_rgb(DTIER[dbox_color]); px(drow(dbox_a), dcol(dbox_a, dbox_f), c.r, c.g, c.b); }
}
