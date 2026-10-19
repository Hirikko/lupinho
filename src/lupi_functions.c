#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

#include "lupi_functions.h"
#include "raylib.h"

#define TEXT_INPUT_QUEUE_SIZE 256

typedef struct {
    char bytes[5];
} TextInputEntry;

static TextInputEntry text_input_queue[TEXT_INPUT_QUEUE_SIZE];
static int text_input_head = 0;
static int text_input_tail = 0;
static int text_input_count = 0;

void lupi_functions_push_text_input(int codepoint) {
    if (codepoint <= 0 || codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return;
        }

        char *out = text_input_queue[text_input_tail].bytes;
    int length = 0;

    if (codepoint <= 0x7F) {
        out[length++] = (char)codepoint;
    } else if (codepoint <= 0x7FF) {
        out[length++] = (char)(0xC0 | (codepoint >> 6));
        out[length++] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out[length++] = (char)(0xE0 | (codepoint >> 12));
        out[length++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[length++] = (char)(0x80 | (codepoint & 0x3F));
    } else {
        out[length++] = (char)(0xF0 | (codepoint >> 18));
        out[length++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        out[length++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[length++] = (char)(0x80 | (codepoint & 0x3F));
    }

    out[length] = '\0';

    if (text_input_count == TEXT_INPUT_QUEUE_SIZE) {
        // Keep the newest input if a game leaves text unread for too long.
        text_input_head = (text_input_head + 1) % TEXT_INPUT_QUEUE_SIZE;
        text_input_count--;
    }

    text_input_tail = (text_input_tail + 1) % TEXT_INPUT_QUEUE_SIZE;
    text_input_count++;
}

void lupi_functions_reset(void) {
    text_input_head = 0;
    text_input_tail = 0;
    text_input_count = 0;
}

//----------------------------------------------------------------------------------
// ui.mouse() -> x, y, buttons, wheel_x, wheel_y
// buttons: bit 0 = left, bit 1 = right.
// Wheel values are currently unsupported by the Lupi API and return 0.
//----------------------------------------------------------------------------------
int lua_mouse(lua_State *L) {
    if (!IsWindowReady()) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        return 5;
    }

    Vector2 position = GetMousePosition();
    int buttons = 0;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        buttons |= 1;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        buttons |= 2;
    }

    lua_pushinteger(L, (lua_Integer)position.x);
    lua_pushinteger(L, (lua_Integer)position.y);
    lua_pushinteger(L, buttons);
    lua_pushinteger(L, 0);
    lua_pushinteger(L, 0);
    return 5;
}

//----------------------------------------------------------------------------------
// ui.readtext() -> next UTF-8 character from the keyboard text buffer, or nil
//----------------------------------------------------------------------------------
int lua_readtext(lua_State *L) {
    if (text_input_count == 0) {
        return 0;
    }

    lua_pushstring(L, text_input_queue[text_input_head].bytes);
    text_input_head = (text_input_head + 1) % TEXT_INPUT_QUEUE_SIZE;
    text_input_count--;
    return 1;
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

int lua_sprites_find(lua_State *L) {
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

void lupi_palette_reset_caches(lua_State *L);

int lua_palette_rgb(lua_State *L) {
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

int lua_palette_hex(lua_State *L) {
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

void lupi_palette_reset_caches(lua_State *L) {
    lua_pushlightuserdata(L, (void *)&lua_palette_rgb);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, (void *)&lua_palette_hex);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

//----------------------------------------------------------------------------------
// Keep Palette.rgb / Palette.hex available without making them enumerable palette
// entries. Some games iterate over Palette with pairs(Palette) and expect every
// entry returned by that loop to be a numeric color.
//
// The compatibility functions therefore live in Palette's __index table instead
// of being stored directly in Palette. This preserves:
//     Palette.hex(...)
//     Palette.rgb(...)
// while keeping pairs(Palette) limited to the actual palette colors.
//----------------------------------------------------------------------------------
void lupi_palette_install_helpers(lua_State *L) {
    lua_getglobal(L, "Palette");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    int palette_index = lua_gettop(L);
    lupi_palette_reset_caches(L);

    // lua_getmetatable() pushes the metatable only when one exists.
    // Do not pop here when it returns false: the Palette table is still
    // below the top of the stack and must remain available as palette_index.
    if (!lua_getmetatable(L, palette_index)) {
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setmetatable(L, palette_index);
    }

    int metatable_index = lua_gettop(L);

    lua_getfield(L, metatable_index, "__index");

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushcfunction(L, lua_palette_rgb);
        lua_setfield(L, -2, "rgb");
        lua_pushcfunction(L, lua_palette_hex);
        lua_setfield(L, -2, "hex");
        lua_setfield(L, metatable_index, "__index");
    } else if (lua_istable(L, -1)) {
        lua_pushcfunction(L, lua_palette_rgb);
        lua_setfield(L, -2, "rgb");
        lua_pushcfunction(L, lua_palette_hex);
        lua_setfield(L, -2, "hex");
        lua_pop(L, 1);
    } else {
        // If the game already uses a function as __index, keep that behavior
        // untouched. The compatibility helpers cannot be inserted into a
        // function-valued __index without wrapping game code, so only install
        // them directly as a last-resort fallback.
        lua_pop(L, 1);
        lua_pushcfunction(L, lua_palette_rgb);
        lua_setfield(L, palette_index, "rgb");
        lua_pushcfunction(L, lua_palette_hex);
        lua_setfield(L, palette_index, "hex");
    }

    lua_pop(L, 1); // metatable
    lua_pop(L, 1); // Palette
}

