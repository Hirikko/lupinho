#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "ui.h"
#include "font.h"
#include "hardware_constraints.h"

//----------------------------------------------------------------------------------
// Global vars
//----------------------------------------------------------------------------------
uint8_t frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
Color palette[PALETTE_SIZE];

//----------------------------------------------------------------------------------
// Persistent file cache
//----------------------------------------------------------------------------------
// The real hardware should not reopen and reread the same asset from the SD card
// every time spr()/tile()/map() is called. Keep loaded files in RAM and account for
// SD traffic only when an asset is actually brought into the cache.
#define LUPI_FILE_CACHE_MAX 256

typedef struct {
    char *path;
    uint8_t *data;
    size_t size;
} LupiFileCacheEntry;

static LupiFileCacheEntry file_cache[LUPI_FILE_CACHE_MAX];
static int file_cache_count = 0;

static void clear_file_cache(void) {
    for (int i = 0; i < file_cache_count; i++) {
        free(file_cache[i].path);
        free(file_cache[i].data);
        file_cache[i].path = NULL;
        file_cache[i].data = NULL;
        file_cache[i].size = 0;
    }
    file_cache_count = 0;
}

static const uint8_t *load_file_cached(const char *path, size_t *out_size, bool *out_cache_hit) {
    if (out_cache_hit) *out_cache_hit = false;

    for (int i = 0; i < file_cache_count; i++) {
        if (strcmp(file_cache[i].path, path) == 0) {
            if (out_size) *out_size = file_cache[i].size;
            if (out_cache_hit) *out_cache_hit = true;
            return file_cache[i].data;
        }
    }

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    long file_size = ftell(f);
    if (file_size < 0) {
        fclose(f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    size_t size = (size_t)file_size;
    uint8_t *data = NULL;
    if (size > 0) {
        data = (uint8_t *)malloc(size);
        if (!data) {
            fclose(f);
            return NULL;
        }

        size_t bytes_read = fread(data, 1, size, f);
        fclose(f);

        if (bytes_read != size) {
            free(data);
            return NULL;
        }
    } else {
        fclose(f);
    }

    // The cache is deliberately bounded for this diagnostic implementation.
    // If full, evict the oldest entry so newly loaded assets can remain resident.
    if (file_cache_count >= LUPI_FILE_CACHE_MAX) {
        free(file_cache[0].path);
        free(file_cache[0].data);
        memmove(&file_cache[0], &file_cache[1],
                sizeof(file_cache[0]) * (LUPI_FILE_CACHE_MAX - 1));
        file_cache_count--;
    }

    char *path_copy = (char *)malloc(strlen(path) + 1);
    if (!path_copy) {
        free(data);
        return NULL;
    }

    strcpy(path_copy, path);

    file_cache[file_cache_count].path = path_copy;
    file_cache[file_cache_count].data = data;
    file_cache[file_cache_count].size = size;
    file_cache_count++;

    hardware_constraints_add_sd_transaction();
    hardware_constraints_add_sd_bytes((uint32_t)size);

    if (out_size) *out_size = size;
    return data;
}

//----------------------------------------------------------------------------------
// Fill pattern
//----------------------------------------------------------------------------------
uint8_t fill_pattern[8] = {0, 0, 0, 0, 0, 0, 0, 0};

//----------------------------------------------------------------------------------
// Camera
//----------------------------------------------------------------------------------
int camera_x = 0;
int camera_y = 0;

void set_camera(int x, int y) { camera_x = x; camera_y = y; }
void reset_camera(void)       { camera_x = 0; camera_y = 0; }

//----------------------------------------------------------------------------------
// Clip region
//----------------------------------------------------------------------------------
int  clip_x = 0;
int  clip_y = 0;
int  clip_w = 0;
int  clip_h = 0;
bool clip_enabled = false;

void set_clip(int x, int y, int w, int h) {
    clip_x = x; clip_y = y; clip_w = w; clip_h = h;
    clip_enabled = true;
}
void reset_clip(void) { clip_enabled = false; }

//----------------------------------------------------------------------------------
// Frame buffer pixel write — enforces bounds and clip region.
// All drawing functions must use this instead of writing frame_buffer directly.
//----------------------------------------------------------------------------------
bool should_draw_pixel_with_pattern(int x, int y) {
    bool is_solid = true;
    for (int i = 0; i < 8; i++) {
        if (fill_pattern[i] != 0) {
            is_solid = false;
            break;
        }
    }
    if (is_solid) return true;

    int row = y & 7;
    int col = x & 7;

    return (fill_pattern[row] & (1 << (7 - col))) != 0;
}

void fb_set(int x, int y, int color, bool check_pattern) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT || (check_pattern && !should_draw_pixel_with_pattern(x, y))) return;
    if (clip_enabled) {
        if (x < clip_x || x >= clip_x + clip_w ||
            y < clip_y || y >= clip_y + clip_h) return;
    }
    frame_buffer[y][x] = (uint8_t)color;
}

