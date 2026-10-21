#ifndef HARDWARE_CONSTRAINTS_H
#define HARDWARE_CONSTRAINTS_H

#include <stdbool.h>
#include <stdint.h>

#include <lua.h>

#define LUPI_DRAWLIST_MAX_BYTES       18432
#define LUPI_SD_MAX_BYTES_PER_FRAME   150000
#define LUPI_SPRITE_MAX_LOADS_FRAME  32
#define LUPI_SPRITE_MAX_PIXELS       46000
#define LUPI_SD_MAX_TRANSACTIONS_FRAME 32
#define LUPI_LUA_WATCHDOG_LIMIT       55000U
#define LUPI_LUA_WATCHDOG_HOOK_INTERVAL 1000U

typedef enum {
    HARDWARE_DRAWLIST_COMMAND_OTHER = 0,
    HARDWARE_DRAWLIST_COMMAND_SPR,
    HARDWARE_DRAWLIST_COMMAND_TILE,
    HARDWARE_DRAWLIST_COMMAND_RECT,
    HARDWARE_DRAWLIST_COMMAND_CIRCLE,
    HARDWARE_DRAWLIST_COMMAND_TRIANGLE,
    HARDWARE_DRAWLIST_COMMAND_PRINT,
    HARDWARE_DRAWLIST_COMMAND_CLS,
    HARDWARE_DRAWLIST_COMMAND_CLIP,
    HARDWARE_DRAWLIST_COMMAND_FILLP,
    HARDWARE_DRAWLIST_COMMAND_COUNT
} HardwareDrawlistCommand;

typedef struct {
    uint32_t drawlist_bytes;
    uint32_t sd_bytes;
    uint32_t sprite_loads;
    uint32_t sprite_rejected_loads;
    uint32_t sd_transactions;
    uint32_t sd_rejected_transactions;
    uint32_t drawlist_rejected_commands;

    uint32_t drawlist_limit;
    uint32_t sd_limit;
    uint32_t sprite_loads_limit;
    uint32_t sprite_pixels_limit;
    uint32_t sd_transactions_limit;

    uint32_t lua_instructions;
    uint32_t lua_instructions_limit;

    // Detailed drawlist diagnostics, indexed by HardwareDrawlistCommand.
    uint32_t drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_COUNT];
    uint32_t drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_COUNT];
    uint32_t drawlist_command_rejected[HARDWARE_DRAWLIST_COMMAND_COUNT];

    // Debug overlay configuration. Values are loaded from game.lua.
    int overlay_x0;
    int overlay_y0;
    int overlay_x1;
    int overlay_y1;
    int overlay_col;
    float overlay_alpha;

    bool enabled;
    bool overlay_enabled;
    bool lua_watchdog_enabled;
    bool lua_watchdog_triggered;
} HardwareConstraintMetrics;

void hardware_constraints_begin_frame(void);

bool hardware_constraints_try_drawlist(uint32_t bytes, HardwareDrawlistCommand command);
// Check the per-frame SD budget before reading a file. Rejected requests are
// counted separately; successful reads are committed only after the full read.
bool hardware_constraints_can_read_sd(uint64_t bytes);
void hardware_constraints_add_sd_read(uint64_t bytes);
bool hardware_constraints_can_load_sprite(void);
void hardware_constraints_add_sprite_load(void);

const HardwareConstraintMetrics *hardware_constraints_get_metrics(void);

bool hardware_constraints_enabled(void);
bool hardware_debug_overlay_enabled(void);

void hardware_constraints_update_from_lua(void);

void hardware_constraints_reset_lua_watchdog(void);
void hardware_constraints_lua_begin_frame(lua_State *L);
void hardware_constraints_lua_end_frame(lua_State *L);
bool hardware_constraints_lua_watchdog_triggered(void);
bool hardware_constraints_lua_watchdog_enabled(void);

#endif
