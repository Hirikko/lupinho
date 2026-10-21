#include <string.h>

#include "lua_api.h"
#include "hardware_constraints.h"
#include "lstate.h"
#include "ui.h"

extern lua_State *globalLuaState;

static HardwareConstraintMetrics hardware_metrics = {
    .drawlist_bytes = 0,
    .sd_bytes = 0,
    .sprite_loads = 0,
    .sprite_rejected_loads = 0,
    .sd_transactions = 0,
    .sd_rejected_transactions = 0,
    .drawlist_rejected_commands = 0,
    .drawlist_limit = LUPI_DRAWLIST_MAX_BYTES,
    .sd_limit = LUPI_SD_MAX_BYTES_PER_FRAME,
    .sprite_loads_limit = LUPI_SPRITE_MAX_LOADS_FRAME,
    .sprite_pixels_limit = LUPI_SPRITE_MAX_PIXELS,
    .sd_transactions_limit = LUPI_SD_MAX_TRANSACTIONS_FRAME,
    .lua_instructions = 0,
    .lua_instructions_limit = LUPI_LUA_WATCHDOG_LIMIT,
    .overlay_x0 = 6,
    .overlay_y0 = 6,
    .overlay_x1 = 246,
    .overlay_y1 = 260,
    .overlay_col = 10,
    .overlay_alpha = 0.82f,
    .enabled = true,
    .overlay_enabled = false,
    .lua_watchdog_enabled = true,
    .lua_watchdog_triggered = false
};

static HardwareDrawlistCommand last_drawlist_command = HARDWARE_DRAWLIST_COMMAND_OTHER;
static lua_State *lua_watchdog_state = NULL;
static uint32_t lua_watchdog_hook_events = 0;

static void hardware_constraints_lua_hook(lua_State *L, lua_Debug *ar) {
    (void)ar;

    lua_watchdog_hook_events++;
    hardware_metrics.lua_instructions =
    lua_watchdog_hook_events * LUPI_LUA_WATCHDOG_HOOK_INTERVAL;

    if (hardware_metrics.lua_instructions >= hardware_metrics.lua_instructions_limit) {
        hardware_metrics.lua_instructions = hardware_metrics.lua_instructions_limit;
        hardware_metrics.lua_watchdog_triggered = true;
        luaL_error(L, "LUPI Lua instruction limit exceeded (%u instructions/frame)",
                   hardware_metrics.lua_instructions_limit);
    }
}

void hardware_constraints_begin_frame(void) {
    hardware_metrics.drawlist_bytes = 0;
    hardware_metrics.sd_bytes = 0;
    hardware_metrics.sprite_loads = 0;
    hardware_metrics.sprite_rejected_loads = 0;
    hardware_metrics.sd_transactions = 0;
    hardware_metrics.sd_rejected_transactions = 0;
    hardware_metrics.drawlist_rejected_commands = 0;
    if (!hardware_metrics.lua_watchdog_triggered) {
        hardware_metrics.lua_instructions = 0;
    }
    memset(hardware_metrics.drawlist_command_bytes, 0, sizeof(hardware_metrics.drawlist_command_bytes));
    memset(hardware_metrics.drawlist_command_calls, 0, sizeof(hardware_metrics.drawlist_command_calls));
    memset(hardware_metrics.drawlist_command_rejected, 0, sizeof(hardware_metrics.drawlist_command_rejected));
    last_drawlist_command = HARDWARE_DRAWLIST_COMMAND_OTHER;
}

bool hardware_constraints_try_drawlist(uint32_t bytes, HardwareDrawlistCommand command) {
    uint32_t cost = bytes;

    if (command == HARDWARE_DRAWLIST_COMMAND_SPR || command == HARDWARE_DRAWLIST_COMMAND_TILE) {
        bool continuous = (last_drawlist_command == command);
        cost = continuous ? 4u : 8u;
    }

    if (command >= HARDWARE_DRAWLIST_COMMAND_COUNT) {
        command = HARDWARE_DRAWLIST_COMMAND_OTHER;
    }

    if (hardware_metrics.enabled &&
        hardware_metrics.drawlist_bytes + cost > hardware_metrics.drawlist_limit) {
        hardware_metrics.drawlist_rejected_commands++;
    hardware_metrics.drawlist_command_rejected[command]++;
    return false;
        }

        hardware_metrics.drawlist_bytes += cost;
        hardware_metrics.drawlist_command_bytes[command] += cost;
        hardware_metrics.drawlist_command_calls[command]++;
        last_drawlist_command = command;
        return true;
}

