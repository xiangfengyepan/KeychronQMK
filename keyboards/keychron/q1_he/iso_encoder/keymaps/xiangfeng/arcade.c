/* On-keyboard arcade: lobby -> countdown -> Tetris / Topo -> score.
 * Rendered on the RGB matrix; opened with Fn+H (see keymap.c). Self-contained. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "arcade.h"
#include <string.h>
#include <math.h>

enum { A_OFF, A_LOBBY, A_COUNT, A_TETRIS, A_TOPO, A_FLAPPY, A_DINO, A_MEMORY, A_REACT, A_SCORE };
static uint8_t st = A_OFF;
bool arcade_active(void) { return st != A_OFF; }

static uint32_t rng = 1;
static uint32_t rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }

/* ---- LED cells: the 82 backlit keys in reading order ---- */
static uint8_t  cR[90], cC[90];
static uint16_t nC = 0;
static void build_cells(void) {
    nC = 0;
    for (uint8_t r = 0; r < MATRIX_ROWS; r++)
        for (uint8_t c = 0; c < MATRIX_COLS; c++)
            if (g_led_config.matrix_co[r][c] != NO_LED) { cR[nC] = r; cC[nC] = c; nC++; }
}
static inline void px(uint8_t r, uint8_t c, uint8_t R, uint8_t G, uint8_t B) {
    uint8_t led = g_led_config.matrix_co[r][c];
    if (led != NO_LED) rgb_matrix_set_color(led, R, G, B);
}
static inline int8_t iround(float x) { return (int8_t)(x >= 0 ? x + 0.5f : x - 0.5f); }
static uint32_t phys_last = 0;                         // shared physics timestamp (flappy/dino)

/* ---- knob: short press vs press-and-hold ---- */
#define KNOB_HOLD_MS 550
static bool     knob_down = false, knob_held = false;
static uint16_t knob_t = 0;

/* ---- games + 5x5 LED font (for the lobby name animation) ---- */
typedef struct { const char *name; uint8_t r, g, b; uint8_t kind; } game_t; // kind 0=tetris 1=topo 2=flappy 3=dino 4=memory 5=react
static const game_t GAMES[] = {
    {"TETRIS", 55,  230, 212, 0},
    {"TOPO",   255, 46,  136, 1},
    {"FLAPPY", 255, 210, 40,  2},
    {"DINO",   150, 240, 170, 3},
    {"MEMORY", 180, 90,  240, 4},
    {"REACT",  255, 120, 40,  5},
};
#define NGAME (sizeof(GAMES) / sizeof(GAMES[0]))
static uint8_t sel = 0;

static const uint8_t FONT[15][5] = {
    {0x1F, 0x04, 0x04, 0x04, 0x04}, // 0  T
    {0x1F, 0x10, 0x1E, 0x10, 0x1F}, // 1  E
    {0x1E, 0x12, 0x1E, 0x14, 0x13}, // 2  R
    {0x1F, 0x04, 0x04, 0x04, 0x1F}, // 3  I
    {0x0F, 0x10, 0x0E, 0x01, 0x1E}, // 4  S
    {0x0E, 0x11, 0x11, 0x11, 0x0E}, // 5  O
    {0x1E, 0x11, 0x1E, 0x10, 0x10}, // 6  P
    {0x1F, 0x10, 0x1E, 0x10, 0x10}, // 7  F
    {0x10, 0x10, 0x10, 0x10, 0x1F}, // 8  L
    {0x0E, 0x11, 0x1F, 0x11, 0x11}, // 9  A
    {0x11, 0x11, 0x0E, 0x04, 0x04}, // 10 Y
    {0x1E, 0x11, 0x11, 0x11, 0x1E}, // 11 D
    {0x11, 0x19, 0x15, 0x13, 0x11}, // 12 N
    {0x11, 0x1B, 0x15, 0x11, 0x11}, // 13 M
    {0x0E, 0x11, 0x10, 0x11, 0x0E}, // 14 C
};
static int8_t gidx(char c) {
    switch (c) { case 'T': return 0; case 'E': return 1; case 'R': return 2;  case 'I': return 3;
                 case 'S': return 4; case 'O': return 5; case 'P': return 6;  case 'F': return 7;
                 case 'L': return 8; case 'A': return 9; case 'Y': return 10; case 'D': return 11;
                 case 'N': return 12; case 'M': return 13; case 'C': return 14; }
    return -1;
}
static void draw_glyph(char c, uint8_t R, uint8_t G, uint8_t B) {
    int8_t gi = gidx(c);
    if (gi < 0) return;
    for (uint8_t r = 0; r < 5; r++)
        for (uint8_t col = 0; col < 5; col++)
            if (FONT[gi][r] & (1 << (4 - col))) px(r, 4 + col, R, G, B); // centred rows 0-4, cols 4-8
}

