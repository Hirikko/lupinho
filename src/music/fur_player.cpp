#include "fur_player.h"
#include "ta-utils.h"
#include "ta-log.h"
#include "raylib.h"
#include "engine.h"

#include <cstdio>
#include <cstring>
#include <new>
#include <cmath>
#include <mutex>



#define FUR_PLAYER_RATE 44100
#define FUR_PLAYER_CHANNELS 2
#define FUR_PLAYER_BUFFER_FRAMES 256
#define FUR_PLAYER_MAX_CHANNELS 128

// Diagnóstico temporário de inicialização e callback.
static unsigned int FurPlayer_InitCount = 0;

static DivEngine *FurPlayer_Engine = NULL;
static AudioStream FurPlayer_Stream = { 0 };

enum FurPlayerState {
    FUR_PLAYER_UNINITIALIZED,
    FUR_PLAYER_INITIALIZING,
    FUR_PLAYER_READY
};

static FurPlayerState FurPlayer_State = FUR_PLAYER_UNINITIALIZED;
static bool FurPlayer_LoggerReady = false;
static bool FurPlayer_StreamReady = false;

static float FurPlayer_Left[FUR_PLAYER_BUFFER_FRAMES];
static float FurPlayer_Right[FUR_PLAYER_BUFFER_FRAMES];

static unsigned int FurPlayer_CallbackCount = 0;

/*
 * Estado independente do volume original do Furnace.
 *
 * O ganho 1.0 representa volume integral.
 * O ganho 0.0 representa silêncio.
 */
static std::mutex FurPlayer_VolumeMutex;

static float FurPlayer_ChannelCurrent[FUR_PLAYER_MAX_CHANNELS];
static float FurPlayer_ChannelTarget[FUR_PLAYER_MAX_CHANNELS];
static float FurPlayer_ChannelRemaining[FUR_PLAYER_MAX_CHANNELS];

/*
 * Estado independente do volume geral da saída.
 */
static float FurPlayer_MasterCurrent = 1.0f;
static float FurPlayer_MasterTarget = 1.0f;
static float FurPlayer_MasterRemaining = 0.0f;

/*
 * Inicializa todos os canais com ganho integral.
 */
static void FurPlayer_ResetChannelGains(void)
{
    std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

    FurPlayer_MasterCurrent = 1.0f;
    FurPlayer_MasterTarget = 1.0f;
    FurPlayer_MasterRemaining = 0.0f;

    for (int i = 0; i < FUR_PLAYER_MAX_CHANNELS; i++) {
        FurPlayer_ChannelCurrent[i] = 1.0f;
        FurPlayer_ChannelTarget[i] = 1.0f;
        FurPlayer_ChannelRemaining[i] = 0.0f;
    }
}

/*
 * Aplica uma fotografia dos ganhos aos canais ativos do Furnace.
 *
 * A sincronização dos dispatches é responsabilidade do DivEngine.
 */
static void FurPlayer_ApplyChannelGains(const float *gains)
{
    DivEngine *engine = FurPlayer_Engine;

    if (!engine || !gains)
        return;

    unsigned int fixedGains[FUR_PLAYER_MAX_CHANNELS];

    for (int channel = 0; channel < FUR_PLAYER_MAX_CHANNELS; channel++) {
        float gain = gains[channel];

        if (!std::isfinite(gain))
            gain = 1.0f;

        if (gain < 0.0f)
            gain = 0.0f;

        if (gain > 1.0f)
            gain = 1.0f;

        fixedGains[channel] =
        (unsigned int)(gain * 65536.0f + 0.5f);
    }

    engine->setChannelGains(
        fixedGains,
        FUR_PLAYER_MAX_CHANNELS
    );
}

/*
 * Copia os ganhos atuais e os aplica sem avançar o fade.
 * Utilizado após carregar um novo módulo.
 */
static void FurPlayer_ApplyStoredChannelGains(void)
{
    float gains[FUR_PLAYER_MAX_CHANNELS];

    {
        std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

        std::memcpy(
            gains,
            FurPlayer_ChannelCurrent,
            sizeof(gains)
        );
    }

    FurPlayer_ApplyChannelGains(gains);
}

/*
 * Avança os fades pelo tempo de áudio processado
 * e aplica os ganhos aos dispatches.
 */
