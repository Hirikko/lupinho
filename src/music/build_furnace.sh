#!/bin/sh
# build_furnace.sh <tag> [furnace-src] [output-dir]
# Exemplo: ./build_furnace.sh web

set -e
cd "$(dirname "$0")/.."

tag="${1:-web}"
src="${2:-furnace}"
out="${3:-libs/web/furnace-web}"

# ─── COMANDO CLEAN ───
if [ "$tag" = "clean" ]; then
    echo "Isso vai apagar o código fonte do Furnace (src/furnace) e a pasta de build temporária."
    echo "As bibliotecas prontas em $out NÃO serão apagadas."
    printf "Continuar? [s/N] "
    read resposta
    case "$resposta" in
        [sS]|[sS][iI][mM])
            rm -rf "furnace"
            rm -rf "music/furnace-build-web"
            echo "Pasta fonte e build temporária removidas."
            ;;
        *)
            echo "Cancelado."
            ;;
    esac
    exit 0
fi

# ─── CHECAGEM E DOWNLOAD ───
if [ ! -f "$src/src/engine/engine.h" ]; then
    echo "--------------------------------------------------------"
    echo "Fonte do Furnace não encontrada em '$src'."
    echo "É necessário baixar o código-fonte para compilar o Furnace."
    echo "Repositório: https://github.com/Hirikko/furnace (branch cli)"
    echo "Aviso: São ~1 GB com submódulos."
    echo "--------------------------------------------------------"
    printf "Deseja baixar agora? [s/N] "
    read resposta

    case "$resposta" in
        [sS]|[sS][iI][mM])
            if [ -d "$src" ]; then
                echo "ERRO: A pasta '$src' existe mas não tem os fontes corretos. Apague-a manualmente."
                exit 1
            fi
            git clone --recursive -b cli https://github.com/Hirikko/furnace.git "$src"
            ;;
        *)
            echo "Cancelado. Não é possível compilar o Furnace sem o código-fonte."
            exit 1
            ;;
    esac
fi

build="music/furnace-build-$tag"
cmake_bin="${CMAKE:-cmake}"

# ─── CONFIGURAÇÃO E BUILD ───
if [ "$tag" = "web" ]; then
    emcmake "$cmake_bin" -S "$src" -B "$build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_GUI=OFF -DUSE_SDL2=OFF -DUSE_SNDFILE=ON \
        -DWITH_JACK=OFF -DWITH_PORTAUDIO=OFF -DUSE_RTMIDI=OFF \
        -DWITH_LOCALE=OFF -DUSE_BACKWARD=OFF -DWITH_JSON=OFF \
        -DUSE_FREETYPE=OFF -DSYSTEM_ZLIB=OFF -DSYSTEM_LIBSNDFILE=OFF \
        -DWARNINGS_ARE_ERRORS=OFF \
        -DCMAKE_CXX_FLAGS="-fexceptions -include cstdlib -include cstdint"
else
    "$cmake_bin" -S "$src" -B "$build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_GUI=OFF -DUSE_SDL2=OFF -DUSE_SNDFILE=ON \
        -DWITH_JACK=OFF -DWITH_PORTAUDIO=OFF -DUSE_RTMIDI=OFF \
        -DWITH_LOCALE=OFF -DUSE_BACKWARD=OFF -DWITH_JSON=OFF \
        -DUSE_FREETYPE=OFF -DSYSTEM_ZLIB=OFF -DSYSTEM_LIBSNDFILE=OFF \
        -DWARNINGS_ARE_ERRORS=OFF
fi

"$cmake_bin" --build "$build" --target furengine

# ─── EMPACOTA ───
mkdir -p "$out/extern"
cp "$build/libfurengine.a" "$out/libfurengine.a"

(cd "$build" && find extern -name "*.a" -exec cp --parents {} "../../$out/" \;)

python3 - "$build" "$out" <<'PY'
import os, re, sys

build = os.path.abspath(sys.argv[1])
out   = sys.argv[2]

ninja = os.path.join(build, "build.ninja")
libs_file = os.path.join(out, "furengine-libs.txt")

with open(ninja, encoding="utf-8", errors="replace") as f:
    text = f.read()

m = re.search(r"^build furnace\.js:.*?\n(?:\s+\S.*\n)*?\s+LINK_LIBRARIES = (.*)$",
              text, re.MULTILINE)
if not m:
    sys.stderr.write("ERRO: LINK_LIBRARIES não encontrado\n")
    sys.exit(1)

libs = []
for item in m.group(1).split():
    if item.endswith(".a"):
        pos = item.find("extern/")
        if pos >= 0:
            rel = item[pos:]
            libs.append(f"{out}/{rel}")
        else:
            libs.append(f"{out}/{os.path.basename(item)}")
    elif item.startswith("-"):
        libs.append(item)

with open(libs_file, "w") as f:
    f.write("\n".join(libs) + "\n")

print(f"Gerado {libs_file} com {len(libs)} entradas")
PY

echo "Build copiado para $out"
