/* palette.h — named HSV color constants for the Q1 HE `xiangfeng` keymap.
 *
 * H, S, V are ALL 0-255 (QMK convention; H is NOT the 0-180 OpenCV range).
 *
 * These are calibrated on the keyboard itself with the "Palette" RGB effect:
 *   - turn the knob         -> move to the next / previous swatch
 *   - HSV adjust macro keys  -> fine-tune the live color (step is 1 per tap);
 *                               each swatch's tuning is kept in RAM as you go
 *   - TAP the knob          -> reset the current swatch to its default (below)
 *   - HOLD the knob (~0.5 s) -> type the live "H,S,V" out over USB, so you can
 *                               read/paste the exact value with no counting
 * Paste me the typed H,S,V and I bake it into the matching macro here.
 *
 * Format is { H, S, V } so each macro can seed an `HSV`.
 */
#pragma once

// ---- named colors (calibrated on-keyboard 2026-07-26) ----
#define COL_RED {0, 255, 255}           // pure red
#define COL_PINK {240, 200, 255}        // "red light": pink
#define COL_ORANGE {10, 255, 255}       // vivid orange -> (255,60,0)
#define COL_GREEN {85, 255, 255}        // pure green
#define COL_GREEN_LIGHT {85, 130, 255}  // mint / soft green
#define COL_CYAN {128, 255, 255}        // "blue light": cyan
#define COL_BLUE {170, 255, 255}        // pure blue
#define COL_PURPLE {190, 255, 255}      // violet
#define COL_MAGENTA {213,255,255}     // magenta / hot pink
#define COL_WHITE {0, 0, 255}           // white

// ---- extra colors (starting guesses; tune on-keyboard like the above) ----
#define COL_CORAL {5, 200, 255}     // warm pinkish-orange
#define COL_GOLD {36, 255, 255}     // amber gold
#define COL_LIME {55, 255, 255}     // yellow-green
#define COL_SKY {145, 160, 255}     // light sky blue
#define COL_ROSE {235, 255, 255}    // soft magenta-pink

#ifndef __ASSEMBLER__

void palette_step(bool forward); // knob turn: advance to next (forward) / previous swatch
void palette_reset(void);        // knob tap:  reload the current swatch's default color
#endif
