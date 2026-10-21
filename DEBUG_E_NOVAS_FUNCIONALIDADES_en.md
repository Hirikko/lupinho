# New Features and Debug System

This version of Lupinho adds new APIs compatible with Lupi games, hardware diagnostics, a debug overlay, and improvements to the Lua/Web infrastructure.

## New Features

### Lua 5.5.1

Lupinho uses Lua 5.5.1 as its Lua runtime.

This provides better compatibility with current games and scripts in the Lupi ecosystem while keeping Lua execution aligned with the version used by the project.

---

## Mouse Input

```lua
ui.mouse()
```

Returns five values:

```lua
local x, y, buttons, wheel_x, wheel_y = ui.mouse()
```

| Value | Description |
|---|---|
| `x` | Horizontal mouse position |
| `y` | Vertical mouse position |
| `buttons` | Mouse button state |
| `wheel_x` | Horizontal wheel movement |
| `wheel_y` | Vertical wheel movement |

Buttons are represented by bits:

```text
bit 0 = left button
bit 1 = right button
```

Example:

```lua
local x, y, buttons = ui.mouse()
local left = (buttons & 1) ~= 0
local right = (buttons & 2) ~= 0
```

Currently `wheel_x` and `wheel_y` are returned as `0`.

---

## Text Input

```lua
ui.readtext()
```

Returns the next character available in the text-input buffer. Each call consumes one character.

```lua
local char = ui.readtext()
if char then
    print(char)
end
```

Suitable for player names, menus, text fields, codes, passwords, and configuration screens.

---

## Sprites.find

```lua
Sprites.find(path)
```

Example:

```lua
local sprite = Sprites.find("player/sonic")
```

Hierarchical paths are supported:

```text
player/sonic
enemies/badnik/crab
ui/menu/button
```

The function can also search by the final sprite name anywhere in the `Sprites` tree:

```lua
local sprite = Sprites.find("sonic")
```

---

## Palette.rgb

```lua
Palette.rgb(r, g, b)
```

Converts an RGB color to the closest available Lupi palette index.

```lua
local color = Palette.rgb(255, 0, 0)
```

The conversion uses color distance to select the closest palette entry.

---

## Palette.hex

```lua
Palette.hex(value)
```

Example:

```lua
local red = Palette.hex(0xFF0000)
```

The function converts an RGB color specified as a number or hexadecimal string to the closest Lupi palette index.

---

## Compatibility with `pairs(Palette)`

`Palette.rgb` and `Palette.hex` are implemented without interfering with normal palette enumeration. Existing games can therefore continue using:

```lua
for idx, color in pairs(Palette) do
    ui.palset(idx - 1, color)
end
```

The helper functions are not treated as palette colors.

---

## Sprite and Asset Cache

Lupinho keeps a persistent cache of assets loaded during game execution.

First use:

```text
SD → RAM → use
```

Subsequent uses:

```text
RAM → use
```

This avoids unnecessary repeated storage reads. The cache is reset when the game is loaded or restarted and supports up to 256 cached files.

---

## Hardware Diagnostic System

Lupinho includes a monitoring system for the hardware limitations defined for the Lupi console.

The system collects per-frame metrics without changing normal game behavior.

It monitors:

- drawlist bytes;
- SD bytes;
- SD transaction count;
- sprite/asset loads;
- graphics command counts;
- commands rejected by diagnostics;
- configured limits.

The goal is to allow games to be developed and tested according to the target hardware characteristics.

---

## Monitored Limits

| Resource | Limit |
|---|---:|
| Drawlist per frame | `18432 bytes` |
| SD per frame | `150000 bytes` |
| SD transactions per frame | `32` |
| Maximum sprite/asset pixels | `46000` |
| Palette slots | `256` |
| Transparency index | `0` |

These values are diagnostic reference limits for the expected hardware capabilities.

---

## Drawlist Costs

| Command | Cost |
|---|---:|
| `line` | 12 bytes |
| `rect` | 12 bytes |
| `rectfill` | 12 bytes |
| `circ` | 12 bytes |
| `circfill` | 12 bytes |
| `trisfill` | 20 bytes |
| initial `spr` | 8 bytes |
| subsequent `spr` | 4 bytes |
| initial `tile` | 8 bytes |
| subsequent `tile` | 4 bytes |
| `clip` | 12 bytes |
| `clip` reset | 4 bytes |
| `fillp` | 12 bytes |
| `fillp` reset | 4 bytes |
| `cls` | 16 bytes |
| `print` | 16 + `ceil(length / 4)` bytes |
| `camera` | 0 bytes |
| `palset` | 0 bytes |

