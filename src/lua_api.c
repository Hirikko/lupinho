#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lua_api.h"
#include "ui.h"
#include "raylib.h"
#include "hardware_constraints.h"

lua_State *globalLuaState = NULL;

static char current_game_dir[512] = "";
static lua_Number frame_counter = 0;

//----------------------------------------------------------------------------------
// UI Functions
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// ui.draw_line(x1:number, y1:number, x2:number, y2:number, color:number)
//----------------------------------------------------------------------------------
int lua_draw_line(lua_State *L) {
    int x1 = (int)luaL_checknumber(L, 1);
    int y1 = (int)luaL_checknumber(L, 2);
    int x2 = (int)luaL_checknumber(L, 3);
    int y2 = (int)luaL_checknumber(L, 4);
    int color = (int)luaL_checknumber(L, 5);

    LineItem line = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, .color_index = color };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_RECT)) {
        draw_line(&line);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.draw_rect(x:number, y:number, width:number, height:number, filled:bool, color:number)
//----------------------------------------------------------------------------------
int lua_draw_rect(lua_State *L) {
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int width = (int)luaL_checknumber(L, 3);
    int height = (int)luaL_checknumber(L, 4);
    bool filled = lua_toboolean(L, 5);
    int color = (int)luaL_checknumber(L, 6);

    RectItem rect = { .x = x, .y = y, .width = width, .height = height, .filled = filled, .color_index = color };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_RECT)) {
        draw_rect(&rect);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.rect(x1, y1, x2, y2, color)
//----------------------------------------------------------------------------------
int lua_rect(lua_State *L) {
    int x1 = (int)luaL_checknumber(L, 1);
    int y1 = (int)luaL_checknumber(L, 2);
    int x2 = (int)luaL_checknumber(L, 3);
    int y2 = (int)luaL_checknumber(L, 4);
    int color = (int)luaL_checknumber(L, 5);

    RectItem rect = { .x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1, .filled = false, .color_index = color };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_RECT)) {
        draw_rect(&rect);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.rectfill(x1, y1, x2, y2, color)
//----------------------------------------------------------------------------------
int lua_rectfill(lua_State *L) {
    int x1 = (int)luaL_checknumber(L, 1);
    int y1 = (int)luaL_checknumber(L, 2);
    int x2 = (int)luaL_checknumber(L, 3);
    int y2 = (int)luaL_checknumber(L, 4);
    int color = (int)luaL_checknumber(L, 5);

    RectItem rect = { .x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1, .filled = true, .color_index = color };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_RECT)) {
        draw_rect(&rect);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.draw_circle(center_x:number, center_y:number, radius:number, filled:bool, color:number, border:bool, border_color:number)
//----------------------------------------------------------------------------------
int lua_draw_circle(lua_State *L) {
    int center_x = (int)luaL_checknumber(L, 1);
    int center_y = (int)luaL_checknumber(L, 2);
    int radius = (int)luaL_checknumber(L, 3);
    bool filled = lua_toboolean(L, 4);
    int color = (int)luaL_checknumber(L, 5);
    bool border = lua_toboolean(L, 6);
    int border_color = (int)luaL_checknumber(L, 7);

    CircleItem circle = {
        .center_x = center_x, .center_y = center_y, .radius = radius,
        .filled = filled, .color_index = color,
        .has_border = border, .border_color_index = border_color,
    };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_CIRCLE)) {
        draw_circle(&circle);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.circfill(x, y, radius, color)
//----------------------------------------------------------------------------------
int lua_circfill(lua_State *L) {
    int center_x = (int)luaL_checknumber(L, 1);
    int center_y = (int)luaL_checknumber(L, 2);
    int radius = (int)luaL_checknumber(L, 3);
    int color = (int)luaL_checknumber(L, 4);

    CircleItem circle = {
        .center_x = center_x, .center_y = center_y, .radius = radius,
        .filled = true, .color_index = color,
        .has_border = true, .border_color_index = color,
    };
    if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_CIRCLE)) {
        draw_circle(&circle);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.trisfill(p1_x:int, p1_y:int, p2_x:int, p2_y:int, p3_x:int, p3_y:int, color:int)
//----------------------------------------------------------------------------------
int lua_trisfill(lua_State *L) {
    int p1_x = (int)luaL_checknumber(L, 1);
    int p1_y = (int)luaL_checknumber(L, 2);
    int p2_x = (int)luaL_checknumber(L, 3);
    int p2_y = (int)luaL_checknumber(L, 4);
    int p3_x = (int)luaL_checknumber(L, 5);
    int p3_y = (int)luaL_checknumber(L, 6);
    int color = (int)luaL_checknumber(L, 7);

    TriangleItem triangle = {
        .p1_x = p1_x, .p1_y = p1_y,
        .p2_x = p2_x, .p2_y = p2_y,
        .p3_x = p3_x, .p3_y = p3_y,
        .color_index = color,
    };
    if (hardware_constraints_try_drawlist(20, HARDWARE_DRAWLIST_COMMAND_TRIANGLE)) {
        draw_triangle(&triangle);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.palset(position:number, color:number)
//----------------------------------------------------------------------------------
int lua_palset(lua_State *L) {
    int position = (int)luaL_checknumber(L, 1);
    int color = (int)luaL_checknumber(L, 2);

    palset(position, color);

    return 0;
}

//----------------------------------------------------------------------------------
// ui.tile(spritesheet:table, tile_index:number, x:number, y:number)
// tile_index can have bit 10 (1024) set to flip horizontally
//----------------------------------------------------------------------------------
int lua_tile(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "path");
    const char *path = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "width");
    int width = (int)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "height");
    int height = (int)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    int tile_index_with_flags = (int)luaL_checknumber(L, 2);
    int x = (int)luaL_checknumber(L, 3);
    int y = (int)luaL_checknumber(L, 4);

    bool flipped = (tile_index_with_flags & 1024) != 0;
    int tile_index = tile_index_with_flags & ~1024;

    draw_tile(path, width, height, tile_index, x, y, flipped);
    return 0;
}

