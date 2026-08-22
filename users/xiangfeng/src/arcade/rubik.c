/* Rubik — speedcube timer + scramble trainer. See src/arcade.c for the hub.
 *
 * Driven by the SPACE bar (the arcade captures every key once open):
 *   A. TIMER    — hold Space >=3 s (the number row fills amber, then breathes
 *                 green = "ready, release any time"); release starts a count-up
 *                 clock shown in BINARY on the number row (key '1'=512 ... '0'=1,
 *                 0-1023 s) in amber. Press Space again to stop: the frozen time
 *                 stays lit but recoloured GREEN to mark it stopped.
 *   B. SCRAMBLE — a quick Space tap (<3 s) shows a 20-move WCA scramble, one move
 *                 every 1 s: the face key (R L U D B F) lit amber, plus '2' (blue)
 *                 for a double and ' (pink, the KC_MINS number-row key) for a prime
 *                 — up to three keys at once. Press Space during the scramble to
 *                 stop and reset it (back to idle; no colour readout).
 *   C. COLOURS  — after the last move, the scrambled cube is drawn: each keyboard
 *                 row is one face (9 stickers on 9 keys). F-row=F(green) number=U
 *                 (white) QWERTY=D(yellow) home=L(orange) shift=R(red) bottom(9
 *                 non-Space keys)=B(blue).
 * From any shown result (a stopped timer, a stopped scramble, or the colour
 * readout) a Space press resets back to idle; from idle, hold Space -> timer,
 * a quick tap -> scramble.
 * Knob-hold exits to the lobby (handled by the hub). */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"
#include <string.h>

#define HOLD_MS 3000   // hold Space at least this long to arm the timer
#define NMOVES  20     // WCA scramble length
#define MOVE_MS 1000   // per-move display time

/* ---- cube state: 6 faces x 9 stickers, each holding a face index 0..5 ---- */
enum { FU, FD, FF, FB, FL, FR };          // face order == keyboard-row order (top->bottom)
static uint8_t cube[6][9];

static const uint8_t ROT[9] = {6,3,0,7,4,1,8,5,2}; // face self-rotation (CW quarter)
typedef struct { uint8_t f; uint8_t i[3]; } strip_t;
static const strip_t CYC[6][4] = {          // the 4 adjacent strips cycled per CW quarter
    /*U*/ {{FF,{0,1,2}},{FL,{0,1,2}},{FB,{0,1,2}},{FR,{0,1,2}}},
    /*D*/ {{FF,{6,7,8}},{FR,{6,7,8}},{FB,{6,7,8}},{FL,{6,7,8}}},
    /*F*/ {{FU,{6,7,8}},{FR,{0,3,6}},{FD,{2,1,0}},{FL,{8,5,2}}},
    /*B*/ {{FU,{2,1,0}},{FL,{0,3,6}},{FD,{6,7,8}},{FR,{8,5,2}}},
    /*L*/ {{FU,{0,3,6}},{FF,{0,3,6}},{FD,{0,3,6}},{FB,{8,5,2}}},
    /*R*/ {{FU,{8,5,2}},{FB,{0,3,6}},{FD,{8,5,2}},{FF,{8,5,2}}},
};
static const uint8_t COL[6][3] = {          // sticker colours by face (match the preview artifact)
    {242,242,242},  // U white
    {255,210, 30},  // D yellow
    {  0,178, 74},  // F green
    { 22,104,227},  // B blue
    {255,122, 24},  // L orange
    {224, 38, 30},  // R red
};
/* Which cube face each physical keyboard row shows in the colour readout.
 * Row order top->bottom: F-row, number, QWERTY, home, shift, bottom. */
enum { PR_F, PR_NUM, PR_QW, PR_HOME, PR_SHIFT, PR_BOT };
static const uint8_t ROW_FACE[6] = { FF, FU, FD, FL, FR, FB };
//  F-row=F(green)  num=U(white)  QWERTY=D(yellow)  home=L(orange)  shift=R(red)  bottom=B(blue)

static void solve(void) { for (uint8_t f = 0; f < 6; f++) for (uint8_t s = 0; s < 9; s++) cube[f][s] = f; }
static void qturn(uint8_t m) {
    uint8_t tmp[9]; memcpy(tmp, cube[m], 9);
    for (uint8_t i = 0; i < 9; i++) cube[m][i] = tmp[ROT[i]];   // rotate the face itself
    uint8_t vals[4][3];
    for (uint8_t k = 0; k < 4; k++)
        for (uint8_t j = 0; j < 3; j++) vals[k][j] = cube[CYC[m][k].f][CYC[m][k].i[j]];
    for (uint8_t k = 0; k < 4; k++) {                           // shift each strip forward by one
        const uint8_t *src = vals[(k + 3) & 3];
        for (uint8_t j = 0; j < 3; j++) cube[CYC[m][k].f][CYC[m][k].i[j]] = src[j];
    }
}

