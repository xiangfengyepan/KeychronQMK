/* Memory (Simon) — whole board. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"

/* ================= MEMORY — Simon on the whole board =================
 * Watch the sequence flash, then repeat it by pressing the keys in order.
 * Each round appends one more random key. A wrong key ends it. */
#define MEM_MAX 64
#define MEM_ON  420
#define MEM_GAP 200
static uint8_t  mseq[MEM_MAX], mlen, mpos, mstage; // stage 0=show 1=input 2=round-clear
static uint32_t mem_t, mflash_t;
static int16_t  mflash;                            // last correct cell (green feedback), -1 none
void memory_start(void) {
    mlen = 1; mseq[0] = rnd() % nC; mpos = 0; mstage = 0; mflash = -1; mem_t = timer_read32(); st = A_MEMORY;
}
void memory_press(uint8_t row, uint8_t col) {
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
void memory_tick(void) {
    if (mstage == 0) { if (timer_elapsed32(mem_t) >= (uint32_t)mlen * (MEM_ON + MEM_GAP)) { mstage = 1; mpos = 0; } }
    else if (mstage == 2) { if (timer_elapsed32(mem_t) >= 500) { mstage = 0; mpos = 0; mem_t = timer_read32(); } }
}
void memory_render(void) {
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
