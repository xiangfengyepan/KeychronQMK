# Q1 HE — Memory & storage map

The keyboard's brain is an **STM32F401xC** microcontroller. It has two on-chip memories
(Flash + RAM), plus a few other storage areas that live elsewhere. Here's the whole picture.

## The components

| # | Component | Size | Volatile? | Where | What it holds |
|---|-----------|------|-----------|-------|----------------|
| 1 | **Flash** (program memory) | **256 KB** | No — survives power off | on the MCU | the firmware itself: code + baked constant data (keymap, HE profiles, the 209-char IME dictionary, RGB effects) |
| 2 | **SRAM** (RAM) | **64 KB** | **Yes** — cleared on power loss | on the MCU | live working memory while running: variables, the RGB frame buffer, USB buffers, the IME's pinyin/candidate buffers |
| 3 | **Emulated EEPROM** (settings) | **~12 KB** carved from Flash | No | inside the 256 KB Flash | persistent user settings: your Launcher edits, RGB mode/color, per-key HE calibration & profiles |
| 4 | **Bootloader ROM** | ~30 KB | No | separate MCU system memory | the built-in STM32 DFU bootloader used to flash — **not** part of the 256 KB, can't be bricked by normal flashing |
| 5 | **Wireless module** (lkbt51) | its own | No | a *separate* co-processor chip | Bluetooth/2.4 GHz radio firmware + pairing info — independent of the main MCU |

## Flash — the one that matters for features

Flash is where everything you "bake in" has to fit, and it's the limited resource.

```
|<---------------------------- 256 KB Flash ---------------------------->|
| firmware (code + data)  ~188 KB | free ~56 KB | emulated-EEPROM ~12 KB  |
```

- **Firmware today:** ~188 KB (includes the 209-char IME dictionary ≈ 41 KB).
- **Free:** ~55 KB usable → room for roughly **another ~200 characters** in the dictionary
  (each character ≈ 0.26 KB) before it gets tight.
- **STM32F401xC has no real EEPROM**, so "saved settings" are emulated by reserving a slice of
  this same Flash (wear-levelled, `backing_size = 12288`). That's why the settings region eats
  into the 256 KB.

## The memory stack (address map)

Each memory is a range of addresses. Here's how the firmware components stack inside them —
Flash (non-volatile) on the left, RAM (volatile) on the right.

```
        FLASH  (256 KB, non-volatile)                RAM / SRAM  (64 KB, volatile)
0x0804_0000 ┌────────────────────────────┐   0x2001_0000 ┌────────────────────────────┐
            │  Emulated EEPROM  (~12 KB)  │               │  ▼ stack  (grows downward) │
            │  settings, wear-levelled   │               │    call frames, locals     │
0x0803_D000 ├────────────────────────────┤               │            ·               │
            │                            │               │            ·               │
            │  free  (~55 KB)            │               │  ▲ heap  (little/none)     │
            │  ← dictionary grows here   │               ├────────────────────────────┤
            ├────────────────────────────┤               │  .bss   zero-init vars     │
            │  .rodata  constant data    │               │  RGB frame buf, IME         │
            │  • 209-char IME dict ~41KB │               │  buffers, QMK state         │
            │  • baked keymap, fonts     │               ├────────────────────────────┤
            │  • RGB/effect tables       │               │  .data  initialised vars   │
            ├────────────────────────────┤   0x2000_0000 └────────────────────────────┘
            │  .text   program code      │
            │  QMK core + custom logic   │           (bootloader lives OUTSIDE this,
0x0800_0000 └────────────────────────────┘            in system memory @ 0x1FFF_0000)
             ▲ vector table (reset + IRQ handlers)
```

**How to read it:**
- **`.text`** = the compiled program (QMK + your custom code). **`.rodata`** = read-only constants —
  this is where the 209-character dictionary, the baked keymap, and the fonts sit. Both are in Flash
  because they must survive power-off.
- **`.data` / `.bss`** = variables, which live in **RAM**; `.data` starts with values copied from Flash
  at boot, `.bss` starts zeroed. The **stack** grows down from the top of RAM.
- The **emulated EEPROM** sits at the very top of Flash, and the **bootloader** is in a *separate*
  ROM (system memory) that flashing never touches — that's why a bad flash can't brick the board.

## RAM — plenty of headroom

64 KB of SRAM, and the custom features use very little of it — the IME added only a few hundred
bytes (pinyin buffer, candidate list, the LED stroke-path array). RAM is **not** a constraint here;
Flash is.

## Quick rules of thumb

- Add features/data → spends **Flash**. Watch the ~55 KB headroom.
- Runtime buffers/variables → spend **RAM** (64 KB, barely touched).
- "It remembers X after unplugging" → stored in the **emulated EEPROM** (Flash slice), and only
  updates to baked defaults take effect on an **EEPROM/factory reset**.
- Flashing replaces components **1** (and re-inits **3**); it never touches **4** or **5**.