static void FurPlayer_UpdateChannelGains(unsigned int frames)
{
    float gains[FUR_PLAYER_MAX_CHANNELS];

    const float elapsed =
    (float)frames / (float)FUR_PLAYER_RATE;

    {
        std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

        for (int i = 0; i < FUR_PLAYER_MAX_CHANNELS; i++) {
            float remaining = FurPlayer_ChannelRemaining[i];

            if (remaining > 0.0f) {
                if (elapsed >= remaining) {
                    FurPlayer_ChannelCurrent[i] =
                    FurPlayer_ChannelTarget[i];

                    FurPlayer_ChannelRemaining[i] = 0.0f;
                } else {
                    float fraction = elapsed / remaining;

                    FurPlayer_ChannelCurrent[i] +=
                    (FurPlayer_ChannelTarget[i] -
                    FurPlayer_ChannelCurrent[i]) * fraction;

                    FurPlayer_ChannelRemaining[i] -= elapsed;
                }
            }

            gains[i] = FurPlayer_ChannelCurrent[i];
        }
    }

    FurPlayer_ApplyChannelGains(gains);
}

/*
 * Define o destino de volume de um canal lógico.
 */
extern "C" void fur_player_set_channel_volume(
    int channel,
    float volume,
    float duration
)
{
    if (channel < 0 ||
        channel >= FUR_PLAYER_MAX_CHANNELS) {
        return;
        }

        if (!std::isfinite(volume) ||
            !std::isfinite(duration)) {
            return;
            }

            if (volume < 0.0f || volume > 1.0f || duration < 0.0f)
                return;

    if (FurPlayer_State != FUR_PLAYER_READY ||
        !FurPlayer_Engine) {
        return;
        }

        std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

    FurPlayer_ChannelTarget[channel] = volume;
    FurPlayer_ChannelRemaining[channel] = duration;

    if (duration == 0.0f) {
        FurPlayer_ChannelCurrent[channel] = volume;
    }
}

/*
 * Define o destino de volume geral da saída.
 */
extern "C" void fur_player_set_volume(
    float volume,
    float duration
)
{
    if (!std::isfinite(volume) ||
        !std::isfinite(duration)) {
        return;
    }

    if (volume < 0.0f ||
        volume > 1.0f ||
        duration < 0.0f) {
        return;
    }

    if (FurPlayer_State != FUR_PLAYER_READY ||
        !FurPlayer_Engine) {
        return;
    }

    std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

    FurPlayer_MasterTarget = volume;
    FurPlayer_MasterRemaining = duration;

    if (duration == 0.0f) {
        FurPlayer_MasterCurrent = volume;
    }
}

/*
 * Aplica o volume geral às amostras estéreo.
 * A transição é processada por amostra.
 */
static void FurPlayer_ApplyMasterGain(
    float *left,
    float *right,
    unsigned int frames
)
{
    if (!left || !right)
        return;

    const float elapsed =
        1.0f / (float)FUR_PLAYER_RATE;

    std::lock_guard<std::mutex> lock(FurPlayer_VolumeMutex);

    for (unsigned int i = 0; i < frames; i++) {

        if (FurPlayer_MasterRemaining > 0.0f) {

            if (elapsed >= FurPlayer_MasterRemaining) {
                FurPlayer_MasterCurrent =
                    FurPlayer_MasterTarget;

                FurPlayer_MasterRemaining = 0.0f;

            } else {
                float fraction =
                    elapsed / FurPlayer_MasterRemaining;

                FurPlayer_MasterCurrent +=
                    (FurPlayer_MasterTarget -
                     FurPlayer_MasterCurrent) * fraction;

                FurPlayer_MasterRemaining -= elapsed;
            }
        }

        left[i] *= FurPlayer_MasterCurrent;
        right[i] *= FurPlayer_MasterCurrent;
    }
}

/*
 * Callback de áudio.
 *
 * O buffer recebido pela Raylib é F32 estéreo intercalado.
 * O Furnace fornece dois buffers separados: esquerdo e direito.
 */

