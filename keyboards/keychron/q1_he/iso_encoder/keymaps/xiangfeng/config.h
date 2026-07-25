#pragma once

/* ------------------------------------------------------------------------
 * Custom firmware config for the `xiangfeng` keymap.
 * These defines used to live in keyboards/keychron/q1_he/config.h; they were
 * moved here so the whole custom build lives in this one keymap folder.
 * (Two files can't move — quantum/mousekey.c and common/analog_matrix/profile.c;
 *  the profile.c logic is gated by the *_PROFILE_INDEX defines below.)
 * ---------------------------------------------------------------------- */

/* Power-management default timeouts (seconds).
 * Baked-in defaults shown in the Launcher after an EEPROM reset:
 *   auto-sleep = 10 min, auto backlight-off = 1 min. */
#define CONNECTED_IDLE_TIME 600
#define CONNECTED_BACKLIGHT_DISABLE_TIMEOUT 60

/* Valorant-tuned "gaming" HE profile = Profile 2 in the Launcher (index 1).
 *   Rapid Trigger ON, actuation 1.2 mm, re-trigger sensitivity 0.2 mm.
 * (Values in 0.1 mm units; stock defaults are 2.0 mm / 0.4 mm.) */
#define GAMING_PROFILE_INDEX 1
#define GAMING_ACTUATION_POINT 12
#define GAMING_RAPID_TRIGGER_SENSITIVITY 2

/* Typing / programming HE profile = Profile 1 in the Launcher (index 0).
 * Regular (static) actuation at 2.6 mm for fewer accidental presses. */
#define TYPING_PROFILE_INDEX 0
#define TYPING_ACTUATION_POINT 26

/* RGB adjust step = 1 for fine control. Holding a key auto-repeats (see keymap
 * process_record_user / housekeeping_task_user), so a hold ramps smoothly. */
#define RGB_MATRIX_HUE_STEP 1
#define RGB_MATRIX_SAT_STEP 1
#define RGB_MATRIX_VAL_STEP 1
#define RGB_MATRIX_SPD_STEP 1

/* Power on into the Spider-Man mask effect (applies on EEPROM reset). */
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CUSTOM_SPIDER_MASK

/* Mouse keys: 5 persistent speed levels, ASCENDING (tap to lock a speed).
 * OFFSET = pixels per report (higher = faster); INTERVAL = ms between reports.
 * On the Fn layer, left to right F1..F5 = slowest .. fastest. */
#define MK_3_SPEED
/* Cursor speeds as multiples of 1x (=offset 16). F1..F5 = 0.1 / 0.2 / 0.4 / 1 / 2. */
#define MK_C_OFFSET_UNMOD 16 /* power-on default = 1.0x */
#define MK_C_INTERVAL_UNMOD 16
#define MK_C_OFFSET_0 2 /* acc0 (F1) = 0.1x */
#define MK_C_INTERVAL_0 16
#define MK_C_OFFSET_1 3 /* acc1 (F2) = 0.2x */
#define MK_C_INTERVAL_1 16
#define MK_C_OFFSET_2 6 /* acc2 (F3) = 0.4x */
#define MK_C_INTERVAL_2 16
#define MK_C_OFFSET_3 16 /* acc4 (F4) = 1.0x */
#define MK_C_INTERVAL_3 16
#define MK_C_OFFSET_4 32 /* acc5 (F5) = 2.0x */
#define MK_C_INTERVAL_4 16
/* Scroll-wheel: higher interval = slower scroll, matched to the same ratios */
#define MK_W_OFFSET_UNMOD 1
#define MK_W_INTERVAL_UNMOD 40 /* 1.0x */
#define MK_W_OFFSET_0 1
#define MK_W_INTERVAL_0 300 /* acc0 0.1x */
#define MK_W_OFFSET_1 1
#define MK_W_INTERVAL_1 180 /* acc1 0.2x */
#define MK_W_OFFSET_2 1
#define MK_W_INTERVAL_2 90 /* acc2 0.4x */
#define MK_W_OFFSET_3 1
#define MK_W_INTERVAL_3 40 /* acc4 1.0x */
#define MK_W_OFFSET_4 1
#define MK_W_INTERVAL_4 20 /* acc5 2.0x */
