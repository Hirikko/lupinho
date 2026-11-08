# Novas Funcionalidades e Sistema de Debug
Esta versão do Lupinho adiciona novas APIs compatíveis com jogos Lupi, suporte a diagnóstico de hardware, overlay de debug, integração nativa com o motor de áudio Furnace, controle virtual SNES e melhorias na infraestrutura Lua/Web.
## Novas funcionalidades
### Lua 5.5.1
O Lupinho utiliza Lua 5.5.1 como runtime Lua.

Isso permite maior compatibilidade com os jogos e scripts atuais do ecossistema Lupi, mantendo a execução Lua alinhada com a versão utilizada no projeto.

---
## Interface do Emulador
- **Favicon**: O ícone da aba do navegador foi atualizado para refletir a nova identidade visual oficial do projeto Lupi (disponível em `https://lupi.api.br/favicon.ico`).

### Controle Virtual SNES
O player web agora inclui um controle virtual no estilo Super Nintendo, que pode ser ativado pelo botão **CONTROLE: LIGADO/DESLIGADO**. 
Ele dispara eventos de teclado mapeados para as teclas do Lupinho, garantindo compatibilidade total com os jogos.

| Botão do Controle | Tecla Mapeada |
|---|---|
| D-Pad (Cima, Baixo, Esquerda, Direita) | W, S, A, D |
| A (Roxo) | J |
| B (Vermelho) | K |
| Y (Verde) | L |
| X (Azul) | M |
| L (Trigger Esquerdo) | G |
| R (Trigger Direito) | H |
| Select | Shift |
| Start | Enter |
### Filtros de Tela
Três filtros de renderização estão disponíveis no menu:
- **Padrão**: `image-rendering: pixelated` (pixels nítidos, ideal para pixel art).
- **Suave**: Aplica a suavização bilinear nativa do navegador (efeito semelhante a shaders de suavização como 2xSaI, com custo zero de processamento).
- **CRT**: Aplica um overlay com linhas de varredura e cintilação para simular monitores de tubo antigos.
### Carregamento e Reinício de Jogos
- Arraste e solte arquivos `.lupi` diretamente na página.
- O botão **REINICIAR JOGO** reinicia o jogo atual sem recarregar a página inteira (mantendo o arquivo em memória).

---
## Entrada de mouse

Foi adicionada a API:
```lua
ui.mouse()
```

Ela retorna cinco valores:
```lua
local x, y, buttons, wheel_x, wheel_y = ui.mouse()
```
### Retorno

| Valor     | Descrição                    |
| --------- | ---------------------------- |
| `x`       | posição horizontal do mouse  |
| `y`       | posição vertical do mouse    |
| `buttons` | estado dos botões do mouse   |
| `wheel_x` | movimento horizontal da roda |
| `wheel_y` | movimento vertical da roda   |

Os botões são representados por bits:
```text
bit 0 = botão esquerdo
bit 1 = botão direito
```

Exemplo:
```lua
local x, y, buttons = ui.mouse()

local left = (buttons & 1) ~= 0
local right = (buttons & 2) ~= 0
```
Atualmente os valores da roda (`wheel_x` e `wheel_y`) são retornados como `0`.

---
### Sistema de Áudio (Furnace)
Integração nativa com o motor Furnace via C++. Suporta:
- Reprodução de arquivos `.fur` comprimidos ou não.
- `music.loop(nome, fade)`: Inicia a música.
- `music.stop(fade)`: Para a música.
- `music.volume(vol, fade)`: Ajusta o volume geral (Master).
- `music.volume(vol, mascara_de_canal, fade)`: Ajusta canais individuais. Use `CH_1` até `CH_128` ou `CH_ALL` para aplicar a todos. O fade é processado por amostra (sample-accurate).

