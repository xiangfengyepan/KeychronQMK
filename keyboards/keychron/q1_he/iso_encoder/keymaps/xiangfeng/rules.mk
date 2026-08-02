VIA_ENABLE = yes

# Absolute pointer (for the full-screen DVD bounce). USB-only feature.
DIGITIZER_ENABLE = yes
DIGITIZER_SHARED_EP = yes

# Custom RGB effects (letters marquee/big + Spider-Man mask) live in this folder
# as USER-level effects, registered in rgb_matrix_user.inc — so no common/rgb edits.
RGB_MATRIX_CUSTOM_USER = yes

# Reusable code modules (all SRC + include/ + docs/) live in the QMK userspace
# users/xiangfeng/, auto-included because this keymap is named `xiangfeng`.
# See users/xiangfeng/rules.mk for the module SRC list.
