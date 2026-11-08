#include "fur_player.h"

#include <stdio.h>

int fur_player_init(void)
{
    printf("[FurPlayer] Furnace ainda não está habilitado no Linux\n");
    return 0;
}

int fur_player_load(const unsigned char *data, size_t size)
{
    (void)data;
    (void)size;
    return 0;
}

void fur_player_play(void)
{
}

void fur_player_stop(void)
{
}

void fur_player_shutdown(void)
{
}
