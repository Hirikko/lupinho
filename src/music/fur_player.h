#ifndef FUR_PLAYER_H
#define FUR_PLAYER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa o Furnace e o stream de áudio da Raylib.
int fur_player_init(void);

// Carrega um módulo Furnace a partir da memória.
int fur_player_load(const unsigned char *data, size_t size);

// Inicia a reprodução em loop infinito.
void fur_player_play(void);

// Interrompe a música atual.
void fur_player_stop(void);

// Define o volume externo de um canal lógico.
void fur_player_set_channel_volume(
    int channel,
    float volume,
    float duration
);

// Define o volume geral da saída de áudio.
void fur_player_set_volume(
    float volume,
    float duration
);

// Encerra o player sem fechar o dispositivo de áudio da Raylib.
void fur_player_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
