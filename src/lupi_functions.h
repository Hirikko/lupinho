#ifndef LUPI_FUNCTIONS_H
#define LUPI_FUNCTIONS_H

#include <lua.h>
#include <lauxlib.h>

void lupi_functions_reset(void);
void lupi_functions_push_text_input(int codepoint);

int lua_mouse(lua_State *L);
int lua_readtext(lua_State *L);

int lua_sprites_find(lua_State *L);
int lua_palette_rgb(lua_State *L);
int lua_palette_hex(lua_State *L);
void lupi_palette_reset_caches(lua_State *L);
void lupi_palette_install_helpers(lua_State *L);

#endif // LUPI_FUNCTIONS_H
