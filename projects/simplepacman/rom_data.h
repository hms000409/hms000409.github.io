#ifndef ROM_DATA_H
#define ROM_DATA_H

#include <stdint.h>

// ROM dumps
extern const uint8_t rom_tiles[4096];
extern const uint8_t rom_sprites[4096];
extern const uint8_t rom_hwcolors[32];
extern const uint8_t rom_palette[256];
extern const uint8_t rom_wavetable[256];

// sound-effect register dumps (recorded from Pacman arcade emulator)
extern const uint32_t snd_dump_prelude[490];
extern const uint32_t snd_dump_dead[90];

#endif // ROM_DATA_H