/* ---- lobby ---- */
static uint32_t lobby_t = 0;
static void enter_lobby(void) { st = A_LOBBY; lobby_t = timer_read32(); }
static void lobby_render(void) {
    const game_t *g = &GAMES[sel];
    uint8_t  n   = strlen(g->name);
    uint32_t per = 560, on = 430;                       // per-letter on/off cadence
    uint32_t t   = timer_elapsed32(lobby_t) % (n * per);
    if ((t % per) < on) draw_glyph(g->name[t / per], g->r, g->g, g->b);
}

/* ---- countdown ---- */
static uint32_t cd_t = 0;
static void start_game(void);
static void cd_tick(void) { if (timer_elapsed32(cd_t) >= 2400) start_game(); }
static void cd_render(void) {
    if ((timer_elapsed32(cd_t) % 800) < 500)
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 230, 20, 40); // red flash x3
}

/* ---- score fill ---- */
static uint32_t sc_t = 0;
static float    sc_frac = 0;
static uint8_t  sR, sG, sB;
static void game_over(uint16_t score, uint8_t kind) {
    float f;
    switch (kind) {
        case 1:  f = 1.0f - expf(-(float)score / 32.0f); break; // topo:   exponential board fill
        case 2:  f = (float)score / 20.0f;               break; // flappy: 20 pipes fills
        case 3:  f = (float)score / 25.0f;               break; // dino:   25 obstacles fills
        case 4:  f = 1.0f - expf(-(float)score / 6.0f);  break; // memory: saturating (rounds)
        case 5:  f = (182.0f - (float)score) / 82.0f;    break; // react: 100ms=82/82, 182ms=0 — exactly 1 key per ms (score = avg ms)
        default: f = (float)score / 15.0f;               break; // tetris: 15 lines fills
    }
    if (f > 1) f = 1;
    if (kind == 5) { if (f < 0) f = 0; }                        // react: exact, down to a true 0
    else if (f < 0.04f) f = 0.04f;                             // others: always show at least a sliver
    sc_frac = f;
    if (f < 0.4f)      { sR = 255; sG = 157; sB = 60;  }  // bronze
    else if (f < 0.8f) { sR = 55;  sG = 230; sB = 212; }  // cyan
    else               { sR = 255; sG = 210; sB = 63;  }  // gold
    sc_t = timer_read32();
    st   = A_SCORE;
}
static void sc_render(void) {
    uint16_t n     = (uint16_t)(nC * sc_frac);
    uint16_t shown = timer_elapsed32(sc_t) / 22;         // grow the fill key-by-key
    if (shown > n) shown = n;
    for (uint16_t i = 0; i < shown; i++) px(cR[i], cC[i], sR, sG, sB);
}

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
static const uint8_t PC[8][3] = {{0,0,0},{55,220,220},{230,205,0},{180,0,230},{0,220,0},{230,0,0},{0,80,235},{235,95,0}};
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
static void tetris_start(void) { memset(tb, 0, sizeof(tb)); tlines = 0; grav_ms = 650; bagi = 7; grav_t = timer_read32(); st = A_TETRIS; tspawn(); }
static void tetris_tick(void) {
    if (!plive) return;
    if (timer_elapsed32(grav_t) > grav_ms) { grav_t = timer_read32(); if (tfits(pty, prot, pf + 1, pa)) pf++; else tlock(); }
}
static void tetris_render(void) {
    for (uint8_t f = 0; f < TLEN; f++)
        for (uint8_t a = 0; a < TWID; a++)
            if (tb[f][a]) px(a, mcol(a, f), PC[tb[f][a]][0], PC[tb[f][a]][1], PC[tb[f][a]][2]); // empty cells stay dark
    if (plive)
        for (uint8_t k = 0; k < 4; k++) {
            int8_t A = pa + PIECES[pty][prot][k][0], F = pf + PIECES[pty][prot][k][1];
            if (A >= 0 && A < TWID && F >= 0 && F < TLEN) px(A, mcol(A, F), PC[pty + 1][0], PC[pty + 1][1], PC[pty + 1][2]);
        }
}
static void tmove(int8_t d) { if (plive && tfits(pty, prot, pf, pa + d)) pa += d; }
static const int8_t KICK[5] = {0, -1, 1, -2, 2};
static void trot(int8_t dir) {
    if (!plive) return;
    uint8_t nr = (prot + (dir < 0 ? 3 : 1)) & 3;
    for (uint8_t i = 0; i < 5; i++) if (tfits(pty, nr, pf, pa + KICK[i])) { prot = nr; pa += KICK[i]; return; }
}
static void tdrop(void) { if (!plive) return; while (tfits(pty, prot, pf + 1, pa)) pf++; tlock(); grav_t = timer_read32(); }