/* ---- scramble (0=normal, 1=prime, 2=double) ---- */
static uint8_t mv_face[NMOVES], mv_type[NMOVES], mi;
static uint32_t move_t;
static void gen_scramble(void) {
    uint8_t last = 0xFF;
    for (uint8_t i = 0; i < NMOVES; i++) {
        uint8_t m; do { m = rnd() % 6; } while (m == last);   // no consecutive same face
        last = m;
        mv_face[i] = m; mv_type[i] = rnd() % 3;
    }
    solve();
    for (uint8_t i = 0; i < NMOVES; i++) {
        uint8_t n = (mv_type[i] == 1) ? 3 : (mv_type[i] == 2) ? 2 : 1; // prime=3q, double=2q
        for (uint8_t q = 0; q < n; q++) qturn(mv_face[i]);
    }
}

/* ---- key positions (resolved once from the base-layer keymap) ---- */
static uint8_t rowsR[6][9], rowsC[6][9];   // colour display: one face per keyboard row
static uint8_t nbR[10],  nbC[10];          // number row 1..9,0  (bit9..bit0)
static uint8_t faceR[6], faceC[6];         // scramble face keys U D F B L R
static uint8_t primeR, primeC, dblR, dblC; // ' (KC_MINS, number row) and 2
static uint8_t spcR, spcC;                 // the Space bar

/* Resolve a key's physical (row,col) from its keycode. Scans the first few layers
 * because some boards keep the F-row keycodes on an alternate base layer (the Q1
 * HE puts F1..F12 on layer 2, media keys on layer 0). The physical position is the
 * same wherever the keycode is mapped, so the first match wins. */
static bool pos_of(uint16_t kc, uint8_t *rr, uint8_t *cc) {
    for (uint8_t layer = 0; layer < 3; layer++)
        for (uint8_t r = 0; r < MATRIX_ROWS; r++)
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                if (g_led_config.matrix_co[r][c] == NO_LED) continue;
                if (keymap_key_to_keycode(layer, (keypos_t){.row = r, .col = c}) == kc) { *rr = r; *cc = c; return true; }
            }
    return false;
}
static void build_positions(void) {
    static const uint16_t QW[9] = {KC_Q,KC_W,KC_E,KC_R,KC_T,KC_Y,KC_U,KC_I,KC_O};
    static const uint16_t HM[9] = {KC_A,KC_S,KC_D,KC_F,KC_G,KC_H,KC_J,KC_K,KC_L};
    static const uint16_t SH[9] = {KC_Z,KC_X,KC_C,KC_V,KC_B,KC_N,KC_M,KC_COMM,KC_DOT};
    static const uint16_t FK[6] = {KC_U,KC_D,KC_F,KC_B,KC_L,KC_R}; // faces U D F B L R
    for (uint8_t i = 0; i < 9; i++) {
        pos_of(KC_F1 + i, &rowsR[FU][i], &rowsC[FU][i]); // U row  = F1..F9
        pos_of(KC_1  + i, &rowsR[FD][i], &rowsC[FD][i]); // D row  = 1..9
        pos_of(QW[i],     &rowsR[FF][i], &rowsC[FF][i]); // F row  = Q..O
        pos_of(HM[i],     &rowsR[FB][i], &rowsC[FB][i]); // B row  = A..L
        pos_of(SH[i],     &rowsR[FL][i], &rowsC[FL][i]); // L row  = Z.. .
        nbR[i] = rowsR[FD][i]; nbC[i] = rowsC[FD][i];    // timer bits 9..1 share the number row
    }
    pos_of(KC_0, &nbR[9], &nbC[9]);                      // timer bit 0 = '0' key
    for (uint8_t i = 0; i < 6; i++) pos_of(FK[i], &faceR[i], &faceC[i]);
    pos_of(KC_MINS, &primeR, &primeC);   // prime ' = the number-row key right of 0 (KC_MINS)
    pos_of(KC_2,    &dblR,   &dblC);
    pos_of(KC_SPC,  &spcR,   &spcC);
    // R row = the 9 non-Space keys on the bottom (Space) matrix row, left->right
    // (skip LED-less spacer columns and any KC_NO gaps around a wide spacebar)
    uint8_t n = 0;
    for (uint8_t c = 0; c < MATRIX_COLS && n < 9; c++) {
        if (g_led_config.matrix_co[spcR][c] == NO_LED || c == spcC) continue;
        if (keymap_key_to_keycode(0, (keypos_t){.row = spcR, .col = c}) == KC_NO) continue;
        rowsR[FR][n] = spcR; rowsC[FR][n] = c; n++;
    }
}