bool hardware_constraints_can_read_sd(uint64_t bytes) {
    if (!hardware_metrics.enabled) return true;

    // Reject before reading if either per-frame budget would be exceeded.
    // Subtraction is used after checking the current value to avoid overflow.
    if (hardware_metrics.sd_transactions >= hardware_metrics.sd_transactions_limit ||
        hardware_metrics.sd_bytes >= hardware_metrics.sd_limit ||
        bytes > (uint64_t)(hardware_metrics.sd_limit - hardware_metrics.sd_bytes)) {
        if (hardware_metrics.sd_rejected_transactions < UINT32_MAX) {
            hardware_metrics.sd_rejected_transactions++;
        }
        return false;
        }

        return true;
}

void hardware_constraints_add_sd_read(uint64_t bytes) {
    if (hardware_metrics.sd_transactions < UINT32_MAX) {
        hardware_metrics.sd_transactions++;
    }

    // Saturate counters rather than allowing them to wrap when limits are off.
    if (bytes > (uint64_t)(UINT32_MAX - hardware_metrics.sd_bytes)) {
        hardware_metrics.sd_bytes = UINT32_MAX;
    } else {
        hardware_metrics.sd_bytes += (uint32_t)bytes;
    }
}

bool hardware_constraints_can_load_sprite(void) {
    if (!hardware_metrics.enabled) return true;

    if (hardware_metrics.sprite_loads >= hardware_metrics.sprite_loads_limit) {
        if (hardware_metrics.sprite_rejected_loads < UINT32_MAX) {
            hardware_metrics.sprite_rejected_loads++;
        }
        return false;
    }

    return true;
}

void hardware_constraints_add_sprite_load(void) {
    if (hardware_metrics.sprite_loads < UINT32_MAX) {
        hardware_metrics.sprite_loads++;
    }
}

const HardwareConstraintMetrics *hardware_constraints_get_metrics(void) {
    return &hardware_metrics;
}

bool hardware_constraints_enabled(void) {
    return hardware_metrics.enabled;
}

bool hardware_debug_overlay_enabled(void) {
    return hardware_metrics.overlay_enabled;
}

void hardware_constraints_update_from_lua(void) {
    if (globalLuaState == NULL) {
        hardware_metrics.enabled = true;
        hardware_metrics.overlay_enabled = false;
        hardware_metrics.overlay_x0 = 6;
        hardware_metrics.overlay_y0 = 6;
        hardware_metrics.overlay_x1 = 246;
        hardware_metrics.overlay_y1 = 260;
        hardware_metrics.overlay_col = 10;
        hardware_metrics.overlay_alpha = 0.82f;
        hardware_metrics.lua_watchdog_enabled = true;
        return;
    }

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_CONSTRAINTS");
    if (lua_isboolean(globalLuaState, -1)) {
        hardware_metrics.enabled = lua_toboolean(globalLuaState, -1) != 0;
    } else {
        hardware_metrics.enabled = true;
    }
    lua_pop(globalLuaState, 1);

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY");
    if (lua_isboolean(globalLuaState, -1)) {
        hardware_metrics.overlay_enabled = lua_toboolean(globalLuaState, -1) != 0;
    } else {
        hardware_metrics.overlay_enabled = false;
    }
    lua_pop(globalLuaState, 1);

    hardware_metrics.overlay_x0 = 6;
    hardware_metrics.overlay_y0 = 6;
    hardware_metrics.overlay_x1 = 246;
    hardware_metrics.overlay_y1 = 260;
    hardware_metrics.overlay_col = 10;

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY_RECT");
    if (lua_istable(globalLuaState, -1)) {
        lua_getfield(globalLuaState, -1, "x0");
        if (lua_isnumber(globalLuaState, -1)) {
            hardware_metrics.overlay_x0 = (int)lua_tointeger(globalLuaState, -1);
        }
        lua_pop(globalLuaState, 1);

        lua_getfield(globalLuaState, -1, "y0");
        if (lua_isnumber(globalLuaState, -1)) {
            hardware_metrics.overlay_y0 = (int)lua_tointeger(globalLuaState, -1);
        }
        lua_pop(globalLuaState, 1);

        lua_getfield(globalLuaState, -1, "x1");
        if (lua_isnumber(globalLuaState, -1)) {
            hardware_metrics.overlay_x1 = (int)lua_tointeger(globalLuaState, -1);
        }
        lua_pop(globalLuaState, 1);

        lua_getfield(globalLuaState, -1, "y1");
        if (lua_isnumber(globalLuaState, -1)) {
            hardware_metrics.overlay_y1 = (int)lua_tointeger(globalLuaState, -1);
        }
        lua_pop(globalLuaState, 1);

        lua_getfield(globalLuaState, -1, "col");
        if (lua_isnumber(globalLuaState, -1)) {
            hardware_metrics.overlay_col = (int)lua_tointeger(globalLuaState, -1);
        }
        lua_pop(globalLuaState, 1);
    }
    lua_pop(globalLuaState, 1);

    if (hardware_metrics.overlay_x0 < 0) hardware_metrics.overlay_x0 = 0;
    if (hardware_metrics.overlay_y0 < 0) hardware_metrics.overlay_y0 = 0;
    if (hardware_metrics.overlay_x1 <= hardware_metrics.overlay_x0) {
        hardware_metrics.overlay_x1 = hardware_metrics.overlay_x0 + 240;
    }
    if (hardware_metrics.overlay_y1 <= hardware_metrics.overlay_y0) {
        hardware_metrics.overlay_y1 = hardware_metrics.overlay_y0 + 254;
    }
    if (hardware_metrics.overlay_x1 > SCREEN_WIDTH) hardware_metrics.overlay_x1 = SCREEN_WIDTH;
    if (hardware_metrics.overlay_y1 > SCREEN_HEIGHT) hardware_metrics.overlay_y1 = SCREEN_HEIGHT;
    if (hardware_metrics.overlay_col < 0) hardware_metrics.overlay_col = 0;
    if (hardware_metrics.overlay_col >= PALETTE_SIZE) hardware_metrics.overlay_col = PALETTE_SIZE - 1;

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA");
    if (lua_isnumber(globalLuaState, -1)) {
        hardware_metrics.overlay_alpha = (float)lua_tonumber(globalLuaState, -1);
        if (hardware_metrics.overlay_alpha < 0.0f) hardware_metrics.overlay_alpha = 0.0f;
        if (hardware_metrics.overlay_alpha > 1.0f) hardware_metrics.overlay_alpha = 1.0f;
    } else {
        hardware_metrics.overlay_alpha = 0.82f;
    }
    lua_pop(globalLuaState, 1);

    lua_getglobal(globalLuaState, "LUPI_LUA_WATCHDOG");
    if (lua_isboolean(globalLuaState, -1)) {
        hardware_metrics.lua_watchdog_enabled = lua_toboolean(globalLuaState, -1) != 0;
    } else {
        hardware_metrics.lua_watchdog_enabled = true;
    }
    lua_pop(globalLuaState, 1);
}

