// graphic.c

#include "graphic.h"
#include "rom_data.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <assert.h>
#include <string.h>

// Display size adapted for pacman_simple (20x18 tiles: 14 game + 4 UI with top margin)
#define DISPLAY_TILES_X 20
#define DISPLAY_TILES_Y 18
#define DISPLAY_PIXELS_X (DISPLAY_TILES_X * TILE_WIDTH)
#define DISPLAY_PIXELS_Y (DISPLAY_TILES_Y * TILE_HEIGHT)

#define TILE_TEXTURE_WIDTH (256 * TILE_WIDTH)
#define TILE_TEXTURE_HEIGHT (TILE_HEIGHT + SPRITE_HEIGHT)
#define MAX_VERTICES (((DISPLAY_TILES_X * DISPLAY_TILES_Y) + 8) * 6)  // tiles + sprites
#define NUM_SPRITES 8

// Vertex structure
typedef struct {
    float x, y;         // screen coords [0..1]
    float u, v;         // tile texture coords
    uint32_t attr;      // x: color code, y: opacity
} vertex_t;

// Sprite structure
typedef struct {
    bool enabled;
    uint8_t tile, color;
    bool flipx, flipy;
    int pixel_x, pixel_y;  // pixel position
} sprite_t;

// Graphics state
static struct {
    // Tile and color RAM
    uint8_t video_ram[DISPLAY_TILES_Y][DISPLAY_TILES_X];
    uint8_t color_ram[DISPLAY_TILES_Y][DISPLAY_TILES_X];

    // Sprites
    sprite_t sprites[NUM_SPRITES];

    // Vertex buffer
    int num_vertices;
    vertex_t vertices[MAX_VERTICES];

    // Tile decoding scratch buffer
    uint8_t tile_pixels[TILE_TEXTURE_HEIGHT][TILE_TEXTURE_WIDTH];

    // Color palette
    uint32_t color_palette[256];

    // Sokol resources
    struct {
        sg_image img;
        sg_view tex_view;
    } tilerom;

    struct {
        sg_image img;
        sg_view tex_view;
    } palette;

    struct {
        sg_image img;
        sg_view tex_view;
    } render;

    sg_sampler linear_smp;
    sg_sampler nearest_smp;

    struct {
        sg_pass pass;
        sg_buffer vbuf;
        sg_pipeline pip;
    } offscreen;

    struct {
        sg_pass_action pass_action;
        sg_buffer quad_vbuf;
        sg_pipeline pip;
    } display;
} gfx;

/*=== TILE DECODING (from src/graphics.c) ===*/

static inline void decode_tile_8x4(
    uint32_t tex_x,
    uint32_t tex_y,
    const uint8_t* tile_base,
    uint32_t tile_stride,
    uint32_t tile_offset,
    uint8_t tile_code)
{
    // ROM tiles are always 8x8, decode to 8x4 chunk
    for (uint32_t tx = 0; tx < 8; tx++) {
        uint32_t ti = tile_code * tile_stride + tile_offset + (7 - tx);
        for (uint32_t ty = 0; ty < 4; ty++) {
            uint8_t p_hi = (tile_base[ti] >> (7 - ty)) & 1;
            uint8_t p_lo = (tile_base[ti] >> (3 - ty)) & 1;
            uint8_t p = (p_hi << 1) | p_lo;
            gfx.tile_pixels[tex_y + ty][tex_x + tx] = p;
        }
    }
}

