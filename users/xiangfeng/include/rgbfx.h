#pragma once
/* rgbfx.h — RGB-adjust feedback + palette-knob helpers (src/rgbfx/rgbfx.c).
 *
 * While you tune the RGB effect (Fn + hue/sat/val/speed keys) this module drives
 * the visual feedback: a red flash at min/max, a blackout when hue wraps past 0,
 * a solid red flash + green layer meter on a default-layer change, auto-repeat on
 * a held adjust key, and a binary readout of the live 0-255 value on the number
 * row. It also handles the Palette effect's knob-hold "type the H,S,V" gesture.
 *
 * keymap.c keeps the process_record dispatch and calls the seams below; the QMK
 * hooks call rgbfx_task (every housekeeping) and rgbfx_render (in the indicator
 * pass, after the arcade/lock/IME early-returns). */
#include "quantum.h"

/* ---- rgbfx seams called from keymap.c ---- */
void rgbfx_flash_layer(void);              // solid red flash + layer meter (default-layer change or LAY_SHOW)
void rgbfx_adjust_press(uint16_t keycode); // an RGB adjust key (UG_*) pressed: arm auto-repeat + binary readout
void rgbfx_adjust_release(uint16_t keycode);// released: disarm and persist the reached value
void rgbfx_pal_knob(bool pressed);         // Palette effect: knob push down/up (hold types H,S,V, tap resets)

/* ---- QMK-hook bodies ---- */
void rgbfx_task(void);                          // per-tick: auto-repeat, min/max checks, knob-hold typing
void rgbfx_render(uint8_t led_min, uint8_t led_max); // paint the flash + binary readout