These values help identify which operations consume the most drawlist space.

---

## Hardware / Debug Control Variables

`LUPI_HARDWARE_CONSTRAINTS` controls hardware metric collection:

```lua
LUPI_HARDWARE_CONSTRAINTS = true
```

To disable:

```lua
LUPI_HARDWARE_CONSTRAINTS = false
```

`LUPI_HARDWARE_DEBUG_OVERLAY` controls the on-screen diagnostic panel:

```lua
LUPI_HARDWARE_DEBUG_OVERLAY = true
LUPI_HARDWARE_DEBUG_OVERLAY_X = 10
LUPI_HARDWARE_DEBUG_OVERLAY_Y = 1
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.45
```

The X and Y values control the overlay position and Alpha controls transparency.

To disable the overlay:

```lua
LUPI_HARDWARE_DEBUG_OVERLAY = false
```

---

## Hardware Overlay

When enabled, the overlay displays information similar to:

```text
drawlist: 6168 / 18432
spr: 40b (8)
tile: 5356b (1336)
rect: 60b (5)
circ: 648b (54)
tris: 0b
print: 36b (2)
cls: 0b
clip: 12b (1)
fillp: 16b (2)

sd: 209920 / 150000b
loads: 8
sd tx: 77 / 32
```

For example, `drawlist: 6168 / 18432` means the frame used 6168 bytes out of a limit of 18432.

Likewise, `sd: 120000 / 150000b` means the frame used 120000 bytes out of a limit of 150000.

---

## Resource Overuse Diagnostics

When a limit is exceeded, the overlay helps identify the problem.

Example:

```text
sd tx: 77 / 32
sd tx over: 45
```

The game performed 77 SD transactions during the frame, exceeding the limit of 32.

The diagnostics can identify excessive drawlist usage, SD traffic, SD transactions, asset loads, and graphics commands.

The system is primarily intended as a development and optimization tool.

---

## SD Cache and Diagnostics

The diagnostic system distinguishes between initial asset loading and later reuse.

First use:

```text
SD → RAM
```

The system accounts for bytes read, transaction count, and asset load.

Later use:

```text
RAM
```

The asset does not need to be loaded from SD again.

This helps detect games performing excessive storage reads per frame.

---

## Modular Structure

The implementation is organized into modules for easier maintenance and future development:

```text
src/
├── main.c
├── ui.c
├── ui.h
├── lua_api.c
├── lua_api.h
├── lupi_functions.c
├── lupi_functions.h
├── hardware_constraints.c
├── hardware_constraints.h
├── types.h
├── font.h
├── zip.c
└── zip.h
```

### `lua_api.c`

Lua API infrastructure, lifecycle, and registration of functions available to games.

### `lupi_functions.c`

Lupi-specific compatibility and auxiliary functionality, including `Sprites.find`, `Palette.rgb`, `Palette.hex`, text input, `ui.mouse`, `ui.readtext`, and related caches.

### `hardware_constraints.c`

Hardware-limit metrics and diagnostics.

### `ui.c`

Graphical interface and framebuffer presentation.

---

## Development Configuration Example

```lua
LUPI_HARDWARE_CONSTRAINTS = true
LUPI_HARDWARE_DEBUG_OVERLAY = true
LUPI_HARDWARE_DEBUG_OVERLAY_X = 10
LUPI_HARDWARE_DEBUG_OVERLAY_Y = 1
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.45

function init()
    -- game initialization
end

function update(frame)
    -- game logic
end
```

For normal execution:

```lua
LUPI_HARDWARE_CONSTRAINTS = false
LUPI_HARDWARE_DEBUG_OVERLAY = false
```

---

## System Goal

These tools are intended not only to detect errors, but also to help developers build games with the real characteristics of Lupi hardware in mind.

During development, the overlay can be used to:

1. identify heavy frames;
2. detect excessive graphics operations;
3. identify excessive SD reads;
4. find repeated asset loads;
5. monitor drawlist usage;
6. optimize games before running them on real hardware.

Lupinho therefore works not only as a simulator, but also as a development and validation tool for Lupi games.
