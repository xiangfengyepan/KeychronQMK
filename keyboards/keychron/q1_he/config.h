/* Copyright 2024 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "eeconfig_kb.h"

/* External EEPROM Configuration*/
#define I2C_DRIVER I2CD3
#define I2C1_SCL_PIN A8
#define I2C1_SDA_PIN C9
#define EXTERNAL_EEPROM_WP_PIN B10

/* Analog Matrix Configuration */
#define ANALOG_MATRIX_POWER_PIN C13
#define ANALOG_MATRIX_POWER_ENABLE_LEVEL 1
#define ANALOG_MATRIX_WAKEUP_PIN C5

#define ENCODER_SWITCH_PIN A3
#define ENCODER_MATRIX_ROW 0
#define ENCODER_MATROX_COL 14

/* Joystick Configuration */
#ifdef JOYSTICK_ENABLE
#    define JOYSTICK_AXIS_COUNT 6
#    define JOYSTICK_BUTTON_COUNT 16
#endif

/* SPI Configuration */
#if defined(RGB_MATRIX_ENABLE) || defined(LK_WIRELESS_ENABLE)
#    define SPI_DRIVER SPID1
#    define SPI_SCK_PIN A5
#    define SPI_MISO_PIN A6
#    define SPI_MOSI_PIN A7
#endif

/* SNLED27351 Driver Configuration */
#if defined(RGB_MATRIX_ENABLE)
#    define SNLED27351_SELECT_PINS \
        { B8, B9 }
#    define SNLED27351_SDB_PIN B7
#    define SNLED27351_PHASE_CHANNEL SNLED27351_SCAN_PHASE_9_CHANNEL
#    define SNLED27351_SPI_DIVISOR 16
#endif

/* Power-management default timeouts (seconds).
 * Baked-in defaults shown in the Launcher after an EEPROM reset:
 *   auto-sleep = 10 min, auto backlight-off = 1 min. */
#define CONNECTED_IDLE_TIME 600
#define CONNECTED_BACKLIGHT_DISABLE_TIMEOUT 60

/* Valorant-tuned "gaming" HE profile = Profile 2 in the Launcher (index 1).
 * Applied when that profile is reset (factory reset or Launcher "Reset profile").
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

/* Wireless Configuration */
#ifdef LK_WIRELESS_ENABLE
/* Hardware Configuration */
#    define SPI_SCK_PIN A5
#    define SPI_MISO_PIN A6
#    define SPI_MOSI_PIN A7

#    define P24G_MODE_SELECT_PIN A10
#    define BT_MODE_SELECT_PIN A9

#    define LKBT51_RESET_PIN C4
#    define WIRELESS_TO_MCU_INT_PIN B1
#    define MCU_TO_WIRELESS_INT_PIN A4

#    define USB_POWER_SENSE_PIN B0
#    define USB_POWER_CONNECTED_LEVEL 0

#    define BAT_CHARGING_PIN B13
#    define BAT_CHARGING_LEVEL 0

#    if defined(RGB_MATRIX_ENABLE)
#        define BT_INDCATION_LED_MATRIX_LIST \
            { 15, 16, 17 }

#        define P24G_INDICATION_LED_INDEX 18

#        define BAT_LEVEL_LED_LIST \
            { 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 }

/* Reinit LED driver on tranport changed */
#        define LED_DRIVER_REINIT_ON_TRANSPORT_CHANGE
#    endif

/* Keep USB connection in wireless mode */
#    define KEEP_USB_CONNECTION_IN_WIRELESS_MODE

/* Enable wireless NKRO */
#    define WIRELESS_NKRO_ENABLE
#endif

/* Factory Test Keys */
#define FN_KEY_1 MO(1)
#define FN_KEY_2 MO(3)
#define FN_BL_TRIG_KEY KC_END
