# Q1 HE — Memory & storage map

The keyboard's brain is an **STM32F401xC** microcontroller. It has two on-chip memories
(Flash + RAM), plus a few other storage areas that live elsewhere. Here's the whole picture.

## The components

| # | Component | Size | Volatile? | Where | What it holds |
|---|-----------|------|-----------|-------|----------------|
| 1 | **Flash** (program memory) | **256 KB** | No — survives power off | on the MCU | the firmware itself: code + baked constant data (keymap, HE profiles, the 209-char IME dictionary, RGB effects, arcade games) |
| 2 | **SRAM** (RAM) | **64 KB** | **Yes** — cleared on power loss | on the MCU | live working memory while running: variables, the RGB frame buffer, USB buffers, the IME's pinyin/candidate buffers |
| 3 | **Emulated EEPROM** (settings) | **~12 KB** carved from Flash | No | inside the 256 KB Flash | persistent user settings: your Launcher edits, RGB mode/color, per-key HE calibration & profiles |
| 4 | **Bootloader ROM** | ~30 KB | No | separate MCU system memory | the built-in STM32 DFU bootloader used to flash — **not** part of the 256 KB, can't be bricked by normal flashing |
| 5 | **Wireless module** (lkbt51) | its own | No | a *separate* co-processor chip | Bluetooth/2.4 GHz radio firmware + pairing info — independent of the main MCU |

## Flash usage — measured (this `xiangfeng` build)

Pulled straight from the compiled ELF (`arm-none-eabi-size`):

| Region | Bytes | KB | What it is |
|--------|------:|---:|------------|
| `.text` | 116,408 | **~114 KB** | program code (QMK core + all custom logic) |
| `.rodata` | 48,860 | **~48 KB** | read-only constants — **includes the IME dictionary ≈ 41 KB** |
| `.data` (flash copy) | 3,116 | **~3 KB** | initial values for RAM variables, stored in flash |
| `.vectors` | 480 | ~0.5 KB | reset + interrupt vector table |
| **firmware content** | **168,864** | **~165 KB** | sum of the above |
| reserved (app offset) | 32,312 | ~32 KB | gap before code — `.text` starts at `0x0800_8000` |
| **flashed image (`.bin`)** | **201,176** | **~196 KB** | what actually gets written to flash |

So of the 256 KB: **~196 KB is the flashed image**, **~12 KB** is the emulated-EEPROM slice, leaving
**~48 KB free**. The dictionary (`.rodata`, ~41 KB) is the biggest single thing *you* added; the arcade added ~6 KB of code across its six games (Tetris, Topo, Flappy, Dino, Memory, Reaction).

## The memory stack (address map, with KB used)

```
        FLASH  (256 KB, non-volatile)                 RAM / SRAM  (64 KB, volatile)
0x0804_0000 ┌───────────────────────────┐   0x2001_0000 ┌───────────────────────────┐
            │ Emulated EEPROM   ~12 KB  │               │ ▼ stack / heap  ~33 KB    │
0x0803_D000 ├───────────────────────────┤               │   (free working room)     │
            │ free              ~48 KB  │               │            ·              │
            │ ← dictionary grows here   │               ├───────────────────────────┤
0x0802_F200 ├───────────────────────────┤               │ .bss            ~28 KB    │
            │ .data (init vals)  ~3 KB  │               │ RGB frame buf, IME state, │
            │ .rodata constants ~47 KB  │               │ QMK state (zero-init)     │
            │   • IME dict ~41 KB       │               ├───────────────────────────┤
            │ .text  code      ~114 KB  │               │ .data           ~3 KB     │
0x0800_8000 ├───────────────────────────┤   0x2000_0000 └───────────────────────────┘
            │ reserved (offset) ~32 KB  │
0x0800_0000 │ .vectors          ~0.5 KB │           (bootloader is OUTSIDE this,
            └───────────────────────────┘            in system memory @ 0x1FFF_0000)
```

**How to read it:**
- **`.text` (~110 KB)** = compiled program. **`.rodata` (~47 KB)** = read-only constants — the
  209-char IME dictionary (~41 KB), the baked keymap, and the fonts live here. Both are in Flash
  because they must survive power-off.
- **`.data` / `.bss`** = variables in **RAM**; `.data` (~3 KB) starts from values copied out of Flash
  at boot, `.bss` (~28 KB) starts zeroed. The **stack** grows down from the top of RAM (~33 KB spare).
- The **emulated EEPROM** (~12 KB) sits at the top of Flash; the **bootloader** is in a *separate*
  ROM that flashing never touches — that's why a bad flash can't brick the board.

## RAM — plenty of headroom

64 KB of SRAM: ~28 KB `.bss` + ~3 KB `.data` in use, leaving **~33 KB** for stack/heap. The custom
features cost very little — the IME added only a few hundred bytes (pinyin buffer, candidate list,
LED stroke-path array). RAM is **not** a constraint here; Flash is.

## Quick rules of thumb

- Add features/data → spends **Flash**. Watch the ~51 KB free (≈ another ~190 dictionary chars at
  ~0.26 KB each).
- Runtime buffers/variables → spend **RAM** (64 KB, ~33 KB spare).
- "It remembers X after unplugging" → stored in the **emulated EEPROM** (Flash slice), and only
  updates to baked defaults take effect on an **EEPROM/factory reset**.
- Flashing replaces component **1** (and re-inits **3**); it never touches **4** or **5**.
