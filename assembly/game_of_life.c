#include <stdlib.h>
#include <stdbool.h>
#ifdef TERMINAL
#include <stdio.h>
#include <unistd.h>
#endif
#include <emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIXEL 5
#define GAMES 10

#define color(R, G, B, A) (R | G << 8 | B << 16 | A << 24)

#define ALIVE_COLOR color(0xda, 0x09, 0xff, 0xff)
#define DEAD_COLOR color(0, 0, 0, 0)

typedef struct {
    int height;
    int board_height;
    int board_width;
    int width;
    int pixel;
    bool **board;
    unsigned int *data;
} game_of_life;

game_of_life *games[GAMES];

void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index);
unsigned int* EMSCRIPTEN_KEEPALIVE render(int index);

game_of_life *init(int height, int width, int pixel);
void clear(game_of_life*);
#ifdef TERMINAL
void show(game_of_life);
#endif
void step(game_of_life);
void reset(game_of_life);

int neighbors(int h, int w, game_of_life);

void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index) {
    srand(seed * index);
    if (!games[index]) games[index] = init(height, width, pixel);
    reset(*games[index]);
}

unsigned int* EMSCRIPTEN_KEEPALIVE render(int index) {
    game_of_life game = *games[index];
    step(game);
    for (int i = 0; i < game.height; i++) {
        int iw = i * game.width;
        for (int j = 0; j < game.width; j++) {
            game.data[iw + j] = game.board[i / game.pixel][j / game.pixel] ? ALIVE_COLOR : DEAD_COLOR;
        }
    }
	return &(game.data)[0];
}

game_of_life *init(int height, int width, int pixel) {
    game_of_life *game;
    bool **board;
    int board_height;
    int board_width;

    board_height = height / pixel;
    board_width = width / pixel;


    board = malloc(board_height*sizeof(bool*));
    for (int i = 0; i < board_height; i++)
        board[i] = malloc(board_width*sizeof(bool));
    game = malloc(sizeof(game_of_life));
    *game = (game_of_life){
        .height = height,
        .width = width,
        .board_height = board_height,
        .board_width = board_width,
        .pixel = pixel,
        .board = board,
        .data = malloc(height*width*sizeof(unsigned int))
    };
    return game;
}


void clear(game_of_life *game) {
    for (int i = 0; i < game->board_height; i++) free(game->board[i]);
    free(game->board);
    free(game);
}

void reset(game_of_life game) {
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            game.board[i][j] = rand() & 1;
        }
    }
}

void step(game_of_life game) {
    int neighbors_count[game.board_height][game.board_width];
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            neighbors_count[i][j] = neighbors(i, j, game);
        }
    }
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            switch(neighbors_count[i][j]) {
                case 2:
                    game.board[i][j] = game.board[i][j];
                    break;
                case 3:
                    game.board[i][j] = 1;
                    break;
                default: // 0-1 && 4-8
                    game.board[i][j] = 0;
                    break;
            }
        }
    }
}

#ifdef TERMINAL
void show(game_of_life game) {
    for (int i = 0; i < game.height; i++) {
        for (int j = 0; j < game.width; j++) {
            printf("%i", game.board[i][j]);
        }
        printf("\n");
    }
}
#endif

int neighbors(int h, int w, game_of_life game) {
    int count = 0;
    if (h > 0) {
        if (w > 0) count += game.board[h - 1][w - 1];
        count += game.board[h - 1][w];
        if (w < game.width - 1) count += game.board[h - 1][w + 1];
    }
    if (h < game.height - 1) {
        if (w > 0) count += game.board[h + 1][w - 1];
        count += game.board[h + 1][w];
        if (w < game.width - 1) count += game.board[h + 1][w + 1];
    }
    if (w > 0) count += game.board[h][w - 1];
    if (w < game.width - 1) count += game.board[h][w + 1];
    return count;
}