//----------------------------------------------------------------------------------
// ui.spr(spritesheet:table, x:number, y:number, flipped:bool = false)
//----------------------------------------------------------------------------------
int lua_spr(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "path");
    const char *path = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "width");
    int width = (int)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "height");
    int height = (int)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);

    bool flipped = false;
    if (lua_gettop(L) > 3) {
        flipped = lua_toboolean(L, 4);
    }

    draw_spr(path, width, height, x, y, flipped);
    return 0;
}

//----------------------------------------------------------------------------------
// Helper function to get the keyboard key corresponding to a gamepad button
//----------------------------------------------------------------------------------
static int get_keyboard_key_for_button(int button) {
    switch (button) {
        // D-pad / AWSD keys
        case GAMEPAD_BUTTON_LEFT_FACE_UP:    return KEY_W;
        case GAMEPAD_BUTTON_LEFT_FACE_DOWN:  return KEY_S;
        case GAMEPAD_BUTTON_LEFT_FACE_LEFT:  return KEY_A;
        case GAMEPAD_BUTTON_LEFT_FACE_RIGHT: return KEY_D;
        // Action buttons
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return KEY_J;
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:  return KEY_K;
        case GAMEPAD_BUTTON_RIGHT_FACE_UP:    return KEY_L;
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:  return KEY_M;
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1:   return KEY_G;
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:  return KEY_H;
        default: return -1;
    }
}

//----------------------------------------------------------------------------------
// ui.btn(button:number, pad:number) -> bool
// Checks both gamepad and keyboard input
//----------------------------------------------------------------------------------
int lua_btn(lua_State *L) {
    int button = (int)luaL_checknumber(L, 1);
    int pad = (int)luaL_optnumber(L, 2, 0);

    bool is_down = IsGamepadButtonDown(pad, button);

    if (!is_down) {
        int key = get_keyboard_key_for_button(button);
        if (key != -1) {
            is_down = IsKeyDown(key);
        }
    }

    lua_pushboolean(L, is_down);

    return 1;
}

//----------------------------------------------------------------------------------
// ui.btnp(button:number, pad:number) -> bool
// Checks both gamepad and keyboard input (pressed this frame)
//----------------------------------------------------------------------------------
int lua_btnp(lua_State *L) {
    int button = (int)luaL_checknumber(L, 1);
    int pad = (int)luaL_optnumber(L, 2, 0);

    bool is_pressed = IsGamepadButtonPressed(pad, button);

    if (!is_pressed) {
        int key = get_keyboard_key_for_button(button);
        if (key != -1) {
            is_pressed = IsKeyPressed(key);
        }
    }

    lua_pushboolean(L, is_pressed);

    return 1;
}

