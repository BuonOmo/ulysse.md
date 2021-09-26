---
layout: post
title: Conway's Game of Life (in WebAssembly C)
author: Ulysse
description: |
    <canvas class="game-of-life" height="10" data-pixel="2"></canvas>
    <script>
    	const wasm = async function wasm() {
    		if (!wasm.data)
    			wasm.data = WebAssembly.instantiateStreaming(fetch('/game_of_life/game_of_life.wasm'))
    		return (await wasm.data).instance.exports
    	}

    	const games = Array.from(document.getElementsByClassName('game-of-life'))
    	let offsetIndex = 0;
    	games.forEach(async (canvas, index) => {
    		const ctx = canvas.getContext('2d')
    		const width = canvas.width = document.querySelector('ul').clientWidth;
    		const height = canvas.height
    		const offset = offsetIndex

    		const { start, render, memory } = await wasm()
    		start(height, width, canvas.dataset.pixel || 5, Date.now(), index)
    		setInterval(() => {
    			const pointer = render(index)
    			const data = new Uint8ClampedArray(memory.buffer, pointer, width * height * 4)
    			const img = new ImageData(data, width, height)
    			ctx.putImageData(img, 0, 0)
    		}, canvas.dataset.interval || 100)
    	})
    	window.addEventListener('keydown', e => {
    		// if (e.key !== 'Enter') return

    		games.forEach(async (canvas, index) => {
    			const { start } = await wasm()
    			start(canvas.height, canvas.width, Date.now(), index)
    		})
    	})
    </script>
meta:
  description: |
    My journey through the Game of Life, or some blobs moving around.
  image: https://raw.githubusercontent.com/BuonOmo/ulysse.md/main/game_of_life/game_of_life.gif
---

<canvas height="25" class="game-of-life" data-interval="160" data-pixel="5" data-frozen></canvas>

It has been a while since I wanted to write my web version of the game of life. It
turns out I just needed this excuse: a Game of life may appear as slow, and use a
lot of CPU when ran in JS. So I have two things to do now, the game of life AND some
WebAssembly, let's go!

# WebAssembly

**TL;DR:** front-end calls an assembly function, which updates a state array, and we can
then render it.

This is my first time digging through web assembly, and I'll have to admit: it still
feels like a very young techno. There is few documentation, and there are lot of people
doing things various ways. The fact that WebAssembly can be used in various languages
really doesn't help in that matter...

However, following [emscripten.org]'s documentation and some examples on GitHub, I
could manage my way through, and hopefully the code used for this article will be yet
another example for one wanting to use the power of WebAssembly.

Basically, WebAssembly works by sharing a `Uint8ClampedArray` between the front-end
and the assembly code. For instance, the `render` function will update an `unsigned int`
array in C:

```c
#define DATA(game) ((unsigned int*)((game).data))

unsigned int* EMSCRIPTEN_KEEPALIVE render(int index) {
	game_of_life game = *games[index];
	gol_step(game);
	gol_render(game);
	return &(DATA(game))[0];
}
```

And this array will be interpreted as an image in JavaScript:

```js
const pointer = render(index)
const size = width * height * 4 // must match size of the C array
const data = new Uint8ClampedArray(memory.buffer, pointer, size)
const img = new ImageData(data, width, height)
canvas.getContext('2d').putImageData(img, 0, 0)
```

Note the `EMSCRIPTEN_KEEPALIVE` used in the C code, it is only here to say: this
function will be called from JavaScript.

The hard part for me was debugging, for now I can only hint that you can hack
your way through the shared data to write a String to it in C, and then read it
in JS. I could not find a way to bind printf correctly however. Hence if you're
reading this article and know more than me about emscripten, please reach to me
to talk about debugging. Otherwise, wait for another article on that topic.

<canvas height="50" class="game-of-life" data-interval="160" data-pixel="10"></canvas>

# The lifer community

If you do not know about Conway's [Game of Life] yet, it is a game with very few
rules, which boils down to this switch statment:

```c
/* A 2d space of cells either alive (1) or dead (0). */
int [][] board;
/* For each cell, the count of alive adjacent neighbors up to 8 */
int [][] neighbors_count;
/* Each turn we update every cell based on its neighbors. */
switch(neighbors_count[i][j]) {
	case 2:
	/* An alive cell will survive on the nex generation
	 * if it has exactly 2 neighbors. */
		board[i][j] = board[i][j];
		break;
	case 3:
	/* A cell — either dead or alive — will live on next
	 * generation if it has exactly 3 neighbors. */
		board[i][j] = 1;
		break;
	/* If a cell has neither 2 nor 3 neighbors, it cannot live
	 * next round. */
	default: // 0-1 && 4-8
		board[i][j] = 0;
		break;
}
```

The idea is brilliant for it is really simple, yet it gives a lot of possiblities.
Such that the game has more than 50 years and still has a large community discovering
new forms of life within its rules.