void hardware_constraints_reset_lua_watchdog(void) {
    if (lua_watchdog_state != NULL) {
        lua_sethook(lua_watchdog_state, NULL, 0, 0);
    }
    lua_watchdog_state = NULL;
    hardware_metrics.lua_instructions = 0;
    hardware_metrics.lua_watchdog_triggered = false;
    lua_watchdog_hook_events = 0;
}

void hardware_constraints_lua_begin_frame(lua_State *L) {
    if (L == NULL) return;

    hardware_metrics.lua_instructions = 0;
    lua_watchdog_hook_events = 0;

    if (!hardware_metrics.lua_watchdog_enabled || hardware_metrics.lua_watchdog_triggered) {
        lua_watchdog_state = NULL;
        return;
    }

    lua_watchdog_state = L;
    lua_sethook(L, hardware_constraints_lua_hook, LUA_MASKCOUNT,
                (int)LUPI_LUA_WATCHDOG_HOOK_INTERVAL);
}

void hardware_constraints_lua_end_frame(lua_State *L) {
    if (L == NULL) return;

    /*
     * The hook itself runs every 1,000 VM instructions. Lua keeps the
     * countdown to the next hook in lua_State::hookcount, so we can expose
     * the exact observed instruction count without paying for a hook on
     * every single instruction.
     */
    if (lua_watchdog_state == L && !hardware_metrics.lua_watchdog_triggered) {
        lua_State *internal = L;
        uint32_t remainder = 0;
        if (internal->basehookcount > internal->hookcount) {
            remainder = (uint32_t)(internal->basehookcount - internal->hookcount);
        }
        hardware_metrics.lua_instructions =
        lua_watchdog_hook_events * LUPI_LUA_WATCHDOG_HOOK_INTERVAL + remainder;
        if (hardware_metrics.lua_instructions > hardware_metrics.lua_instructions_limit) {
            hardware_metrics.lua_instructions = hardware_metrics.lua_instructions_limit;
        }
    }

    lua_sethook(L, NULL, 0, 0);
    if (lua_watchdog_state == L) {
        lua_watchdog_state = NULL;
    }
}

bool hardware_constraints_lua_watchdog_triggered(void) {
    return hardware_metrics.lua_watchdog_triggered;
}

bool hardware_constraints_lua_watchdog_enabled(void) {
    return hardware_metrics.lua_watchdog_enabled;
}
