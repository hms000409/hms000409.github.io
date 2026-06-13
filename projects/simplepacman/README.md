# Pacman GUI

Pacman game with Sokol graphics (Metal/OpenGL).

## Build & Run

```bash
make        # build
make run    # run
make clean  # clean
```

## Controls

- **Arrow Keys**: Move Pacman
- **Q / ESC**: Quit

## Code Structure

```
pacman_simple/
├── pacman.c      # Game logic + main loop
├── graphic.c/h   # Sokol renderer (tiles & sprites)
├── rom_data.c/h  # Pacman arcade ROM data (tiles, sprites, palettes)
├── sokol/        # Sokol library headers
└── Makefile
```

### Architecture

**pacman.c** - Game logic
- Map management, collision detection
- Ghost AI, power-up handling
- Calls `gfx_*` functions for rendering

**graphic.c** - Rendering
- `gfx_init()` - Initialize graphics
- `gfx_set_tile(x, y, tile, color)` - Set tilemap
- `gfx_set_sprite(idx, tile, color, x, y, flipx, flipy)` - Set sprite
- `gfx_draw()` - Render frame

**rom_data.c** - Assets
- Original Pacman tile/sprite bitmaps
- Color palettes from arcade ROM
