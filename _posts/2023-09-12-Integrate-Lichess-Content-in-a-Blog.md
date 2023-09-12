---
layout: post
title: "Integrate Lichess Content in a Blog"
author: Ulysse
---

See [Lichess Documentation on How to Embed](https://lichess.org/developers#embed-game).

<iframe src="https://lichess.org/tv/frame?theme=brown&bg=dark" style="width: 400px; height: 444px; margin: 0 auto; display: block;" allowtransparency="true" frameborder="0"></iframe>

Another more flexible solution is to directly use [Chessground](https://github.com/lichess-org/chessground), the _Mobile/Web chess UI for lichess.org_.

<style>
	.chessground {
		width: 500px;
		height: 500px;
		display: block;
		margin: 0 auto;
	}
</style>

 <div class="chessground" id="board"></div>

 <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/chessground@9.0.2/assets/chessground.base.css">
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/chessground@9.0.2/assets/chessground.brown.css">
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/chessground@9.0.2/assets/chessground.cburnett.css">

<script type="module">
	import { Chessground } from 'https://cdn.jsdelivr.net/npm/chessground@9.0.2/+esm'
	Chessground(document.getElementById('board'), {
		fen: 'r2q2k1/1p6/p2p4/2pN1rp1/N1Pb2Q1/8/PP1B4/R6K b - - 2 25',
		turnColor: 'white',
	})
</script>

And another example is this blogpost, which source can be found [on Github](https://github.com/BuonOmo/ulysse.md/blob/main/_posts/2023-09-12-Integrate-Lichess-Content-in-a-Blog.md).
