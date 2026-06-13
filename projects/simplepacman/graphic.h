#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <stdint.h>
#include <stdbool.h>

// Constants matching pacman_gui.c
#define MAP_HEIGHT 14
#define MAP_WIDTH 20
#define NUM_GHOSTS 2
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 16

// Tile and sprite constants matching src/types.h
enum {
    TILE_SPACE          = 0x40,
    TILE_DOT            = 0x10,
    TILE_PILL           = 0x14,
    TILE_GHOST          = 0xB0,
    TILE_LIFE           = 0x20,

    SPRITETILE_INVISIBLE = 30,
    SPRITETILE_PACMAN_CLOSED_MOUTH = 48,

    COLOR_BLANK         = 0x00,
    COLOR_DEFAULT       = 0x0F,
    COLOR_DOT           = 0x10,
    COLOR_PACMAN        = 0x09,
    COLOR_BLINKY        = 0x01,
    COLOR_PINKY         = 0x03,
    COLOR_INKY          = 0x05,
    COLOR_CLYDE         = 0x07,
    COLOR_FRIGHTENED    = 0x11,
    COLOR_WHITE_BORDER  = 0x1F,
    COLOR_EYES          = 0x19,
};

// Graphics initialization and shutdown
void gfx_init(void);
void gfx_shutdown(void);

// Update game state for rendering
void gfx_clear_tilemap(void);
void gfx_set_tile(int x, int y, uint8_t tile_code, uint8_t color_code);

// Sprite management
void gfx_clear_sprites(void);
void gfx_set_sprite(int index, bool enabled, int pixel_x, int pixel_y,
                    uint8_t tile, uint8_t color, bool flipx, bool flipy);

// Render the frame
void gfx_draw(void);

// High-level rendering functions
typedef struct {
    int x, y;
} gfx_pos_t;

typedef struct {
    gfx_pos_t pos;
    int scared;
    uint8_t color;
} gfx_ghost_t;

void gfx_render_text(int x, int y, const char* text, uint8_t color);
void gfx_render_number(int x, int y, int number, int digits, uint8_t color);
void gfx_render_ui(int score, int highscore, int lives, int powered, int level);
void gfx_render_map(const char* map_data, int map_width, int map_height, int ui_offset);
void gfx_render_player(int x, int y, int direction, int ui_offset, int death_frame);
void gfx_render_ghosts(const gfx_ghost_t* ghosts, int num_ghosts, int ui_offset);

#endif // GRAPHIC_H
