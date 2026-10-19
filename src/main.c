#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "ui.h"
#include "lua_api.h"
#include "hardware_constraints.h"

#if !defined(PLATFORM_WEB)
#include "zip.h"
#endif

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_WEB)
static volatile int game_is_loading = 0;
#endif

#if !defined(PLATFORM_WEB)
static char temp_game_dir[512] = "";
#endif

static void draw_lupinho_cursor(void) {
    if (!IsCursorOnScreen()) return;

    Vector2 p = GetMousePosition();
    if (p.x < 0 || p.y < 0 || p.x >= SCREEN_WIDTH || p.y >= SCREEN_HEIGHT) return;

    // Small pixel-style arrow: black outline with a white interior.
    Vector2 shadow[] = {
        {p.x, p.y},
        {p.x, p.y + 16},
        {p.x + 5, p.y + 12},
        {p.x + 9, p.y + 19},
        {p.x + 13, p.y + 17},
        {p.x + 9, p.y + 10},
        {p.x + 16, p.y + 10}
    };

    DrawTriangleFan(shadow, 7, BLACK);

    Vector2 cursor[] = {
        {p.x + 1, p.y + 1},
        {p.x + 1, p.y + 13},
        {p.x + 5, p.y + 10},
        {p.x + 9, p.y + 17},
        {p.x + 11, p.y + 16},
        {p.x + 7, p.y + 9},
        {p.x + 14, p.y + 9}
    };

    DrawTriangleFan(cursor, 7, RAYWHITE);
}