---
### `ui.stat(index)`
Retorna métricas do sistema em tempo real. Os índices agora estão alinhados com a documentação oficial do Lupi e com as necessidades do Debug Lab:
- `0`: Uso de memória Lua em bytes.
- `1`: Uso de CPU (aproximado em porcentagem do frame).
- `2`: Tempo de jogo (uptime) em segundos.
- `3`: Tempo do frame atual em milissegundos.
- `4`: Contador de frames.
- `5`: Memória consumida pelo Lua em KB.
- `6`: Uso atual dinâmico do WASM em KB (mapeado para a RAM do Lua).
- `7`: FPS Atual.
- `8`: Teto Máximo de memória do WebAssembly em KB (apenas Web).

---
## Entrada de texto e Peek
Foi adicionada a API `ui.readtext()`, que retorna o próximo caractere disponível no buffer de entrada de texto e **consome** um caractere da fila. 
É ideal para nomes de jogadores, menus, campos de texto, códigos, senhas e telas de configuração.
```lua
local char = ui.readtext()
if char then
    print(char)
end
```

Cada chamada consome um caractere do buffer.

Também foi adicionada a API ui.peektext(), que espia o próximo caractere do buffer sem consumi-lo. 
Isso permite que o jogo verifique qual é a próxima tecla antes de decidir se deve lê-la.
```lua
local next_char = ui.peektext()
if next_char == "\8" then
    -- O próximo caractere é um Backspace
end
```

Notas de Comportamento:

- O Backspace já está mapeado e envia o caractere de controle \8 (ASCII 8) para o buffer.
- O Enter (quebra de linha) ainda não está implementado no buffer de texto e está sendo analisado para uma atualização futura.

---
# Sprites.find

Foi adicionada compatibilidade para:
```lua
Sprites.find(path)
```

Exemplo:
```lua
local sprite = Sprites.find("player/sonic")
```

A busca suporta caminhos hierárquicos:
```text
player/sonic
enemies/badnik/crab
ui/menu/button
```

Além da busca pelo caminho completo, o Lupinho também consegue localizar o sprite pelo nome dentro da árvore `Sprites`.

Exemplo:
```lua
local sprite = Sprites.find("sonic")
```

Isso melhora a compatibilidade com diferentes organizações de sprites utilizadas pelos jogos.

---
# Palette.rgb

Foi adicionada:
```lua
Palette.rgb(r, g, b)
```

A função converte uma cor RGB para o índice de cor mais próximo da paleta Lupi.

Exemplo:
```lua
local color = Palette.rgb(255, 0, 0)
```

O resultado é um índice de cor da paleta.

A conversão utiliza distância entre as cores e procura a entrada mais próxima disponível na paleta.

---
# Palette.hex

Também está disponível:
```lua
Palette.hex(value)
```

Exemplo:
```lua
local red = Palette.hex(0xFF0000)
```

A função converte a cor RGB especificada para o índice correspondente na paleta Lupi.

---
# Compatibilidade com `pairs(Palette)`
As funções auxiliares `Palette.rgb` e `Palette.hex` foram implementadas de forma que não interfiram na enumeração tradicional da paleta.

Isso permite que jogos existentes continuem utilizando:
```lua
for idx, color in pairs(Palette) do
    ...
end
```

sem que as funções auxiliares sejam interpretadas como cores.

Esse comportamento é importante para manter compatibilidade com jogos que aplicam a paleta dinamicamente:
```lua
for idx, color in pairs(Palette) do
    ui.palset(idx - 1, color)
end
```

---
# Cache de sprites e assets

O Lupinho possui cache persistente de assets carregados durante a execução do jogo.

Quando um asset é utilizado pela primeira vez:
```text
SD → RAM → utilização
```

Nas utilizações seguintes:

```text
RAM → utilização
```

Isso evita recarregamentos desnecessários do armazenamento.
O cache é reiniciado quando o jogo é carregado ou reiniciado.

O sistema suporta até 256 arquivos em cache.

---
# Sistema de diagnóstico de hardware
O Lupinho possui um sistema de monitoramento das limitações de hardware previstas para o console Lupi.

