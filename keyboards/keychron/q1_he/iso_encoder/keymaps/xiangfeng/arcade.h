#pragma once
#include <stdint.h>
#include <stdbool.h>

// On-keyboard arcade: lobby -> countdown -> Tetris / Topo -> score.
// Opened with Fn+H. While active it swallows all keys. Knob: turn = browse/rotate,
// short press = select/continue, press-and-HOLD = quit to lobby / exit arcade.
void arcade_open(uint32_t seed);
bool arcade_active(void);
void arcade_key(uint8_t row, uint8_t col, bool pressed); // a physical key event
void arcade_encoder(bool clockwise);                     // a knob turn
void arcade_tick(void);                                  // call every housekeeping
void arcade_render(uint8_t led_min, uint8_t led_max);    // paint the board
