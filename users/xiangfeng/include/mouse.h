#pragma once
/* mouse.h — public API of the mouse-animation engine (src/mouse/mouse.c).
 *
 * Three features share one per-tick driver (mouse_task, called every
 * housekeeping):
 *   - auto shape movers   : trace a shape with the relative mouse (layer 1 · 1..0)
 *   - full-screen DVD bounce: absolute digitizer report (layer 1 · F9)
 *   - "draw with the mouse" : traces glyph stroke polylines, holding the left
 *                             button per stroke — also driven by the pinyin IME.
 *
 * keymap.c keeps the custom_keycodes switch and calls the semantic seams below;
 * it never touches the module's internal state. */
#include "quantum.h"

// Auto mouse-shape mover. One shape per number key (layer 1 · 1..0):
//   1 ∞infinity  2 circle  3 triangle  4 square  5 hexagon
//   6 star       7 heart   8 spirograph 9 spiral 0 lissajous
enum { SHP_OFF = 0, SHP_INF, SHP_INFH, SHP_WAVE, SHP_SPIRAL,
       SHP_CIRCLE, SHP_TRI, SHP_SQUARE, SHP_PENTA, SHP_HEX,
       SHP_STAR, SHP_HEART, SHP_ROSE, SHP_LISS, SHP_SPIRO };

/* ---- mouse seams called from keymap.c ---- */
void mouse_shape_toggle(uint8_t s); // tap a shape key: start it, or stop it if already running
void mouse_dvd_toggle(void);        // layer 1 · F9: full-screen DVD bounce on/off
void mouse_stop(void);              // stop any running animation (shape / DVD / draw)
void mouse_task(void);              // per-tick driver — call every housekeeping

/* ---- mouse character-drawing engine (shared with the pinyin IME, src/ime).
 *      ime_confirm() traces a glyph with it; the IME toggle rewinds the carriage. */
void draw_begin(const int16_t *x, const int16_t *y, const uint8_t *len, uint8_t ns);
extern float carriage_x; // x-origin for the next drawn character
extern float draw_cx;    // continuous virtual pen position (x)
extern float draw_cy;    // continuous virtual pen position (y)
