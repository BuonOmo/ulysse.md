---
layout: post
title: Show cursor click (script injection in chrome)
author: Ulysse
---

Copy paste the code below in any console (or read it first). You can also use
[whitchcraft](https://luciopaiva.com/witchcraft/) if you want this feature
everywhere. Or look at my [chrome-scripts](https://github.com/BuonOmo/chrome-scripts)
for meaningful browser hacks.

```js
const div = document.createElement('div')
div.id = "clickboom"
div.style = `
	position: fixed;
	width: 60px;
	height: 60px;
	display: none;
`

const inner = document.createElement('div')
inner.style = `
	border-radius: 50%;
	border: 2px solid #61afef;
	width: 0px;
	height: 0px;
`
document.body.append(div)
div.append(inner)

let t
function animate() {
	if (t) clearTimeout(t)
	div.style.display = 'block'
	const nextFrame = (num) => {
		t = setTimeout(() => {
			inner.style.width = `${num}px`
			inner.style.height = `${num}px`
			inner.style.margin = `${30 - num / 2}px`
			if (num < 60) nextFrame(num + 1)
			else div.style.display = 'none'
		}, 2);
	}
	nextFrame(0)
}

document.body.addEventListener('mousemove', e => { div.style.left = `${e.clientX-30}px`; div.style.top = `${e.clientY-30}px` })
document.body.addEventListener('click', e => {
	div.style.display = "block"
	animate()
})
```

<script async defer>
	eval(document.querySelector('.language-js').innerText)
</script>
