
# ====================
# Lupinho - Makefile
# Integração Web com Furnace e Raylib
# Compatível com Linux e Ubuntu no WSL
# ====================

# Diretório absoluto deste Makefile
SRC_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
cdummy :=

# ====================
# COMPILADORES
# ====================

CC = emcc
CXX = em++

CC_NATIVE = gcc
# ====================
# FURNACE
# ====================

# Fonte do Furnace (só necessária para REBUILDAR a lib, não para compilar o Lupinho)
FURNACE_SRC := $(SRC_DIR)/furnace

# Pasta versionada com os artefatos pré-compilados
FURNACE_WEB_DIR := $(SRC_DIR)/libs/web/furnace-web
FURNACE_LIB := $(FURNACE_WEB_DIR)/libfurengine.a
FURNACE_DEPS_FILE := $(FURNACE_WEB_DIR)/furengine-libs.txt

# Lê o manifesto de dependências diretamente (já contém paths relativos a src/)
FURNACE_DEPS := $(shell cat "$(FURNACE_DEPS_FILE)" 2>/dev/null)

FUR_PLAYER_OBJ := $(FURNACE_WEB_DIR)/fur_player-web.o

# Headers do Furnace para compilar fur_player.cpp
# (usados quando FURNACE_SRC existe; caso contrário, fur_player-web.o é versionado)
ifneq ($(wildcard $(FURNACE_SRC)/src/engine/engine.h),)
    FUR_CXXFLAGS := -std=c++14 -DHAVE_SNDFILE \
        -I$(FURNACE_SRC)/src \
        -I$(FURNACE_SRC)/src/engine \
        -I$(FURNACE_SRC)/extern/fmt/include \
        -I$(FURNACE_SRC)/extern/blip_buf \
        -I$(FURNACE_SRC)/extern/libsndfile-modified/include \
        -I$(SRC_DIR)/libs/web/raylib-web
else
    # Sem fonte do Furnace, assume que fur_player-web.o está versionado
    FUR_CXXFLAGS := -std=c++14 -I$(SRC_DIR)/libs/web/raylib-web
endif

FUR_LINK_FLAGS := -s USE_ZLIB=1 -s DISABLE_EXCEPTION_CATCHING=0

FUR_LIBS := $(FURNACE_LIB) $(FURNACE_DEPS) $(FUR_LINK_FLAGS)

# ====================
# RAYLIB
# ====================

RAYLIB_INCLUDE_DIR := $(SRC_DIR)/libs/web/raylib-web
RAYLIB_LIB := $(RAYLIB_INCLUDE_DIR)/libraylib.web.a

RAYLIB_INCLUDE := -I$(RAYLIB_INCLUDE_DIR)

RAYLIB_LINUX_STATIC := $(SRC_DIR)/libs/linux/raylib/src/libraylib.a

# ====================
# LUA
# ====================

LUA_WEB_INCLUDE_DIR := $(SRC_DIR)/libs/linux/lua-5.5.1/src
LUA_WEB_LIB := $(SRC_DIR)/libs/web/lua-web/liblua.web.a

LUA_55_INCLUDE_DIR := $(SRC_DIR)/libs/linux/lua-5.5.1/src
LUA_55_LIB := $(LUA_55_INCLUDE_DIR)/liblua.a

LUA_WEB_INCLUDE := -I$(LUA_WEB_INCLUDE_DIR)
LUA_55_INCLUDE := -I$(LUA_55_INCLUDE_DIR)

# ====================
# ARQUIVOS-FONTE
# ====================

SRC := main.c ui.c lua_api.c lupi_functions.c hardware_constraints.c lupi_music.c

SRC_NATIVE := main.c ui.c lua_api.c zip.c \
	lupi_functions.c hardware_constraints.c lupi_music.c \
	music/fur_player_stub.c

WEB_OBJECTS := $(SRC:.c=.web.o)
WEB_DEBUG_OBJECTS := $(SRC:.c=.debug.web.o)
PROD_OBJECTS := $(SRC:.c=.prod.web.o)

WEB_GAME_OBJECTS := $(filter-out main.web.o,$(WEB_OBJECTS)) main.web-game.o

