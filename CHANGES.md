# Change Log

All notable changes to this project will be documented in this file.

## 2.0.1 - 2025-10-12

- Minor documentation cleanups and removal of dead code comments after sprite removal.

## 2.0.0 - 2025-10-12

- Permanently removed sprite system (drawing, movement, collision) from firmware.
- Removed `disableCollision` configuration option.
- Removed sprite-related ESC commands and collision reporting from terminal codes.
- Deleted `sprite/` host-side tools and samples.
- Ensured bitmap drawing via ESC `[#...d]` still works (direct blit implementation).

## 1.1.x

- Mode switching, font selection, and other enhancements carried from PiGFX base.
- Setup dialog and file-based configuration added.
- PWM bell with configurable sound level.
- Font build pipeline and registry implemented.
