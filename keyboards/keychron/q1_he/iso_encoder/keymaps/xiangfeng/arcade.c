/* On-keyboard arcade: lobby -> countdown -> Tetris / Topo -> score.
 * Rendered on the RGB matrix; opened with Fn+H (see keymap.c). Self-contained. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "arcade.h"
#include <string.h>
#include <math.h>

enum { A_OFF, A_LOBBY, A_COUNT, A_TETRIS, A_TOPO, A_SCORE };
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

/* ---- knob: short press vs press-and-hold ---- */
#define KNOB_HOLD_MS 550
static bool     knob_down = false, knob_held = false;
static uint16_t knob_t = 0;

/* ---- games + 5x5 LED font (for the lobby name animation) ---- */
typedef struct { const char *name; uint8_t r, g, b; uint8_t kind; } game_t; // kind 0=tetris 1=topo
static const game_t GAMES[] = {
    {"TETRIS", 55, 230, 212, 0},
    {"TOPO",   255, 46, 136, 1},
};
#define NGAME (sizeof(GAMES) / sizeof(GAMES[0]))
static uint8_t sel = 0;

static const uint8_t FONT[7][5] = {
    {0x1F, 0x04, 0x04, 0x04, 0x04}, // T
    {0x1F, 0x10, 0x1E, 0x10, 0x1F}, // E
    {0x1E, 0x12, 0x1E, 0x14, 0x13}, // R
    {0x1F, 0x04, 0x04, 0x04, 0x1F}, // I
    {0x0F, 0x10, 0x0E, 0x01, 0x1E}, // S
    {0x0E, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x1E, 0x10, 0x10}, // P
};
static int8_t gidx(char c) {
    switch (c) { case 'T': return 0; case 'E': return 1; case 'R': return 2; case 'I': return 3;
                 case 'S': return 4; case 'O': return 5; case 'P': return 6; }
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
    float f = (kind == 1) ? (1.0f - expf(-(float)score / 32.0f))  // topo: exponential
                          : ((float)score / 15.0f);               // tetris: 15 lines fills
    if (f > 1) f = 1;
    if (f < 0.04f) f = 0.04f;
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
static const uint8_t FALLCOL[TLEN] = {0,1,2,3,4,5,6,7,8,9,10,12,13}; // skip matrix col 11 (missing key @ row4)
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
        for (uint8_t a = 0; a < TWID; a++) {
            if (tb[f][a]) px(a, FALLCOL[f], PC[tb[f][a]][0], PC[tb[f][a]][1], PC[tb[f][a]][2]);
            else          px(a, FALLCOL[f], 8, 10, 20); // dim empty well
        }
    if (plive)
        for (uint8_t k = 0; k < 4; k++) {
            int8_t A = pa + PIECES[pty][prot][k][0], F = pf + PIECES[pty][prot][k][1];
            if (A >= 0 && A < TWID && F >= 0 && F < TLEN) px(A, FALLCOL[F], PC[pty + 1][0], PC[pty + 1][1], PC[pty + 1][2]);
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

/* ================= dispatch ================= */
static void start_game(void) { if (GAMES[sel].kind == 0) tetris_start(); else topo_start(); }

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
        if (pressed) { knob_down = true; knob_held = false; knob_t = timer_read(); }
        else { if (knob_down && !knob_held) knob_short(); knob_down = false; }
        return;
    }
    if (!pressed) return;
    if (st == A_TETRIS) {
        if      (row == 1 && col == 14) tmove(-1);     // PgUp
        else if (row == 2 && col == 14) tmove(1);      // PgDn
        else if (row == 3 && col == 13) tdrop();       // Home
    } else if (st == A_TOPO) {
        topo_hit(row, col);
    }
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
        case A_SCORE:  sc_render();     break;
        default: break;
    }
}
