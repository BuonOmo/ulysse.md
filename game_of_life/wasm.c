
#include <emscripten.h>

#include "lib/game_of_life.h"

#define RGBA(R, G, B, A) (R | G << 8 | B << 16 | A << 24)

#define ALIVE_COLOR RGBA(0xda, 0x09, 0xff, 0xff)
#define DEAD_COLOR RGBA(0, 0, 0, 0)

#define MAX_GAMES 100

#define DATA(game) ((unsigned int*)((game).data))

game_of_life *games[MAX_GAMES];

void set_pixel(game_of_life game, size_t h, size_t w, bool is_alive) {
	DATA(game)[h * game.width + w] = is_alive ? ALIVE_COLOR : DEAD_COLOR;
}

void EMSCRIPTEN_KEEPALIVE start(size_t height, size_t width, size_t pixel, size_t seed, size_t index, bool is_tor, char pattern) {
    srand(seed * index);
    if (!games[index]) {
		games[index] = gol_init(height, width, pixel, is_tor, set_pixel);
		games[index]->data = malloc(sizeof(unsigned int) * height * width);
	}
    switch (pattern) {
        case 'g':
            gol_empty(*games[index]);
            if (P48_LWSS_GUN(*games[index], 2, 2)) gol_fill_random(*games[index]);
            break;
        case 's':
            gol_empty(*games[index]);
            if (PI_SHIP(*games[index], height / pixel - 40, 0)) gol_fill_random(*games[index]);
            break;
        case 'r':
        default:
            gol_fill_random(*games[index]);
            break;
    }
}

unsigned int* EMSCRIPTEN_KEEPALIVE render(int index) {
	game_of_life game = *games[index];
	gol_step(game);
	gol_render(game);
	return &(DATA(game))[0];
}

// NOTE: we don't care about clearing data since we never reset anyway.
