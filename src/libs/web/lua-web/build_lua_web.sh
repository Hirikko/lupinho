#!/usr/bin/env bash
set -euo pipefail

# Compile a biblioteca estática do Lua 5.5.1 para WebAssembly usando Emscripten.
# A fonte é compartilhada com a versão Linux para preservar o patch 0b/0B do Lupi.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../" && pwd)"
LUA_SRC="$ROOT/src/libs/linux/lua-5.5.1/src"
OUT_DIR="$ROOT/src/libs/web/lua-web"
BUILD_DIR="$OUT_DIR/build-5.5.1"

if ! command -v emcc >/dev/null 2>&1; then
  echo "Erro: emcc não encontrado. Ative/instale o Emscripten SDK antes de continuar." >&2
  exit 1
fi
if ! command -v emar >/dev/null 2>&1; then
  echo "Erro: emar não encontrado. Ative/instale o Emscripten SDK antes de continuar." >&2
  exit 1
fi
if [[ ! -f "$LUA_SRC/lua.h" || ! -f "$LUA_SRC/llex.c" ]]; then
  echo "Erro: fonte do Lua 5.5.1 não encontrada em: $LUA_SRC" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR"
rm -f "$BUILD_DIR"/*.o "$OUT_DIR/liblua.web.a"

# Equivalente aos CORE_O + LIB_O do Makefile oficial do Lua.
SOURCES=(
  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
  lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c
  lundump.c lvm.c lzio.c lauxlib.c lbaselib.c lcorolib.c ldblib.c liolib.c
  lmathlib.c loadlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c linit.c
)

for source in "${SOURCES[@]}"; do
  echo "[Lua 5.5.1/WASM] $source"
  emcc -O2 -std=c99 -I"$LUA_SRC" -c "$LUA_SRC/$source" -o "$BUILD_DIR/${source%.c}.o"
done

emar rcs "$OUT_DIR/liblua.web.a" "$BUILD_DIR"/*.o

echo "Biblioteca criada: $OUT_DIR/liblua.web.a"
echo "Versão-fonte: Lua 5.5.1 (com patch Lupi 0b/0B da árvore Linux)."
