# OpenTyrian - the picotyrian branch

This branch is the game half of [picotyrian](https://github.com/anight/picotyrian),
which runs Tyrian on a Raspberry Pi Pico 2 W through
[picosdl](https://github.com/anight/picosdl), a subset of SDL2 for the RP2040
and RP2350. It is built from there, not from here: the firmware's CMake and the
desktop build that runs the same code against picosdl both live in picotyrian.

It starts from [georgik/OpenTyrian](https://github.com/georgik/OpenTyrian), the
ESP32 port by Gadget Workbench updated by georgik, which is itself
[OpenTyrian](https://github.com/opentyrian/opentyrian) as of early 2015. The
game logic is theirs, largely unchanged; what this branch replaces is the
platform underneath it, and the ESP-IDF project, which cannot build here, is
gone from it.

What changed, and why, is in the comments at each change. In short:

- **No filesystem.** `file.c` serves the game's files out of a table compiled
  into flash by picotyrian's asset converter: raw where the game blits or
  mixes straight from them, deflated in 4 KB blocks (`inflate.c`) where it
  parses them once.
- **Flash instead of RAM.** Sprite sheets, level tiles and sound effects are
  used where they lie; the item and enemy tables, the palettes and the text are
  compiled into flash by running the game's own loaders (`gamedata.c`) on the
  host at build time; the level maps hold tile numbers rather than pointers.
- **picosdl, not SDL3.** Video presents the game's three screens as they are,
  the palette is the panel's colour table, input reads the board's stick and
  I2C pad, and audio is a mixer on the second core.
- **DBOPL** (`dbopl.cpp`, from picopop) replaces the OPL emulator, level-matched
  to the one it replaces.
- **No allocation** in the game: the music player, the sound effects and the
  load screen that used malloc no longer do.

`data/tyrian` is Tyrian 2.1, the release the OpenTyrian project distributes as
freeware; it came with the ESP32 port.

GPL-2.0-or-later, as OpenTyrian is: see `LICENSE`.
