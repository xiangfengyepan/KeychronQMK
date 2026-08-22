#pragma once
/* arcade_internal.h — shared plumbing between the arcade hub (src/arcade.c) and
 * each per-game file (src/arcade/<game>.c). NOT part of the public keymap API;
 * the keymap only ever sees include/arcade.h. Included by arcade.c and every
 * per-game file under src/arcade. */
#include <stdint.h>
#include <stdbool.h>

/* ---- state machine (owned by arcade.c; games set `st` to their own state) ---- */
enum { A_OFF, A_LOBBY, A_COUNT, A_TETRIS, A_TOPO, A_FLAPPY, A_DINO, A_MEMORY, A_REACT, A_DROP, A_PONG, A_RUBIK, A_SNAKE, A_SCORE };
extern uint8_t st;

/* ---- shared RNG (defined in arcade.c) ---- */
uint32_t rnd(void);

/* ---- shared LED helpers (defined in arcade.c) ---- */
void   px(uint8_t r, uint8_t c, uint8_t R, uint8_t G, uint8_t B);
int8_t iround(float x);

/* ---- shared cell table: the lit keys in reading order (defined in arcade.c) ---- */
extern uint8_t  cR[90], cC[90];
extern uint16_t nC;

/* ---- shared physics timestamp (flappy / dino / pong) ---- */
extern uint32_t phys_last;

/* ---- score / game-over screen (games call game_over; pong writes it directly) ---- */
extern float    sc_frac;
extern uint8_t  sR, sG, sB;
extern uint32_t sc_t;
void game_over(uint16_t score, uint8_t kind);

/* ================= per-game hooks (defined in src/arcade/<game>.c) ================= */
/* Tetris */
void tetris_start(void);
void tetris_tick(void);
void tetris_render(void);
void tmove(int8_t d);
void trot(int8_t dir);
void tdrop(void);
/* Topo */
void topo_start(void);
void topo_tick(void);
void topo_render(void);
void topo_hit(uint8_t r, uint8_t c);
/* Flappy */
void flappy_start(void);
void flappy_tick(void);
void flappy_render(void);
void flappy_flap(void);
/* Dino */
void dino_start(void);
void dino_tick(void);
void dino_render(void);
void dino_jump(void);
void dino_key(uint8_t row, uint8_t col, bool pressed);
/* Memory */
void memory_start(void);
void memory_tick(void);
void memory_render(void);
void memory_press(uint8_t row, uint8_t col);
/* Reaction */
void react_start(void);
void react_tick(void);
void react_render(void);
void react_press(void);
/* Drop-merge */
void drop_start(void);
void drop_tick(void);
void drop_render(void);
void drop_move(int8_t d);
void drop_hard(void);
/* Pong */
void pong_start(void);
void pong_tick(void);
void pong_render(void);
void pong_key(uint8_t row, uint8_t col, bool pressed);
/* Rubik */
void rubik_start(void);
void rubik_tick(void);
void rubik_render(void);
void rubik_key(uint8_t row, uint8_t col, bool pressed);
/* Snake */
void snake_start(void);
void snake_tick(void);
void snake_render(void);
void snake_dir(uint8_t d);