static inline void decode_tile(uint8_t tile_code) {
    // Decode 8x8 ROM tile to temporary position
    uint32_t x_base = tile_code * TILE_WIDTH;
    uint32_t y0 = 0;
    uint32_t y1 = 4;

    // Decode to 8x8 first
    decode_tile_8x4(x_base, y0, rom_tiles, 16, 8, tile_code);
    decode_tile_8x4(x_base, y1, rom_tiles, 16, 0, tile_code);

    // Now upscale from 8x8 to 16x16 (2x2 pixel replication)
    // Read backwards and write to avoid overwriting source data
    for (int sy = 7; sy >= 0; sy--) {
        for (int sx = 7; sx >= 0; sx--) {
            uint8_t pixel = gfx.tile_pixels[y0 + sy][x_base + sx];
            // Write 2x2 block
            gfx.tile_pixels[y0 + sy*2 + 0][x_base + sx*2 + 0] = pixel;
            gfx.tile_pixels[y0 + sy*2 + 0][x_base + sx*2 + 1] = pixel;
            gfx.tile_pixels[y0 + sy*2 + 1][x_base + sx*2 + 0] = pixel;
            gfx.tile_pixels[y0 + sy*2 + 1][x_base + sx*2 + 1] = pixel;
        }
    }
}

static inline void decode_sprite(uint8_t sprite_code) {
    // Sprites are 16x16, composed of two 8-wide columns
    uint32_t x0 = sprite_code * SPRITE_WIDTH;
    uint32_t x1 = x0 + 8;  // Hardcoded 8 since sprite is made of 8x8 chunks
    uint32_t y0 = TILE_HEIGHT;
    uint32_t y1 = y0 + 4;
    uint32_t y2 = y1 + 4;
    uint32_t y3 = y2 + 4;
    decode_tile_8x4(x0, y0, rom_sprites, 64, 40, sprite_code);
    decode_tile_8x4(x1, y0, rom_sprites, 64,  8, sprite_code);
    decode_tile_8x4(x0, y1, rom_sprites, 64, 48, sprite_code);
    decode_tile_8x4(x1, y1, rom_sprites, 64, 16, sprite_code);
    decode_tile_8x4(x0, y2, rom_sprites, 64, 56, sprite_code);
    decode_tile_8x4(x1, y2, rom_sprites, 64, 24, sprite_code);
    decode_tile_8x4(x0, y3, rom_sprites, 64, 32, sprite_code);
    decode_tile_8x4(x1, y3, rom_sprites, 64,  0, sprite_code);
}

static void gfx_decode_tiles(void) {
    for (uint32_t tile_code = 0; tile_code < 256; tile_code++) {
        decode_tile(tile_code);
    }
    for (uint32_t sprite_code = 0; sprite_code < 64; sprite_code++) {
        decode_sprite(sprite_code);
    }
    // Special opaque 16x16 block for fade effect
    for (uint32_t y = TILE_HEIGHT; y < TILE_TEXTURE_HEIGHT; y++) {
        for (uint32_t x = 64*SPRITE_WIDTH; x < 65*SPRITE_WIDTH; x++) {
            gfx.tile_pixels[y][x] = 1;
        }
    }
}

static void gfx_decode_color_palette(void) {
    uint32_t hw_colors[32];
    for (int i = 0; i < 32; i++) {
        uint8_t rgb = rom_hwcolors[i];
        uint8_t r = ((rgb>>0)&1) * 0x21 + ((rgb>>1)&1) * 0x47 + ((rgb>>2)&1) * 0x97;
        uint8_t g = ((rgb>>3)&1) * 0x21 + ((rgb>>4)&1) * 0x47 + ((rgb>>5)&1) * 0x97;
        uint8_t b = ((rgb>>6)&1) * 0x47 + ((rgb>>7)&1) * 0x97;
        hw_colors[i] = 0xFF000000 | (b<<16) | (g<<8) | r;
    }
    for (int i = 0; i < 256; i++) {
        gfx.color_palette[i] = hw_colors[rom_palette[i] & 0xF];
        // First color in each color block is transparent
        if ((i & 3) == 0) {
            gfx.color_palette[i] &= 0x00FFFFFF;
        }
    }
}

/*=== SHADER SOURCES ===*/