<canvas height="700" class="game-of-life" data-interval="100" data-pixel="7" data-is-tor="1" data-pattern="s"></canvas>

_Press any key to see it from the beginning._

Forms of life, such as the one above are divided in a lot of categories, such as
spaceships (things that move) or still lives (things that don't) and
[many more](https://www.conwaylife.com/wiki/Category:Patterns).

For more on the game of life itself, I suggets reading the [wiki][conwaylife.com].


# What thumbnail for this article?

I know of WebAssembly, I know of Conway's Game of Life, but the journey is not over yet!
A good article has a good thumbnail, and it is obvious that I have to make some
kind of gif out of my Game of Life implementation.

Fortunately, I did not have to use one of the huge C image library, as there is a
really straightforward, less than 1k LOC lib on GitHub: [lecram/gifenc].

<canvas height="276" class="game-of-life" data-interval="40" data-pixel="4" data-pattern="g"></canvas>

Of course, this was not _that_ easy, support for transparency was missing. However,
as this is Open Source and people are nice, I quickly could discuss over transparency
support in [#11](https://github.com/lecram/gifenc/pull/11). And I discovered something:
working on gif makes for really cool discussions full of colors, and easy reproductions!

Now the PR is merged, and if you share this article you'll directly share a Game of Life
example :)

# The end of the journey

I've wrote a simple implemantation used for this article, feel free to
[use it][game_of_life.h] to generate [a gif][gif.c], [ascii art][term.c] or
[some WebAssembly][wasm.c] as in this article.

I have to thanks the ruby language once again, for it helped me to find a way
to generalise the `game_of_life.h` implementation with the `set_pixel` and
`data` elements, which are a pale copy of C extension data integration in ruby.

Of course, I'm welcoming PRs and criticism :)

<canvas height="700" class="game-of-life" data-pixel="1"></canvas>

[emscripten.org]: https://emscripten.org/
[conwaylife.com]: https://www.conwaylife.com/wiki/Main_Page
[lecram/gifenc]: https://github.com/lecram/gifenc
[game of life]: https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
[game_of_life.h]: https://github.com/BuonOmo/ulysse.md/blob/main/game_of_life/lib/game_of_life.h
[gif.c]: https://github.com/BuonOmo/ulysse.md/blob/main/game_of_life/gif.c
[term.c]: https://github.com/BuonOmo/ulysse.md/blob/main/game_of_life/term.c
[wasm.c]: https://github.com/BuonOmo/ulysse.md/blob/main/game_of_life/wasm.c

<script>
const wasm = async function wasm() {
	if (!wasm.data)
		wasm.data = WebAssembly.instantiateStreaming(fetch('/game_of_life/game_of_life.wasm'),
			// debug attempt
			{ env: { _log(x) { console.log(x) }}}
		)
	return (await wasm.data).instance.exports
}

// debug hack
// window.debug = async function debug() {
// 	const { memory, debug_pointer, debug_size } = await wasm()
// 	if (!debug.pointer) debug.pointer = debug_pointer()
// 	if (!debug.decoder) debug.decoder = new TextDecoder()

// 	const data = new Uint8ClampedArray(memory.buffer, debug.pointer, debug_size())
// 	console.debug(debug.decoder.decode(data))
// }

const t = Date.now()
const games = Array.from(document.getElementsByClassName('game-of-life'))
games.forEach(async (canvas, index) => {
	const ctx = canvas.getContext('2d')
	const width = canvas.width = document.querySelector('section').clientWidth;
	const height = canvas.height
	let lastUpdate = Date.now()
	const { start, render, memory } = await wasm()
	const update = () => {
		const interval = canvas.dataset.interval
		const now = Date.now()
		if (interval === undefined || now - lastUpdate >= interval) {
			lastUpdate = now
			try {
				const pointer = render(index, now - t)
				const data = new Uint8ClampedArray(memory.buffer, pointer, width * height * 4)
				const img = new ImageData(data, width, height)
				ctx.putImageData(img, 0, 0)
			} catch (e) {
				console.log(canvas, index)
				console.error(e)
				throw (e)
			}
		}
		window.requestAnimationFrame(update)
	}
	start(
		height, width,
		canvas.dataset.pixel || 5,
		Date.now(), // seed
		index,
		canvas.dataset.isTor || false,
		(canvas.dataset.pattern || 'r').charCodeAt(0)
	)
	window.requestAnimationFrame(update);
})
window.addEventListener('keydown', e => {
	games.forEach(async (canvas, index) => {
		const { start } = await wasm()
		start(
			canvas.height, canvas.width,
			canvas.dataset.pixel || 5,
			Date.now(), // seed
			index,
			canvas.dataset.isTor || false,
			(canvas.dataset.pattern || 'r').charCodeAt(0)
		)
	})
})
</script>
