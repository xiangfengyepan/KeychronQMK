/* Snake — classic snake on the whole key grid. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"

/* ================= SNAKE — every key is a cell =================
 * Ported from snake_game.html. The board's lit keys form a ragged per-key grid,
 * binned into rows by their physical LED y (6 rows on the M1). LEFT/RIGHT walk the
 * current row (wrapping within the row) so horizontal runs are dead straight;
 * UP/DOWN jump to the nearest-center-x key in the adjacent row (wrapping top<->bottom).
 * One apple at a time on a random free cell; eating grows +1 and speeds up. Edges
 * wrap; running into your own body ends the game (score = apples eaten). */
#define SNK_MAX 90
enum { SD_U = 0, SD_D = 1, SD_L = 2, SD_R = 3 };      // opposite of d is (d ^ 1)

/* precomputed per-cell adjacency: snk_nb[cell][dir] -> neighbour cell index */
static uint8_t snk_nb[SNK_MAX][4];
static uint8_t sx[SNK_MAX];                           // cell physical x (for nearest-x)
static uint8_t srow[SNK_MAX];                         // cell row index (0..nrows-1)
static uint8_t nrows;

static uint8_t  snk[SNK_MAX];                         // body cells, head at [0]
static uint16_t snk_len;
static uint8_t  snk_food;
static uint8_t  snk_dir, snk_ndir;
static uint16_t snk_eaten;
static uint32_t snk_step_t;
static uint16_t snk_ms;
static bool     snk_dead;

/* ---- build the ragged per-key grid + adjacency from the LED geometry ---- */
static void snk_build_grid(void) {
    uint8_t sy[SNK_MAX];
    for (uint16_t i = 0; i < nC; i++) {
        uint8_t led = g_led_config.matrix_co[cR[i]][cC[i]];
        sx[i] = g_led_config.point[led].x;
        sy[i] = g_led_config.point[led].y;
    }
    // bin cells into rows by y (tolerance = half a row pitch)
    uint8_t bucketY[16];
    nrows = 0;
    for (uint16_t i = 0; i < nC; i++) {
        int8_t b = -1;
        for (uint8_t r = 0; r < nrows; r++) { int d = (int)sy[i] - bucketY[r]; if (d < 0) d = -d; if (d <= 7) { b = r; break; } }
        if (b < 0) { bucketY[nrows] = sy[i]; b = nrows; nrows++; }
        srow[i] = b;
    }
    // sort the buckets top->bottom and remap srow to the sorted rank
    uint8_t order[16], rank[16];
    for (uint8_t r = 0; r < nrows; r++) order[r] = r;
    for (uint8_t a = 0; a < nrows; a++)
        for (uint8_t b = a + 1; b < nrows; b++)
            if (bucketY[order[b]] < bucketY[order[a]]) { uint8_t t = order[a]; order[a] = order[b]; order[b] = t; }
    for (uint8_t r = 0; r < nrows; r++) rank[order[r]] = r;
    for (uint16_t i = 0; i < nC; i++) srow[i] = rank[srow[i]];
    // adjacency
    for (uint16_t i = 0; i < nC; i++) {
        uint8_t r = srow[i];
        int rIdx = -1, rBestX = 999, wMinIdx = -1, wMinX = 999;   // RIGHT + row wrap-min
        int lIdx = -1, lBestX = -1,  wMaxIdx = -1, wMaxX = -1;    // LEFT  + row wrap-max
        for (uint16_t j = 0; j < nC; j++) if (srow[j] == r) {
            if (sx[j] < wMinX) { wMinX = sx[j]; wMinIdx = j; }
            if (sx[j] > wMaxX) { wMaxX = sx[j]; wMaxIdx = j; }
            if (sx[j] > sx[i] && sx[j] < rBestX) { rBestX = sx[j]; rIdx = j; }
            if (sx[j] < sx[i] && sx[j] > lBestX) { lBestX = sx[j]; lIdx = j; }
        }
        snk_nb[i][SD_R] = (rIdx >= 0) ? rIdx : wMinIdx;           // next in row, wrap to first
        snk_nb[i][SD_L] = (lIdx >= 0) ? lIdx : wMaxIdx;           // prev in row, wrap to last
        uint8_t ur = (r == 0) ? nrows - 1 : r - 1;               // adjacent rows, wrapping
        uint8_t dr = (r + 1 == nrows) ? 0 : r + 1;
        int uIdx = i, uBest = 9999, dIdx = i, dBest = 9999;
        for (uint16_t j = 0; j < nC; j++) {
            if (srow[j] == ur) { int dd = (int)sx[j] - sx[i]; if (dd < 0) dd = -dd; if (dd < uBest) { uBest = dd; uIdx = j; } }
            if (srow[j] == dr) { int dd = (int)sx[j] - sx[i]; if (dd < 0) dd = -dd; if (dd < dBest) { dBest = dd; dIdx = j; } }
        }
        snk_nb[i][SD_U] = uIdx;                                   // nearest-center-x in the row above
        snk_nb[i][SD_D] = dIdx;                                   // ...and below
    }
}