static const char* get_offscreen_vs(void) {
    switch (sg_query_backend()) {
        case SG_BACKEND_METAL_MACOS:
            return
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "struct vs_in {\n"
                "  float4 pos [[attribute(0)]];\n"
                "  float2 uv [[attribute(1)]];\n"
                "  float4 data [[attribute(2)]];\n"
                "};\n"
                "struct vs_out {\n"
                "  float4 pos [[position]];\n"
                "  float2 uv;\n"
                "  float4 data;\n"
                "};\n"
                "vertex vs_out _main(vs_in in [[stage_in]]) {\n"
                "  vs_out out;\n"
                "  out.pos = float4((in.pos.xy - 0.5) * float2(2.0, -2.0), 0.5, 1.0);\n"
                "  out.uv  = in.uv;"
                "  out.data = in.data;\n"
                "  return out;\n"
                "}\n";
        case SG_BACKEND_GLCORE:
            return
                "#version 410\n"
                "layout(location=0) in vec4 pos;\n"
                "layout(location=1) in vec2 uv_in;\n"
                "layout(location=2) in vec4 data_in;\n"
                "out vec2 uv;\n"
                "out vec4 data;\n"
                "void main() {\n"
                "  gl_Position = vec4((pos.xy - 0.5) * vec2(2.0, -2.0), 0.5, 1.0);\n"
                "  uv  = uv_in;"
                "  data = data_in;\n"
                "}\n";
        default:
            return "";
    }
}

static const char* get_offscreen_fs(void) {
    switch (sg_query_backend()) {
        case SG_BACKEND_METAL_MACOS:
            return
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "struct ps_in {\n"
                "  float2 uv;\n"
                "  float4 data;\n"
                "};\n"
                "fragment float4 _main(ps_in in [[stage_in]],\n"
                "                      texture2d<float> tile_tex [[texture(0)]],\n"
                "                      texture2d<float> pal_tex [[texture(1)]],\n"
                "                      sampler tile_smp [[sampler(0)]],\n"
                "                      sampler pal_smp [[sampler(1)]])\n"
                "{\n"
                "  float color_code = in.data.x;\n"
                "  float tile_color = tile_tex.sample(tile_smp, in.uv).x;\n"
                "  float2 pal_uv = float2(color_code * 4 + tile_color, 0);\n"
                "  float4 color = pal_tex.sample(pal_smp, pal_uv) * float4(1, 1, 1, in.data.y);\n"
                "  return color;\n"
                "}\n";
        case SG_BACKEND_GLCORE:
            return
                "#version 410\n"
                "uniform sampler2D tile_tex;\n"
                "uniform sampler2D pal_tex;\n"
                "in vec2 uv;\n"
                "in vec4 data;\n"
                "out vec4 frag_color;\n"
                "void main() {\n"
                "  float color_code = data.x;\n"
                "  float tile_color = texture(tile_tex, uv).x;\n"
                "  vec2 pal_uv = vec2(color_code * 4 + tile_color, 0);\n"
                "  frag_color = texture(pal_tex, pal_uv) * vec4(1, 1, 1, data.y);\n"
                "}\n";
        default:
            return "";
    }
}

static const char* get_display_vs(void) {
    switch (sg_query_backend()) {
        case SG_BACKEND_METAL_MACOS:
            return
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "struct vs_in {\n"
                "  float4 pos [[attribute(0)]];\n"
                "};\n"
                "struct vs_out {\n"
                "  float4 pos [[position]];\n"
                "  float2 uv;\n"
                "};\n"
                "vertex vs_out _main(vs_in in[[stage_in]]) {\n"
                "  vs_out out;\n"
                "  out.pos = float4((in.pos.xy - 0.5) * float2(2.0, -2.0), 0.0, 1.0);\n"
                "  out.uv = in.pos.xy;\n"
                "  return out;\n"
                "}\n";
        case SG_BACKEND_GLCORE:
            return
                "#version 410\n"
                "layout(location=0) in vec4 pos;\n"
                "out vec2 uv;\n"
                "void main() {\n"
                "  gl_Position = vec4((pos.xy - 0.5) * 2.0, 0.0, 1.0);\n"
                "  uv = pos.xy;\n"
                "}\n";
        default:
            return "";
    }
}

