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

#include "rgb_matrix_kb_config.h"

#if defined(KEYCHRON_RGB_ENABLE) && defined(EECONFIG_SIZE_CUSTOM_RGB)

RGB_MATRIX_EFFECT(PER_KEY_RGB)
RGB_MATRIX_EFFECT(MIXED_RGB)
// Custom effects must stay below MIXED_RGB (the VIA mixed-layer list rejects any
// effect id >= RGB_MATRIX_CUSTOM_MIXED_RGB, and PER_KEY_RGB/MIXED_RGB ids are fixed).
RGB_MATRIX_EFFECT(LETTERS_MARQUEE)
RGB_MATRIX_EFFECT(LETTERS_BIG)

#    ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

bool PER_KEY_RGB(effect_params_t *params) {
    extern bool per_key_rgb(effect_params_t * params);
    return per_key_rgb(params);
}

bool MIXED_RGB(effect_params_t *params) {
    extern bool mixed_rgb(effect_params_t * params);
    return mixed_rgb(params);
}

bool LETTERS_MARQUEE(effect_params_t *params) {
    extern bool letters_marquee(effect_params_t * params);
    return letters_marquee(params);
}

bool LETTERS_BIG(effect_params_t *params) {
    extern bool letters_big(effect_params_t * params);
    return letters_big(params);
}

#    endif

#endif
