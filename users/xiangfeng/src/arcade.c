/* On-keyboard arcade hub: lobby -> countdown -> game -> score.
 * Rendered on the RGB matrix; opened with Fn+H (see keymap.c). This file owns the
 * state machine, dispatch, and everything shared by more than one game. Each game
 * itself lives in src/arcade/<game>.c and is wired in via include/arcade_internal.h. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade.h"
#include "include/arcade_internal.h"
#include "include/palette.h" // named color constants for the game palettes
#include <string.h>
#include <math.h>

uint8_t st = A_OFF;
bool arcade_active(void) { return st != A_OFF; }

static uint32_t rng = 1;
uint32_t rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }

/* ---- LED cells: the 82 backlit keys in reading order ---- */
uint8_t  cR[90], cC[90];
uint16_t nC = 0;
static void build_cells(void) {
    nC = 0;
    for (uint8_t r = 0; r < MATRIX_ROWS; r++)
        for (uint8_t c = 0; c < MATRIX_COLS; c++)
            if (g_led_config.matrix_co[r][c] != NO_LED) { cR[nC] = r; cC[nC] = c; nC++; }
}
void px(uint8_t r, uint8_t c, uint8_t R, uint8_t G, uint8_t B) {
    uint8_t led = g_led_config.matrix_co[r][c];
    if (led != NO_LED) rgb_matrix_set_color(led, R, G, B);
}
int8_t iround(float x) { return (int8_t)(x >= 0 ? x + 0.5f : x - 0.5f); }
uint32_t phys_last = 0;                                // shared physics timestamp (flappy/dino/pong)

/* ---- knob: short press vs press-and-hold ---- */
#define KNOB_HOLD_MS 550
static bool     knob_down = false, knob_held = false;
static uint16_t knob_t = 0;

/* ---- games + 5x5 LED font (for the lobby name animation) ---- */
typedef struct { const char *name; HSV col; uint8_t kind; } game_t; // kind 0=tetris 1=topo 2=flappy 3=dino 4=memory 5=react 6=drop 7=pong 8=rubik
static const game_t GAMES[] = {                     // accent color = nearest palette constant
    {"TETRIS", COL_CYAN,        0},
    {"TOPO",   COL_PINK,        1},
    {"FLAPPY", COL_GOLD,        2},
    {"DINO",   COL_GREEN_LIGHT, 3},
    {"MEMORY", COL_PURPLE,      4},
    {"REACT",  COL_CORAL,       5},
    {"DROP",   COL_LIME,        6},
    {"PONG",   COL_CYAN,        7},
    {"RUBIK",  COL_WHITE,       8},
};
#define NGAME (sizeof(GAMES) / sizeof(GAMES[0]))
static uint8_t sel = 0;

static const uint8_t FONT[19][5] = {
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
    {0x0F, 0x10, 0x13, 0x11, 0x0E}, // 15 G
    {0x11, 0x11, 0x11, 0x11, 0x0E}, // 16 U
    {0x1E, 0x11, 0x1E, 0x11, 0x1E}, // 17 B
    {0x11, 0x12, 0x1C, 0x12, 0x11}, // 18 K
};
static int8_t gidx(char c) {
    switch (c) { case 'T': return 0; case 'E': return 1; case 'R': return 2;  case 'I': return 3;
                 case 'S': return 4; case 'O': return 5; case 'P': return 6;  case 'F': return 7;
                 case 'L': return 8; case 'A': return 9; case 'Y': return 10; case 'D': return 11;
                 case 'N': return 12; case 'M': return 13; case 'C': return 14; case 'G': return 15;
                 case 'U': return 16; case 'B': return 17; case 'K': return 18; }
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
    if ((t % per) < on) { RGB c = hsv_to_rgb(g->col); draw_glyph(g->name[t / per], c.r, c.g, c.b); }
}

/* ---- countdown ---- */
static uint32_t cd_t = 0;
static void start_game(void);
static void cd_tick(void) { if (timer_elapsed32(cd_t) >= 2400) start_game(); }
static void cd_render(void) {
    if ((timer_elapsed32(cd_t) % 800) < 500)
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 230, 20, 40); // red flash x3
}

/* ---- score fill (shared: game_over is called by every game) ---- */
uint32_t sc_t = 0;
float    sc_frac = 0;
uint8_t  sR, sG, sB;
void game_over(uint16_t score, uint8_t kind) {
    float f;
    switch (kind) {
        case 1:  f = 1.0f - expf(-(float)score / 32.0f); break; // topo:   exponential board fill
        case 2:  f = (float)score / 20.0f;               break; // flappy: 20 pipes fills
        case 3:  f = (float)score / 25.0f;               break; // dino:   25 obstacles fills
        case 4:  f = 1.0f - expf(-(float)score / 6.0f);  break; // memory: saturating (rounds)
        case 5:  f = (182.0f - (float)score) / 82.0f;    break; // react: 100ms=82/82, 182ms=0 — exactly 1 key per ms (score = avg ms)
        case 6:  f = (float)score / 8.0f;                break; // drop:   score = top tier reached (0-8); tier 8 fills
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

/* ================= dispatch ================= */
static void start_game(void) {
    switch (GAMES[sel].kind) {
        case 0:  tetris_start(); break;
        case 1:  topo_start();   break;
        case 2:  flappy_start(); break;
        case 3:  dino_start();   break;
        case 4:  memory_start(); break;
        case 5:  react_start();  break;
        case 6:  drop_start();   break;
        case 7:  pong_start();   break;
        default: rubik_start();  break;
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
        dino_key(row, col, pressed);
        return;
    }
    if (st == A_REACT) { if (pressed) react_press(); return; } // any key = hit
    if (st == A_PONG) {                                // paddles are held keys (track press + release)
        pong_key(row, col, pressed);
        return;
    }
    if (st == A_RUBIK) { rubik_key(row, col, pressed); return; } // Space hold/tap (needs release)
    if (!pressed) return;
    if (st == A_TETRIS) {
        if      (row == 1 && col == 14) tmove(-1);     // PgUp
        else if (row == 2 && col == 14) tmove(1);      // PgDn
        else if (row == 3 && col == 13) tdrop();       // Home
    } else if (st == A_DROP) {
        if      (row == 1 && col == 14) drop_move(-1); // PgUp
        else if (row == 2 && col == 14) drop_move(1);  // PgDn
        else if (row == 3 && col == 13) drop_hard();   // Home
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
        case A_DROP:   drop_tick();   break;
        case A_PONG:   pong_tick();   break;
        case A_RUBIK:  rubik_tick();  break;
        case A_SCORE:  if (timer_elapsed32(sc_t) >= 10000) enter_lobby(); break; // idle 10s -> lobby
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
        case A_DROP:   drop_render();   break;
        case A_PONG:   pong_render();   break;
        case A_RUBIK:  rubik_render();  break;
        case A_SCORE:  sc_render();     break;
        default: break;
    }
}