O sistema coleta métricas por frame sem alterar o comportamento normal do jogo.

Entre as métricas monitoradas estão:
- bytes utilizados pelo drawlist;
- bytes utilizados no acesso ao SD;
- quantidade de transações no SD;
- carregamentos de sprites/assets;
- quantidade de comandos gráficos;
- comandos rejeitados pelo diagnóstico;
- limites configurados para cada recurso.

O objetivo é permitir que jogos sejam desenvolvidos e testados respeitando as características do hardware alvo.

---
# Limites monitorados

Os limites atualmente utilizados pelo diagnóstico são:

| Recurso | Limite |
|---|---:|
| Drawlist por frame | `18432 bytes` |
| SD por frame | `150000 bytes` |
| Transações SD por frame | `32` |
| Pixels máximos de sprite/asset | `46000` |
| Slots de paleta | `256` |
| Índice de transparência | `0` |

Esses valores são utilizados pelo sistema de diagnóstico para indicar situações que ultrapassam as capacidades previstas do hardware.

---
# Custos do Drawlist

O sistema também contabiliza o custo estimado dos comandos gráficos.

| Comando            |                          Custo |
| ------------------ | -----------------------------: |
| `line`             |                       12 bytes |
| `rect`             |                       12 bytes |
| `rectfill`         |                       12 bytes |
| `circ`             |                       12 bytes |
| `circfill`         |                       12 bytes |
| `trisfill`         |                       20 bytes |
| `spr` inicial      |                        8 bytes |
| `spr` subsequente  |                        4 bytes |
| `tile` inicial     |                        8 bytes |
| `tile` subsequente |                        4 bytes |
| `clip`             |                       12 bytes |
| `clip` reset       |                        4 bytes |
| `fillp`            |                       12 bytes |
| `fillp` reset      |                        4 bytes |
| `cls`              |                       16 bytes |
| `print`            | 16 + `ceil(tamanho / 4)` bytes |
| `camera`           |                        0 bytes |
| `palset`           |                        0 bytes |
Esses valores permitem identificar quais operações estão consumindo mais espaço do drawlist.

---
# Variáveis de controle de hardware/debug
Por padrão, a variável `LUPI_HARDWARE_CONSTRAINTS` já vem ativada.

O jogo pode habilitar o sistema de Limitações através das variáveis globais:
```lua
LUPI_HARDWARE_CONSTRAINTS = true
```

e Overlay das métricas:
```lua
LUPI_HARDWARE_DEBUG_OVERLAY = true
```

Exemplo:
```lua
LUPI_HARDWARE_CONSTRAINTS = true
LUPI_HARDWARE_DEBUG_OVERLAY = true
```
Ativa limitações e overlay.
## `LUPI_HARDWARE_CONSTRAINTS`

Controla a coleta das métricas de hardware.
```lua
LUPI_HARDWARE_CONSTRAINTS = true
```

Quando habilitado, o Lupinho acompanha os recursos utilizados pelo jogo durante cada frame.

Para desabilitar:
```lua
LUPI_HARDWARE_CONSTRAINTS = false
```

---
## `LUPI_HARDWARE_DEBUG_OVERLAY`
Controla a exibição do painel de diagnóstico na tela.
```lua
LUPI_HARDWARE_DEBUG_OVERLAY = true
```

Funciona de forma parecida com ui.rect.

x0 Controla posição horizontal do canto superior esquerda.
x1 controla posição horizontal canto inferior direito.
y0 Controla posição vertical do canto superior esquerdo.
y1 Controla posição vertical do canto inferior direito.
Alpha controla Transparência.
Col controla índice da cor.
```lua
LUPI_HARDWARE_CONSTRAINTS = true
LUPI_LUA_WATCHDOG = true
LUPI_HARDWARE_DEBUG_OVERLAY = true
LUPI_HARDWARE_DEBUG_OVERLAY_RECT = { x0 = 0, y0 = 0, x1 = 164, y1 = 270, col = 2 }
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.5
```
Com o overlay habilitado, as métricas do frame atual são exibidas diretamente sobre o jogo.

