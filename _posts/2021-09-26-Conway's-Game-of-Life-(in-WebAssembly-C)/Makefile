
libs_h=$(wildcard lib/*.h)
libs_c=$(libs_h:.h=.c)
CC=gcc
CFLAGS=-Wall
CPPFLAGS=-Wall

game_of_life.wasm: ## generate webassembly script (default)

FMT_TITLE='\\033[7\;1m'
FMT_PRIMARY='\\033[36m'
FMT_END='\\033[0m'

.PHONY: help
help:
	@printf "Usage: make \033[36m<target>\033[0m\n"
	@printf "\n"
	@awk ' \
			BEGIN {FS = ":.*##"} \
			/^[.a-zA-Z0-9_-]+:.*##/ { printf "  $(FMT_PRIMARY)%-30s$(FMT_END) %s\n", $$1, $$2 } \
	' $(MAKEFILE_LIST)

.PHONY: clean
clean: ## Clean what should be untracked
	@rm -f *.o lib/*.o *.out

.PHONY: rle
rle: term.out ## Prints to terminal, useful for generating comments of RLE, or debugging.
	./$<

game_of_life.gif: gif.out ## Generate a gif, perfect for Open Graph image!
	./$< $@

# Dependencies

lib/game_of_life.o: lib/game_of_life.c lib/game_of_life.h
lib/gifenc.o: lib/gifenc.c lib/gifenc.h

gif.out: gif.o lib/game_of_life.o lib/gifenc.o
term.out: term.o lib/game_of_life.o
game_of_life.wasm: wasm.o lib/game_of_life.o

# Rules

%.o: %.c
	$(CC) $(CPPFLAGS) -c $< -o $@


%.out: %.o
	$(CC) $(CFLAGS) $^ -o $@

game_of_life.wasm: wasm.o
	$(CC) $(CFLAGS) $^ -o $@

%.wasm: CC = emcc
%.wasm: CPPFLAGS = -O3 --no-entry
%.wasm: CFLAGS = -O3 -gsource-map -s EXPORTED_FUNCTIONS="['_start', '_render']" --no-entry
