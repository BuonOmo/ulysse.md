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
	int height = 46;
	int width = 46;
    game_of_life *game;
    srand(time(NULL)); // for randomly filled games.
    game = gol_init(height, width, 1, false, set_pixel);
	game->data = malloc(sizeof(char*)*height);
	for (size_t i = 0; i < height; i++)
	{
		DATA(*game)[i] = malloc(sizeof(char)*width);
	}

	if (P49_BUMPER_LOOP(*game, 0, 0)) abort();

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