//----------------------------------------------------------------------------------
// ui.cls(color:int)
//----------------------------------------------------------------------------------
int lua_cls(lua_State *L) {
    int color = (int)luaL_checknumber(L, 1);
    ClearItem clear = { .color_index = color };
    if (hardware_constraints_try_drawlist(16, HARDWARE_DRAWLIST_COMMAND_CLS)) {
        draw_clear(&clear);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.log(message:string)
//----------------------------------------------------------------------------------
int lua_log(lua_State *L) {
    char *message = luaL_checkstring(L, 1);
    printf("[LUPINHO] %s\n", message);
    return 0;
}

//----------------------------------------------------------------------------------
// ui.fillp([pattern_bytes...])
//----------------------------------------------------------------------------------
int lua_fillp(lua_State *L) {
    int nargs = lua_gettop(L);

    uint8_t bytes[8] = {0};
    for (int i = 0; i < nargs && i < 8; i++) {
        bytes[i] = (uint8_t)(int)luaL_checknumber(L, i + 1);
    }

    uint32_t cost = (nargs == 0) ? 4u : 12u;
    if (hardware_constraints_try_drawlist(cost, HARDWARE_DRAWLIST_COMMAND_FILLP)) {
        set_fillp(bytes, nargs);
    }
    return 0;
}

//----------------------------------------------------------------------------------
// Helper: Recursively search for a tileset by name in the Sprites table
// Returns 1 if found (table is on top of stack), 0 otherwise
//----------------------------------------------------------------------------------
static int find_tileset_in_sprites(lua_State *L, int sprites_idx, const char *tileset_name) {
    lua_pushnil(L);
    while (lua_next(L, sprites_idx) != 0) {
        if (lua_istable(L, -1)) {
            // Check if this table has a "path" field (it's a tileset)
            lua_getfield(L, -1, "path");
            bool has_path = lua_isstring(L, -1);
            lua_pop(L, 1);

            if (has_path) {
                // This is a tileset entry, check the key name
                lua_pushvalue(L, -2);  // Push the key
                const char *key = lua_tostring(L, -1);
                lua_pop(L, 1);

                if (key && strcmp(key, tileset_name) == 0) {
                    // Found it! Move to top and return success
                    lua_insert(L, -2);  // Move found table below the key
                    lua_pop(L, 1);      // Remove the key
                    return 1;
                }
            } else {
                // This is a subdirectory, recurse into it
                if (find_tileset_in_sprites(L, lua_gettop(L), tileset_name)) {
                    lua_insert(L, -3);  // Move result below current key/value
                    lua_pop(L, 2);      // Pop key and subdirectory table
                    return 1;
                }
            }
        }
        lua_pop(L, 1);  // Pop value, keep key for next iteration
    }
    return 0;
}

//----------------------------------------------------------------------------------
// ui.map(map_data, cam_x, cam_y)
// map_data is a layer table, e.g. M.map.BG1:
//   lupi_metadata = { width, height, tile_size }
//   ["scene_a"]   = { [1]=tile_id, [2]=tile_id, ... }  (sparse, 1-indexed)
// The tileset key (e.g. "scene_a") is looked up dynamically anywhere in the
// Sprites table hierarchy to get the sprite path and dimensions.
// cam_x/cam_y: screen-space offset where the map is drawn.
//----------------------------------------------------------------------------------
int lua_map(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int arg_cam_x = (int)luaL_optnumber(L, 2, 0);
    int arg_cam_y = (int)luaL_optnumber(L, 3, 0);

    // --- find metadata ---
    int meta_idx = 0;
    lua_getfield(L, 1, "metadata");
    if (lua_istable(L, -1)) {
        meta_idx = lua_absindex(L, -1);
    } else {
        lua_pop(L, 1);
        lua_getfield(L, 1, "lupi_metadata");
        if (lua_istable(L, -1)) {
            meta_idx = lua_absindex(L, -1);
        } else {
            lua_pop(L, 1);
            printf("MAP - NOT FOUND!\n");
            return 0;
        }
    }

    lua_getfield(L, meta_idx, "width");
    int map_width = (int)lua_tonumber(L, -1); lua_pop(L, 1);

    lua_getfield(L, meta_idx, "height");
    int map_height = (int)lua_tonumber(L, -1); lua_pop(L, 1);

    lua_getfield(L, meta_idx, "tile_size");
    int tile_size = (int)lua_tonumber(L, -1); lua_pop(L, 1);

    lua_pop(L, 1); // pop metadata table

    if (map_width <= 0 || map_height <= 0 || tile_size <= 0) return 0;

    // --- get Sprites table ---
    lua_getglobal(L, "Sprites");
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return 0; }
    int sprites_idx = lua_absindex(L, -1);

    int tile_count = map_width * map_height;

    // --- iterate tileset keys in map_data ---
    int map_data_idx = 1;
    lua_pushnil(L);
    while (lua_next(L, map_data_idx) != 0) {
        int key_idx   = lua_absindex(L, -2);
        int value_idx = lua_absindex(L, -1);

        if (lua_type(L, key_idx) != LUA_TSTRING || !lua_istable(L, value_idx)) {
            lua_pop(L, 1); // pop value, keep key
            continue;
        }

        const char *layer_key = lua_tostring(L, key_idx);
        if (strcmp(layer_key, "metadata") == 0 ||
            strcmp(layer_key, "lupi_metadata") == 0 ||
            strcmp(layer_key, "tilesets") == 0) {
            lua_pop(L, 1); // pop value, keep key
            continue;
        }

        // resolve tileset name via tilesets indirection
        const char *tileset_name = layer_key;
        lua_getfield(L, map_data_idx, "tilesets");
        if (lua_istable(L, -1)) {
            int tilesets_idx = lua_absindex(L, -1);
            lua_getfield(L, tilesets_idx, layer_key);
            if (lua_isstring(L, -1)) {
                tileset_name = lua_tostring(L, -1);
            }
            lua_pop(L, 1); // pop resolved name
        }
        lua_pop(L, 1); // pop tilesets table

        // find tileset in Sprites
        if (!find_tileset_in_sprites(L, sprites_idx, tileset_name)) {
            lua_pop(L, 1); // pop value, keep key
            continue;
        }
        int tileset_tbl_idx = lua_absindex(L, -1);

        lua_getfield(L, tileset_tbl_idx, "path");
        const char *path = lua_tostring(L, -1); lua_pop(L, 1);

        lua_getfield(L, tileset_tbl_idx, "width");
        int tile_w = (int)lua_tonumber(L, -1); lua_pop(L, 1);

        lua_getfield(L, tileset_tbl_idx, "height");
        int tile_h = (int)lua_tonumber(L, -1); lua_pop(L, 1);

        lua_pop(L, 1); // pop tileset table

        if (!path || tile_w <= 0 || tile_h <= 0) {
            lua_pop(L, 1); // pop value, keep key
            continue;
        }

        // build tile_ids array from the Lua layer table
        int *tile_ids = (int *)malloc(tile_count * sizeof(int));
        if (!tile_ids) { lua_pop(L, 1); continue; }

        for (int i = 0; i < tile_count; i++) tile_ids[i] = -1;

        lua_pushnil(L);
        while (lua_next(L, value_idx) != 0) {
            if (lua_type(L, -2) == LUA_TNUMBER) {
                int idx = (int)lua_tonumber(L, -2) - 1;
                if (idx >= 0 && idx < tile_count) {
                    tile_ids[idx] = lua_isnil(L, -1) ? -1 : (int)lua_tonumber(L, -1);
                }
            }
            lua_pop(L, 1);
        }

        MapLayerData data = {
            .path     = path,
            .tile_w   = tile_w,
            .tile_h   = tile_h,
            .tile_ids = tile_ids,
            .count    = tile_count,
        };

        draw_map_layer(&data, map_width, tile_size, arg_cam_x, arg_cam_y);
        free(tile_ids);

        lua_pop(L, 1); // pop value, keep key
    }

    lua_pop(L, 1); // pop Sprites table
    return 0;
}

//----------------------------------------------------------------------------------
// ui.camera(x, y) — offset all drawing; ui.camera() resets to (0, 0)
//----------------------------------------------------------------------------------
int lua_camera(lua_State *L) {
    if (lua_gettop(L) == 0) {
        reset_camera();
    } else {
        int x = (int)luaL_checknumber(L, 1);
        int y = (int)luaL_checknumber(L, 2);
        set_camera(x, y);
    }
    return 0;
}

//----------------------------------------------------------------------------------
// ui.clip(x, y, w, h) — restrict drawing to region; ui.clip() resets
//----------------------------------------------------------------------------------
int lua_clip(lua_State *L) {
    if (lua_gettop(L) == 0) {
        if (hardware_constraints_try_drawlist(4, HARDWARE_DRAWLIST_COMMAND_CLIP)) {
            reset_clip();
        }
    } else {
        int x = (int)luaL_checknumber(L, 1);
        int y = (int)luaL_checknumber(L, 2);
        int w = (int)luaL_checknumber(L, 3);
        int h = (int)luaL_checknumber(L, 4);
        if (hardware_constraints_try_drawlist(12, HARDWARE_DRAWLIST_COMMAND_CLIP)) {
            set_clip(x, y, w, h);
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------
// ui.preload_spritesheet(spritesheet)
//----------------------------------------------------------------------------------
int lua_preload_spritesheet(lua_State *L) {
    int spritesheet = (int)luaL_checknumber(L, 1);

    return 0;
}

//----------------------------------------------------------------------------------
// ui.draw_sprite(x, y, sprite_index, size)
//----------------------------------------------------------------------------------
int lua_draw_sprite(lua_State *L) {
    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int sprite_index = (int)luaL_checknumber(L, 3);
    int size = (int)luaL_checknumber(L, 4);

    return 0;
}

//----------------------------------------------------------------------------------
// ui.print(text, x, y, color)
//----------------------------------------------------------------------------------
int lua_print(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int x = (int)luaL_checknumber(L, 2);
    int y = (int)luaL_checknumber(L, 3);
    int color = (int)luaL_checknumber(L, 4);

    size_t text_length = strlen(text);
    uint32_t cost = (uint32_t)(16 + (text_length + 3) / 4);
    if (hardware_constraints_try_drawlist(cost, HARDWARE_DRAWLIST_COMMAND_PRINT)) {
        draw_print(text, x, y, color);
    }

    return 0;
}

//----------------------------------------------------------------------------------
// ui.set_pallet(start_index, count, palette_table)
//----------------------------------------------------------------------------------
int lua_set_pallet(lua_State *L) {
    int start_index = (int)luaL_checknumber(L, 1);
    int count = (int)luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);  // Check that argument 3 is a table

    return 0;
}

//----------------------------------------------------------------------------------
// ui.mid(x, y, z) -> number
// Returns the middle value of three numbers (useful for clamping)
//----------------------------------------------------------------------------------
int lua_mid(lua_State *L) {
    lua_Number x = luaL_checknumber(L, 1);
    lua_Number y = luaL_checknumber(L, 2);
    lua_Number z = luaL_checknumber(L, 3);

    lua_Number result;
    if ((x <= y && y <= z) || (z <= y && y <= x)) {
        result = y;
    } else if ((y <= x && x <= z) || (z <= x && x <= y)) {
        result = x;
    } else {
        result = z;
    }

    lua_pushnumber(L, result);
    return 1;
}

//----------------------------------------------------------------------------------
// Global Lua State Functions
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Parse lupi_manifest.txt and inject a nested `Sprites` Lua global.
// Each bitmap entry becomes: Sprites[namespace][key] = { path, width, height, ntiles }
//----------------------------------------------------------------------------------
void inject_sprites_global(lua_State *L, const char *manifest_path, const char *game_dir) {
    FILE *f = fopen(manifest_path, "r");
    if (!f) {
        printf("Warning: Could not open manifest %s\n", manifest_path);
        return;
    }

    lua_newtable(L); // Sprites table

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        long id;
        int size;
        char path[256];
        char json[256];

        if (sscanf(line, "%ld %d %255s %255[^\n]", &id, &size, path, json) < 4)
            continue;

        if (!strstr(json, "\"type\":\"bitmap\""))
            continue;

        int width = 0, height = 0, ntiles = 1;
        char *p;

        p = strstr(json, "\"width\":");
        if (p) sscanf(p + 8, "%d", &width);

        p = strstr(json, "\"height\":");
        if (p) sscanf(p + 9, "%d", &height);

        p = strstr(json, "\"tiles\":");
        if (p) sscanf(p + 8, "%d", &ntiles);

        // Create nested table structure from path (e.g., "img/sprites/star/star_a")
        // First, duplicate path since strtok modifies it
        char path_copy[256];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        // Build full path for the file system
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", game_dir, path);

        // Navigate/create nested tables for each path component except the last
        char *token = strtok(path_copy, "/");
        int depth = 0;

        while (token != NULL) {
            char *next_token = strtok(NULL, "/");

            if (next_token == NULL) {
                // Last token - create the entry table
                lua_newtable(L);
                lua_pushstring(L, full_path);
                lua_setfield(L, -2, "path");
                lua_pushinteger(L, width);
                lua_setfield(L, -2, "width");
                lua_pushinteger(L, height);
                lua_setfield(L, -2, "height");
                lua_pushinteger(L, ntiles);
                lua_setfield(L, -2, "ntiles");
                lua_setfield(L, -2, token);
            } else {
                // Intermediate token - get or create sub-table
                lua_getfield(L, -1, token);
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 1);
                    lua_newtable(L);
                    lua_pushvalue(L, -1);
                    lua_setfield(L, -3, token);
                }
                depth++;
            }

            token = next_token;
        }

        // Pop all nested tables to get back to Sprites
        for (int i = 0; i < depth; i++) {
            lua_pop(L, 1);
        }
    }

    fclose(f);
    lua_setglobal(L, "Sprites");
}

