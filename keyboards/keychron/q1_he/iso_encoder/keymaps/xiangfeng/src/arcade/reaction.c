/* Reaction — light up, press ASAP. See src/arcade.c for the hub / dispatch. */
#include "quantum.h"
#include "rgb_matrix.h"
#include "include/arcade_internal.h"

/* ================= REACTION — light up, press ASAP =================
 * 3 rounds: board waits on a dim red, flashes bright green, you hit any key. Score = avg reaction ms. */
static uint8_t  rx_round, rx_state;               // rx_state 0=wait 1=go
static uint32_t rx_sum, rx_wait_t, rx_go_t, rx_delay, rx_false_t;
static bool     rx_false;
static void rx_newround(void) { rx_state = 0; rx_delay = 900 + rnd() % 2400; rx_wait_t = timer_read32(); }
void react_start(void) { rx_round = 0; rx_sum = 0; rx_false = false; rx_newround(); st = A_REACT; }
void react_press(void) {
    if (rx_state == 0) { rx_false = true; rx_false_t = timer_read32(); rx_newround(); return; } // jumped the gun -> redo
    rx_sum += timer_elapsed32(rx_go_t);
    if (++rx_round >= 3) game_over((uint16_t)(rx_sum / 3), 5);
    else rx_newround();
}
void react_tick(void) {
    if (rx_state == 0 && timer_elapsed32(rx_wait_t) >= rx_delay) { rx_state = 1; rx_go_t = timer_read32(); }
}
void react_render(void) {
    if (rx_state == 1) { for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], 40, 230, 80); }        // GO: bright green
    else {                                                                                          // wait: dim red
        uint8_t r = (rx_false && timer_elapsed32(rx_false_t) < 400) ? 150 : 26;                     // brighter on a false start
        for (uint16_t i = 0; i < nC; i++) px(cR[i], cC[i], r, 0, 0);
    }
}
