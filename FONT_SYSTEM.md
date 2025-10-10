# Font System — Current Implementation and Build Artifacts

This document describes the current font system, how fonts are built and registered, and how the generated artifacts are consumed by the PiGFX build.

## Overview

PiGFX now uses a small, generic font registry layered over a set of binary, built‑in fonts. The built‑in fonts are generated from source images (png) and compiled into the firmware as read‑only assets. At boot, those fonts are registered and become selectable in the setup dialog.

Key components:

- `src/font_registry.h/.c`: font descriptors and registry API
- Generated font assets: `src/binary_assets.s` and `src/buildin_fonts.inc`
- Consumer sites: `src/gfx.c` (includes `buildin_fonts.inc`) and `src/setup.c` (enumerates fonts via the registry)

## Font Registry (runtime)

- The registry stores an array of `font_descriptor_t` entries (name, width, height, glyph data pointer, glyph accessor).
- A generated function `gfx_register_builtin_fonts()` is compiled in and called early in init to register every compiled‑in font.
- The setup dialog uses `font_registry_get_count()` and `font_registry_get_info()` to list fonts and switch between them at runtime.

## Built‑in Fonts (compile time)

Built‑in fonts are produced and wired in by the `fonts/` sub-build. Current example font set (subject to change):

- System 8x16
- System 8x24
- VT100 10x20   (found as a png on VT100.net)
- VT220 10x20   (created from png)

I also tried to convert ttf fonts like glasstty to png or binary format but was not happy with the result. So I stayed with the fonts above recretated from png files. Both VT fonts give a very authentic VT100 terminal screen with simulated scan lines.

You can list what’s currently included by checking `fonts/bin/`.

### Source formats and tools

Two conversion paths exist in the repository:

- PNG → BIN (current path used by `fonts/Makefile`)
  - Tooling: `fonts/buildfont.py` and helper code under `fonts/src/`
  - Input PNGs live in `fonts/png/`
  - Output BINs are written to `fonts/bin/`
- BDF → BIN (legacy/alternate path)
  - Tooling: `fonts/bdf2pigfx` (C++)
  - Use this if you have BDF sources instead of PNGs

The bdf path is currently not used, as I was not able to convert a ttf font like glasstty to usable bdf fonts with fontforge.
For PNGs, the PNG must contain all 256 glyphs in a grid, using the target glyph width × height. See `fonts/README.md` for details and examples.

## Fonts Sub‑Build: What it Generates

Running `make` inside `fonts/` generates two artifacts that the main build consumes:

1. `src/binary_assets.s` (assembly)

  - Section `.rodata` with one symbol per font
  - Each symbol uses `.incbin "fonts/bin/<font>.bin"`
  - Example symbol name: `G_SYSTEM_8X16_GLYPHS`

2. `src/buildin_fonts.inc` (C include file)

  - `extern` declarations for all symbols defined in `binary_assets.s`
  - A function `void gfx_register_builtin_fonts(void)` that calls
    `font_registry_register("<Name>", W, H, G_<...>_GLYPHS, font_get_glyph_address)`
    for every built‑in font, in a stable order (the first entry acts as the default/system font)

Both files live under the repository’s `src/` directory (not under `fonts/`) so that the top‑level build can include them directly.

The generator scripts are:

- `fonts/src/gen_bin_assets.py` → writes `src/binary_assets.s`
- `fonts/src/gen_buildin_inc.py` → writes `src/buildin_fonts.inc`

Note: The `fonts/Makefile` also redirects the scripts’ console output to files under `fonts/src/` (for convenience logs). The authoritative artifacts are the ones under `src/`.

## How to Add or Change Fonts

1) Place your source PNG(s) under `fonts/png/` (one file per font, 256 glyphs, correct grid and size).
2) Build the fonts package:
   - `cd fonts`
   - `make`
   - This creates/updates `fonts/bin/*.bin` and regenerates `src/binary_assets.s` and `src/buildin_fonts.inc`.
3) Build PiGFX at the repo root (see next section).

If you use BDF sources instead, convert them with the `bdf2pigfx` tool to BINs, then place the BINs to `fonts/bin/` and re‑run `make` in `fonts/` to regenerate the two `src/` include files.

## Main Build: How Fonts Are Consumed

At the repository root, PiGFX is built with the unified Makefile:

- Pi 1 (default): `make RPI=1`
- Pi 2: `make RPI=2`
- Pi 3: `make RPI=3`
- Pi 4: `make RPI=4`

The main build will compile `src/binary_assets.s` into the firmware image and include `src/buildin_fonts.inc` via `#include` in `src/gfx.c`. No additional wiring is needed as long as those two generated files exist.

Important:

- If you changed anything under `fonts/` (added/removed fonts), you must run `make` in `fonts/` first so that the two generated files are up‑to‑date before the root build.
- If you run `make clean` in `fonts/`, regenerate the font assets before you build the root project.

## Setup Dialog Integration

- The setup dialog enumerates fonts via the registry and switches fonts immediately for preview.
- The selected font index is persisted in configuration and applied on boot.

## References

- Runtime API: `src/font_registry.h/.c`
- Asset include: `src/buildin_fonts.inc` (included by `src/gfx.c`)
- Asset definitions: `src/binary_assets.s` (symbols used by `buildin_fonts.inc`)
- Generators: `fonts/src/gen_bin_assets.py`, `fonts/src/gen_buildin_inc.py`
- PNG path and rules: `fonts/Makefile`, `fonts/README.md`