/* ================= TOPO — whole board ================= */
#define TOPO_MAX 8
#define MOLE_LIFE 3000
static struct { uint8_t r, c; uint32_t born; } mole[TOPO_MAX];
static uint8_t  nmole = 0;
static uint16_t topo_score = 0;
static uint32_t spawn_t = 0;
static void topo_start(void) { nmole = 0; topo_score = 0; spawn_t = timer_read32(); st = A_TOPO; }
static void topo_tick(void) {
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
static void topo_render(void) {
    for (uint8_t i = 0; i < nmole; i++) {
        uint32_t age = timer_elapsed32(mole[i].born); if (age > MOLE_LIFE) age = MOLE_LIFE;
        uint8_t g = 40 + (uint8_t)((215u * (MOLE_LIFE - age)) / MOLE_LIFE); // fresh=yellow -> old=red
        px(mole[i].r, mole[i].c, 255, g, 40);
    }
}
static void topo_hit(uint8_t r, uint8_t c) {
    for (uint8_t i = 0; i < nmole; i++) if (mole[i].r == r && mole[i].c == c) { mole[i] = mole[--nmole]; topo_score++; return; }
    game_over(topo_score, 1); // wrong key = miss = over
}

/* ================= FLAPPY — 5 rows tall, scrolls right->left =================
 * Bird fixed at col 2; pipes have a 3-tall gap. Ported 1:1 from arcade.html. */
#define FB_COL 2
static float    f_bt, f_vy, f_speed;
static bool     f_started;
static uint16_t f_score;
static int32_t  f_spawn;
static struct { float col; uint8_t gap; bool passed; } f_pipe[6];
static uint8_t  f_np;
static void flappy_start(void) {
    f_bt = 1.5f; f_vy = 0; f_np = 0; f_score = 0; f_speed = 0.05f; f_spawn = 1400;
    f_started = false; phys_last = timer_read32(); st = A_FLAPPY;
}
static void flappy_flap(void) { f_started = true; f_vy = -0.066f; } // ~1-block lift
static void flappy_tick(void) {
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
static void flappy_render(void) {
    for (uint8_t i = 0; i < f_np; i++) {
        int8_t c = iround(f_pipe[i].col); if (c < 0 || c > 13) continue;
        for (uint8_t r = 0; r < 5; r++) if (r < f_pipe[i].gap || r > f_pipe[i].gap + 2) px(r, c, 40, 190, 60);
    }
    int8_t br = iround(f_bt); if (br < 0) br = 0; if (br > 5) br = 5;
    px(br, FB_COL, 255, 220, 40);
}

/* ================= DINO — ground runner =================
 * Dino at col 2. Bottom obstacles (row 4): jump (knob). Top obstacles (row 3): duck (hold a key). */
#define D_COL 2
static float    d_bottom, d_vy, d_speed;
static bool     d_duck, d_space_held;
static uint16_t d_score;
static int32_t  d_spawn;
static struct { float col; uint8_t row; bool passed; } d_obs[8];
static uint8_t  d_no;
static void dino_start(void) {
    d_bottom = 4; d_vy = 0; d_no = 0; d_score = 0; d_speed = 0.06f; d_spawn = 700;
    d_duck = false; d_space_held = false; phys_last = timer_read32(); st = A_DINO;
}
static void dino_jump(void) { if (d_bottom >= 3.99f) d_vy = -0.35f; } // impulse: tap clears ~2 rows
static void dino_tick(void) {
    uint32_t now = timer_read32(); float dt = (float)(now - phys_last); phys_last = now;
    float s = dt / 16.67f;
    float g = (d_vy < 0 && d_space_held) ? 0.015f : 0.03f; // hold Space while rising = higher jump (~3 rows)
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
static void dino_render(void) {
    for (uint8_t c = 0; c < 14; c++) px(4, c, 16, 16, 20);      // ground line
    for (uint8_t i = 0; i < d_no; i++) {
        int8_t c = iround(d_obs[i].col); if (c < 0 || c > 13) continue;
        if (d_obs[i].row == 3) px(3, c, 235, 120, 40); else px(4, c, 230, 80, 40);
    }
    bool grounded = d_bottom >= 3.99f, ducking = d_duck && grounded;
    if (ducking) px(4, D_COL, 110, 230, 140);
    else { int8_t rb = iround(d_bottom); for (int8_t r = rb - 1; r <= rb; r++) if (r >= 0 && r < 5) px(r, D_COL, 110, 230, 140); }
}

/* ================= MEMORY — Simon on the whole board =================
 * Watch the sequence flash, then repeat it by pressing the keys in order.
 * Each round appends one more random key. A wrong key ends it. */
#define MEM_MAX 64
#define MEM_ON  420
#define MEM_GAP 200
static uint8_t  mseq[MEM_MAX], mlen, mpos, mstage; // stage 0=show 1=input 2=round-clear
static uint32_t mem_t, mflash_t;
static int16_t  mflash;                            // last correct cell (green feedback), -1 none
static void memory_start(void) {
    mlen = 1; mseq[0] = rnd() % nC; mpos = 0; mstage = 0; mflash = -1; mem_t = timer_read32(); st = A_MEMORY;
}
static void memory_press(uint8_t row, uint8_t col) {
    if (mstage != 1) return;                        // only during the player's turn
    uint8_t want = mseq[mpos];
    if (row == cR[want] && col == cC[want]) {
        mflash = want; mflash_t = timer_read32();
        if (++mpos >= mlen) {                        // whole sequence entered -> next round
            if (mlen < MEM_MAX) mseq[mlen++] = rnd() % nC;
            mstage = 2; mem_t = timer_read32();
        }
    } else {
        game_over(mlen - 1, 4);                      // wrong key = rounds completed = mlen-1
    }
}
static void memory_tick(void) {
    if (mstage == 0) { if (timer_elapsed32(mem_t) >= (uint32_t)mlen * (MEM_ON + MEM_GAP)) { mstage = 1; mpos = 0; } }
    else if (mstage == 2) { if (timer_elapsed32(mem_t) >= 500) { mstage = 0; mpos = 0; mem_t = timer_read32(); } }
}
static void memory_render(void) {
    if (mstage == 0) {                               // playback: light each step in turn
        uint32_t step = MEM_ON + MEM_GAP, e = timer_elapsed32(mem_t), idx = e / step;
        if (idx < mlen && (e % step) < MEM_ON) { uint8_t ci = mseq[idx]; px(cR[ci], cC[ci], 60, 180, 255); }
    } else if (mstage == 1) {                        // player's turn: faint wash + green flash on hits
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 6, 8, 16);
        if (mflash >= 0 && timer_elapsed32(mflash_t) < 180) px(cR[mflash], cC[mflash], 0, 230, 80);
        else mflash = -1;
    } else {                                         // round cleared
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 0, 180, 70);
    }
}

/* ================= REACTION — light up, press ASAP =================
 * 3 rounds: board waits on a dim red, flashes bright green, you hit any key. Score = avg reaction ms. */
static uint8_t  rx_round, rx_state;               // rx_state 0=wait 1=go
static uint32_t rx_sum, rx_wait_t, rx_go_t, rx_delay, rx_false_t;
static bool     rx_false;
static void rx_newround(void) { rx_state = 0; rx_delay = 900 + rnd() % 2400; rx_wait_t = timer_read32(); }
static void react_start(void) { rx_round = 0; rx_sum = 0; rx_false = false; rx_newround(); st = A_REACT; }
static void react_press(void) {
    if (rx_state == 0) { rx_false = true; rx_false_t = timer_read32(); rx_newround(); return; } // jumped the gun -> redo
    rx_sum += timer_elapsed32(rx_go_t);
    if (++rx_round >= 3) game_over((uint16_t)(rx_sum / 3), 5);
    else rx_newround();
}
static void react_tick(void) {
    if (rx_state == 0 && timer_elapsed32(rx_wait_t) >= rx_delay) { rx_state = 1; rx_go_t = timer_read32(); }
}
static void react_render(void) {
    if (rx_state == 1) { for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 40, 230, 80); }        // GO: bright green
    else {                                                                                          // wait: dim red
        uint8_t r = (rx_false && timer_elapsed32(rx_false_t) < 400) ? 150 : 26;                     // brighter on a false start
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], r, 0, 0);
    }
}

