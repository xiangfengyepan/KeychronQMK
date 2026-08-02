#pragma once
/* utils.h — small generic helpers shared across the keymap modules.
 *
 * These have no state of their own; they are pure helpers pulled out so the
 * RGB-feedback (src/rgbfx), the IME (src/ime) and the keymap glue all use one
 * copy instead of local duplicates. */
#include "quantum.h"

// HSV -> RGB, forcing the value (brightness) to v. Handy for rendering a
// palette color constant (COL_*) at a specific brightness (max-channel).
RGB pal_rgb(HSV c, uint8_t v);

// Append v (0-255) as decimal to s (no NUL); returns the number of chars written.
uint8_t pal_put_u8(char *s, uint8_t v);