//----------------------------------------------------------------------------------
// Native Lupi compatibility helpers
//
// These replace the temporary Lua files:
//   - lupinho-compat.lua  -> Sprites.find
//   - palt-hex-compat.lua -> Palette.rgb / Palette.hex
//
// The behavior follows the supplied Lua implementations. The helpers live in
// the Lupinho runtime so games no longer need to load these compatibility files.
//----------------------------------------------------------------------------------

static int lua_sprites_find_recursive(lua_State *L, int table_index, const char *name, int visited_index) {
    table_index = lua_absindex(L, table_index);
    visited_index = lua_absindex(L, visited_index);

    if (!lua_istable(L, table_index)) {
        return 0;
    }

    // Avoid revisiting the same table (Sprites may contain shared tables/cycles).
    lua_pushvalue(L, table_index);
    lua_rawget(L, visited_index);
    if (lua_toboolean(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    lua_pop(L, 1);

    lua_pushvalue(L, table_index);
    lua_pushboolean(L, 1);
    lua_rawset(L, visited_index);

    // First, match the requested name at this level.
    lua_pushnil(L);
    while (lua_next(L, table_index) != 0) {
        // Stack: ... key, value
        if (lua_isstring(L, -2) &&
            strcmp(lua_tostring(L, -2), name) == 0 &&
            lua_istable(L, -1)) {
            // The value is already the top stack item. Returning 1 returns it.
            return 1;
        }
        lua_pop(L, 1); // value; keep key for lua_next
    }

    // Then recurse into child tables.
    lua_pushnil(L);
    while (lua_next(L, table_index) != 0) {
        // Stack: ... key, value
        if (lua_istable(L, -1)) {
            if (lua_sprites_find_recursive(L, -1, name, visited_index)) {
                // Recursive call leaves the found table as the top stack item.
                // Any iteration bookkeeping below it is discarded automatically
                // when this C function returns its single result.
                return 1;
            }
        }
        lua_pop(L, 1); // value; keep key for lua_next
    }

    return 0;
}

static int lua_sprites_find(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    lua_getglobal(L, "Sprites");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "Sprites.find: sprite não encontrado: %s", path);
    }

    // Direct path traversal, matching the supplied Lua implementation:
    // for part in string.gmatch(path, "[^/]+") do current = current[part] end
    int current_index = lua_gettop(L);
    const char *cursor = path;
    char part[256];
    bool direct_failed = false;

    while (*cursor != '\0') {
        size_t len = 0;
        while (cursor[len] != '\0' && cursor[len] != '/') {
            len++;
        }

        if (len == 0 || len >= sizeof(part)) {
            direct_failed = true;
            break;
        }

        memcpy(part, cursor, len);
        part[len] = '\0';

        if (!lua_istable(L, current_index)) {
            direct_failed = true;
            break;
        }

        lua_getfield(L, current_index, part);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            direct_failed = true;
            break;
        }

        // Replace the current table with the next table without leaving
        // temporary values on the stack.
        lua_replace(L, current_index);

        cursor += len;
        if (*cursor == '/') {
            cursor++;
        }
    }

    if (!direct_failed && lua_istable(L, current_index)) {
        // The sprite table is the only result returned by this function.
        return 1;
    }

    lua_pop(L, 1); // failed direct result (Sprites)

    // Fallback: search by the final path component, exactly like the supplied
    // Lua compatibility function.
    const char *last_slash = strrchr(path, '/');
    const char *name = last_slash ? last_slash + 1 : path;

    if (*name != '\0') {
        lua_getglobal(L, "Sprites");
        if (lua_istable(L, -1)) {
            int sprites_index = lua_gettop(L);
            lua_newtable(L);
            int visited_index = lua_gettop(L);

            if (lua_sprites_find_recursive(L, sprites_index, name, visited_index)) {
                // The found sprite table is the top stack item.
                return 1;
            }

            lua_pop(L, 1); // visited
        }
        lua_pop(L, 1); // Sprites
    }

    return luaL_error(L, "Sprites.find: sprite não encontrado: %s", path);
}