/* ================= dispatch ================= */
static void start_game(void) {
    switch (GAMES[sel].kind) {
        case 0:  tetris_start(); break;
        case 1:  topo_start();   break;
        case 2:  flappy_start(); break;
        case 3:  dino_start();   break;
        case 4:  memory_start(); break;
        default: react_start();  break;
    }
}

void arcade_open(uint32_t seed) { rng = seed ? seed : 1; build_cells(); sel = 0; knob_down = knob_held = false; enter_lobby(); }

static void knob_short(void) {
    if (st == A_LOBBY)      { cd_t = timer_read32(); st = A_COUNT; } // start selected game
    else if (st == A_SCORE) enter_lobby();                          // back to lobby
}
static void knob_hold_act(void) {
    if (st == A_LOBBY) st = A_OFF;  // exit the arcade
    else               enter_lobby(); // game / countdown / score -> lobby
}

void arcade_key(uint8_t row, uint8_t col, bool pressed) {
    if (row == 0 && col == 14) {                       // the knob press
        if (pressed) {
            knob_down = true; knob_held = false; knob_t = timer_read();
            if      (st == A_FLAPPY) flappy_flap();     // knob tap = flap / jump
            else if (st == A_DINO)   dino_jump();
            else if (st == A_REACT)  react_press();
        } else { if (knob_down && !knob_held) knob_short(); knob_down = false; }
        return;
    }
    if (st == A_DINO) {                                // Space = jump (hold = higher); Ctrl = duck
        if (row == 5 && col == 6) { d_space_held = pressed; if (pressed) dino_jump(); }
        else if (row == 5 && (col == 0 || col == 11)) d_duck = pressed;
        return;
    }
    if (st == A_REACT) { if (pressed) react_press(); return; } // any key = hit
    if (!pressed) return;
    if (st == A_TETRIS) {
        if      (row == 1 && col == 14) tmove(-1);     // PgUp
        else if (row == 2 && col == 14) tmove(1);      // PgDn
        else if (row == 3 && col == 13) tdrop();       // Home
    } else if (st == A_TOPO)   topo_hit(row, col);
    else if (st == A_FLAPPY)   flappy_flap();          // any key also flaps
    else if (st == A_MEMORY)   memory_press(row, col);
}
void arcade_encoder(bool cw) {
    if (st == A_LOBBY)       { sel = (sel + (cw ? 1 : NGAME - 1)) % NGAME; lobby_t = timer_read32(); }
    else if (st == A_TETRIS) trot(cw ? 1 : -1);
}
void arcade_tick(void) {
    if (knob_down && !knob_held && timer_elapsed(knob_t) > KNOB_HOLD_MS) { knob_held = true; knob_hold_act(); }
    switch (st) {
        case A_COUNT:  cd_tick();     break;
        case A_TETRIS: tetris_tick(); break;
        case A_TOPO:   topo_tick();   break;
        case A_FLAPPY: flappy_tick(); break;
        case A_DINO:   dino_tick();   break;
        case A_MEMORY: memory_tick(); break;
        case A_REACT:  react_tick();  break;
        default: break;
    }
}
void arcade_render(uint8_t led_min, uint8_t led_max) {
    for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, 0, 0, 0); // dark background
    switch (st) {
        case A_LOBBY:  lobby_render();  break;
        case A_COUNT:  cd_render();     break;
        case A_TETRIS: tetris_render(); break;
        case A_TOPO:   topo_render();   break;
        case A_FLAPPY: flappy_render(); break;
        case A_DINO:   dino_render();   break;
        case A_MEMORY: memory_render(); break;
        case A_REACT:  react_render();  break;
        case A_SCORE:  sc_render();     break;
        default: break;
    }
}