static const char* get_display_fs(void) {
    switch (sg_query_backend()) {
        case SG_BACKEND_METAL_MACOS:
            return
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "struct ps_in {\n"
                "  float2 uv;\n"
                "};\n"
                "fragment float4 _main(ps_in in [[stage_in]],\n"
                "                      texture2d<float> tex [[texture(0)]],\n"
                "                      sampler smp [[sampler(0)]])\n"
                "{\n"
                "  return tex.sample(smp, in.uv);\n"
                "}\n";
        case SG_BACKEND_GLCORE:
            return
                "#version 410\n"
                "uniform sampler2D tex;\n"
                "in vec2 uv;\n"
                "out vec4 frag_color;\n"
                "void main() {\n"
                "  frag_color = texture(tex, uv);\n"
                "}\n";
        default:
            return "";
    }
}

/*=== SOKOL RESOURCE CREATION ===*/

static void gfx_create_resources(void) {
    // Pass action for clearing background to black
    gfx.offscreen.pass.action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
        },
    };
    gfx.display.pass_action = gfx.offscreen.pass.action;

    // Dynamic vertex buffer for tiles and sprites
    gfx.offscreen.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage = {
            .vertex_buffer = true,
            .stream_update = true,
        },
        .size = sizeof(gfx.vertices),
    });

    // Quad vertex buffer for display
    float quad_verts[]= { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    gfx.display.quad_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(quad_verts)
    });

    // Offscreen pipeline
    gfx.offscreen.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(&(sg_shader_desc){
           .attrs = {
                [0] = { .glsl_name="pos" },
                [1] = { .glsl_name="uv_in" },
                [2] = { .glsl_name="data_in" },
            },
            .vertex_func.source = get_offscreen_vs(),
            .fragment_func.source = get_offscreen_fs(),
            .views = {
                [0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_texture_n = 0 },
                [1].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_texture_n = 1 },
            },
            .samplers = {
                [0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_sampler_n = 0 },
                [1] = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_sampler_n = 1 },
            },
            .texture_sampler_pairs = {
                [0] = {
                    .stage = SG_SHADERSTAGE_FRAGMENT,
                    .view_slot = 0,
                    .sampler_slot = 0,
                    .glsl_name = "tile_tex"
                },
                [1] = {
                    .stage = SG_SHADERSTAGE_FRAGMENT,
                    .view_slot = 1,
                    .sampler_slot = 1,
                    .glsl_name = "pal_tex"
                },
            },
        }),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2,
                [2].format = SG_VERTEXFORMAT_UBYTE4N,
            }
        },
        .depth.pixel_format = SG_PIXELFORMAT_NONE,
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            }
        }
    });

    // Display pipeline
    gfx.display.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(&(sg_shader_desc){
            .attrs[0] = { .glsl_name="pos" },
            .vertex_func.source = get_display_vs(),
            .fragment_func.source = get_display_fs(),
            .views[0].texture = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_texture_n = 0 },
            .samplers[0] = { .stage = SG_SHADERSTAGE_FRAGMENT, .msl_sampler_n = 0 },
            .texture_sampler_pairs[0] = {
                .stage = SG_SHADERSTAGE_FRAGMENT,
                .view_slot = 0,
                .sampler_slot = 0,
                .glsl_name = "tex"
            },
        }),
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });

    // Render target image
    gfx.render.img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = DISPLAY_PIXELS_X * 2,
        .height = DISPLAY_PIXELS_Y * 2,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    });
    gfx.render.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = gfx.render.img,
    });
    gfx.offscreen.pass.attachments.colors[0] = sg_make_view(&(sg_view_desc){
        .color_attachment.image = gfx.render.img,
    });

    // Tile ROM texture
    gfx.tilerom.img = sg_make_image(&(sg_image_desc){
        .width  = TILE_TEXTURE_WIDTH,
        .height = TILE_TEXTURE_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_R8,
        .data.mip_levels[0] = SG_RANGE(gfx.tile_pixels)
    });
    gfx.tilerom.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = gfx.tilerom.img,
    });

    // Palette texture
    gfx.palette.img = sg_make_image(&(sg_image_desc){
        .width = 256,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = SG_RANGE(gfx.color_palette)
    });
    gfx.palette.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = gfx.palette.img,
    });

    // Samplers
    gfx.linear_smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });

    gfx.nearest_smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });
}

