/* utils.c — tiny generic helpers (see include/utils.h). */
#include "quantum.h"
#include "include/utils.h"

// snap an indicator color to a palette constant rendered at brightness v (max-channel)
RGB pal_rgb(HSV c, uint8_t v) { c.v = v; return hsv_to_rgb(c); }

// append v as decimal, return chars written
uint8_t pal_put_u8(char *s, uint8_t v) {
    uint8_t n = 0;
    if (v >= 100) s[n++] = '0' + v / 100;
    if (v >= 10)  s[n++] = '0' + (v / 10) % 10;
    s[n++] = '0' + v % 10;
    return n;
}