static void palette_reset_caches(lua_State *L);

static int lua_palette_rgb(lua_State *L) {
    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);

    uint32_t key = ((uint32_t)(r & 0xFF) << 16) |
                   ((uint32_t)(g & 0xFF) << 8) |
                   (uint32_t)(b & 0xFF);

    // Private cache table in the Lua registry.
    lua_pushlightuserdata(L, (void *)&lua_palette_rgb);
    lua_rawget(L, LUA_REGISTRYINDEX);
    if (lua_istable(L, -1)) {
        lua_pushinteger(L, (lua_Integer)key);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            return 1;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    int best_idx = 1;
    int64_t min_dist = INT64_MAX;

    lua_getglobal(L, "Palette");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushinteger(L, 0);
        return 1;
    }

    size_t palette_len = lua_rawlen(L, -1);
    for (size_t i = 1; i <= palette_len; i++) {
        lua_rawgeti(L, -1, (lua_Integer)i);
        if (lua_isnumber(L, -1)) {
            uint32_t value = (uint32_t)lua_tointeger(L, -1);

            // Exact conversion from the supplied palt-hex-compat.lua.
            int b5 = (int)((value / 1024U) % 32U);
            int g5 = (int)((value / 32U) % 32U);
            int r5 = (int)(value % 32U);

            int cr = r5 * 8 + r5 / 4;
            int cg = g5 * 8 + g5 / 4;
            int cb = b5 * 8 + b5 / 4;

            int dr = cr - r;
            int dg = cg - g;
            int db = cb - b;
            int64_t dist = (int64_t)dr * dr + (int64_t)dg * dg + (int64_t)db * db;

            if (dist < min_dist) {
                min_dist = dist;
                best_idx = (int)i;
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1); // Palette

    lua_pushinteger(L, (lua_Integer)(best_idx - 1));
    lua_pushlightuserdata(L, (void *)&lua_palette_rgb);
    lua_rawget(L, LUA_REGISTRYINDEX);
    if (lua_istable(L, -1)) {
        lua_pushinteger(L, (lua_Integer)key);
        lua_pushvalue(L, -3); // result
        lua_rawset(L, -3);
    }
    lua_pop(L, 1); // cache table

    return 1;
}

static int lua_palette_hex(lua_State *L) {
    // Cache is keyed by the original Lua value, just like the supplied Lua code.
    lua_pushlightuserdata(L, (void *)&lua_palette_hex);
    lua_rawget(L, LUA_REGISTRYINDEX);
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 1);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            return 1;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    int r = 0, g = 0, b = 0;

    if (lua_isnumber(L, 1)) {
        lua_Integer number = lua_tointeger(L, 1);
        r = (int)((number / 65536) % 256);
        g = (int)((number / 256) % 256);
        b = (int)(number % 256);
    } else if (lua_isstring(L, 1)) {
        const char *value = lua_tostring(L, 1);
        size_t len = strlen(value);
        const char *clean = value;

        if (len > 0 && clean[0] == '#') {
            clean++;
        }
        if (clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
            clean += 2;
        }

        char *end = NULL;
        unsigned long number = strtoul(clean, &end, 16);
        if (end == clean || *end != '\0') {
            lua_pushinteger(L, 0);
            return 1;
        }

        r = (int)((number / 65536UL) % 256UL);
        g = (int)((number / 256UL) % 256UL);
        b = (int)(number % 256UL);
    } else {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushcfunction(L, lua_palette_rgb);
    lua_pushinteger(L, r);
    lua_pushinteger(L, g);
    lua_pushinteger(L, b);
    lua_call(L, 3, 1);

    lua_pushlightuserdata(L, (void *)&lua_palette_hex);
    lua_rawget(L, LUA_REGISTRYINDEX);
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 1);
        lua_pushvalue(L, -3); // result
        lua_rawset(L, -3);
    }
    lua_pop(L, 1); // cache table

    return 1;
}