/*=== PUBLIC API ===*/

void gfx_init(void) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 2,
        .image_pool_size = 3,
        .shader_pool_size = 2,
        .pipeline_pool_size = 2,
        .view_pool_size = 8,
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    memset(&gfx, 0, sizeof(gfx));
    gfx_clear_sprites();
    gfx_decode_tiles();
    gfx_decode_color_palette();
    gfx_create_resources();
}

void gfx_shutdown(void) {
    sg_shutdown();
}

void gfx_clear_tilemap(void) {
    memset(gfx.video_ram, TILE_SPACE, sizeof(gfx.video_ram));
    memset(gfx.color_ram, COLOR_BLANK, sizeof(gfx.color_ram));
}

void gfx_set_tile(int x, int y, uint8_t tile_code, uint8_t color_code) {
    if (x >= 0 && x < DISPLAY_TILES_X && y >= 0 && y < DISPLAY_TILES_Y) {
        gfx.video_ram[y][x] = tile_code;
        gfx.color_ram[y][x] = color_code;
    }
}

void gfx_clear_sprites(void) {
    memset(gfx.sprites, 0, sizeof(gfx.sprites));
}

void gfx_set_sprite(int index, bool enabled, int pixel_x, int pixel_y,
                    uint8_t tile, uint8_t color, bool flipx, bool flipy) {
    if (index >= 0 && index < NUM_SPRITES) {
        gfx.sprites[index].enabled = enabled;
        gfx.sprites[index].pixel_x = pixel_x;
        gfx.sprites[index].pixel_y = pixel_y;
        gfx.sprites[index].tile = tile;
        gfx.sprites[index].color = color;
        gfx.sprites[index].flipx = flipx;
        gfx.sprites[index].flipy = flipy;
    }
}

/*=== RENDERING ===*/

static void gfx_add_vertex(float x, float y, float u, float v, uint8_t color_code, uint8_t opacity) {
    assert(gfx.num_vertices < MAX_VERTICES);
    vertex_t* vtx = &gfx.vertices[gfx.num_vertices++];
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
    vtx->attr = (opacity<<8) | color_code;
}

static void gfx_add_tile_vertices(uint32_t tx, uint32_t ty, uint8_t tile_code, uint8_t color_code) {
    const float dx = 1.0f / DISPLAY_TILES_X;
    const float dy = 1.0f / DISPLAY_TILES_Y;
    const float du = (float)TILE_WIDTH / TILE_TEXTURE_WIDTH;
    const float dv = (float)TILE_HEIGHT / TILE_TEXTURE_HEIGHT;

    const float x0 = tx * dx;
    const float x1 = x0 + dx;
    const float y0 = ty * dy;
    const float y1 = y0 + dy;
    const float u0 = tile_code * du;
    const float u1 = u0 + du;
    const float v0 = 0.0f;
    const float v1 = dv;

    gfx_add_vertex(x0, y0, u0, v0, color_code, 0xFF);
    gfx_add_vertex(x1, y0, u1, v0, color_code, 0xFF);
    gfx_add_vertex(x1, y1, u1, v1, color_code, 0xFF);
    gfx_add_vertex(x0, y0, u0, v0, color_code, 0xFF);
    gfx_add_vertex(x1, y1, u1, v1, color_code, 0xFF);
    gfx_add_vertex(x0, y1, u0, v1, color_code, 0xFF);
}