# ====================
# FLAGS DE COMPILAÇÃO
# ====================

CFLAGS := -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces

OPTIMIZATION := -O2

# ====================
# FLAGS DE COMPILAÇÃO WEB
# ====================

EM_COMPILE_FLAGS := -DPLATFORM_WEB

# ====================
# FLAGS DE LINK WEB
# ====================

EM_LINK_FLAGS := \
	-s USE_GLFW=3 \
	-s USE_ZLIB=1 \
	-s TOTAL_MEMORY=67108864 \
	-s STACK_SIZE=1048576 \
	-s FORCE_FILESYSTEM=1 \
	--shell-file $(RAYLIB_INCLUDE_DIR)/minshell.html \
	-s EXPORTED_FUNCTIONS='["_main", "_lupi_load_game", "_malloc", "_free"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall", "FS"]' \
	-Wl,--gc-sections

# ====================
# DEBUG E PRODUÇÃO
# ====================

DEBUG_FLAGS := -DDEBUG_MODE -DLUPI_HARDWARE_DEBUG

# Separado em dois porque -s só é válido no link.
# -D* entra na compilação (define código condicional nos .c),
# -s* entra só no link (configura o runtime JS do Emscripten).
PROD_CFLAGS := -O3 -DNDEBUG -DPRODUCTION

PROD_LDFLAGS := -O3 \
	-s ASSERTIONS=0 \
	-s ELIMINATE_DUPLICATE_FUNCTIONS=1

# ====================
# BIBLIOTECAS NATIVAS LINUX
# ====================

NATIVE_INCLUDE := $(LUA_55_INCLUDE)

NATIVE_LIBS := -lraylib $(LUA_55_LIB) \
	-lm -ldl -lpthread -lX11 -larchive

STATIC_LIBS_FLAGS := \
	-Wl,-Bstatic \
	$(RAYLIB_LINUX_STATIC) \
	$(LUA_55_LIB) \
	-Wl,-Bdynamic \
	-lm -ldl -lpthread -lX11 -larchive

# ====================
# DIRETÓRIOS DE SAÍDA
# ====================

# Cada build tem um diretório próprio. As variáveis _DIR guardam a pasta
# e as variáveis sem sufixo guardam o arquivo .html principal.
WEB_OUTPUT_DIR       := $(SRC_DIR)/../site/dist
WEB_OUTPUT           := $(WEB_OUTPUT_DIR)/game.html

WEB_GAME_OUTPUT_DIR  := $(SRC_DIR)/../dist/webgame
WEB_GAME_OUTPUT      := $(WEB_GAME_OUTPUT_DIR)/game.html

LINUX_OUTPUT_DIR     := $(SRC_DIR)/../dist
LINUX_OUTPUT         := $(LINUX_OUTPUT_DIR)/lupinho
LINUX_STATIC_OUTPUT  := $(LINUX_OUTPUT_DIR)/lupinho-static

# ====================
# COMPILAÇÃO DOS ARQUIVOS C PARA WEB
# ====================