static void palette_reset_caches(lua_State *L) {
    lua_pushlightuserdata(L, (void *)&lua_palette_rgb);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, (void *)&lua_palette_hex);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

static void install_sprites_find(lua_State *L) {
    lua_getglobal(L, "Sprites");
    if (lua_istable(L, -1)) {
        lua_pushcfunction(L, lua_sprites_find);
        lua_setfield(L, -2, "find");
    }
    lua_pop(L, 1);
}

static void install_palette_helpers(lua_State *L) {
    lua_getglobal(L, "Palette");
    if (lua_istable(L, -1)) {
        palette_reset_caches(L);

        lua_pushcfunction(L, lua_palette_rgb);
        lua_setfield(L, -2, "rgb");

        lua_pushcfunction(L, lua_palette_hex);
        lua_setfield(L, -2, "hex");
    }
    lua_pop(L, 1);
}

//----------------------------------------------------------------------------------
// Sprites preload loader — called when Lua does require("sprites")
//----------------------------------------------------------------------------------
static int lua_sprites_loader(lua_State *L) {
    char manifest_path[512];
    snprintf(manifest_path, sizeof(manifest_path), "%s/lupi_manifest.txt", current_game_dir);
    inject_sprites_global(L, manifest_path, current_game_dir);
    install_sprites_find(L);
    lua_getglobal(L, "Sprites");
    return 1;
}