static void FurPlayer_AudioCallback(void *bufferData, unsigned int frames)
{
    float *output = static_cast<float *>(bufferData);

    if (!output)
        return;

    // Registra somente a primeira execução do callback.
    if (FurPlayer_CallbackCount == 0) {
        std::printf(
            "[FurPlayer] Audio callback: primeira execução | frames=%u\n",
            frames
        );
    }

    FurPlayer_CallbackCount++;

    if (!FurPlayer_Engine) {
        std::memset(
            output,
            0,
            frames * FUR_PLAYER_CHANNELS * sizeof(float)
        );
        return;
    }

    unsigned int processed = 0;

    while (processed < frames) {
        unsigned int amount = frames - processed;

        if (amount > FUR_PLAYER_BUFFER_FRAMES)
            amount = FUR_PLAYER_BUFFER_FRAMES;

        float *channels[FUR_PLAYER_CHANNELS] = {
            FurPlayer_Left,
            FurPlayer_Right
        };

        FurPlayer_Engine->nextBuf(
            NULL,
            channels,
            0,
            FUR_PLAYER_CHANNELS,
            amount
        );

        /*
         * Avança os fades depois que nextBuf() libera isBusy.
         * O novo ganho será utilizado no próximo bloco.
         */
        FurPlayer_UpdateChannelGains(amount);

        FurPlayer_ApplyMasterGain(
            FurPlayer_Left,
            FurPlayer_Right,
            amount
        );

        for (unsigned int i = 0; i < amount; i++) {
            output[(processed + i) * 2] =
            FurPlayer_Left[i];

            output[(processed + i) * 2 + 1] =
            FurPlayer_Right[i];
        }

        processed += amount;
    }
}

/*
 * Inicializa o motor Furnace.
 */
extern "C" int fur_player_init(void)
{

    FurPlayer_InitCount++;

    std::printf(
        "[FurPlayer] init #%u: entrada | engine=%p | initialized=%d\n",
        FurPlayer_InitCount,
        static_cast<void *>(FurPlayer_Engine),
                FurPlayer_State
    );

    std::printf("[FurPlayer] init: entrada\n");

    if (FurPlayer_State == FUR_PLAYER_READY) {
        std::printf("[FurPlayer] init: Pronto\n");
        return 1;
    }

    if (FurPlayer_State == FUR_PLAYER_INITIALIZING) {
        std::printf("[FurPlayer] init: Inicializando\n");
        return 0;
    }

    FurPlayer_State = FUR_PLAYER_INITIALIZING;
    FurPlayer_ResetChannelGains();

    std::printf("[FurPlayer] init: Inicializado\n");

    // Inicializa o logger do Furnace apenas uma vez.
    if (!FurPlayer_LoggerReady) {
        std::printf("[FurPlayer] init: inicializando logger\n");

        initLog(stdout);

        FurPlayer_LoggerReady = true;

        std::printf("[FurPlayer] init: logger inicializado\n");
    }

    std::printf("[FurPlayer] init: criando DivEngine\n");

    FurPlayer_Engine = new (std::nothrow) DivEngine();

    if (!FurPlayer_Engine) {
        std::printf("[FurPlayer] ERRO: sem memória para DivEngine\n");
        return 0;
    }

    std::printf("[FurPlayer] preInit: iniciando\n");
    FurPlayer_Engine->preInit();
    std::printf("[FurPlayer] preInit: concluído\n");

    FurPlayer_Engine->setAudio(DIV_AUDIO_DUMMY);

    std::printf("[FurPlayer] init: antes de DivEngine::init\n");

    bool engine_result = FurPlayer_Engine->init();

    std::printf(
        "[FurPlayer] init: DivEngine::init retornou %d\n",
        engine_result
    );

    if (!engine_result) {
        FurPlayer_Engine->quit(false);
        delete FurPlayer_Engine;
        FurPlayer_Engine = NULL;
        return 0;
    }

    std::printf("[FurPlayer] init: verificando dispositivo Raylib\n");

    bool audio_ready = IsAudioDeviceReady();

    std::printf(
        "[FurPlayer] init: dispositivo Raylib pronto = %d\n",
        audio_ready
    );

    if (!audio_ready)
        return 0;

    std::printf("[FurPlayer] init: antes de LoadAudioStream\n");

    FurPlayer_Stream = LoadAudioStream(
        FUR_PLAYER_RATE,
        32,
        FUR_PLAYER_CHANNELS
    );

    std::printf("[FurPlayer] init: depois de LoadAudioStream\n");

    if (!IsAudioStreamValid(FurPlayer_Stream)) {
        std::printf("[FurPlayer] ERRO: falha ao criar AudioStream");

        FurPlayer_Engine->quit(false);
        delete FurPlayer_Engine;
        FurPlayer_Engine = NULL;
        return 0;
    }

    FurPlayer_StreamReady = true;

    std::printf("[FurPlayer] init: antes de SetAudioStreamCallback\n");

    SetAudioStreamCallback(
        FurPlayer_Stream,
        FurPlayer_AudioCallback
    );

    std::printf("[FurPlayer] init: antes de PlayAudioStream\n");

    PlayAudioStream(FurPlayer_Stream);

    std::printf("[FurPlayer] init: depois de PlayAudioStream\n");

    FurPlayer_State = FUR_PLAYER_READY;

    std::printf("[FurPlayer] Furnace inicializado\n");

    return 1;
}

