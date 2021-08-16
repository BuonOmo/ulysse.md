#include <stdio.h>

#include "lib/game_of_life.h"
#include "lib/gifenc.h"

// uncomment next line for a random game_of_life rather than
// predetermined shape.
// #define RANDOM

#define DATA(game) (((ge_GIF*)((game).data))->frame)

void set_pixel(game_of_life game, size_t h, size_t w, bool is_alive) {
	DATA(game)[h * game.width + w] = is_alive;
}

bool top_square(game_of_life game) {
	for (size_t w = 0; w < game.board_width - 1; w++)
	{
		if (game.board[0][w] & game.board[0][w + 1] & game.board[1][w] & game.board[1][w + 1]) return true;
	}
	return false;
}

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <file.gif>\n", argv[0]);
		return 1;
	}

	size_t height = 380;
	size_t width = 1000;
	size_t pixel = 5;
    game_of_life *game;
	ge_GIF* gif;
	gif = ge_new_gif(
		argv[1], width, height,
		(uint8_t []) { /* R, G, B */
			0xff, 0xff, 0xff,
			0xda, 0x09, 0xff
		},
		1,
		0, /* infinite loop */
		0  /* transparent background */
	);
	game = gol_init(height, width, pixel, false, set_pixel);
	game->data = gif;
#ifdef RANDOM
	gol_fill_random(*game);

	for (size_t i = 0; i < 2000; i++)
	{
		gol_step(*game);
		gol_render(*game);
		ge_add_frame(gif, 0);
	}
#else
	ge_set_disposal(gif, 2);

	if (P48_LWSS_GUN(*game, 5, 1)) abort();

	while (!top_square(*game)) gol_step(*game);

	do {
		gol_step(*game);
		gol_render(*game);
		ge_add_frame(gif, 10);
	} while(top_square(*game));
	do {
		gol_step(*game);
		gol_render(*game);
		ge_add_frame(gif, 10);
	} while(!top_square(*game));
#endif

	ge_close_gif(gif);
	// gol_clear(game);
	return 0;
}
