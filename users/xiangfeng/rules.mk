# QMK userspace: reusable modules for the `xiangfeng` keymaps.
# Auto-included by QMK when building any keymap named `xiangfeng` (or with
# USER_NAME = xiangfeng). QMK adds this dir to VPATH and the compiler -I path,
# so bare `src/…` paths resolve here and headers use `#include "include/foo.h"`.

# Custom RGB effects (letters marquee/big + Spider-Man mask + palette/heatmap/audio)
SRC += src/letters.c src/spider_mask.c src/arcade.c src/palette.c src/heatmap.c src/audio.c src/piano.c
# extracted subsystems: mouse-animation engine, RGB-adjust feedback, generic helpers
SRC += src/mouse/mouse.c src/rgbfx/rgbfx.c src/utils/utils.c
# pinyin IME (compose mode) + its baked stroke dictionary
SRC += src/ime/ime.c src/ime/hanzi_data.c
# per-game arcade sources (shared/dispatch stays in src/arcade.c)
SRC += src/arcade/tetris.c src/arcade/topo.c src/arcade/flappy.c src/arcade/dino.c
SRC += src/arcade/memory.c src/arcade/reaction.c src/arcade/drop.c src/arcade/pong.c
SRC += src/arcade/rubik.c
SRC += src/arcade/snake.c
