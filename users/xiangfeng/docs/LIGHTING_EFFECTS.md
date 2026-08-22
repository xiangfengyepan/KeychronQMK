# Q1 HE (ISO) — Lighting effect order

Order of the RGB effects as cycled with **layer 1 · Q** (next) / **layer 1 · A** (previous), and as
selected by number in VIA (`id_qmk_rgb_matrix_effect`). The last six (25–30) are custom.

**Default power-on effect: Spider-Man (#27)** — set via `RGB_MATRIX_DEFAULT_MODE` in this folder's `config.h`; applies on an EEPROM reset.

| # | Effect | Type | Notes |
|---|--------|------|-------|
| 0 | None | off | all LEDs off |
| 1 | Solid Color | static | |
| 2 | Breathing | ambient | |
| 3 | Band Spiral Val | ambient | |
| 4 | Cycle All | ambient | |
| 5 | Cycle Left Right | ambient | |
| 6 | Cycle Up Down | ambient | |
| 7 | Rainbow Moving Chevron | ambient | |
| 8 | Cycle Out In | ambient | |
| 9 | Cycle Out In Dual | ambient | |
| 10 | Cycle Pinwheel | ambient | |
| 11 | Cycle Spiral | ambient | |
| 12 | Dual Beacon | ambient | |
| 13 | Rainbow Beacon | ambient | |
| 14 | Jellybean Raindrops | ambient | |
| 15 | Pixel Rain | ambient | |
| 16 | Typing Heatmap | reactive | |
| 17 | Digital Rain | ambient | "Matrix" rain |
| 18 | Reactive Simple | reactive | |
| 19 | Reactive Multiwide | reactive | |
| 20 | Reactive Multinexus | reactive | |
| 21 | Splash | reactive | |
| 22 | Solid Splash | reactive | |
| 23 | Per Key RGB | Keychron custom | per-key colors |
| 24 | Mix RGB | Keychron custom | layered regions |
| **25** | **Letters Marquee** | **custom · reactive** | typed text scrolls L→R (layer 3 · Backspace clears) |
| **26** | **Letters Big** | **custom · reactive** | last key drawn big, fades |
| **27** | **Spider-Man** ⭐ | **custom · reactive** | red mask + white eyes + blink; web-burst on keypress — **power-on default** |
| **28** | **Palette** 🎨 | **custom · tool** | fills the board with one named color for calibration. See below. Source: `src/palette.c`, colors in `include/palette.h` |
| **29** | **Pressure Heatmap** 🔥 | **custom · analog** | dark board; each key lights by its **live analog Hall-effect travel** (how far it's pressed) and bleeds heat into neighbors with falloff — deeper press spreads farther; released keys cool back to black. Cool→hot thermal ramp; brightness = Fn+W/S; **cool-down rate = Fn+T/G (RGB speed)** — higher speed fades faster. Source: `src/heatmap.c` (`analog_matrix_get_travel`) |
| **30** | **Audio** 🎵 | **custom · Raw HID** | PC audio spectrum visualizer: vertical EQ bars (green→yellow→red, rising from the bottom) driven by the `~/audio-keyboard` companion app over Raw HID (command `0xAC`). Bars fall to black when no app is running. Dims with Fn+W/S. Source: `src/audio.c` (+ `kc_custom_hid_rx` hook in `common/keychron_raw_hid.c`) |

**Palette (color calibration)**
A tool effect for dialling in exact `HSV` values (all channels 0–255). The named
defaults live in `include/palette.h` — 15 named colors: `COL_RED`, `COL_CORAL`,
`COL_ORANGE`, `COL_GOLD`, `COL_LIME`, `COL_GREEN`, `COL_GREEN_LIGHT`,
`COL_CYAN` (= "blue light"), `COL_SKY`, `COL_BLUE`, `COL_PURPLE`,
`COL_MAGENTA`, `COL_ROSE`, `COL_PINK` (= "red light"), `COL_WHITE`.
The knob steps them in that order — a walk around the color wheel so
similar swatches sit next to each other, with white last. These
constants are also the single source of color for Spider-Man, the
keyboard indicators (IME / lock / layer meter / min-max flash) and the arcade
game palettes.
While this effect is active, the **knob** is repurposed:
- **turn** → next / previous swatch (loads that color's value into the live HSV);
- **HSV adjust keys** (layer 1 · E/D · R/F · W/S) fine-tune it — each swatch's
  tuning is remembered in RAM as you move between swatches;
- **tap the knob** → reset the current swatch to its baked default;
- **hold the knob (~0.5 s)** → **types** the live `H,S,V` out over USB (e.g.
  `10,255,255`) so you can paste the exact value with no counting.

**Selecting effects**
- On the keyboard: **layer 1 · Q / A** to cycle. The six custom effects (25–30) are at the end,
  so **layer 1 · A** from the first effect wraps straight to Audio (30), then Pressure Heatmap (29)…
- In VIA (usevia.app, with the custom definition loaded): Lighting → Effect dropdown lists all
  of the above by name.

**Adjusting the active effect** (step = 1, hold to repeat):
layer 1 · E/D hue · layer 1 · R/F saturation · layer 1 · W/S brightness · layer 1 · T/G speed.
Saturation/brightness/speed **blink red** while pinned at min or max (a blink, not a solid hold). Hue
is cyclic, so instead the board **blanks once** each time it passes through 0 — hue 0 is red, so a red
flash there wouldn't be visible.