static void gfx_add_playfield_vertices(void) {
    for (uint32_t ty = 0; ty < DISPLAY_TILES_Y; ty++) {
        for (uint32_t tx = 0; tx < DISPLAY_TILES_X; tx++) {
            const uint8_t tile_code = gfx.video_ram[ty][tx];
            const uint8_t color_code = gfx.color_ram[ty][tx] & 0x1F;
            gfx_add_tile_vertices(tx, ty, tile_code, color_code);
        }
    }
}

static void gfx_add_sprite_vertices(void) {
    const float dx = 1.0f / DISPLAY_PIXELS_X;
    const float dy = 1.0f / DISPLAY_PIXELS_Y;
    const float du = (float)SPRITE_WIDTH / TILE_TEXTURE_WIDTH;
    const float dv = (float)SPRITE_HEIGHT / TILE_TEXTURE_HEIGHT;
    const float sprite_scale = 1.5f;  // Scale factor for sprites

    for (int i = 0; i < NUM_SPRITES; i++) {
        const sprite_t* spr = &gfx.sprites[i];
        if (spr->enabled) {
            // Calculate center of sprite and scale from center
            float center_x = (spr->pixel_x + SPRITE_WIDTH / 2.0f) * dx;
            float center_y = (spr->pixel_y + SPRITE_HEIGHT / 2.0f) * dy;
            float half_w = dx * SPRITE_WIDTH * sprite_scale / 2.0f;
            float half_h = dy * SPRITE_HEIGHT * sprite_scale / 2.0f;

            float x0, x1, y0, y1;
            if (spr->flipx) {
                x1 = center_x - half_w;
                x0 = center_x + half_w;
            } else {
                x0 = center_x - half_w;
                x1 = center_x + half_w;
            }
            if (spr->flipy) {
                y1 = center_y - half_h;
                y0 = center_y + half_h;
            } else {
                y0 = center_y - half_h;
                y1 = center_y + half_h;
            }
            const float u0 = spr->tile * du;
            const float u1 = u0 + du;
            const float v0 = ((float)TILE_HEIGHT / TILE_TEXTURE_HEIGHT);
            const float v1 = v0 + dv;
            const uint8_t color = spr->color;

            gfx_add_vertex(x0, y0, u0, v0, color, 0xFF);
            gfx_add_vertex(x1, y0, u1, v0, color, 0xFF);
            gfx_add_vertex(x1, y1, u1, v1, color, 0xFF);
            gfx_add_vertex(x0, y0, u0, v0, color, 0xFF);
            gfx_add_vertex(x1, y1, u1, v1, color, 0xFF);
            gfx_add_vertex(x0, y1, u0, v1, color, 0xFF);
        }
    }
}

void gfx_draw(void) {
    // Update vertex buffer
    gfx.num_vertices = 0;
    gfx_add_playfield_vertices();
    gfx_add_sprite_vertices();
    assert(gfx.num_vertices <= MAX_VERTICES);
    sg_update_buffer(gfx.offscreen.vbuf, &(sg_range){
        .ptr=gfx.vertices,
        .size=gfx.num_vertices * sizeof(vertex_t)
    });

    // Render tiles and sprites into offscreen render target
    sg_begin_pass(&gfx.offscreen.pass);
    sg_apply_pipeline(gfx.offscreen.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = gfx.offscreen.vbuf,
        .views = {
            [0] = gfx.tilerom.tex_view,
            [1] = gfx.palette.tex_view,
        },
        .samplers = {
            [0] = gfx.nearest_smp,
            [1] = gfx.nearest_smp,
        },
    });
    sg_draw(0, gfx.num_vertices, 1);
    sg_end_pass();

    // Upscale-render offscreen to display
    sg_begin_pass(&(sg_pass){
        .action = gfx.display.pass_action,
        .swapchain = sglue_swapchain(),
    });
    sg_apply_pipeline(gfx.display.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = gfx.display.quad_vbuf,
        .views[0] = gfx.render.tex_view,
        .samplers[0] = gfx.linear_smp,
    });
    sg_draw(0, 4, 1);
    sg_end_pass();
    sg_commit();
}