static void snk_place_food(void) {
    for (uint16_t tries = 0; tries < 500; tries++) {
        uint8_t c = rnd() % nC;
        bool on = false;
        for (uint16_t i = 0; i < snk_len; i++) if (snk[i] == c) { on = true; break; }
        if (!on) { snk_food = c; return; }
    }
    for (uint8_t c = 0; c < nC; c++) {                            // fallback: first free cell
        bool on = false;
        for (uint16_t i = 0; i < snk_len; i++) if (snk[i] == c) { on = true; break; }
        if (!on) { snk_food = c; return; }
    }
    snk_food = 0;
}

void snake_start(void) {
    snk_build_grid();
    uint8_t r = (nrows > 2) ? 2 : nrows / 2;                      // start on the QWERTY row, facing right
    uint8_t rowcells[32], rc = 0;
    for (uint16_t i = 0; i < nC && rc < 32; i++) if (srow[i] == r) rowcells[rc++] = i;
    for (uint8_t a = 0; a < rc; a++)                             // sort the row by x
        for (uint8_t b = a + 1; b < rc; b++)
            if (sx[rowcells[b]] < sx[rowcells[a]]) { uint8_t t = rowcells[a]; rowcells[a] = rowcells[b]; rowcells[b] = t; }
    uint8_t mid = rc / 2;
    if (mid < 2) mid = 2;
    snk_len = 3;
    snk[0] = rowcells[mid];                                       // head first
    snk[1] = rowcells[mid - 1];
    snk[2] = rowcells[mid - 2];
    snk_dir = snk_ndir = SD_R;
    snk_eaten = 0;
    snk_ms = 180;
    snk_dead = false;
    snk_place_food();
    snk_step_t = timer_read32();
    st = A_SNAKE;
}

void snake_dir(uint8_t d) {                                       // steer; no direct reverse
    if (snk_dead) return;
    if (d == (snk_dir ^ 1)) return;
    snk_ndir = d;
}

void snake_tick(void) {
    if (snk_dead) return;
    if (timer_elapsed32(snk_step_t) < snk_ms) return;
    snk_step_t = timer_read32();
    snk_dir = snk_ndir;
    uint8_t head = snk_nb[snk[0]][snk_dir];
    bool eat = (head == snk_food);
    // self-collision: the tail cell vacates unless we're eating this step
    uint16_t check = eat ? snk_len : (snk_len ? snk_len - 1 : 0);
    for (uint16_t i = 0; i < check; i++) if (snk[i] == head) { snk_dead = true; game_over(snk_eaten, 9); return; }
    if (eat && snk_len < SNK_MAX) snk_len++;
    for (int i = snk_len - 1; i > 0; i--) snk[i] = snk[i - 1];    // shuffle body toward tail
    snk[0] = head;
    if (eat) {
        snk_eaten++;
        int ms = 180 - snk_eaten * 6;                            // speed up per apple, floor 70
        snk_ms = (ms < 70) ? 70 : (uint16_t)ms;
        snk_place_food();
    }
}

void snake_render(void) {
    // apple: pulsing red
    uint16_t ph  = timer_read32() % 600;
    uint8_t  add = (ph < 300) ? (uint8_t)(ph * 40 / 300) : (uint8_t)((600 - ph) * 40 / 300);
    px(cR[snk_food], cC[snk_food], 255, 20 + add, 20 + add);
    // snake: head brightest green, fading toward the tail
    uint16_t denom = (snk_len > 1) ? (snk_len - 1) : 1;
    for (uint16_t i = 0; i < snk_len; i++) {
        int t = i;
        uint8_t R = (uint8_t)(94  + (18  - 94)  * t / denom);
        uint8_t G = (uint8_t)(240 + (72  - 240) * t / denom);
        uint8_t B = (uint8_t)(138 + (36  - 138) * t / denom);
        px(cR[snk[i]], cC[snk[i]], R, G, B);
    }
}
