# 🎮 Lupinho
### Created by: [Juneira](https://github.com/juneira)
**Lupinho** is a simulator for the Brazilian **Lupi** console.
It runs directly in your browser using WebAssembly, allowing you to create and play games using the Lua programming API.

### 🕹️ [Try the Demo](https://lupinho.juneira.com/)

> The demo game is ["caio-pernocas"](https://github.com/lupi-org-br/caio-pernocas).
> Use W, A, S, D to move and K as the action button. You can also use a controller.

## Simulator vs Emulator

### **Emulator**
> Attempts to faithfully reproduce original hardware at a low level (processor, memory, graphics chips, etc.). Its goal is to run software made for that exact physical machine.

### **Simulator**
> Recreates the behavior and execution environment of a system at a more abstract or modern level, without physically reproducing the original hardware.

Lupinho simulates the Lupi console using modern technologies such as WebAssembly and Raylib, focusing on games created specifically for the Lupi API rather than ROMs from pre-existing hardware.

You can find more information about the **hardware limits** in this issue: [**LIMITATIONS**](https://github.com/lupi-org-br/lupinho/issues/12)

## ✨ Features

- 🌐 **Runs in the browser** — WebAssembly-based, requiring little installation and configuration
- 📜 **Lua scripts** — Write games in Lua using a simple API
- 🎨 **2D graphics** — Draw text, lines, rectangles, circles, and triangles
- 🖼️ **Sprites and Tiles** — Load and draw spritesheets with flip support
- 🗺️ **Tilemap system** — Draw large maps with camera scrolling
- 🎮 **Controller and Keyboard** — Input support for controllers and keyboard
- 🖱️ **Mouse and text input** — Mouse state and character-buffer APIs
- ⚡ **60 FPS** — Smooth gameplay at 60 frames per second
- 🔧 **Built with Raylib** — Built on the lightweight Raylib graphics library

## 🛠️ Technologies Used

| Component | Technology |
|-----------|------------|
| Language | C99 |
| Compiler | Emscripten (emcc) |
| Graphics | Raylib |
| Scripting | Lua 5.5.1 |
| Platform | WebAssembly |
| Screen | 480×270 px @ 60 FPS |

## 📁 Project Structure

```text
lupinho/
├── dist/                    # Compiled WebAssembly output
│   ├── game.html
│   ├── game.js
│   ├── game.wasm
│   └── game.data
├── site/
│   ├── index.html
│   └── dist/                # Place compiled WebAssembly output here
├── src/
│   ├── main.c
│   ├── lua_api.c            # Lua API bindings and lifecycle
│   ├── lua_api.h
│   ├── lupi_functions.c     # Lupi compatibility and auxiliary functions
│   ├── lupi_functions.h
│   ├── hardware_constraints.c
│   ├── hardware_constraints.h
│   ├── ui.c
│   ├── ui.h
│   ├── types.h
│   ├── font.h
│   ├── zip.c
│   ├── zip.h
│   ├── Makefile
│   └── libs/
│       ├── lua-web/
│       └── raylib-web/
├── game-example/
└── README.md
```

## 🚀 Getting Started

### Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed and activated
- On Linux, install Emscripten using your distribution's package manager when available
- On Windows, Ubuntu through WSL is recommended

### Building

```bash
cd src

# Development build
make web

# Production build
make production

# Development build with hardware limits and new functions
make debug
```

## Still in Development

`make debug` is the development build for the [hardware limits](https://github.com/lupi-org-br/lupinho/issues/12) and additional Lupi functionality.

Currently available compatibility features include `Palette.hex`, `Palette.rgb`, `Sprites.find`, `ui.mouse`, and `ui.readtext`.

To enable hardware diagnostics and the overlay:

```lua
LUPI_HARDWARE_CONSTRAINTS = true
LUPI_HARDWARE_DEBUG_OVERLAY = true
LUPI_HARDWARE_DEBUG_OVERLAY_X = 13
LUPI_HARDWARE_DEBUG_OVERLAY_Y = 13
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.45
```

See [`DEBUG_E_NOVAS_FUNCIONALIDADES.md`](DEBUG_E_NOVAS_FUNCIONALIDADES.md) for complete documentation.

### Running

After compiling, copy the contents of **`lupinho/dist/`** into **`lupinho/site/dist/`**.
Then start a local HTTP server from **`lupinho/site/`**.

#### Using Node.js
```bash
cd site
npx serve
```

#### Using Python 3
```bash
cd site
python3 -m http.server 8080
```

Then open `http://localhost:8080` in your browser.

## 📖 Lua API

Games are written in Lua. Your script should define `update()`, which is called every frame (60 times per second).

### `update()`
Called every frame. Update game logic and perform drawing here.

### Color Palette System

Lupinho uses a 256-color palette in BGR555 format.

| Function | Description |
|---|---|
| `ui.palset(index, bgr555)` | Sets a palette color at index 0–255 |
| `Palette.rgb(r, g, b)` | Finds the closest palette color to an RGB value |
| `Palette.hex(value)` | Converts an RGB/hex color to the closest indexed palette color |

### Drawing Functions

| Function | Description |
|---|---|
| `ui.rect(x1, y1, x2, y2, color)` | Draws a rectangle outline |
| `ui.rectfill(x1, y1, x2, y2, color)` | Draws a filled rectangle |
| `ui.draw_rect(x, y, w, h, filled, color)` | Draws a filled or outlined rectangle |
| `ui.circfill(x, y, radius, color)` | Draws a filled circle |
| `ui.draw_circle(cx, cy, r, filled, color, border, border_color)` | Draws a circle with an optional border |
| `ui.trisfill(x1, y1, x2, y2, x3, y3, color)` | Draws a filled triangle |
| `ui.line(x1, y1, x2, y2, color)` | Draws a line |
| `ui.print(text, x, y, color)` | Draws bitmap-font text |

### Screen Functions

| Function | Description |
|---|---|
| `ui.cls(color)` | Clears the screen using a palette color |
| `ui.camera(x, y)` | Sets the camera offset; `ui.camera()` resets it |
| `ui.clip(x, y, w, h)` | Sets the clipping region; `ui.clip()` resets it |
| `ui.fillp(b1, b2, ...)` | Sets the 8×8 fill pattern |

### Sprites and Tiles

| Function | Description |
|---|---|
| `Sprites.find(path)` | Finds a sprite by path or name |
| `ui.spr(sprite_table, x, y, flipped)` | Draws a sprite |
| `ui.tile(tileset_table, tile_index, x, y)` | Draws a tile; bit 10 flips it horizontally |

### Map Functions

| Function | Description |
|---|---|
| `ui.map(layer_table, cam_x, cam_y)` | Draws a tilemap layer |

### Input Functions

| Function | Description |
|---|---|
| `ui.btn(button, pad)` | Checks whether a button is pressed |
| `ui.btnp(button, pad)` | Checks whether a button was just pressed |
| `ui.mouse()` | Returns mouse position and button state |
| `ui.readtext()` | Returns and consumes the next character from the text-input buffer |

Button constants: `UP`, `DOWN`, `LEFT`, `RIGHT`, `BTN_Z`, `BTN_Q`, `BTN_E`, `BTN_F`, `BTN_G`

### Utility Functions

| Function | Description |
|---|---|
| `ui.log(message)` | Prints to the console |
| `ui.mid(a, b, c)` | Returns the middle value of three numbers |

## 🧹 Cleaning

```bash
cd src
make clean
```

## 🤝 Contributing

Contributions are welcome! However, **please open an issue before submitting a PR** so changes can be discussed first.
Issues can be written in **Portuguese, Spanish, or English**.

Join our Discord: [Lupi](https://lupi.api.br/l/discord)

## 📝 License

This project is open source.

---
<div align="center">

**Made with ❤️ in Brazil 🇧🇷**  
**Reviewed and Updated by: [Hiriko](https://github.com/Hirikko) 🩷 and [Felipowsky](https://github.com/felipowsky) 🧡**

</div>