/*=== HIGH-LEVEL RENDERING ===*/

#include <stdio.h>

// Helper: Convert char to tile code (ASCII works directly for 0-9, A-Z)
static uint8_t char_to_tile(char c) {
    if (c == ' ') return TILE_SPACE;
    return (uint8_t)c;
}

void gfx_render_text(int x, int y, const char* text, uint8_t color) {
    for (int i = 0; text[i] != '\0' && x + i < DISPLAY_TILES_X; i++) {
        gfx_set_tile(x + i, y, char_to_tile(text[i]), color);
    }
}

void gfx_render_number(int x, int y, int number, int digits, uint8_t color) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%0*d", digits, number);
    gfx_render_text(x, y, buffer, color);
}

void gfx_render_ui(int score, int highscore, int lives, int powered, int level) {
    // Row 0: Empty (top margin)
    // Row 1: Title and Level
    gfx_render_text(DISPLAY_TILES_X/2 - 3, 1, "PACMAN", COLOR_PACMAN);
    gfx_render_text(1, 1, "LV", COLOR_DEFAULT);
    gfx_render_number(3, 1, level, 1, COLOR_DEFAULT);

    // Row 2: Score and Lives
    gfx_render_number(1, 2, score, 4, COLOR_DEFAULT);

    gfx_render_text(6, 2, "HI", COLOR_DEFAULT);
    int display_hi = (score > highscore) ? score : highscore;
    gfx_render_number(9, 2, display_hi, 4, COLOR_DEFAULT);

    // Lives icons - big pill
    for (int i = 0; i < lives && i < 3; i++) {
        gfx_set_tile(14 + i * 2, 2, TILE_PILL, 0x0F);
    }

    // Power mode indicator
    if (powered > 0) {
        gfx_render_text(DISPLAY_TILES_X/2 - 5, 3, "POWER MODE", COLOR_PACMAN);
    }
}

// Helper: Get wall tile based on surrounding walls
static uint8_t get_wall_tile(const char* map_data, int map_width, int map_height, int x, int y) {
    #define IS_WALL(dx, dy) ((x+(dx)) >= 0 && (x+(dx)) < map_width && \
                             (y+(dy)) >= 0 && (y+(dy)) < map_height && \
                             map_data[(y+(dy)) * map_width + (x+(dx))] == '#')

    // Check 8 directions
    bool up    = IS_WALL(0, -1);
    bool down  = IS_WALL(0, 1);
    bool left  = IS_WALL(-1, 0);
    bool right = IS_WALL(1, 0);
    bool ul = IS_WALL(-1, -1);
    bool ur = IS_WALL(1, -1);
    bool dl = IS_WALL(-1, 1);
    bool dr = IS_WALL(1, 1);

    // Border walls (special handling for outer edges)
    if (x == 0 && y == 0) return 0xD1;  // Top-left corner
    if (x == map_width-1 && y == 0) return 0xD0;  // Top-right corner
    if (x == 0 && y == map_height-1) return 0xD5;  // Bottom-left corner
    if (x == map_width-1 && y == map_height-1) return 0xD4;  // Bottom-right corner
    if (y == 0) return 0xDB;  // Top edge
    if (y == map_height-1) return 0xDC;  // Bottom edge
    if (x == 0) return 0xD3;  // Left edge
    if (x == map_width-1) return 0xD2;  // Right edge

    // Interior walls - check for corners (rounded)
    // Inner corners (rounded corners pointing inward)
    if (!up && !left && right && down && !ul) return 0xC8;  // Top-left inner corner
    if (!up && !right && left && down && !ur) return 0xC9;  // Top-right inner corner
    if (!down && !left && right && up && !dl) return 0xCA;  // Bottom-left inner corner
    if (!down && !right && left && up && !dr) return 0xCB;  // Bottom-right inner corner

    // Outer corners (rounded corners pointing outward)
    if (up && left && !right && !down && !dr) return 0xC4;  // Bottom-right outer corner
    if (up && right && !left && !down && !dl) return 0xC5;  // Bottom-left outer corner
    if (down && left && !right && !up && !ur) return 0xC6;  // Top-right outer corner
    if (down && right && !left && !up && !ul) return 0xC7;  // Top-left outer corner

    // Straight lines
    if (up && down && !left && !right) return 0xD2;  // Vertical line
    if (left && right && !up && !down) return 0xDB;  // Horizontal line

    // T-junctions
    if (up && down && right && !left) return 0xD3;  // Left T
    if (up && down && left && !right) return 0xD2;  // Right T
    if (left && right && down && !up) return 0xDB;  // Top T
    if (left && right && up && !down) return 0xDC;  // Bottom T

    // Default: blob tile for filled areas
    return 0xDF;

    #undef IS_WALL
}

