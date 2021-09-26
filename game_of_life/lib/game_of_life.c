#include "game_of_life.h"

game_of_life *gol_init(size_t height, size_t width, size_t pixel, bool is_tor, set_pixel_fn set_pixel) {
	game_of_life *game;
	bool **board;
	size_t board_height;
	size_t board_width;

	board_height = height / pixel;
	board_width = width / pixel;


	board = malloc(sizeof(*board)*board_height);
	for (size_t i = 0; i < board_height; i++)
		board[i] = malloc(sizeof(*board[i])*board_width);
	game = malloc(sizeof(*game));
	*game = (game_of_life){
		.height = height,
		.width = width,
		.board_height = board_height,
		.board_width = board_width,
		.pixel = pixel,
		.board = board,
		.is_tor = is_tor,
		.set_pixel = set_pixel
	};
	return game;
}

void gol_clear(game_of_life *game) {
    for (int i = 0; i < game->board_height; i++) free(game->board[i]);
    free(game->board);
    free(game->data);
    free(game);
}

void gol_step(game_of_life game) {
	int neighbors_count[game.board_height][game.board_width];
	for (int i = 0; i < game.board_height; i++) {
		for (int j = 0; j < game.board_width; j++) {
			neighbors_count[i][j] = gol_neighbors(game, i, j);
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

static void render_circle(game_of_life game, int h, int w) {
	float r = ((float)game.pixel) / 2.0;
	float r2 = r*r;
	bool self = game.board[h][w];
	// The 0.5 delta allows same computation for both odd and even
	// diameters.
	float h_center = (float)(h * game.pixel) + r - 0.5;
	float w_center = (float)(w * game.pixel) + r - 0.5;
	float pos_init = game.pixel % 2 ? 0 : 0.5;
	//  NW  N  NE
	//  W       E
	//  SW  S  SE
	bool N  = gol_get(game, h - 1, w);
	bool NE = gol_get(game, h - 1, w + 1);
	bool E  = gol_get(game, h, w + 1);
	bool SE = gol_get(game, h + 1, w + 1);
	bool S  = gol_get(game, h + 1, w);
	bool SW = gol_get(game, h + 1, w - 1);
	bool W  = gol_get(game, h, w - 1);
	bool NW = gol_get(game, h - 1, w - 1);
	#define SET_NW(px_val) (game.set_pixel(game, (size_t)(h_center - i), (size_t)(w_center - j), px_val))
	#define SET_NE(px_val) (game.set_pixel(game, (size_t)(h_center - i), (size_t)(w_center + j), px_val))
	#define SET_SW(px_val) (game.set_pixel(game, (size_t)(h_center + i), (size_t)(w_center - j), px_val))
	#define SET_SE(px_val) (game.set_pixel(game, (size_t)(h_center + i), (size_t)(w_center + j), px_val))
	#define HAS_FILLED_ANGLE(self_filled, side_a, side_b, diagonal) (((side_a) & (side_b)) | (self_filled) & ((side_a) | (side_b) | (diagonal)))
	for (float i = pos_init; i < r; i++) {
		float i2 = i*i;
		for (float j = pos_init; j < r; j++) {
			if (j*j+i2 <= r2) {
				SET_NW(self);
				SET_NE(self);
				SET_SW(self);
				SET_SE(self);
			} else {
				SET_NW(HAS_FILLED_ANGLE(self, W, N, NW));
				SET_NE(HAS_FILLED_ANGLE(self, N, E, NE));
				SET_SW(HAS_FILLED_ANGLE(self, S, W, SW));
				SET_SE(HAS_FILLED_ANGLE(self, S, E, SE));
			}
	   }
	}
	#undef SET_NW
	#undef SET_NE
	#undef SET_SW
	#undef SET_SE
	#undef HAS_FILLED_ANGLE
}

void gol_render(game_of_life game) {
	for (int i = 0; i < game.board_height; i++) {
		for (int j = 0; j < game.board_width; j++) {
			if (game.pixel == 1) {
				game.set_pixel(game, i, j, game.board[i][j]);
			} else {
				render_circle(game, i, j);
			}
		}
	}
}

bool gol_get(game_of_life game, size_t h, size_t w) {
	if (game.is_tor) {
		return game.board
			[(h + game.board_height) % game.board_height]
			[(w + game.board_width) % game.board_width];
	}

	if (h < 0 || h >= game.board_height
		|| w < 0 || w >= game.board_width)
		return 0;

	return game.board[h][w];
}

int gol_insert_rle(game_of_life game, size_t h_orig, size_t w_orig, size_t height, size_t width, const char *rle) {
	char numbuff[100];
	size_t numbuff_index = 0;
	size_t to_insert = 1;
	size_t h = h_orig;
	size_t w = w_orig;
	// cannot insert rle there.
	if (game.board_height - h < height || game.board_width - w < width) return 1;

	#define INSERT(code) do {                        \
	if (numbuff_index) {                             \
		numbuff[numbuff_index] = '\0';               \
		to_insert = (int)strtol(numbuff, NULL, 10);  \
		if (!to_insert) return 1;                    \
	}                                                \
	code                                             \
	numbuff_index = 0;                               \
	to_insert = 1;                                   \
	} while(0)

	char c;
	for (size_t i = 0, len = strlen(rle); i < len; i++) {
		c = rle[i];
		switch (c) {
			case '0' ... '9':
				numbuff[numbuff_index] = c;
				if (++numbuff_index > 99) return 1;
				break;
			case 'b': // dead cell
				INSERT(
					w += to_insert;
				);
				break;
			case 'o': // alive cell
				INSERT(
					for (int i = 0; i < to_insert; i++) game.board[h][w++] = true;
				);
				break;
			case '$':
				INSERT(
					h+= to_insert;
					w = w_orig;
				);
				break;
			case '!':
				return numbuff_index > 0;
			case '\r':
			case '\n':
				break;
		}
	}
	#undef INSERT
	return 1;
}

void gol_fill_random(game_of_life game) {
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            game.board[i][j] = rand() & 1;
        }
    }
}

void gol_empty(game_of_life game) {
    for (int i = 0; i < game.board_height; i++) {
        for (int j = 0; j < game.board_width; j++) {
            game.board[i][j] = 0;
        }
    }
}

size_t gol_neighbors(game_of_life game, size_t h, size_t w) {
	int count = 0;

	count += gol_get(game, h - 1, w - 1);
	count += gol_get(game, h - 1, w + 1);
	count += gol_get(game, h + 1, w - 1);
	count += gol_get(game, h + 1, w + 1);
	count += gol_get(game, h - 1, w);
	count += gol_get(game, h + 1, w);
	count += gol_get(game, h, w - 1);
	count += gol_get(game, h, w + 1);

	return count;
}