/*
 * Carrega um módulo.
 *
 * O Furnace assume a propriedade do buffer passado para load().
 * Por isso, entregamos uma cópia alocada com new[].
 * Não liberar essa cópia depois da chamada.
 */
/*
 * Carrega um módulo com o callback temporariamente suspenso.
 *
 * Isso evita que nextBuf() acesse dispatches durante
 * a destruição e recriação dos sistemas pelo Furnace.
 */
extern "C" int fur_player_load(
    const unsigned char *data,
    size_t size
)
{
    if (FurPlayer_State != FUR_PLAYER_READY ||
        !FurPlayer_Engine) {
        return 0;
        }

        if (!data || size == 0)
            return 0;

    /*
     * Prepara a cópia antes de interromper o stream.
     */
    unsigned char *copy =
    new (std::nothrow) unsigned char[size];

    if (!copy) {
        std::printf("[FurPlayer] ERRO: falha ao copiar módulo\n");
        return 0;
    }

    std::memcpy(copy, data, size);

    /*
     * Suspende o callback da Raylib.
     * StopAudioStream sincroniza com a execução do callback.
     */
    StopAudioStream(FurPlayer_Stream);

    /*
     * O callback não poderá acessar o engine enquanto
     * o módulo estiver sendo substituído.
     */
    FurPlayer_Engine->stop();

    /*
     * O Furnace assume a propriedade da cópia.
     * Não liberar copy após esta chamada.
     */
    bool loaded = FurPlayer_Engine->load(
        copy,
        size,
        "music.fur"
    );

    if (loaded) {
        FurPlayer_Engine->setLoops(-1);

        /*
         * Os dispatches novos recebem os ganhos que já
         * estavam armazenados no FurPlayer.
         */
        FurPlayer_ApplyStoredChannelGains();

        std::printf("[FurPlayer] Módulo carregado\n");
    } else {
        std::printf("[FurPlayer] ERRO: Furnace rejeitou o módulo\n");
    }

    /*
     * Retoma o stream mesmo quando o carregamento falha.
     * O dispositivo continua pertencendo ao Lupinho.
     */
    PlayAudioStream(FurPlayer_Stream);

    return loaded ? 1 : 0;
}

/*
 * Inicia a reprodução.
 */
extern "C" void fur_player_play(void)
{
    if (FurPlayer_State != FUR_PLAYER_READY || !FurPlayer_Engine)
        return;

    FurPlayer_Engine->setLoops(-1);
    FurPlayer_Engine->play();

    std::printf("[FurPlayer] Reprodução iniciada\n");
}

/*
 * Para a música, mas mantém o stream ativo.
 * Assim, o callback continua entregando silêncio quando necessário.
 */
extern "C" void fur_player_stop(void)
{
    if (FurPlayer_State != FUR_PLAYER_READY || !FurPlayer_Engine)
        return;

    FurPlayer_Engine->stop();

    std::printf("[FurPlayer] Reprodução interrompida\n");
}

/*
 * Encerra o player.
 *
 * StopAudioStream sincroniza com o callback da Raylib.
 * Só depois disso podemos destruir o engine.
 *
 * O dispositivo de áudio da Raylib pertence ao Lupinho
 * e não deve ser fechado aqui.
 */

extern "C" void fur_player_shutdown(void)
{
    if (FurPlayer_State == FUR_PLAYER_UNINITIALIZED)
        return;

    if (FurPlayer_StreamReady) {
        StopAudioStream(FurPlayer_Stream);
    }

    if (FurPlayer_Engine) {
        FurPlayer_Engine->stop();
        FurPlayer_Engine->quit(false);

        delete FurPlayer_Engine;
        FurPlayer_Engine = NULL;
    }

    if (FurPlayer_StreamReady) {
        UnloadAudioStream(FurPlayer_Stream);
        FurPlayer_StreamReady = false;
    }

    FurPlayer_State = FUR_PLAYER_UNINITIALIZED;

    std::printf("[FurPlayer] Player encerrado\n");
}

void reportError(String what)
{
    logE("%s", what);
}