/* ---- phase machine ---- */
enum { RB_IDLE, RB_HOLD, RB_TIMER, RB_STOP, RB_SCRAMBLE, RB_COLORS };
static uint8_t  rb;
static bool     space_down;
static uint32_t space_t;    // Space press timestamp (hold arming)
static uint32_t timer_t0;   // count-up start
static uint16_t timer_frozen;

static uint16_t cur_seconds(void) { uint32_t s = timer_elapsed32(timer_t0) / 1000; return s > 1023 ? 1023 : (uint16_t)s; }
static void start_scramble(void) { gen_scramble(); mi = 0; move_t = timer_read32(); rb = RB_SCRAMBLE; }

void rubik_start(void) { build_positions(); solve(); rb = RB_IDLE; space_down = false; st = A_RUBIK; }

void rubik_key(uint8_t row, uint8_t col, bool pressed) {
    if (row != spcR || col != spcC) return;              // only the Space bar drives the game
    if (pressed) {
        if (rb == RB_TIMER) { timer_frozen = cur_seconds(); rb = RB_STOP; return; }   // running timer -> stop
        if (rb == RB_STOP || rb == RB_SCRAMBLE || rb == RB_COLORS) { rb = RB_IDLE; return; } // any shown result -> reset to idle
        rb = RB_HOLD; space_down = true; space_t = timer_read32();                     // begin arming (from idle)
    } else if (space_down) {
        space_down = false;
        if (timer_elapsed32(space_t) >= HOLD_MS) { rb = RB_TIMER; timer_t0 = timer_read32(); } // armed -> timer
        else start_scramble();                                                                 // quick tap -> scramble
    }
}

void rubik_tick(void) {
    if (rb == RB_SCRAMBLE && timer_elapsed32(move_t) >= MOVE_MS) {
        if (++mi >= NMOVES) rb = RB_COLORS; else move_t = timer_read32();
    }
}

static void show_binary(uint16_t v, uint8_t r, uint8_t g, uint8_t bl) {
    for (uint8_t b = 0; b < 10; b++) {
        if ((v >> (9 - b)) & 1) px(nbR[b], nbC[b], r, g, bl);    // set bit = given colour
        else                    px(nbR[b], nbC[b], 14, 11, 4);   // clear bit = very dim
    }
}

void rubik_render(void) {
    switch (rb) {
        case RB_IDLE:
            px(spcR, spcC, 60, 48, 16);                          // dim prompt on the Space bar
            for (uint8_t b = 0; b < 10; b++) px(nbR[b], nbC[b], 14, 11, 4);
            break;
        case RB_HOLD: {
            uint32_t e = timer_elapsed32(space_t);
            if (e < HOLD_MS) {                                   // filling: amber keys 1-> accumulate
                uint8_t lit = (uint8_t)(e * 10 / HOLD_MS);
                for (uint8_t b = 0; b < 10; b++)
                    px(nbR[b], nbC[b], b < lit ? 224 : 14, b < lit ? 184 : 11, b < lit ? 58 : 4);
            } else {                                             // armed: whole row breathes green
                uint16_t ph = e % 1200, tri = ph < 600 ? ph : 1200 - ph; // 0..600
                uint8_t val = 90 + (uint8_t)((uint32_t)tri * 165 / 600);  // 90..255
                for (uint8_t b = 0; b < 10; b++) px(nbR[b], nbC[b], 0, val, val * 40 / 255);
            }
            break;
        }
        case RB_TIMER: show_binary(cur_seconds(), 224, 184, 58); break; // running = amber
        case RB_STOP:  show_binary(timer_frozen,   0, 196, 90); break; // stopped = green
        case RB_SCRAMBLE: {
            if (mi >= NMOVES) break;
            uint8_t m = mv_face[mi], ty = mv_type[mi];
            px(faceR[m], faceC[m], 224, 184, 58);                // face key amber
            if (ty == 2) px(dblR,   dblC,   74, 163, 255);       // double -> '2' blue
            if (ty == 1) px(primeR, primeC, 255, 93, 162);       // prime  -> ' pink
            break;
        }
        case RB_COLORS:
            for (uint8_t pr = 0; pr < 6; pr++) {                 // each physical row = one cube face
                uint8_t f = ROW_FACE[pr];
                for (uint8_t s = 0; s < 9; s++) {
                    uint8_t c = cube[f][s];                      // the scrambled sticker's face colour
                    px(rowsR[pr][s], rowsC[pr][s], COL[c][0], COL[c][1], COL[c][2]);
                }
            }
            break;
    }
}