//----------------------------------------------------------------------------------
// Line Functions
//----------------------------------------------------------------------------------
void draw_line(LineItem *line) {
    int x1 = line->x1 - camera_x;
    int y1 = line->y1 - camera_y;
    int x2 = line->x2 - camera_x;
    int y2 = line->y2 - camera_y;

    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while(1) {
        fb_set(x, y, line->color_index, true);

        if(x == x2 && y == y2) break;

        int e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x += sx; }
        if(e2 < dx)  { err += dx; y += sy; }
    }
}

//----------------------------------------------------------------------------------
// Rect Functions
//----------------------------------------------------------------------------------
void draw_rect(RectItem *rect) {
    int rx = rect->x - camera_x;
    int ry = rect->y - camera_y;
    int rw = rect->width;
    int rh = rect->height;

    if(rect->filled) {
        for (int py = ry; py < ry + rh; py++) {
            for (int px = rx; px < rx + rw; px++) {
                fb_set(px, py, rect->color_index, true);
            }
        }
    } else {
        // Top line
        for (int px = rx; px < rx + rw; px++) fb_set(px, ry, rect->color_index, true);
        // Bottom line
        for (int px = rx; px < rx + rw; px++) fb_set(px, ry + rh - 1, rect->color_index, true);
        // Left line
        for (int py = ry; py < ry + rh; py++) fb_set(rx, py, rect->color_index, true);
        // Right line
        for (int py = ry; py < ry + rh; py++) fb_set(rx + rw - 1, py, rect->color_index, true);
    }
}

//----------------------------------------------------------------------------------
// Circle Functions
//----------------------------------------------------------------------------------
void draw_circle_pixels(int cx, int cy, int x, int y, int color_index) {
    fb_set(cx + x, cy + y, color_index, true);
    fb_set(cx - x, cy + y, color_index, true);
    fb_set(cx + x, cy - y, color_index, true);
    fb_set(cx - x, cy - y, color_index, true);
    fb_set(cx + y, cy + x, color_index, true);
    fb_set(cx - y, cy + x, color_index, true);
    fb_set(cx + y, cy - x, color_index, true);
    fb_set(cx - y, cy - x, color_index, true);
}

void draw_circle(CircleItem *circle) {
    int cx = circle->center_x - camera_x;
    int cy = circle->center_y - camera_y;

    if(circle->filled) {
        int radius_squared = circle->radius * circle->radius;
        for (int py = cy - circle->radius; py <= cy + circle->radius; py++) {
            for (int px = cx - circle->radius; px <= cx + circle->radius; px++) {
                int dx = px - cx;
                int dy = py - cy;
                if (dx * dx + dy * dy <= radius_squared) {
                    fb_set(px, py, circle->color_index, true);
                }
            }
        }
    }

    if(circle->has_border) {
        int x = 0;
        int y = circle->radius;
        int d = 1 - circle->radius;

        draw_circle_pixels(cx, cy, x, y, circle->border_color_index);

        while (x < y) {
            if (d < 0) {
                d = d + 2 * x + 3;
            } else {
                d = d + 2 * (x - y) + 5;
                y--;
            }
            x++;
            draw_circle_pixels(cx, cy, x, y, circle->border_color_index);
        }
    }
}

//----------------------------------------------------------------------------------
// Triangle Functions
//----------------------------------------------------------------------------------
void draw_horizontal_line_fb(int x1, int x2, int y, int color_index) {
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    for (int x = x1; x <= x2; x++) fb_set(x, y, color_index, true);
}

