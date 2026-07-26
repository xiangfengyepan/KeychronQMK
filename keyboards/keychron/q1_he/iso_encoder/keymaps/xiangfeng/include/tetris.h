#pragma once
#include <stdint.h>
#include <stdbool.h>

// Tetris on the RGB grid. Well = matrix rows 0..4 (across) x cols 0..10 (fall).
// Hold the board vertically: pieces fall left->right, you move them top<->bottom.
void tetris_start(uint32_t seed);   // begin / restart a game
void tetris_stop(void);             // quit back to normal
bool tetris_active(void);           // is a game running?
bool tetris_over(void);             // game-over screen showing?
void tetris_move(int8_t d);         // d = -1 (toward row 0) / +1 (toward row 4)
void tetris_rotate(void);           // rotate current piece (knob press)
void tetris_harddrop(void);         // slam to the floor
void tetris_tick(void);             // gravity + timing; call every housekeeping
void tetris_render(uint8_t led_min, uint8_t led_max); // paint the board