//----------------------------------------------------------------------------------
// lua_api_init — create Lua state, bind ui.* table, expose button constants
//----------------------------------------------------------------------------------
void lua_api_init(void) {
    frame_counter = 0;
    globalLuaState = luaL_newstate();
    luaL_openlibs(globalLuaState);

    lua_newtable(globalLuaState);

    lua_pushcfunction(globalLuaState, lua_draw_line);
    lua_setfield(globalLuaState, -2, "line");

    lua_pushcfunction(globalLuaState, lua_draw_rect);
    lua_setfield(globalLuaState, -2, "draw_rect");

    lua_pushcfunction(globalLuaState, lua_rect);
    lua_setfield(globalLuaState, -2, "rect");

    lua_pushcfunction(globalLuaState, lua_rectfill);
    lua_setfield(globalLuaState, -2, "rectfill");

    lua_pushcfunction(globalLuaState, lua_draw_circle);
    lua_setfield(globalLuaState, -2, "draw_circle");

    lua_pushcfunction(globalLuaState, lua_circfill);
    lua_setfield(globalLuaState, -2, "circfill");

    lua_pushcfunction(globalLuaState, lua_trisfill);
    lua_setfield(globalLuaState, -2, "trisfill");

    lua_pushcfunction(globalLuaState, lua_palset);
    lua_setfield(globalLuaState, -2, "palset");

    lua_pushcfunction(globalLuaState, lua_tile);
    lua_setfield(globalLuaState, -2, "tile");

    lua_pushcfunction(globalLuaState, lua_spr);
    lua_setfield(globalLuaState, -2, "spr");

    lua_pushcfunction(globalLuaState, lua_btn);
    lua_setfield(globalLuaState, -2, "btn");

    lua_pushcfunction(globalLuaState, lua_btnp);
    lua_setfield(globalLuaState, -2, "btnp");

    lua_pushcfunction(globalLuaState, lua_log);
    lua_setfield(globalLuaState, -2, "log");

    lua_pushcfunction(globalLuaState, lua_camera);
    lua_setfield(globalLuaState, -2, "camera");

    lua_pushcfunction(globalLuaState, lua_clip);
    lua_setfield(globalLuaState, -2, "clip");

    lua_pushcfunction(globalLuaState, lua_cls);
    lua_setfield(globalLuaState, -2, "cls");

    lua_pushcfunction(globalLuaState, lua_preload_spritesheet);
    lua_setfield(globalLuaState, -2, "preload_spritesheet");

    lua_pushcfunction(globalLuaState, lua_draw_sprite);
    lua_setfield(globalLuaState, -2, "draw_sprite");

    lua_pushcfunction(globalLuaState, lua_print);
    lua_setfield(globalLuaState, -2, "print");

    lua_pushcfunction(globalLuaState, lua_map);
    lua_setfield(globalLuaState, -2, "map");

    lua_pushcfunction(globalLuaState, lua_set_pallet);
    lua_setfield(globalLuaState, -2, "set_pallet");

    lua_pushcfunction(globalLuaState, lua_fillp);
    lua_setfield(globalLuaState, -2, "fillp");

    lua_pushcfunction(globalLuaState, lua_mid);
    lua_setfield(globalLuaState, -2, "mid");

    lua_setglobal(globalLuaState, "ui");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    lua_setglobal(globalLuaState, "RIGHT");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    lua_setglobal(globalLuaState, "LEFT");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_LEFT_FACE_UP);
    lua_setglobal(globalLuaState, "UP");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    lua_setglobal(globalLuaState, "DOWN");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    lua_setglobal(globalLuaState, "BTN_X");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    lua_setglobal(globalLuaState, "BTN_E");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    lua_setglobal(globalLuaState, "BTN_Q");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    lua_setglobal(globalLuaState, "BTN_Z");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    lua_setglobal(globalLuaState, "BTN_F");

    lua_pushinteger(globalLuaState, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    lua_setglobal(globalLuaState, "BTN_G");
}

