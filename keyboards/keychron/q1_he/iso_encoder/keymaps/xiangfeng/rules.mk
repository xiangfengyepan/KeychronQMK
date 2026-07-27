VIA_ENABLE = yes

# Absolute pointer (for the full-screen DVD bounce). USB-only feature.
DIGITIZER_ENABLE = yes
DIGITIZER_SHARED_EP = yes

# Custom RGB effects (letters marquee/big + Spider-Man mask) live in this folder
# as USER-level effects, registered in rgb_matrix_user.inc — so no common/rgb edits.
RGB_MATRIX_CUSTOM_USER = yes

# All custom sources kept in this keymap folder.
SRC += src/letters.c src/spider_mask.c src/arcade.c src/crab.c src/palette.c src/heatmap.c src/audio.c
# extracted subsystems: mouse-animation engine, RGB-adjust feedback, generic helpers
SRC += src/mouse/mouse.c src/rgbfx/rgbfx.c src/utils/utils.c
# pinyin IME (compose mode) + its baked stroke dictionary
SRC += src/ime/ime.c src/ime/hanzi_data.c
# per-game arcade sources (shared/dispatch stays in src/arcade.c)
SRC += src/arcade/tetris.c src/arcade/topo.c src/arcade/flappy.c src/arcade/dino.c
SRC += src/arcade/memory.c src/arcade/reaction.c src/arcade/drop.c src/arcade/pong.c