%.web.o: %.c
	$(CC) $(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
                $(CFLAGS) $(EM_COMPILE_FLAGS) $(OPTIMIZATION) \
                -c $< -o $@

%.debug.web.o: %.c
	$(CC) $(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
                $(CFLAGS) $(EM_COMPILE_FLAGS) $(DEBUG_FLAGS) $(OPTIMIZATION) \
                -c $< -o $@

production: check-libs $(PROD_OBJECTS) $(FUR_PLAYER_OBJ)
	@mkdir -p "$(dir $(WEB_OUTPUT))"
	$(CXX) $(PROD_OBJECTS) $(FUR_PLAYER_OBJ) \
		-o $(WEB_OUTPUT) \
		$(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
		$(EM_LINK_FLAGS) $(PROD_LDFLAGS) \
		$(LUA_WEB_LIB) $(RAYLIB_LIB) $(FUR_LIBS)

# ====================
# COMPILAÇÃO DO ADAPTADOR FURNACE C++
# ====================
ifeq ($(wildcard $(FURNACE_SRC)/src/engine/engine.h),)
    # Sem fonte do Furnace: .o é tratado como artefato pré-compilado
    $(FUR_PLAYER_OBJ):
	@echo "Usando fur_player-web.o pré-compilado."
	@test -f "$@" || (echo "ERRO: $@ não encontrado. Regenere-o com ./build_furnace.sh"; exit 1)
	@if [ music/fur_player.cpp -nt "$@" ]; then \
		echo ""; \
		echo "AVISO: music/fur_player.cpp é mais novo que $@."; \
		echo "       Edições neste arquivo NÃO entrarão no build."; \
		echo "       Para recompilar, restaure src/furnace/ e rode build_furnace.sh."; \
		echo ""; \
	fi
else
    # Com fonte do Furnace: recompila normalmente
    $(FUR_PLAYER_OBJ): music/fur_player.cpp music/fur_player.h
		$(CXX) $(FUR_CXXFLAGS) -Wall $(OPTIMIZATION) -c $< -o $@
endif
# ====================
# VERIFICAÇÃO DAS LIBS
# ====================

# Confere se as libs pré-compiladas necessárias existem.
# Roda automaticamente como dependência de todas as builds (web, debug,
# production, web-game, linux, static).
check-libs:
	@missing=""; \
	[ -f "$(FURNACE_LIB)" ]       || missing="$$missing  - $(FURNACE_LIB)\n"; \
	[ -f "$(FURNACE_DEPS_FILE)" ] || missing="$$missing  - $(FURNACE_DEPS_FILE)\n"; \
	[ -f "$(FUR_PLAYER_OBJ)" ]    || missing="$$missing  - $(FUR_PLAYER_OBJ)\n"; \
	[ -f "$(RAYLIB_LIB)" ]        || missing="$$missing  - $(RAYLIB_LIB)\n"; \
	[ -f "$(LUA_WEB_LIB)" ]       || missing="$$missing  - $(LUA_WEB_LIB)\n"; \
	if [ -n "$$missing" ]; then \
		echo ""; \
		echo "=========================================="; \
		echo " Bibliotecas faltando"; \
		echo "=========================================="; \
		echo ""; \
		printf "$$missing"; \
		echo ""; \
		echo "Se for o Furnace, regenere com:"; \
		echo "  ./build_furnace.sh web ./furnace libs/web/furnace-web"; \
		echo ""; \
		echo "Se for Raylib ou Lua, verifique se o repositório"; \
		echo "está completo (git status, git checkout)."; \
		exit 1; \
	fi
	@echo "Libs OK (Furnace, Raylib, Lua)."

# ====================
# WEB - DESENVOLVIMENTO SEM DEBUG DE HARDWARE
# ====================

web: check-libs $(WEB_OBJECTS) $(FUR_PLAYER_OBJ)
	@mkdir -p "$(WEB_OUTPUT_DIR)"
	$(CXX) $(WEB_OBJECTS) $(FUR_PLAYER_OBJ) \
		-o $(WEB_OUTPUT) \
		$(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
		$(EM_LINK_FLAGS) $(OPTIMIZATION) \
		$(LUA_WEB_LIB) $(RAYLIB_LIB) $(FUR_LIBS)

# ====================
# WEB - DEBUG DE HARDWARE
# ====================

debug: check-libs $(WEB_DEBUG_OBJECTS) $(FUR_PLAYER_OBJ)
	@mkdir -p "$(dir $(WEB_OUTPUT))"
	$(CXX) $(WEB_DEBUG_OBJECTS) $(FUR_PLAYER_OBJ) \
                -o $(WEB_OUTPUT) \
                $(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
                $(EM_LINK_FLAGS) $(DEBUG_FLAGS) $(OPTIMIZATION) \
                $(LUA_WEB_LIB) $(RAYLIB_LIB) $(FUR_LIBS)

# ====================
# WEB-GAME - JOGO EMBUTIDO
# ====================

main.web-game.o: main.c
	$(CC) $(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
		$(CFLAGS) $(EM_COMPILE_FLAGS) $(OPTIMIZATION) \
		-DAUTO_LOAD_GAME -c $< -o $@

web-game: check-libs $(WEB_GAME_OBJECTS) $(FUR_PLAYER_OBJ)
	@if [ -z "$(GAME_PATH)" ]; then \
		echo "ERRO: GAME_PATH é obrigatório."; \
		echo "Uso: make web-game GAME_PATH=game.lupi"; \
		exit 1; \
	fi

	@echo "Extraindo $(GAME_PATH)..."
	@rm -rf /tmp/lupi-web-game
	@mkdir -p /tmp/lupi-web-game
	@unzip -q "$(GAME_PATH)" -d /tmp/lupi-web-game -x "*.git*" "*.lupi"

	@mkdir -p "$(dir $(WEB_GAME_OUTPUT))"

	$(CXX) $(WEB_GAME_OBJECTS) $(FUR_PLAYER_OBJ) \
		-o $(WEB_GAME_OUTPUT) \
		$(RAYLIB_INCLUDE) $(LUA_WEB_INCLUDE) \
		$(EM_LINK_FLAGS) $(OPTIMIZATION) \
		$(LUA_WEB_LIB) $(RAYLIB_LIB) $(FUR_LIBS) \
		--preload-file /tmp/lupi-web-game@/loaded_game

	@echo "Build concluído: $(WEB_GAME_OUTPUT)"

# ====================
# LUA WEB
# ====================

lua-web:
	./libs/web/lua-web/build_lua_web.sh

# ====================
# LINUX NATIVO
# ====================

linux: $(SRC_NATIVE)
	@mkdir -p "$(dir $(LINUX_OUTPUT))"
	$(CC_NATIVE) $(SRC_NATIVE) \
		-o $(LINUX_OUTPUT) \
		$(NATIVE_INCLUDE) $(CFLAGS) \
		$(NATIVE_LIBS) $(OPTIMIZATION)

# ====================
# LINUX ESTÁTICO
# ====================

static: $(SRC_NATIVE) $(RAYLIB_LINUX_STATIC)
	@mkdir -p "$(dir $(LINUX_STATIC_OUTPUT))"
	$(CC_NATIVE) $(SRC_NATIVE) \
		-o $(LINUX_STATIC_OUTPUT) \
		$(NATIVE_INCLUDE) $(CFLAGS) \
		$(STATIC_LIBS_FLAGS) $(OPTIMIZATION) -s
# ====================
# LIMPEZA
# ====================

# Remove objetos compilados e saídas das builds do Lupinho.
#
# O QUE É REMOVIDO:
#   - src/*.web.o, src/main.web-game.o       → objetos compilados
#   - site/dist/game.{html,js,wasm,data}     → saída de make web/debug/production
#   - dist/webgame/game.{html,js,wasm,data}  → saída de make web-game
#   - dist/lupinho, dist/lupinho-static      → saída de make linux/static
#
# O QUE NÃO É REMOVIDO:
#   - Código-fonte, Makefile, scripts
#   - src/libs/web/furnace-web/*              → artefatos versionados do Furnace
#   - src/libs/web/raylib-web/*               → Raylib pré-compilado
#   - src/libs/web/lua-web/*                  → Lua pré-compilado
#   - src/libs/linux/*                        → libs nativas
#   - src/furnace/                            → fonte do Furnace (gitignored)
#   - src/music/furnace-build-web/            → build intermediária (gitignored)
#   - site/index.html                         → página HTML da raiz
#   - As pastas site/dist/ e dist/ em si     → só o conteúdo é limpo
#   - .keep files                             → marcadores git
#   - sprites/, music/*.fur, tilemap/         → assets do jogo
#
# Para remover também os artefatos do Furnace, use 'make clean-furnace'.
clean:
	@echo ""
	@echo "Isso vai remover:"
	@echo "  - Objetos compilados em src/ (*.web.o, main.web-game.o)"
	@echo "  - Saídas em site/dist/ e dist/webgame/"
	@echo "  - Binários em dist/ (lupinho, lupinho-static)"
	@echo ""
	@echo "Não vai mexer em: fontes, libs, assets, Furnace."
	@echo ""
	@printf "Continuar? [s/N] "; \
	read resposta; \
	case "$$resposta" in \
		[sS]|[sS][iI][mM]) \
			rm -f $(WEB_OBJECTS); \
			rm -f $(PROD_OBJECTS); \
			rm -f main.web-game.o; \
			rm -f "$(WEB_OUTPUT)"; \
			rm -f "$(WEB_OUTPUT_DIR)/game.js"; \
			rm -f "$(WEB_OUTPUT_DIR)/game.wasm"; \
			rm -f "$(WEB_OUTPUT_DIR)/game.data"; \
			rm -f "$(WEB_GAME_OUTPUT)"; \
			rm -f "$(WEB_GAME_OUTPUT_DIR)/game.js"; \
			rm -f "$(WEB_GAME_OUTPUT_DIR)/game.wasm"; \
			rm -f "$(WEB_GAME_OUTPUT_DIR)/game.data"; \
			rm -f "$(LINUX_OUTPUT)"; \
			rm -f "$(LINUX_STATIC_OUTPUT)"; \
			echo ""; \
			echo "Build do Lupinho limpa."; \
			;; \
		*) \
			echo "Cancelado."; \
			;; \
	esac
# ====================
# LIMPEZA DO FURNACE
# ====================

# ─── TARGET DESABILITADO ─────────────────────────────────────────
# Remover o Furnace compilado exige rodar ./build_furnace.sh depois
# (e o script pode ter que baixar ~1 GB do source do Furnace).
#
# Para habilitar:
#   1. Descomente TODAS as linhas abaixo (remova o "# " do início)
#   2. Salve o Makefile
#   3. Rode: make clean-furnace
#
# Depois de habilitar, o comando pede confirmação interativa antes
# de apagar libs/web/furnace-web/.
#
# NÃO DESCOMENTE POR ENGANO. O Furnace é a parte mais cara de
# rebuildar do projeto (10-20 min numa máquina média).
# ──────────────────────────────────────────────────────────────────
#
# clean-furnace:
# 	@if [ ! -d "$(FURNACE_WEB_DIR)" ]; then \
# 		echo "Nada a limpar: $(FURNACE_WEB_DIR) já não existe."; \
# 		exit 0; \
# 	fi; \
# 	echo ""; \
# 	echo "ATENÇÃO: isso vai apagar $(FURNACE_WEB_DIR) inteira."; \
# 	echo "Inclui libfurengine.a (13 MB), extern/, fur_player-web.o."; \
# 	echo ""; \
# 	echo "Depois disso, 'make web' falhará até rodar:"; \
# 	echo "  ./build_furnace.sh web ./furnace libs/web/furnace-web"; \
# 	echo ""; \
# 	printf "Continuar? [s/N] "; \
# 	read resposta; \
# 	case "$$resposta" in \
# 		[sS]|[sS][iI][mM]) \
# 			rm -rf "$(FURNACE_WEB_DIR)"; \
# 			echo ""; \
# 			echo "Artefatos removidos."; \
# 			echo "Para regenerar: ./build_furnace.sh web ./furnace libs/web/furnace-web"; \
# 			;; \
# 		*) \
# 			echo "Cancelado."; \
# 			;; \
# 	esac

# ====================
# AJUDA
# ====================
help:
	@echo "Lupinho Makefile"
	@echo "================"
	@echo ""
	@echo "Builds:"
	@echo "  make web            - Web sem overlay de debug"
	@echo "  make debug          - Web com overlay de debug"
	@echo "  make production     - Web otimizado"
	@echo "  make web-game GAME_PATH=game.lupi"
	@echo "  make linux          - Linux nativo"
	@echo "  make static         - Linux estático"
	@echo "  make lua-web        - Compila Lua para WebAssembly"
	@echo ""
	@echo "Verificação (automática em todas as builds):"
	@echo "  make check-libs     - Confere se as libs Web existem"
	@echo ""
	@echo "Limpeza (pedem confirmação):"
	@echo "  make clean          - Remove objetos e saídas do Lupinho"

.PHONY: web debug production web-game linux static lua-web \
        check-libs clean help