#ifdef TERMINAL
int main()
{
    game = init(30, 60);

	int rounds = 1000;
	for (int i = 0; i < rounds; i++)
	{
		step(game);
		show(game);
		usleep(100000);
		if (i < rounds - 1) printf("%c[%iA", 27, game.height);
	}

    clear(game);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

/*
#include <stdlib.h>
#include <stdbool.h>
#ifdef TERMINAL
#include <stdio.h>
#include <unistd.h>
#endif
#include <emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIXEL 4

#define color(R, G, B, A) (R | G << 8 | B << 16 | A << 24)

#define ALIVE_COLOR color(0xda, 0x09, 0xff, 0xff)
#define DEAD_COLOR color(0, 0, 0, 0)

typedef struct {
    int height;
    int width;
    bool **board;
} game_of_life;

game_of_life game;
unsigned int *data;

game_of_life init(int height, int width);
void clear(game_of_life);
#ifdef TERMINAL
void show(game_of_life);
#endif
void step(game_of_life);

int neighbors(int h, int w, game_of_life);

void EMSCRIPTEN_KEEPALIVE start(int height, int width, int seed) {
    srand(seed);
    game = init(height / PIXEL - height % PIXEL, width / PIXEL - width % PIXEL);
    data = malloc(height*width*sizeof(unsigned int));
}

void EMSCRIPTEN_KEEPALIVE stop() {
    clear(game);
    free(data);
}

unsigned int* EMSCRIPTEN_KEEPALIVE render() {
	step(game);

	for (int i = 0; i < game.height; i++) {
		int iw = i * game.width;
        for (int j = 0; j < game.width; j++) {
            for (int k = 0; k < PIXEL; k++)
            {
                for (int l = 0; l < PIXEL; l++)
                {
    			data[(i * PIXEL + l) * game.width * PIXEL + j * PIXEL + k] = game.board[i][j] ? ALIVE_COLOR : DEAD_COLOR;
                }
            }

		}
	}
	return &data[0];
}

game_of_life init(int height, int width) {
    game_of_life game;
    bool **board;

    board = malloc(height*sizeof(bool*));
    for (int i = 0; i < height; i++) {
        board[i] = malloc(width*sizeof(bool));
        for (int j = 0; j < width; j++) {
            board[i][j] = rand() & 1;
        }
    }
    game = (game_of_life){.height = height, .width = width, .board = board};
    return game;
}

void clear(game_of_life game) {
    for (int i = 0; i < game.height; i++) free(game.board[i]);
    free(game.board);
}

void step(game_of_life game) {
    int neighbors_count[game.height][game.width];
    for (int i = 0; i < game.height; i++) {
        for (int j = 0; j < game.width; j++) {
            neighbors_count[i][j] = neighbors(i, j, game);
        }
    }
    for (int i = 0; i < game.height; i++) {
        for (int j = 0; j < game.width; j++) {
            switch(neighbors_count[i][j]) {
                case 2:
                    game.board[i][j] = game.board[i][j];
                    break;
                case 3:
                    game.board[i][j] = 1;
                    break;
                default: // 0-1 && 4-8
                    game.board[i][j] = 0;
                    break;
            }
        }
    }
}

#ifdef TERMINAL
void show(game_of_life game) {
    for (int i = 0; i < game.height; i++) {
        for (int j = 0; j < game.width; j++) {
            printf("%i", game.board[i][j]);
        }
        printf("\n");
    }
}
#endif

int neighbors(int h, int w, game_of_life game) {
    int count = 0;
    if (h > 0) {
        if (w > 0) count += game.board[h - 1][w - 1];
        count += game.board[h - 1][w];
        if (w < game.width - 1) count += game.board[h - 1][w + 1];
    }
    if (h < game.height - 1) {
        if (w > 0) count += game.board[h + 1][w - 1];
        count += game.board[h + 1][w];
        if (w < game.width - 1) count += game.board[h + 1][w + 1];
    }
    if (w > 0) count += game.board[h][w - 1];
    if (w < game.width - 1) count += game.board[h][w + 1];
    return count;
}

#ifdef TERMINAL
int main()
{
    game = init(30, 60);

	int rounds = 1000;
	for (int i = 0; i < rounds; i++)
	{
		step(game);
		show(game);
		usleep(100000);
		if (i < rounds - 1) printf("%c[%iA", 27, game.height);
	}

    clear(game);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
*/
