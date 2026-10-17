#include <string.h>

#include "lua_api.h"
#include "hardware_constraints.h"

extern lua_State *globalLuaState;

static HardwareConstraintMetrics hardware_metrics = {
    .drawlist_bytes = 0,
    .sd_bytes = 0,
    .sprite_loads = 0,
    .sd_transactions = 0,
    .sd_rejected_transactions = 0,
    .drawlist_rejected_commands = 0,
    .drawlist_limit = LUPI_DRAWLIST_MAX_BYTES,
    .sd_limit = LUPI_SD_MAX_BYTES_PER_FRAME,
    .sprite_loads_limit = LUPI_SPRITE_MAX_LOADS_FRAME,
    .sprite_pixels_limit = LUPI_SPRITE_MAX_PIXELS,
    .sd_transactions_limit = LUPI_SD_MAX_TRANSACTIONS_FRAME,
    .enabled = true,
    .overlay_enabled = false
};

static HardwareDrawlistCommand last_drawlist_command = HARDWARE_DRAWLIST_COMMAND_OTHER;

void hardware_constraints_begin_frame(void) {
    hardware_metrics.drawlist_bytes = 0;
    hardware_metrics.sd_bytes = 0;
    hardware_metrics.sprite_loads = 0;
    hardware_metrics.sd_transactions = 0;
    hardware_metrics.sd_rejected_transactions = 0;
    hardware_metrics.drawlist_rejected_commands = 0;
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

void hardware_constraints_add_sd_bytes(uint32_t bytes) {
    hardware_metrics.sd_bytes += bytes;
}

void hardware_constraints_add_sd_transaction(void) {
    hardware_metrics.sd_transactions++;
    if (hardware_metrics.enabled && hardware_metrics.sd_transactions > hardware_metrics.sd_transactions_limit) {
        hardware_metrics.sd_rejected_transactions++;
    }
}

void hardware_constraints_add_sprite_load(void) {
    hardware_metrics.sprite_loads++;
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
        hardware_metrics.overlay_x = 6;
        hardware_metrics.overlay_y = 6;
        hardware_metrics.overlay_alpha = 0.82f;
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

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY_X");
    if (lua_isnumber(globalLuaState, -1)) {
        hardware_metrics.overlay_x = (int)lua_tointeger(globalLuaState, -1);
    } else {
        hardware_metrics.overlay_x = 6;
    }
    lua_pop(globalLuaState, 1);

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY_Y");
    if (lua_isnumber(globalLuaState, -1)) {
        hardware_metrics.overlay_y = (int)lua_tointeger(globalLuaState, -1);
    } else {
        hardware_metrics.overlay_y = 6;
    }
    lua_pop(globalLuaState, 1);

    lua_getglobal(globalLuaState, "LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA");
    if (lua_isnumber(globalLuaState, -1)) {
        hardware_metrics.overlay_alpha = (float)lua_tonumber(globalLuaState, -1);
        if (hardware_metrics.overlay_alpha < 0.0f) hardware_metrics.overlay_alpha = 0.0f;
        if (hardware_metrics.overlay_alpha > 1.0f) hardware_metrics.overlay_alpha = 1.0f;
    } else {
        hardware_metrics.overlay_alpha = 0.82f;
    }
    lua_pop(globalLuaState, 1);
}
