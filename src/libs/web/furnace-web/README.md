# 🎮 Lupinho
### Criado por: [Juneira](https://github.com/juneira)

**Lupinho** é um Simulador para o Console Brasileiro **Lupi**.
Funciona diretamente no seu navegador usando WebAssembly, permitindo que você
crie e jogue jogos usando a API de programação Lua.

### 🕹️ [Teste a Demo](https://lupinho.juneira.com/)

> O jogo de demonstração é ["caio-pernocas"](https://github.com/lupi-org-br/caio-pernocas).
> Use W, A, S, D para mover e K como botão de ação. Você também pode usar um controle.

---

## Simulador vs Emulador

### **Emulador**
> Tenta recriar o hardware original de forma fiel e em nível de baixo nível
> (processador, memória, chips gráficos, etc.). O objetivo é rodar o software
> original feito para aquela máquina física exata, processando instrução por
> instrução da CPU original.

### **Simulador**
> Recria o comportamento e o ambiente de execução de um sistema, muitas vezes
> em um nível mais abstrato ou moderno, sem a necessidade de replicar o
> hardware original fisicamente.

No caso do Lupinho, ele simula o console Lupi diretamente sobre tecnologias
modernas (WebAssembly e Raylib), focando em rodar **jogos criados
especificamente para essa API**, e não em rodar ROMs de um hardware físico
pré-existente.

Você pode ter mais informações sobre os **Limites** nesta issue:
[**LIMITAÇÕES**](https://github.com/lupi-org-br/lupinho/issues/12).

---

## ✨ Recursos

- 🌐 **Funciona no navegador** — WebAssembly, zero instalação para o jogador
- 📜 **Scripts em Lua 5.5.1** — mesma linguagem do console
- 🎨 **Gráficos 2D** — texto, linhas, retângulos, círculos, triângulos
- 🖼️ **Sprites e Tiles** — bitmaps com suporte a flip
- 🗺️ **Tilemap** — grandes mapas com scrolling de câmera
- 🎮 **Controle, teclado e mouse** — `ui.btn`, `ui.btnp`, `ui.mouse`
- ⌨️ **Entrada de texto** — `ui.readtext`, `ui.peektext`
- 🔊 **Áudio nativo com Furnace** — música `.fur` com fade por amostra e volume por canal
- ⚡ **60 FPS** — 480×270 px, paleta indexada BGR555
- 🧪 **Hardware Limits Debug** — `make debug` embute overlay e watchdog para validar jogos antes do hardware real

---

## 🛠️ Tecnologias Utilizadas

| Componente | Tecnologia |
|---|---|
| Linguagem do núcleo | C99 / C++14 (adapter Furnace) |
| Compilador | Emscripten (emcc / em++) |
| Gráficos | Raylib |
| Scripting | Lua 5.5.1 |
| Áudio | Furnace engine |
| Plataforma | WebAssembly |
| Tela | 480×270 px @ 60 FPS |

---

## 📁 Estrutura do Projeto
```
lupinho/  
├── site/ # Página web do player  
│ ├── index.html  
│ └── dist/ # ← saída de make web / debug / production  
│ ├── game.html  
│ ├── game.js  
│ └── game.wasm  
├── dist/ # ← binários de outras plataformas  
│ ├── lupinho # make linux  
│ ├── lupinho-static # make static  
│ └── webgame/ # make web-game (HTML com jogo embutido)  
├── src/  
│ ├── main.c # Loop principal, cursor, overlay de hardware  
│ ├── ui.c / ui.h # Framebuffer, paleta, primitivas, sprites, fonte  
│ ├── lua_api.c / .h # Bindings Lua ↔ ui.*  
│ ├── lupi_functions.c # Sprites.find, Palette.rgb/hex, mouse, readtext  
│ ├── hardware_constraints.c # Watchdog Lua + métricas de hardware  
│ ├── lupi_music.c # music.loop / music.stop / music.volume  
│ ├── types.h # Structs de desenho  
│ ├── font.h # Fonte bitmap embutida  
│ ├── zip.c / zip.h # Leitura de .lupi (build nativa)  
│ ├── music/  
│ │ ├── fur_player.cpp # Adapter C++ → Furnace (C ABI em fur_player.h)  
│ │ ├── fur_player.h  
│ │ ├── fur_player_stub.c # Stub para build nativa sem Furnace  
│ │ └── build_furnace.sh # Regenera libs/web/furnace-web/  
│ ├── Makefile  
│ ├── furnace/ # ← gitignored (1 GB, só para rebuildar libs)  
│ └── libs/  
│ ├── linux/ # Lua 5.5.1 + Raylib nativa  
│ └── web/  
│ ├── lua-web/ # liblua.web.a (versionado)  
│ ├── raylib-web/ # libraylib.web.a + minshell.html (versionado)  
│ └── furnace-web/ # libfurengine.a + extern/*.a + fur_player-web.o  
├── game-example/  
├── README.md  
└── README_en.md
```
### As 3 camadas de compilação
```
┌──────────────────────────────────────────────────────────────┐  
│ 1. BINÁRIOS PRÉ-COMPILADOS (libs/web/)                       │  
│ Raylib, Lua, Furnace — versionados, usuário não recompila    │  
├──────────────────────────────────────────────────────────────┤  
│ 2. ADAPTERS                                                  │               
│ fur_player.cpp (C++ → C ABI), zip.c                          │  
│ → versionados como artefato (.o / .c)                        │  
├──────────────────────────────────────────────────────────────┤  
│ 3. CÓDIGO DO LUPINHO                                         │  
│ main.c, ui.c, lua_api.c, lupi_functions.c, ...               │  
│ → recompilam sempre, é onde você mexe no dia-a-dia           │  
└──────────────────────────────────────────────────────────────┘
```

**Consequência:** para `make web` / `debug` / `production`, você **não precisa**
do fonte do Furnace (~1 GB). Basta ter `src/libs/web/furnace-web/` no
repositório. Só precisa do fonte quem quiser **editar `fur_player.cpp`** para otimizar, criar suporte a mais chips e sistemas ou **atualizar a versão do Furnace**.

---

## 🚀 Primeiros Passos

### Pré-requisitos

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) instalado e ativado
- Linux recomendado;
	   no Windows, use Ubuntu via WSL
- `zlib`, `X11`, `OpenGL` (para build nativa opcional)

### Compilando

```bash
cd src

# Versão de desenvolvimento (com hardware limits, overlay e debug)
make debug

# Versão de produção (otimizada, sem debug)
make production

# Versão de desenvolvimento sem overlay de hardware
make web

# Embute um jogo no HTML (jogável direto do arquivo)
make web-game GAME_PATH=meu-jogo.lupi

# Binário Linux nativo (para profiling com perf)
make linux
make static   # versão estática portátil

# Confere se as libs web existem (roda automaticamente em toda build)
make check-libs
```
	### Saídas

|Comando|Saída|
|---|---|
|`make web` / `debug` / `production`|`site/dist/game.{html,js,wasm}`|
|`make web-game GAME_PATH=x.lupi`|`dist/webgame/game.{html,js,wasm,data}`|
|`make linux`|`dist/lupinho`|
|`make static`|`dist/lupinho-static`|

### Executando

Depois de `make web` (ou `debug` / `production`), basta servir `site/`:
```
cd site
python3 -m http.server 8080
# ou
npx serve
```

Abra `http://localhost:8080` no navegador.

> **Não é mais preciso copiar `dist/` para `site/dist/`.** 
> O Makefile já escreve direto em `site/dist/`.

---

## 🧪 Build de Debug: Hardware Limits

O alvo `make debug` compila com `-DDEBUG_MODE -DLUPI_HARDWARE_DEBUG` e adiciona:

- **Overlay** com métricas por frame (drawlist, SD bytes, transações, watchdog)
    
- **Watchdog Lua** — aborta `update()` se ultrapassar 55 000 instruções/frame
    
- **Limites de hardware** aplicados às operações gráficas e de SD
    
Ativar/desativar em tempo de execução (no `game.lua` ou `main.lua`):
```
LUPI_HARDWARE_CONSTRAINTS = true           -- true Ativa / false Desativa limitações
LUPI_HARDWARE_DEBUG_OVERLAY = true         -- Controle do overlay
LUPI_HARDWARE_DEBUG_OVERLAY_X = 13         -- Posição horizontal
LUPI_HARDWARE_DEBUG_OVERLAY_Y = 13         -- Posição vertical
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.45   -- Transparência
```
### Limites monitorados

|Recurso|Limite|
|---|---|
|Drawlist por frame|18 432 bytes|
|SD por frame|150 000 bytes|
|Transações SD por frame|32|
|Sprites carregados por frame|32|
|Instruções Lua por frame|55 000|

O overlay mostra **consumo / limite** por categoria (spr, tile, rect, circ,  
tris, print, cls, clip, fillp) e marca em vermelho qualquer estouro.

---

## 🔊 Áudio via Furnace

O Lupinho usa o motor [Furnace](https://github.com/tildearrow/furnace) para  
tocar arquivos `.fur` (formato tracker). O adapter `src/music/fur_player.cpp`  
expõe uma ABI C mínima (`fur_player.h`), e o motor compilado vive em  
`src/libs/web/furnace-web/`.

### Bindings Lua

|Função|Descrição|
|---|---|
|`music.loop(path, fade)`|Inicia a música (repete infinitamente). `fade` é em segundos|
|`music.stop(fade)`|Para com fade-out|
|`music.volume(vol, fade)`|Volume master (0.0–1.0) com fade|
|`music.volume(vol, channel_mask, fade)`|Volume por canal (`CH_1`..`CH_128` ou `CH_ALL`)|

Fades são **sample-accurate** (processados por amostra, não por frame).

### Estrutura `libs/web/furnace-web/`
```
furnace-web/
├── libfurengine.a        # Engine Furnace (13 MB)
├── fur_player-web.o      # Adapter C++ já compilado (33 KB)
├── furengine-libs.txt    # Manifesto de libs lidas pelo Makefile
└── extern/               # Dependências estáticas (fftw, fmt, sndfile, flac, ...)
```
### Rebuildar o Furnace (avançado)

Só é necessário se você mexer em `fur_player.cpp`, adicionar chips e sistemas ou atualizar a versão do Furnace. 
O fonte (~1 GB) é **gitignored**.
```
# Restaure o fonte (via git ou clone)
git clone --recursive https://github.com/Hirikko/furnace src/furnace

# Regenere os artefatos em libs/web/furnace-web/
cd src
./build_furnace.sh web ./furnace libs/web/furnace-web

# Opcional: apagar o fonte novamente (o Makefile não precisa dele)
rm -rf furnace
```
O script usa CMake + Ninja + `emcmake` e pode demorar **10–20 min**.

> ⚠️ **Sem `src/furnace/`, `make` não recompila `fur_player.cpp`.**  
> Se você editar o arquivo sem restaurar o fonte, o `.o` pré-compilado  
> continua valendo e suas mudanças **não** entram no build. Fique atento  
> ao aviso do Makefile.

---

## 📖 API Lua

Os jogos são escritos em Lua 5.5.1. Seu script deve definir `update()`,  
chamada a cada quadro (60 vezes por segundo).
```
function update(frame)
    -- lógica e desenho aqui
end
```
### Paleta de Cores
O Lupinho usa 256 cores no formato **BGR555** (5 bits por canal).

|Função|Descrição|
|---|---|
|`ui.palset(index, bgr555)`|Define cor da paleta no índice 0–255|
|`Palette.rgb(r, g, b)`|Converte RGB888 para o índice mais próximo da paleta|
|`Palette.hex(0xRRGGBB)`|Converte RGB888 em forma hex para índice|

`Palette.rgb` e `Palette.hex` **não interferem** em `pairs(Palette)`, então o  
padrão tradicional continua funcionando:
```
for idx, color in pairs(Palette) do
    ui.palset(idx - 1, color)
end
```
### Desenho

Todas as funções recebem **índices de paleta** (0–255) como cor.

|Função|Descrição|
|---|---|
|`ui.rect(x1, y1, x2, y2, color)`|Contorno de retângulo|
|`ui.rectfill(x1, y1, x2, y2, color)`|Retângulo preenchido|
|`ui.draw_rect(x, y, w, h, filled, color)`|Retângulo (contorno ou preenchido)|
|`ui.circfill(x, y, r, color)`|Círculo preenchido|
|`ui.draw_circle(cx, cy, r, filled, color, border, border_color)`|Círculo com borda opcional|
|`ui.trisfill(x1, y1, x2, y2, x3, y3, color)`|Triângulo preenchido|
|`ui.line(x1, y1, x2, y2, color)`|Linha|
|`ui.print(text, x, y, color)`|Texto com fonte bitmap|

### Tela

|Função|Descrição|
|---|---|
|`ui.cls(color)`|Limpa a tela|
|`ui.camera(x, y)` / `ui.camera()`|Define / reseta câmera|
|`ui.clip(x, y, w, h)` / `ui.clip()`|Define / reseta clipping|
|`ui.fillp(b1, ..., b8)` / `ui.fillp()`|Define / reseta pattern 8×8|

### Sprites e Tiles

|Função|Descrição|
|---|---|
|`Sprites.find(path)`|Busca sprite por caminho ou nome|
|`ui.spr(sprite_table, x, y, flipped)`|Desenha sprite|
|`ui.tile(tileset_table, tile_index, x, y)`|Desenha tile (bit 10 inverte horizontal)|

### Mapa

|Função|Descrição|
|---|---|
|`ui.map(layer_table, cam_x, cam_y)`|Desenha camada de tilemap|

### Entrada

|Função|Descrição|
|---|---|
|`ui.btn(button, pad)`|Botão pressionado (teclado ou controle)|
|`ui.btnp(button, pad)`|Botão acabou de ser pressionado (edge)|
|`ui.mouse()`|Retorna `x, y, buttons, wheel_x, wheel_y`|
|`ui.readtext()`|Lê e consome o próximo caractere do buffer|
|`ui.peektext()`|Espia o próximo caractere (não consome)|

Bits de `ui.mouse()` em `buttons`:

text

bit 0 = botão esquerdo
bit 1 = botão direito

> Os campos `wheel_x` e `wheel_y` atualmente retornam `0`. A correção para  
> ler `GetMouseWheelMove()` está no roadmap.

Constantes de botões: `UP`, `DOWN`, `LEFT`, `RIGHT`, `BTN_Z`, `BTN_Q`,  
`BTN_E`, `BTN_F`, `BTN_G`.

### Áudio

|Função|Descrição|
|---|---|
|`music.loop(path, fade)`|Inicia música `.fur` em loop|
|`music.stop(fade)`|Para música|
|`music.volume(vol, fade)`|Volume master|
|`music.volume(vol, mask, fade)`|Volume por canal (`CH_1`..`CH_128`, `CH_ALL`)|

### Utilitários

|Função|Descrição|
|---|---|
|`ui.log(message)`|Loga no console do navegador|
|`ui.mid(a, b, c)`|Valor do meio entre três números|
|`ui.stat(index)`|Métricas de sistema (ver abaixo)|

#### `ui.stat(index)`

|Índice|Retorno|
|---|---|
|`0`|FPS atual|
|`1`|Uptime em segundos|
|`2`|Tempo do frame atual em ms|
|`3`|Contador de frames|
|`4`|Memória Lua em MB|
|`5`|Memória total do WebAssembly em MB|

---

## 🧹 Limpeza

Remove objetos e saídas (pede confirmação):

bash

cd src
make clean

Remove artefatos do Furnace (desabilitado por padrão; exige rebuild com  
`build_furnace.sh` depois):

bash

# Descomente no Makefile primeiro
make clean-furnace

---

## 🤝 Contribuindo

Contribuições são bem-vindas!  
**Por favor, abra uma issue antes de enviar um PR** para discutirmos a mudança.  
Issues podem ser escritas em **português, espanhol ou inglês**.

Junte-se ao Discord:

### 💬 [Lupi](https://lupi.api.br/l/discord)

---

## 📝 Licença

Código aberto.

---

<div align="center">

**Feito com ❤️ no Brasil 🇧🇷**  
**Revisado e Atualizado por: [Hiriko](https://github.com/Hirikko) 🩷 e [Felipowsky](https://github.com/felipowsky) 🧡**

</div> ```
