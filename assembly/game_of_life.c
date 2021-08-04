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
#define PI_SHIP(game, h, w) insert_rle(game, h, w, 29, 99, "7bo83bo$6b3o81b3o$4b2ob3o20b3o9b3o9b3o9b3o20b3ob2o$5bo2bo"\
"b2o4bo4bo7bo3bo7bo3bo7bo3bo7bo3bo7bo4bo4b2obo2bo$2b2obo4bobob2ob2ob3o5b2o3b2o5b2o3b2o5b2o3b2o5b2o3b2o5b3ob2ob2obobo4"\
"bob2o$2b2obobo2bobo7b4o3b2obobob2o3b2obobob2o3b2obobob2o3b2obobob2o3b4o7bobo2bobob2o$2bo8b3obobob2o2b2ob2ob2ob2ob2ob"\
"2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o2b2obobob3o8bo$b2o7b2o12bobo3bobo3bobo3bobo3bobo3bobo3bobo3bobo3bobo12b2o7b2o2"\
"$5b3o15b2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2ob2o15b3o$4bo3bo81bo3bo$3b2o4bo11b57o11bo4b2o$2bobob2ob2o3b"\
"3o2b2obo4bo5bo8bo6bo6bo8bo5bo4bob2o2b3o3b2ob2obobo$b2obo4bob2ob3obo61bob3ob2obo4bob2o$o4bo3bo4bobo4bo4bobobobobo6bob"\
"ob2obobob2obobo6bobobobobo4bo4bobo4bo3bo4bo$12bo5b2o16bo2bo19bo2bo16b2o5bo$2o7b2o25bo2bo19bo2bo25b2o7b2o2$36bo2bo19b"\
"o2bo$37b2o21b2o5$49bo$48b3o$47bo3bo$37b2o8b2ob2o8b2o$37b2o21b2o!")
// https://www.conwaylife.com/wiki/Period-24_glider_gun
#define P48_LWSS_GUN(game, h, w) insert_rle(game, h, w, 67, 71, "10b2ob2o15b2ob2o$10b2obo4b2o5b2o4bob2o$13bo2bo3b2ob2"\
"o3bo2bo$13b2obo11bob2o6bo$16bo11bo8b3o$8bobo6bo3bobo3bo7b2o3bo$6b3ob3o5b3o3b3o8b3o2b2o$5bo7bo5bo5bo14bobo$6bob6o3b3o"\
"5b3o4b2o2b4ob3o$6b2o4bo4b3o5b3o4b2obo4bo3bo$8bo3b2o4b2o5b2o4bobobob2obo3b2o$7bobo2bobo5bo3bo5b2o3bob2obobobo$8bo2bo6"\
"bo2bobo2bo4bo3bo4bob2o$11bo3bobo3bobo3bo4b3ob4o2b2o$11bo3bo2bo2bobo2bo6bobo16bo3bo$11bo6b3o3b3o7b2o2b3o10bobobobo$11"\
"bo2bo20bo3b2o11b2ob2o$12bo7bo15b3o$10bobobo5bo16bo$9bobob2o33b2o9b2o$9bobo6bo3bo7bo17bob4ob4obo$10b2o4b3obob3o6bo17b"\
"o3bobo3bo$15bo3b2o4bo3b3o17bo3bobo3bo$15bob2o2b2obobo23b3o3b3o$16bo2b2o2b2obo$17b2o2b2obob2o$11b3o5bo2bobobo2bo6bo$1"\
"0b6o3b4ob2o2b2o7bo4bo2bo20bo2bo$10bob3obo6bo11b3o8bo23bo$9bo3bob2o4bobo7b2o9bo3bo19bo3bo$9b4ob4o3b2o9b2o9b4o20b4o$7b"\
"2o4bob3o13bo$5b2obob2obo2b2o$4b2o2bob2obob2o17b2o$4b3obo4b2o18b2o$4b4ob4o12b2o8bo$2o3b2obo3bo13b2o25b2o$2o3bob3obo13"\
"bo26bobo$6b6o40bo$4o4b3o29b2o4b2o2b2ob4o$o2bo35b2o5bo2bobo4bo$19b2o5b2o13bo6b2obob2o2b2o$20b2o5bo21bobo3b2o2bo$10bo8"\
"bo5bo23bobob2o2b2obo$10b2o11b4o23bo4bo4bo$2b3o6b2o9bo4bo2b2o14b2o3b3obob3o4b2o$3bobo4b2o9bob2ob2o2bo9b2o3b2o6b2ob2o6"\
"bobo$4b2o15bo2bo3b2o2bo6bo2bo4bo13b2obobo$20b2obo2b2o2b5o3bobobo18bobobo$4b2o12bo4bobobobo5bobo2bob2o19bo$3bobo4b2o6"\
"bob2o2bo2bobob2ob2ob2obo3bo17b2o$2b3o6b2o5bo3bobob2obob2obo5b2ob3o15b3o$10b2o8b2obobo3bo5bobobo3b3o15b3o$10bo10bobob"\
"o3bob3obo2b2o3b3o4bo5bo4b3o$21bo2bo5b2o2b2o7b3o3b3o3b3o3b3o3bo$22b2o9bobo2bob2o2bo3bo2b2ob2o2bo3b2o2bobo$o2bo14b2o8b"\
"3ob2ob4obob2o4b3o5b3o7bo2bo$4o3b2o10bo8bo2bo3bo3b2obo20bo$6bo3b9o10bobob2ob2o3bo2bo18b6obo$2o4bobob7o2b3o6b2ob2obobo"\
"b2obob2o17bo7bo$2o3b2ob2o2b4o2bo2bo18bobo20b3ob3o$8bo2bo7b2o19bobo8b2ob2o9bobo$5b2obobobo28bo7b2obobob2o$6bo4bo32b2o"\
"3bo7bo3b2o$6bob3o33bo4b3o3b3o4bo$7b2o32b2obo17bob2o$41b2ob2o15b2ob2o!")
typedef struct {
    int height;
    int board_height;
    int board_width;
    int width;
    int pixel;
    bool **board;
    bool is_tor;
    unsigned int *data;
} game_of_life;

