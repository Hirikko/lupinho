#include "lupi_music.h"
#include "music/fur_player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <zlib.h>

#define LUPI_MUSIC_MAX_FILE_SIZE (64 * 1024 * 1024)

static char LupiMusic_GameDir[512] = "";

/*
 * Máscara de canais do Furnace.
 *
 * Dois blocos de 64 bits representam os 128 canais possíveis.
 * low representa os canais 1 a 64.
 * high representa os canais 65 a 128.
 */
typedef struct {
    uint64_t low;
    uint64_t high;
} LupiMusic_ChannelMask;

#define LUPI_MUSIC_MASK_METATABLE "LupiMusic.ChannelMask"
#define LUPI_MUSIC_MAX_CHANNELS 128

static const unsigned char LupiMusic_FurMagic[16] = {
    '-', 'F', 'u', 'r', 'n', 'a', 'c', 'e',
    ' ', 'm', 'o', 'd', 'u', 'l', 'e', '-'
};

static uint16_t LupiMusic_ReadU16(const unsigned char *data)
{
    return (uint16_t)data[0] |
           ((uint16_t)data[1] << 8);
}

static uint32_t LupiMusic_ReadU32(const unsigned char *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

void LupiMusic_SetGameDir(const char *game_dir)
{
    if (!game_dir) {
        LupiMusic_GameDir[0] = '\0';
        return;
    }

    snprintf(
        LupiMusic_GameDir,
        sizeof(LupiMusic_GameDir),
        "%s",
        game_dir
    );
}

/*
 * Lê o arquivo completo para memória.
 */
static unsigned char *LupiMusic_ReadFile(
    const char *path,
    size_t *file_size
)
{
    FILE *file = fopen(path, "rb");

    if (!file) {
        printf("[Lupi_music] ERROR: cannot open file: %s\n", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);

    if (size <= 0 || size > LUPI_MUSIC_MAX_FILE_SIZE) {
        printf("[Lupi_music] ERROR: invalid file size\n");
        fclose(file);
        return NULL;
    }

    rewind(file);

    unsigned char *buffer = malloc((size_t)size);

    if (!buffer) {
        printf("[Lupi_music] ERROR: memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, (size_t)size, file);

    fclose(file);

    if (read_size != (size_t)size) {
        printf("[Lupi_music] ERROR: file read failed\n");
        free(buffer);
        return NULL;
    }

    *file_size = read_size;

    return buffer;
}

/*
 * Descomprime arquivos Furnace usando zlib.
 */
static unsigned char *LupiMusic_Decompress(
    const unsigned char *input,
    size_t input_size,
    size_t *output_size
)
{
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit(&stream) != Z_OK) {
        printf("[Lupi_music] ERROR: zlib initialization failed\n");
        return NULL;
    }

    size_t capacity = input_size * 4;

    if (capacity < 65536)
        capacity = 65536;

    if (capacity > LUPI_MUSIC_MAX_FILE_SIZE)
        capacity = LUPI_MUSIC_MAX_FILE_SIZE;

    unsigned char *output = malloc(capacity);

    if (!output) {
        inflateEnd(&stream);
        return NULL;
    }

    stream.next_in = (Bytef *)input;
    stream.avail_in = (uInt)input_size;

    int result = Z_OK;

    while (result == Z_OK) {
        if (stream.total_out >= capacity) {
            if (capacity >= LUPI_MUSIC_MAX_FILE_SIZE) {
                printf("[Lupi_music] ERROR: decompressed file too large\n");
                free(output);
                inflateEnd(&stream);
                return NULL;
            }

            size_t new_capacity = capacity * 2;

            if (new_capacity > LUPI_MUSIC_MAX_FILE_SIZE)
                new_capacity = LUPI_MUSIC_MAX_FILE_SIZE;

            unsigned char *new_output = realloc(output, new_capacity);

            if (!new_output) {
                free(output);
                inflateEnd(&stream);
                return NULL;
            }

            output = new_output;
            capacity = new_capacity;
        }

        stream.next_out = output + stream.total_out;
        stream.avail_out = (uInt)(capacity - stream.total_out);

        result = inflate(&stream, Z_NO_FLUSH);
    }

    if (result != Z_STREAM_END) {
        printf("[Lupi_music] ERROR: invalid zlib stream\n");
        free(output);
        inflateEnd(&stream);
        return NULL;
    }

    *output_size = (size_t)stream.total_out;

    inflateEnd(&stream);

    return output;
}

/* Realiza validação dos blocos após INFO */
static int LupiMusic_ValidateChunks(
    const unsigned char *data,
    size_t size,
    size_t offset
){
    unsigned int flag_count = 0;
    unsigned int adir_count = 0;
    unsigned int ins2_count = 0;
    unsigned int smp2_count = 0;
    unsigned int patn_count = 0;
    unsigned int other_count = 0;

    unsigned int total_count = 0;

    while (offset < size) {

        /* Todo bloco/chunk precisa de ao menos 8 bytes:
           4 bytes para a tag + 4 bytes por tamanho */
        if (size - offset < 8) {
            printf(
                "[Lupi_music] ERROR: incomplete chunk header at offset %zu\n",
                offset
            );
            return 0;
        }

        const unsigned char *chunk = data + offset;

        uint32_t chunk_size = LupiMusic_ReadU32(chunk + 4);

        /* Verifica se o chunk cabe dentro do arquivo */
        if ((size_t)chunk_size > size - offset - 8) {
            printf(
                "[Lupi_music] ERROR: invalid chunk size at offset %zu\n",
                offset
            );
            return 0;
        }

        if (memcmp(chunk, "FLAG", 4) == 0) {
            flag_count++;
        }
        else if (memcmp(chunk, "ADIR", 4) == 0) {
            adir_count++;
        }
        else if (memcmp(chunk, "INS2", 4) == 0) {
            ins2_count++;
        }
        else if (memcmp(chunk, "SMP2", 4) == 0) {
            smp2_count++;
        }
        else if (memcmp(chunk, "PATN", 4) == 0) {
            patn_count++;
        }
        else {
            other_count++;
        }

        total_count++;

        /* Move para próximo chunk */
        offset += 8 + (size_t)chunk_size;
    }

    printf("[Lupi_music] Chunk validation successful\n");
    printf("[Lupi_music] Total chunks: %u\n", total_count);
    printf("[Lupi_music] FLAG chunks: %u\n", flag_count);
    printf("[Lupi_music] ADIR chunks: %u\n", adir_count);
    printf("[Lupi_music] INS2 chunks: %u\n", ins2_count);
    printf("[Lupi_music] SMP2 chunks: %u\n", smp2_count);
    printf("[Lupi_music] PATN chunks: %u\n", patn_count);
    printf("[Lupi_music] Padrões encontrados: %u\n", patn_count);
    printf("[Lupi_music] Other chunks: %u\n", other_count);

    return 1;
}

/* Mostra informações básicas dos instrumentos e padrões do módulo */
static void LupiMusic_InspectMusicData(
    const unsigned char *data,
    size_t size,
    size_t offset
)
{
    unsigned int instrument_index = 0;
    unsigned int pattern_index = 0;

    while (offset < size) {

        /* Verifica se existe espaço suficiente para o cabeçalho */
        if (size - offset < 8) {
            return;
        }

        const unsigned char *chunk = data + offset;
        uint32_t chunk_size = LupiMusic_ReadU32(chunk + 4);

        /* Evita acessar dados além do final do módulo */
        if ((size_t)chunk_size > size - offset - 8) {
            return;
        }

        if (memcmp(chunk, "INS2", 4) == 0) {
            instrument_index++;
        }
        else if (memcmp(chunk, "PATN", 4) == 0) {
            pattern_index++;
        }

        offset += 8 + (size_t)chunk_size;
    }

    printf(
        "[Lupi_music] Instrumentos encontrados: %u\n",
        instrument_index
    );

    printf(
        "[Lupi_music] Padrões encontrados: %u\n",
        pattern_index
    );
}

/* Analisa o cabeçalho Furnace.  */
static int LupiMusic_ParseFur(
    const unsigned char *data,
    size_t size
)
{
    if (size < 32) {
        printf("[Lupi_music] ERROR: file is too small\n");
        return 0;
    }

    if (memcmp(data, LupiMusic_FurMagic, 16) != 0) {
        printf("[Lupi_music] ERROR: invalid Furnace signature\n");
        return 0;
    }

    uint16_t version = LupiMusic_ReadU16(data + 16);
    uint32_t info_pointer = LupiMusic_ReadU32(data + 20);

    printf("[Lupi_music] Furnace module detected\n");
    printf("[Lupi_music] Format version: %u\n", version);

    if ((size_t)info_pointer > size - 8) {
        printf("[Lupi_music] ERROR: invalid information block pointer\n");
        return 0;
    }

    const unsigned char *info = data + info_pointer;

    if (memcmp(info, "INFO", 4) != 0) {
        printf("[Lupi_music] ERROR: INFO block not found\n");
        return 0;
    }

    uint32_t info_size = LupiMusic_ReadU32(info + 4);

    printf("[Lupi_music] Information block: %.4s\n", (const char *)info);
    printf(
        "[Lupi_music] Information block size: %u bytes\n",
        info_size
    );

    if ((size_t)info_size > size - (size_t)info_pointer - 8) {
        printf("[Lupi_music] ERROR: invalid information block size\n");
        return 0;
    }

    /* Primeiro chunk começa imediatamente após INFO */
    size_t next_chunk = (size_t)info_pointer + 8 + (size_t)info_size;

    printf("[Lupi_music] Validando blocos restantes...\n");

    if (!LupiMusic_ValidateChunks(data, size, next_chunk)) {
        return 0;
    }

    /* Faz uma inspeção básica dos instrumentos e padrões */
    LupiMusic_InspectMusicData(data, size, next_chunk);

    return 1;
}

/*
 * music.loop(nome)
 *
 * Carrega um arquivo Furnace e inicia sua reprodução.
 */
static int LupiMusic_Loop(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    char path[1024];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        LupiMusic_GameDir,
        name
    );

    printf("[Lupi_music] Loading: %s\n", name);

    size_t file_size = 0;

    unsigned char *file_data = LupiMusic_ReadFile(
        path,
        &file_size
    );

    if (!file_data)
        return 0;

    unsigned char *fur_data = file_data;
    size_t fur_size = file_size;

    /*
     * Se já começa com a assinatura Furnace,
     * o arquivo não precisa ser descomprimido.
     */
    if (file_size < 16 ||
        memcmp(file_data, LupiMusic_FurMagic, 16) != 0) {

        printf("[Lupi_music] Compressed file detected\n");

        fur_data = LupiMusic_Decompress(
            file_data,
            file_size,
            &fur_size
        );

        free(file_data);

        if (!fur_data)
            return 0;

        printf("[Lupi_music] Decompression successful\n");
    }

    /*
     * Mantém a inspeção auxiliar existente.
     * O resultado não bloqueia o Furnace:
     * DivEngine::load() é a validação definitiva.
     */
    if (!LupiMusic_ParseFur(fur_data, fur_size)) {
        printf(
            "[Lupi_music] WARNING: parser auxiliar encontrou inconsistências\n"
        );
    }


    /*
     * Diagnóstico: inicialização do player Furnace.
     */
    printf("[Lupi_music] Antes de fur_player_init()\n");

    int init_result = fur_player_init();

    printf(
        "[Lupi_music] fur_player_init() retornou: %d\n",
           init_result
    );

    if (!init_result) {
        printf("[Lupi_music] ERROR: Furnace initialization failed\n");
        free(fur_data);
        return 0;
    }

    /*
     * Diagnóstico: carregamento do módulo.
     */
    printf("[Lupi_music] Antes de fur_player_load()\n");

    int load_result = fur_player_load(fur_data, fur_size);

    printf(
        "[Lupi_music] fur_player_load() retornou: %d\n",
           load_result
    );

    if (!load_result) {
        printf("[Lupi_music] ERROR: Furnace load failed\n");
        free(fur_data);
        return 0;
    }

    free(fur_data);

    /*
     * Diagnóstico: início da reprodução.
     */
    printf("[Lupi_music] Antes de fur_player_play()\n");

    fur_player_play();

    printf("[Lupi_music] fur_player_play() executado\n");
    printf("[Lupi_music] Playback started: %s\n", name);

    return 0;
}

/*
 * music.stop()
 */
static int LupiMusic_Stop(lua_State *L)
{
    (void)L;

    fur_player_stop();

    printf("[Lupi_music] music.stop()\n");

    return 0;
}

/*
 * Combina duas máscaras de canais usando o operador | do Lua.
 */
static int LupiMusic_MaskOr(lua_State *L)
{
    LupiMusic_ChannelMask *a =
    (LupiMusic_ChannelMask *)luaL_testudata(
        L,
        1,
        LUPI_MUSIC_MASK_METATABLE
    );

    LupiMusic_ChannelMask *b =
    (LupiMusic_ChannelMask *)luaL_testudata(
        L,
        2,
        LUPI_MUSIC_MASK_METATABLE
    );

    if (!a || !b) {
        return luaL_error(
            L,
            "operador | exige duas mascaras de canais"
        );
    }

    LupiMusic_ChannelMask *result =
    (LupiMusic_ChannelMask *)lua_newuserdatauv(
        L,
        sizeof(LupiMusic_ChannelMask),
                                               0
    );

    result->low = a->low | b->low;
    result->high = a->high | b->high;

    luaL_setmetatable(L, LUPI_MUSIC_MASK_METATABLE);

    return 1;
}

/*
 * Registra o tipo userdata utilizado pelas máscaras.
 */
static void LupiMusic_RegisterMaskMetatable(lua_State *L)
{
    if (luaL_newmetatable(L, LUPI_MUSIC_MASK_METATABLE)) {
        lua_pushcfunction(L, LupiMusic_MaskOr);
        lua_setfield(L, -2, "__bor");

        lua_pushstring(L, "ChannelMask");
        lua_setfield(L, -2, "__name");
    }

    lua_pop(L, 1);
}

/*
 * Cria e coloca uma máscara na pilha Lua.
 */
static void LupiMusic_PushMask(
    lua_State *L,
    uint64_t low,
    uint64_t high
)
{
    LupiMusic_ChannelMask *mask =
    (LupiMusic_ChannelMask *)lua_newuserdatauv(
        L,
        sizeof(LupiMusic_ChannelMask),
                                               0
    );

    mask->low = low;
    mask->high = high;

    luaL_setmetatable(L, LUPI_MUSIC_MASK_METATABLE);
}

/*
 * Registra CH_1 até CH_128 e CH_ALL como globais Lua.
 */
static void LupiMusic_RegisterChannelConstants(lua_State *L)
{
    char name[24];

    for (int channel = 1;
         channel <= LUPI_MUSIC_MAX_CHANNELS;
    channel++) {

        uint64_t low = 0;
        uint64_t high = 0;

        if (channel <= 64) {
            low = UINT64_C(1) << (channel - 1);
        } else {
            high = UINT64_C(1) << (channel - 65);
        }

        snprintf(
            name,
            sizeof(name),
                 "CH_%d",
                 channel
        );

        LupiMusic_PushMask(L, low, high);
        lua_setglobal(L, name);
    }

    LupiMusic_PushMask(
        L,
        UINT64_MAX,
        UINT64_MAX
    );

    lua_setglobal(L, "CH_ALL");
}

/*
 * music.volume(volume, duracao)
 * music.volume(volume, mascara, duracao)
 *
 * Sem mascara: controla o volume geral.
 * Com mascara: controla os canais selecionados.
 */
static int LupiMusic_Volume(lua_State *L)
{
    lua_Number volume = luaL_checknumber(L, 1);

    LupiMusic_ChannelMask *mask =
        (LupiMusic_ChannelMask *)luaL_testudata(
            L,
            2,
            LUPI_MUSIC_MASK_METATABLE
        );

    lua_Number duration;

    if (!isfinite((double)volume) ||
        volume < 0.0 ||
        volume > 1.0) {

        return luaL_argerror(
            L,
            1,
            "volume deve estar entre 0.0 e 1.0"
        );
    }

    if (!mask) {

        duration = luaL_checknumber(L, 2);

        if (!isfinite((double)duration) ||
            duration < 0.0) {

            return luaL_argerror(
                L,
                2,
                "duracao deve ser um numero finito nao negativo"
            );
        }

        fur_player_set_volume(
            (float)volume,
            (float)duration
        );

        return 0;
    }

    duration = luaL_checknumber(L, 3);

    if (!isfinite((double)duration) ||
        duration < 0.0) {

        return luaL_argerror(
            L,
            3,
            "duracao deve ser um numero finito nao negativo"
        );
    }

    for (int channel = 0;
         channel < LUPI_MUSIC_MAX_CHANNELS;
         channel++) {

        int selected;

        if (channel < 64) {
            selected =
                (mask->low & (UINT64_C(1) << channel)) != 0;
        } else {
            selected =
                (mask->high &
                 (UINT64_C(1) << (channel - 64))) != 0;
        }

        if (selected) {
            fur_player_set_channel_volume(
                channel,
                (float)volume,
                (float)duration
            );
        }
    }

    return 0;
}

static const luaL_Reg LupiMusic_Lib[] = {
    {"loop", LupiMusic_Loop},
    {"stop", LupiMusic_Stop},
    {"volume", LupiMusic_Volume},
    {NULL, NULL}
};

int LupiMusic_Register(lua_State *L)
{
    LupiMusic_RegisterMaskMetatable(L);

    luaL_newlib(L, LupiMusic_Lib);
    lua_setglobal(L, "music");

    LupiMusic_RegisterChannelConstants(L);

    return 0;
}
