#pragma once
#include <stdint.h>

typedef struct {
    const int16_t *x;
    const int16_t *y;
    const uint8_t *len;
    uint8_t  nstroke;
    const char *py;
} hanzi_t;

#define HANZI_MAX_STROKES 19
extern const hanzi_t hanzi_table[];
extern const uint16_t hanzi_count;
