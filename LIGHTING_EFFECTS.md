# Q1 HE (ISO) — Lighting effect order

Order of the RGB effects as cycled with **Fn+Q** (next) / **Fn+A** (previous), and as
selected by number in VIA (`id_qmk_rgb_matrix_effect`). The last three are custom.

**Default power-on effect: Spider-Man (#27)** — set via `RGB_MATRIX_DEFAULT_MODE` in `q1_he/config.h`; applies on an EEPROM reset.

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
| **25** | **Letters Marquee** | **custom · reactive** | typed text scrolls L→R (Fn+Backspace clears) |
| **26** | **Letters Big** | **custom · reactive** | last key drawn big, fades |
| **27** | **Spider-Man** ⭐ | **custom · reactive** | red mask + white eyes + blink; web-burst on keypress — **power-on default** |

**Selecting effects**
- On the keyboard: **Fn+Q / Fn+A** to cycle. The three custom effects (25–27) are at the end,
  so **Fn+A** from the first effect wraps straight to Spider-Man (27).
- In VIA (usevia.app, with the custom definition loaded): Lighting → Effect dropdown lists all
  of the above by name.

**Adjusting the active effect** (step = 1, hold to repeat):
Fn+E/D hue · Fn+R/F saturation · Fn+W/S brightness · Fn+T/G speed.
The board flashes **red** when saturation/brightness/speed hits its min or max (hue wraps, so no limit).