void draw_triangle(TriangleItem *triangle) {
    // Sort vertices by Y coordinate (v1.y <= v2.y <= v3.y)
    int x1 = triangle->p1_x - camera_x, y1 = triangle->p1_y - camera_y;
    int x2 = triangle->p2_x - camera_x, y2 = triangle->p2_y - camera_y;
    int x3 = triangle->p3_x - camera_x, y3 = triangle->p3_y - camera_y;

    // Bubble sort by Y
    if (y1 > y2) {
        int tmp = y1; y1 = y2; y2 = tmp;
        tmp = x1; x1 = x2; x2 = tmp;
    }
    if (y2 > y3) {
        int tmp = y2; y2 = y3; y3 = tmp;
        tmp = x2; x2 = x3; x3 = tmp;
    }
    if (y1 > y2) {
        int tmp = y1; y1 = y2; y2 = tmp;
        tmp = x1; x1 = x2; x2 = tmp;
    }

    // Degenerate triangle (all points on same horizontal line)
    if (y1 == y3) {
        int minX = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        int maxX = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        draw_horizontal_line_fb(minX, maxX, y1, triangle->color_index);
        return;
    }

    // Draw triangle using scanline algorithm
    for (int y = y1; y <= y3; y++) {
        bool secondHalf = y > y2 || y2 == y1;
        int segmentHeight = secondHalf ? y3 - y2 : y2 - y1;

        if (segmentHeight == 0) continue;

        float alpha = (float)(y - y1) / (float)(y3 - y1);
        float beta = secondHalf ? (float)(y - y2) / (float)(y3 - y2) : (float)(y - y1) / (float)(y2 - y1);

        int xa = x1 + (int)((x3 - x1) * alpha);
        int xb = secondHalf ? x2 + (int)((x3 - x2) * beta) : x1 + (int)((x2 - x1) * beta);

        draw_horizontal_line_fb(xa, xb, y, triangle->color_index);
    }
}

//----------------------------------------------------------------------------------
// Palette Functions
//----------------------------------------------------------------------------------
void palset(int position, int bgr555) {
    if (position < 0 || position >= PALETTE_SIZE) return;

    // Extract BGR555 components (5 bits each)
    int r5 = (bgr555 >> 0) & 0x1F;
    int g5 = (bgr555 >> 5) & 0x1F;
    int b5 = (bgr555 >> 10) & 0x1F;

    // Scale from 5-bit (0-31) to 8-bit (0-255)
    palette[position].r = (r5 << 3) | (r5 >> 2);
    palette[position].g = (g5 << 3) | (g5 >> 2);
    palette[position].b = (b5 << 3) | (b5 >> 2);
    palette[position].a = 255;
}

Color get_palette_color(int index) {
    if (index == 0) {
        return (Color) {0, 0, 0, 0}; // transparent
    }

    return palette[index];
}

//----------------------------------------------------------------------------------
// Clear Functions
//----------------------------------------------------------------------------------
void draw_clear(ClearItem *clear) {
    // Fill entire frame buffer with the color index
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        memset(frame_buffer[y], clear->color_index, SCREEN_WIDTH);
    }
}

//----------------------------------------------------------------------------------
// Print (bitmap font) Functions
//----------------------------------------------------------------------------------
void draw_print(const char *text, int x, int y, int color_index) {
    int cursor_x = x - camera_x;
    int base_y   = y - camera_y;

    for (int i = 0; text[i] != '\0'; i++) {
        uint8_t c = (uint8_t)text[i];

        if (c < 32 || c > 126) continue;

        const uint8_t *glyph = font_data[c - 32];

        for (int col = 0; col < FONT_CHAR_WIDTH; col++) {
            for (int row = 0; row < FONT_CHAR_HEIGHT; row++) {
                if (glyph[col] & (1 << row))
                    fb_set(cursor_x + col, base_y + row, color_index, false);
            }
        }

        cursor_x += FONT_CHAR_ADVANCE;
    }
}

//----------------------------------------------------------------------------------
// Frame Buffer Functions
//----------------------------------------------------------------------------------
Texture scene;

Image generate_image_from_frame_buffer() {
    Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
    Color *pixels = (Color *)image.data;

    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        for(int j = 0; j < SCREEN_WIDTH; j++) {
            int pixel_index = j + (SCREEN_WIDTH * i);
            pixels[pixel_index] = get_palette_color(frame_buffer[i][j]);
        }
    }

    return image;
}

void draw_frame_buffer() {
    Image image = generate_image_from_frame_buffer();

    // Keep one persistent 480x270 texture instead of creating and destroying a
    // GPU texture every frame. Reusing the texture avoids the repeated
    // LoadTexture/UnloadTexture messages in the browser console and matches
    // the intended framebuffer -> VRAM update model more closely.
    if (scene.id == 0) {
        scene = LoadTextureFromImage(image);
    } else {
        UpdateTexture(scene, image.data);
    }

    Rectangle source = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    Rectangle dest = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    Vector2 origin = { 0, 0 };

    DrawTexturePro(scene, source, dest, origin, 0, WHITE);
    UnloadImage(image);
}

void clear_frame_buffer() {
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        memset(frame_buffer[i], 0, SCREEN_WIDTH);
    }
}