game_of_life *games[GAMES];

#ifndef TERMINAL
void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index, bool is_tor, char pattern);
unsigned int* EMSCRIPTEN_KEEPALIVE render(int index);
#endif

game_of_life *init(int height, int width, int pixel, bool is_tor);
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
void EMSCRIPTEN_KEEPALIVE start(int height, int width, int pixel, int seed, int index, bool is_tor, char pattern) {
    srand(seed * index);
    if (!games[index]) games[index] = init(height, width, pixel, is_tor);
    switch (pattern) {
        case 'g':
            empty(*games[index]);
            if (P48_LWSS_GUN(*games[index], 2, 2)) fill_random(*games[index]);
            break;
        case 's':
            empty(*games[index]);
            if (PI_SHIP(*games[index], height / pixel - 40, 0)) fill_random(*games[index]);
            break;
        case 'r':
        default:
            fill_random(*games[index]);
            break;
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

game_of_life *init(int height, int width, int pixel, bool is_tor) {
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
        .is_tor = is_tor,
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

    if (game.is_tor) {
#define COUNT(hdim, wdim) do {                       \
count += game.board                                  \
    [(hdim + game.board_height) % game.board_height] \
    [(wdim + game.board_width) % game.board_width];  \
} while(0)

        COUNT(h - 1, w - 1);
        COUNT(h - 1, w + 1);
        COUNT(h + 1, w - 1);
        COUNT(h + 1, w + 1);
        COUNT(h - 1, w);
        COUNT(h + 1, w);
        COUNT(h, w - 1);
        COUNT(h, w + 1);

#undef COUNT
    } else {
#define COUNT(hdim, wdim) do {            \
count += (                                \
    hdim < 0 || hdim >= game.board_height \
 || wdim < 0 || wdim >= game.board_width  \
 ) ? 0 : game.board[hdim][wdim];          \
} while(0)

        COUNT(h - 1, w - 1);
        COUNT(h - 1, w + 1);
        COUNT(h + 1, w - 1);
        COUNT(h + 1, w + 1);
        COUNT(h - 1, w);
        COUNT(h + 1, w);
        COUNT(h, w - 1);
        COUNT(h, w + 1);

#undef COUNT
    }
    return count;
}

#ifdef TERMINAL
int main()
{
    srand(time(NULL));
    game_of_life *game;
    game = init(60, 101, 1, false);
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
