#include <stdio.h>
#include <time.h>

#include "lib/game_of_life.h"

#define DATA(game) ((char**)((game).data))

void set_pixel(game_of_life game, size_t h, size_t w, bool is_alive) {
	DATA(game)[h][w] = is_alive ? 'O' : '.';
}

void show(game_of_life game) {
    for (int i = 0; i < game.height; i++) {
        for (int j = 0; j < game.width; j++) {
            printf("%c", DATA(game)[i][j]);
        }
        printf("\n");
    }
}

int main()
{
	int height = 70;
	int width = 70;
    game_of_life *game;
    srand(time(NULL)); // for randomly filled games.
    game = gol_init(height, width, 70, false, set_pixel);
	game->data = malloc(sizeof(char*)*height);
	for (size_t i = 0; i < height; i++)
	{
		DATA(*game)[i] = malloc(sizeof(char)*width);
	}

    // if (P48_LWSS_GUN(*game, 0, 0)) abort();
	if (gol_insert_rle(*game, 0, 0, 1, 1, "o!")) abort();

	gol_render(*game);
	show(*game);
	// printf("%c[%iA", 27, game->height);

	for (size_t i = 0; i < height; i++)
	{
		free(DATA(*game)[i]);
	}
	free(game->data);
    // clear(game);

    return 0;
}