//----------------------------------------------------------------------------------
// Tile Functions
//----------------------------------------------------------------------------------
void draw_tile(const char *path, int width, int height, int tile_index, int x, int y, bool flipped) {
    if (!hardware_constraints_try_drawlist(0, HARDWARE_DRAWLIST_COMMAND_TILE)) return;

    x -= camera_x;
    y -= camera_y;

    int tile_size = width * height;
    long data_offset = (long)tile_index * tile_size;

    size_t file_size = 0;
    bool cache_hit = false;
    const uint8_t *file_data = load_file_cached(path, &file_size, &cache_hit);
    (void)cache_hit;
    if (!file_data) {
        printf("Warning: Could not load tile file %s\n", path);
        return;
    }

    if (data_offset < 0 || (size_t)data_offset + (size_t)tile_size > file_size) {
        printf("Warning: Tile %d is outside file %s\n", tile_index, path);
        return;
    }

    const uint8_t *data = file_data + data_offset;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t idx = data[row * width + col];
            if (idx == 0) continue;
            int px = flipped ? (x + width - 1 - col) : (x + col);
            int py = y + row;
            fb_set(px, py, idx, false);
        }
    }
}

//----------------------------------------------------------------------------------
// Sprite Functions
//----------------------------------------------------------------------------------
void draw_spr(const char *path, int width, int height, int x, int y, bool flipped) {
    if (!hardware_constraints_try_drawlist(0, HARDWARE_DRAWLIST_COMMAND_SPR)) return;

    x -= camera_x;
    y -= camera_y;

    size_t file_size = 0;
    bool cache_hit = false;
    const uint8_t *data = load_file_cached(path, &file_size, &cache_hit);
    if (!data) {
        printf("Warning: Could not load sprite file %s\n", path);
        return;
    }

    int data_size = width * height;
    if (data_size < 0 || (size_t)data_size > file_size) {
        printf("Warning: Sprite %s is smaller than %dx%d\n", path, width, height);
        return;
    }

    // A sprite load now means an asset entering RAM, not every spr() call.
    if (!cache_hit) {
        hardware_constraints_add_sprite_load();
    }

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t idx = data[row * width + col];
            if (idx == 0) continue;
            int px = flipped ? (x + width - 1 - col) : (x + col);
            int py = y + row;
            fb_set(px, py, idx, false);
        }
    }
}

//----------------------------------------------------------------------------------
// Fill Pattern Functions
//----------------------------------------------------------------------------------
void set_fillp(uint8_t *bytes, int n) {
    for (int i = 0; i < n && i < 8; i++) fill_pattern[i] = bytes[i];
    for (int i = n; i < 8; i++)          fill_pattern[i] = 0;
}

//----------------------------------------------------------------------------------
// UI Reset
//----------------------------------------------------------------------------------
void reset_ui_state(void) {
    clear_file_cache();
    reset_camera();
    reset_clip();
    clear_frame_buffer();
    memset(palette, 0, sizeof(palette));
    memset(fill_pattern, 0, sizeof(fill_pattern));
}

//----------------------------------------------------------------------------------
// Map Functions
//----------------------------------------------------------------------------------
void draw_map_layer(MapLayerData *data, int map_width, int tile_size, int cam_x, int cam_y) {
    size_t file_size = 0;
    bool cache_hit = false;
    const uint8_t *file_buf = load_file_cached(data->path, &file_size, &cache_hit);
    (void)cache_hit;
    if (!file_buf) {
        printf("Warning: Could not load tileset file %s\n", data->path);
        return;
    }

    int tile_pixels = data->tile_w * data->tile_h;

    for (int map_idx = 0; map_idx < data->count; map_idx++) {
        if (data->tile_ids[map_idx] < 0) continue;

        int tile_id_flags = data->tile_ids[map_idx];
        bool flip_x = (tile_id_flags & 1024) != 0;
        bool flip_y = (tile_id_flags & 2048) != 0;
        int tile_id = tile_id_flags & ~(1024 | 2048);

        long offset = (long)tile_id * tile_pixels;
        if (offset < 0 || offset + tile_pixels > file_size) continue;

        int col = map_idx % map_width;
        int row = map_idx / map_width;
        int sx  = col * tile_size + cam_x - camera_x;
        int sy  = row * tile_size + cam_y - camera_y;

        if (!hardware_constraints_try_drawlist(0, HARDWARE_DRAWLIST_COMMAND_TILE)) continue;

        const uint8_t *tile_data = file_buf + offset;
        for (int ty = 0; ty < data->tile_h; ty++) {
            for (int tx = 0; tx < data->tile_w; tx++) {
                uint8_t idx = tile_data[ty * data->tile_w + tx];
                if (idx == 0) continue;
                int px = flip_x ? (sx + data->tile_w - 1 - tx) : (sx + tx);
                int py = flip_y ? (sy + data->tile_h - 1 - ty) : (sy + ty);
                fb_set(px, py, idx, false);
            }
        }
    }

}
