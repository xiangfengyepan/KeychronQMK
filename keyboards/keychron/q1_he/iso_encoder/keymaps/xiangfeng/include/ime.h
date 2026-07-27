#pragma once
/* ime.h — public API of the baked pinyin IME (src/ime/ime.c).
 *
 * Fn+I toggles compose mode. While on: type pinyin (letters) -> candidates load;
 * Left/Right (or Tab) cycle; the F-row (F1..F12) jumps to a candidate; Space/Enter
 * confirm and draw the character with the mouse; Backspace deletes a letter; Esc
 * cancels. The current candidate is animated stroke-by-stroke across the RGB LEDs.
 *
 * keymap.c keeps only thin call-outs to the three seams below. The IME dictionary
 * lives in src/ime/hanzi_data.c (include/hanzi_data.h). */
#include "quantum.h"

/* ---- IME seams called from keymap.c ---- */
bool ime_active(void);                                        // is compose mode on?
void ime_toggle(void);                                        // Fn+I: flip IME on/off (resets buffer + draw carriage)
bool ime_process_record(uint16_t keycode, keyrecord_t *record); // compose-mode key handling; true = consumed
void ime_render(uint8_t led_min, uint8_t led_max);            // candidate animation + length meter + F-row strip

/* The mouse character-drawing engine ime_confirm() traces glyphs with (draw_begin
 * + carriage_x / draw_cx / draw_cy) lives in src/mouse — see include/mouse.h. */