void UpdateDrawFrame() {
    #if defined(PLATFORM_WEB)
    if (game_is_loading) return;
    #endif
    hardware_constraints_begin_frame();
    lua_api_call_update();

    BeginDrawing();

    ClearBackground(BLACK);

    draw_frame_buffer();

    #if defined(LUPI_HARDWARE_DEBUG)
    if (hardware_debug_overlay_enabled()) {
        const HardwareConstraintMetrics *metrics = hardware_constraints_get_metrics();
        const int panel_width = 228;
        const int panel_height = 208;
        const int panel_x = metrics->overlay_x;
        const int panel_y = metrics->overlay_y;
        const int line_height = 12;

        DrawRectangle(panel_x, panel_y, panel_width, panel_height, Fade(BLACK, metrics->overlay_alpha));
        DrawRectangleLines(panel_x, panel_y, panel_width, panel_height, YELLOW);
        DrawText("LUPI HARDWARE DEBUG", panel_x + 7, panel_y + 5, 8, YELLOW);
        DrawText(TextFormat("Drawlist: %u / %u B", metrics->drawlist_bytes, metrics->drawlist_limit), panel_x + 7, panel_y + 21, 8, RAYWHITE);
        DrawText(TextFormat("SPR: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_SPR], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_SPR]), panel_x + 7, panel_y + 21 + line_height, 8, RAYWHITE);
        DrawText(TextFormat("TILE: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_TILE], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_TILE]), panel_x + 7, panel_y + 21 + line_height * 2, 8, RAYWHITE);
        DrawText(TextFormat("RECT: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_RECT], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_RECT]), panel_x + 7, panel_y + 21 + line_height * 3, 8, RAYWHITE);
        DrawText(TextFormat("CIRC: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_CIRCLE], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_CIRCLE]), panel_x + 7, panel_y + 21 + line_height * 4, 8, RAYWHITE);
        DrawText(TextFormat("TRIS: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_TRIANGLE], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_TRIANGLE]), panel_x + 7, panel_y + 21 + line_height * 5, 8, RAYWHITE);
        DrawText(TextFormat("PRINT: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_PRINT], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_PRINT]), panel_x + 7, panel_y + 21 + line_height * 6, 8, RAYWHITE);
        DrawText(TextFormat("CLS: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_CLS], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_CLS]), panel_x + 7, panel_y + 21 + line_height * 7, 8, RAYWHITE);
        DrawText(TextFormat("CLIP: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_CLIP], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_CLIP]), panel_x + 7, panel_y + 21 + line_height * 8, 8, RAYWHITE);
        DrawText(TextFormat("FILLP: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_FILLP], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_FILLP]), panel_x + 7, panel_y + 21 + line_height * 9, 8, RAYWHITE);
        DrawText(TextFormat("OTHER: %u B (%u)", metrics->drawlist_command_bytes[HARDWARE_DRAWLIST_COMMAND_OTHER], metrics->drawlist_command_calls[HARDWARE_DRAWLIST_COMMAND_OTHER]), panel_x + 7, panel_y + 21 + line_height * 10, 8, RAYWHITE);
        DrawText(TextFormat("SD: %u/%u B", metrics->sd_bytes, metrics->sd_limit), panel_x + 7, panel_y + 21 + line_height * 11, 8, RAYWHITE);
        DrawText(TextFormat("Loads: %u  SD tx: %u/%u", metrics->sprite_loads, metrics->sd_transactions, metrics->sd_transactions_limit), panel_x + 7, panel_y + 21 + line_height * 12, 8, RAYWHITE);
        DrawText(TextFormat("SD tx over: %u", metrics->sd_rejected_transactions), panel_x + 7, panel_y + 21 + line_height * 13, 8, metrics->sd_rejected_transactions > 0 ? RED : RAYWHITE);
        DrawText(TextFormat("Rejected: %u", metrics->drawlist_rejected_commands), panel_x + 7, panel_y + 21 + line_height * 14, 8, metrics->drawlist_rejected_commands > 0 ? RED : RAYWHITE);
        DrawText(hardware_constraints_enabled() ? "Limits: ON" : "Limits: OFF", panel_x + 7, panel_y + 21 + line_height * 15, 8, hardware_constraints_enabled() ? GREEN : YELLOW);
    }
    #endif

    draw_lupinho_cursor();

    #ifndef PRODUCTION
    // DrawFPS(10, 10); // DEBUG
    #endif

    EndDrawing();

    clear_frame_buffer();
}

#if defined(PLATFORM_WEB)
EMSCRIPTEN_KEEPALIVE
void lupi_load_game(const char *game_dir) {
    game_is_loading = 1;
    lua_api_close();
    lua_api_init();
    reset_ui_state();
    lua_api_setup_game(game_dir);
    game_is_loading = 0;
}
#endif

#if !defined(PLATFORM_WEB)
static void print_usage(const char *program_name) {
    printf("Usage: %s [game.lupi | game_directory]\n", program_name);
    printf("\nLupi Emulator - Run Lupi games on Linux\n");
    printf("\nArguments:\n");
    printf("  game.lupi         Path to a .lupi archive file\n");
    printf("  game_directory    Path to an unpacked game directory\n");
    printf("\nExamples:\n");
    printf("  %s balao-gatinho.lupi\n", program_name);
    printf("  %s game-example/\n", program_name);
}

static int ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}
#endif

#if defined(PLATFORM_WEB)
int main() {
    SetTraceLogLevel(LOG_WARNING);

    lua_api_init();

    #if defined(AUTO_LOAD_GAME)
    lua_api_setup_game("/loaded_game");
    #endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lupi Emulator");
    HideCursor();
    SetTargetFPS(60);

    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);

    CloseWindow();
    lua_api_close();
    return 0;
}
#else
int main(int argc, char *argv[]) {
    const char *game_dir = NULL;
    int extracted_to_tmp = 0;

    // Parse command-line arguments
    if (argc < 2) {
        fprintf(stderr, "Error: No game specified\n");
        print_usage(argv[0]);
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "Error: Too many arguments\n");
        print_usage(argv[0]);
        return 1;
    }

    // Check for help flag
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    // Determine if input is a .lupi file or directory
    if (ends_with(argv[1], ".lupi")) {
        // Extract .lupi file to temp directory
        printf("Extracting %s...\n", argv[1]);
        if (extract_lupi_to_tmp(argv[1], temp_game_dir, sizeof(temp_game_dir)) != 0) {
            fprintf(stderr, "Failed to extract %s\n", argv[1]);
            return 1;
        }
        printf("Extracted to: %s\n", temp_game_dir);
        game_dir = temp_game_dir;
        extracted_to_tmp = 1;
    } else {
        // Use directory directly
        game_dir = argv[1];
    }

    // Initialize Lua
    lua_api_init();

    // Set up the game
    lua_api_setup_game(game_dir);

    // Initialize Raylib window
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lupi Emulator");
    HideCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    // Cleanup
    CloseWindow();
    lua_api_close();

    // Clean up temp directory if we extracted a .lupi file
    if (extracted_to_tmp) {
        cleanup_lupi_tmp(temp_game_dir);
    }

    return 0;
}
#endif
