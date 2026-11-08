#ifndef LUPI_MUSIC_H
#define LUPI_MUSIC_H

#include <lua.h>

/*
 * Registra a biblioteca nativa "music" no Lua.
 * API inicial:
 *   music.loop(nome)
 *   music.stop()
 */
void LupiMusic_SetGameDir(const char *game_dir);
int LupiMusic_Register(lua_State *L);

#endif
