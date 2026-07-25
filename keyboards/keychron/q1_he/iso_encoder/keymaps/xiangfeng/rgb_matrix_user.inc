/* USER-level custom RGB effects for the `xiangfeng` keymap.
 * Moved out of common/rgb/rgb_matrix_kb.inc so the whole custom build lives in
 * this folder. These enumerate AFTER the keyboard's KB effects (PER_KEY_RGB,
 * MIXED_RGB), so their effect ids are unchanged (Letters Marquee/Big, Spider-Man).
 * Implementations compile from letters.c / spider_mask.c (SRC in rules.mk). */

RGB_MATRIX_EFFECT(LETTERS_MARQUEE)
RGB_MATRIX_EFFECT(LETTERS_BIG)
RGB_MATRIX_EFFECT(SPIDER_MASK)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

bool LETTERS_MARQUEE(effect_params_t *params) {
    extern bool letters_marquee(effect_params_t * params);
    return letters_marquee(params);
}

bool LETTERS_BIG(effect_params_t *params) {
    extern bool letters_big(effect_params_t * params);
    return letters_big(params);
}

bool SPIDER_MASK(effect_params_t *params) {
    extern bool spider_mask(effect_params_t * params);
    return spider_mask(params);
}

#endif
