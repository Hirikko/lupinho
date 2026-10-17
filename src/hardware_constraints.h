#ifndef HARDWARE_CONSTRAINTS_H
#define HARDWARE_CONSTRAINTS_H

#include <stdbool.h>
#include <stdint.h>

#define LUPI_DRAWLIST_MAX_BYTES       18432
#define LUPI_SD_MAX_BYTES_PER_FRAME   150000
#define LUPI_SPRITE_MAX_LOADS_FRAME  32
#define LUPI_SPRITE_MAX_PIXELS       46000
#define LUPI_SD_MAX_TRANSACTIONS_FRAME 32

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
    uint32_t sd_transactions;
    uint32_t sd_rejected_transactions;
    uint32_t drawlist_rejected_commands;

    uint32_t drawlist_limit;
    uint32_t sd_limit;
    uint32_t sprite_loads_limit;
    uint32_t sprite_pixels_limit;
    uint32_t sd_transactions_limit;

    // Detailed drawlist diagnostics, indexed by HardwareDrawlistCommand.
    uint32_t drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_COUNT];
    uint32_t drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_COUNT];
    uint32_t drawlist_command_rejected[HARDWARE_DRAWLIST_COMMAND_COUNT];

    // Debug overlay configuration. Values are loaded from game.lua.
    int overlay_x;
    int overlay_y;
    float overlay_alpha;

    bool enabled;
    bool overlay_enabled;
} HardwareConstraintMetrics;

void hardware_constraints_begin_frame(void);

bool hardware_constraints_try_drawlist(uint32_t bytes, HardwareDrawlistCommand command);
void hardware_constraints_add_sd_bytes(uint32_t bytes);
void hardware_constraints_add_sd_transaction(void);
void hardware_constraints_add_sprite_load(void);

const HardwareConstraintMetrics *hardware_constraints_get_metrics(void);

bool hardware_constraints_enabled(void);
bool hardware_debug_overlay_enabled(void);

void hardware_constraints_update_from_lua(void);

#endif
