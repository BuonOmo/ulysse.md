#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#ifdef TERMINAL
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#else
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PIXEL 5
#define GAMES 10

#define color(R, G, B, A) (R | G << 8 | B << 16 | A << 24)

#define ALIVE_COLOR color(0xda, 0x09, 0xff, 0xff)
#define DEAD_COLOR color(0, 0, 0, 0)

#define BLOCK(game, h, w) insert_rle(game, h, w, 2, 2, "2o$2o!")
// https://www.conwaylife.com/wiki/Pi_ship_1
#define PI_SHIP(game, h, w) insert_rle(game, h, w, 29, 99, "7bo83bo$6b3o81b3o$4b2ob3o20b3o9b3o9b3o9b3o20b3ob2o$5bo2bob2o4bo4bo7bo3bo7bo3bo7bo3bo7bo3bo7bo4bo4b2obo2bo$2b2obo4bobob2ob2ob3o5b2o3b2o5b2o3b2o5b2o3b2o5b2o3b2o5b3ob2ob2obobo4bob2o$2b2obobo2bobo7b4o3b2obobob2o3b2obobob2o3b2obobob2o3b2obobob2o3b4o7bobo2bobob2o$2bo8b3obobob2o2b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o2b2obobob3o8bo$b2o7b2o12bobo3bobo3bobo3bobo3bobo3bobo3bobo3bobo3bobo12b2o7b2o2$5b3o15b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o15b3o$4bo3bo81bo3bo$3b2o4bo11b57o11bo4b2o$2bobob2ob2o3b3o2b2obo4bo5bo8bo6bo6bo8bo5bo4bob2o2b3o3b2ob2obobo$b2obo4bob2ob3obo61bob3ob2obo4bob2o$o4bo3bo4bobo4bo4bobobobobo6bobob2obobob2obobo6bobobobobo4bo4bobo4bo3bo4bo$12bo5b2o16bo2bo19bo2bo16b2o5bo$2o7b2o25bo2bo19bo2bo25b2o7b2o2$36bo2bo19bo2bo$37b2o21b2o5$49bo$48b3o$47bo3bo$37b2o8b2ob2o8b2o$37b2o21b2o!")
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

#ifndef TERMINAL
void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index);
unsigned int* EMSCRIPTEN_KEEPALIVE render(int index);
#endif

game_of_life *init(int height, int width, int pixel);
void clear(game_of_life*);
#ifdef TERMINAL
void show(game_of_life);
#endif
void step(game_of_life);
void fill_random(game_of_life);
void empty(game_of_life);
int insert_rle(game_of_life, int h, int w, int height, int width, char *rle); // 1 error, 0 ok

int neighbors(int h, int w, game_of_life);

#ifndef TERMINAL
void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index) {
    srand(seed * index);
    if (!games[index]) games[index] = init(height, width, pixel);
    if (height == 626) {
        // fill_random(*games[index]);
        empty(*games[index]);
        PI_SHIP(*games[index], 626 / 2 - 40, 2);
    } else {
        fill_random(*games[index]);
    }
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
#endif

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

void fill_random(game_of_life game) {
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            game.board[i][j] = rand() & 1;
        }
    }
}

void empty(game_of_life game) {
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            game.board[i][j] = 0;
        }
    }
}

int insert_rle(game_of_life game, int h_init, int w_init, int height, int width, char *rle) {
    char numbuff[100];
    int numbuff_index = 0;
    int to_insert = 1;
    int h = h_init;
    int w = w_init;
    // cannot insert rle there.
    if (game.board_height - h < height || game.board_width - w < width) return 1;

#define INSERT(code) do {                        \
if (numbuff_index) {                             \
    numbuff[numbuff_index] = '\0';           \
    to_insert = (int)strtol(numbuff, NULL, 10);  \
    if (!to_insert) return 1;                    \
}                                                \
code                                             \
numbuff_index = 0;                               \
to_insert = 1;                                   \
} while(0)

    //printf("len: %lu\n", strlen(rle));
    char c;
    for (size_t i = 0, len = strlen(rle); i < len; i++) {
        c = rle[i];
        //printf("current: %c\n", c);
        switch (c) {
            case '0' ... '9':
                //printf("0..9 %c\n", c);
                numbuff[numbuff_index] = c;
                if (++numbuff_index > 99) return 1;
                break;
            case 'b': // dead cell
                INSERT(
                    w += to_insert;
                );
                break;
            case 'o': // alive cell
                //printf("o\n");
                INSERT(
                    for (int i = 0; i < to_insert; i++) game.board[h][w++] = true;
                );
                //printf("end o\n");
                break;
            case '$':
                //printf("$\n");
                INSERT(
                    h+= to_insert;
                    w = w_init;
                );
                //printf("end $\n");
                break;
            case '!':
                return numbuff_index > 0;
            case '\r':
            case '\n':
                break;
        }
    }

    return 1;
#undef INSERT
};

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
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            printf("%c", game.board[i][j] ? 'O' : '.');
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
    srand(time(NULL));
    game_of_life *game;
    game = init(60, 101, 1);
    if (PI_SHIP(*game, 29, 2)) abort();

	int rounds = 1000;
	for (int i = 0; i < rounds; i++)
	{
		step(*game);
		show(*game);
		usleep(100000);
		if (i < rounds - 1) printf("%c[%iA", 27, game->height);
	}

    clear(game);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