void gfx_render_map(const char* map_data, int map_width, int map_height, int ui_offset) {
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            char cell = map_data[y * map_width + x];
            uint8_t tile = TILE_SPACE;
            uint8_t color = COLOR_BLANK;

            if (cell == '#') {  // WALL
                tile = get_wall_tile(map_data, map_width, map_height, x, y);
                color = COLOR_DOT;
            } else if (cell == '.') {  // DOT
                tile = TILE_DOT;
                color = COLOR_DOT;
            } else if (cell == 'O') {  // POWERUP
                tile = TILE_PILL;
                color = COLOR_DOT;
            }

            gfx_set_tile(x, y + ui_offset, tile, color);
        }
    }
}

void gfx_render_player(int x, int y, int direction, int ui_offset, int death_frame) {
    static int anim_counter = 0;
    anim_counter++;

    uint8_t tile;
    bool flipx = false, flipy = false;

    if (death_frame > 0) {
        tile = 52 + death_frame - 1;
    } else {
        // Animation frame (0-3), changes every 8 frames (for 60fps)
        int phase = (anim_counter / 8) % 4;

        // Sprite sequences for horizontal and vertical movement
        static const uint8_t tiles[2][4] = {
            { 44, 46, 48, 46 },  // Horizontal (RIGHT/LEFT)
            { 45, 47, 48, 47 }   // Vertical (DOWN/UP)
        };

        // Select sprite based on direction
        switch (direction) {
            case 0:  // Right
                tile = tiles[0][phase];
                break;
            case 1:  // Left
                tile = tiles[0][phase];
                flipx = true;
                break;
            case 2:  // Up
                tile = tiles[1][phase];
                flipy = true;
                break;
            case 3:  // Down
                tile = tiles[1][phase];
                break;
            default:
                tile = 48;
                break;
        }
    }

    gfx_set_sprite(0, true,
                   x * TILE_WIDTH,
                   (y + ui_offset) * TILE_HEIGHT,
                   tile,
                   COLOR_PACMAN,
                   flipx,
                   flipy);
}

void gfx_render_ghosts(const gfx_ghost_t* ghosts, int num_ghosts, int ui_offset) {
    for (int i = 0; i < num_ghosts; i++) {
        uint8_t sprite_tile = (ghosts[i].scared == 3) ? 32 : 8;  // 3 = eyes state
        gfx_set_sprite(1 + i, true,
                       ghosts[i].pos.x * TILE_WIDTH,
                       (ghosts[i].pos.y + ui_offset) * TILE_HEIGHT,
                       sprite_tile,
                       ghosts[i].color,
                       false, false);
    }
}