//----------------------------------------------------------------------------------
// Startup palette hook
//
// Some games create/replace the global Palette table while game.lua is loading
// and then call Palette.hex() from a required module such as const.lua.
// Installing the helper only after luaL_dofile(game.lua) is therefore too late.
// The hook runs only during startup and installs the native helpers as soon as
// a Palette table exists. It is removed immediately after game.lua finishes.
//----------------------------------------------------------------------------------
static void lua_palette_startup_hook(lua_State *L, lua_Debug *ar) {
    (void)ar;

    lua_getglobal(L, "Palette");
    bool has_palette = lua_istable(L, -1);
    bool has_hex = false;

    if (has_palette) {
        lua_getfield(L, -1, "hex");
        has_hex = lua_isfunction(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (has_palette && !has_hex) {
        install_palette_helpers(L);
    }
}

//----------------------------------------------------------------------------------
// lua_api_setup_game — configure package path, register sprites preload, load game
//----------------------------------------------------------------------------------
void lua_api_setup_game(const char *game_dir) {
    snprintf(current_game_dir, sizeof(current_game_dir), "%s", game_dir);

    lua_getglobal(globalLuaState, "package");

    lua_getfield(globalLuaState, -1, "path");
    const char *current_path = lua_tostring(globalLuaState, -1);
    lua_pop(globalLuaState, 1);
    lua_pushfstring(globalLuaState, "%s;%s/?.lua", current_path, game_dir);
    lua_setfield(globalLuaState, -2, "path");

    lua_getfield(globalLuaState, -1, "preload");
    lua_pushcfunction(globalLuaState, lua_sprites_loader);
    lua_setfield(globalLuaState, -2, "sprites");
    lua_pop(globalLuaState, 2); // pop preload and package

    char game_path[512];
    snprintf(game_path, sizeof(game_path), "%s/game.lua", game_dir);

    // Palette may be created inside game.lua before const.lua (or another
    // required module) calls Palette.hex(). Keep the hook active only while
    // the game is loading so runtime performance is unaffected.
    lua_sethook(globalLuaState, lua_palette_startup_hook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKCOUNT, 1);

    if (luaL_dofile(globalLuaState, game_path) != LUA_OK) {
        printf("Error loading game: %s\n", lua_tostring(globalLuaState, -1));
        lua_pop(globalLuaState, 1);
    }

    // Never keep the startup hook during gameplay.
    lua_sethook(globalLuaState, NULL, 0, 0);

    // Final installation covers games that create Palette late during startup.
    install_sprites_find(globalLuaState);
    install_palette_helpers(globalLuaState);

    hardware_constraints_update_from_lua();
}

//----------------------------------------------------------------------------------
// lua_api_call_update — call the Lua update() function if it exists
//----------------------------------------------------------------------------------
void lua_api_call_update(void) {
    if (globalLuaState == NULL) return;

    lua_getglobal(globalLuaState, "update");
    if (lua_isfunction(globalLuaState, -1)) {
        lua_pushnumber(globalLuaState, frame_counter);
        frame_counter++;
        if (lua_pcall(globalLuaState, 1, 0, 0) != LUA_OK) {
            printf("Error in update(): %s\n", lua_tostring(globalLuaState, -1));
            lua_pop(globalLuaState, 1);
        }
    } else {
        lua_pop(globalLuaState, 1);
    }
}

//----------------------------------------------------------------------------------
// lua_api_close — shut down the Lua state
//----------------------------------------------------------------------------------
void lua_api_close(void) {
    lua_close(globalLuaState);
    globalLuaState = NULL;
}