Para desabilitar:
```lua
LUPI_HARDWARE_DEBUG_OVERLAY = false
```

---
# Overlay de hardware
Quando habilitado, o overlay apresenta informações semelhantes a:
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

As informações permitem identificar rapidamente os principais consumidores de recursos.

Por exemplo:
```text
drawlist: 6168 / 18432
```

significa que o frame utilizou `6168` bytes de um limite de `18432`.

Da mesma forma:
```text
sd: 120000 / 150000b
```

indica que o frame utilizou `120000` bytes do limite de `150000`.

---
# Diagnóstico de excesso de recursos

Quando um limite é ultrapassado, o overlay permite identificar o problema.

Exemplo:
```text
sd tx: 77 / 32
sd tx over: 45
```

Nesse caso, o jogo realizou 77 transações no SD durante o frame, ultrapassando o limite de 32.

O diagnóstico também permite identificar excesso de:
- drawlist;
- tráfego do SD;
- transações do SD;
- carregamentos de assets;
- comandos gráficos.

O sistema foi projetado principalmente como ferramenta de desenvolvimento e otimização.

---
# Cache e diagnóstico do SD

O diagnóstico diferencia o carregamento inicial de um asset de reutilizações posteriores.

Primeiro uso:
```text
SD → RAM
```

Nesse caso são contabilizados:
- bytes lidos;
- transação;
- carregamento do asset.

Próximos usos:
```text
RAM
```

O asset não precisa ser novamente carregado do SD.

Isso permite detectar jogos que estão realizando leituras excessivas do armazenamento por frame.

---
# Estrutura modular

A implementação foi organizada em módulos para facilitar manutenção e evolução do projeto:
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
Responsável pela infraestrutura da API Lua e registro das funções disponíveis ao jogo.

### `lupi_functions.c`
Concentra funcionalidades específicas de compatibilidade Lupi, incluindo:

- `Sprites.find`;
- `Palette.rgb`;
- `Palette.hex`;
- entrada de texto;
- `ui.mouse`;
- `ui.readtext`;
- caches relacionados a essas funcionalidades.

### `hardware_constraints.c`
Responsável pelo sistema de métricas e diagnóstico das limitações de hardware.

### `ui.c`
Responsável pela interface gráfica e apresentação do framebuffer.

---
# Exemplo de configuração para desenvolvimento

Um jogo pode habilitar o diagnóstico durante o desenvolvimento:
```lua
require("sprites")
require("player")

LUPI_HARDWARE_CONSTRAINTS = true
LUPI_HARDWARE_DEBUG_OVERLAY = true
LUPI_HARDWARE_DEBUG_OVERLAY_X = 10
LUPI_HARDWARE_DEBUG_OVERLAY_Y = 1
LUPI_HARDWARE_DEBUG_OVERLAY_ALPHA = 0.45

function init()
    -- inicialização do jogo
end

function update(frame)
    -- lógica do jogo
end
```

Para uma execução normal, o overlay pode ser desabilitado:
```lua
LUPI_HARDWARE_CONSTRAINTS = false
LUPI_HARDWARE_DEBUG_OVERLAY = false
```

---
# Objetivo do sistema

O objetivo dessas ferramentas não é apenas detectar erros, mas permitir que jogos sejam desenvolvidos levando em consideração as características reais do hardware Lupi.

Durante o desenvolvimento, o overlay pode ser utilizado para:
1. identificar frames pesados;
2. detectar excesso de operações gráficas;
3. identificar leituras excessivas do SD;
4. encontrar carregamentos repetidos de assets;
5. acompanhar o consumo do drawlist;
6. otimizar jogos antes de executá-los no hardware real.

Assim, o Lupinho funciona não apenas como simulador, mas também como uma ferramenta de desenvolvimento e validação para jogos Lupi.
