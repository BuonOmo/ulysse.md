import WaveSurfer from 'https://unpkg.com/wavesurfer.js@7/dist/wavesurfer.esm.js'

document.querySelectorAll('.sound').forEach((el) => {
	const soundId = el.dataset.sound
	// Create progress element and append it to el
	const progressEl = document.createElement('progress')
	progressEl.classList.add('sound-progress')
	progressEl.setAttribute('max', 100)
	progressEl.setAttribute('value', 0)
	const progressWrapper = document.createElement('div')
	progressWrapper.classList.add('sound-wrapper')
	progressWrapper.append(progressEl)

	// Array.from is important, because childNodes is a live collection.
	// We want to fix the current state of the collection.
	const placeholder = Array.from(el.childNodes)

	const wavesurfer = WaveSurfer.create({
		container: el,
		waveColor: 'hsl(219 14% 71%)',
		progressColor: 'hsl(219 64% 71%)',
		url: `https://sunico.hd.free.fr/sound/${soundId}`,
		barWidth: 4,
		barGap: 2,
		barRadius: 4,
	})
	wavesurfer.on('load', () => {
		for (const node of placeholder) el.removeChild(node)
		el.prepend(progressWrapper)
	})
	wavesurfer.on('loading', (percent) => {
		progressEl.setAttribute('value', percent)
	})
	wavesurfer.once('interaction', () => {
		wavesurfer.play()
	})
	wavesurfer.on('redrawcomplete', () => {
		progressWrapper.remove()
	})
	wavesurfer.on('finish', () => {
		wavesurfer.destroy()
		for (const node of placeholder) el.appendChild(node)
	})
})

function rand(min, max = null) {
	if (max === null) {
		max = min
		min = 0
	}
	return Math.floor(Math.random() * (max - min + 1) + min)
}

function debounce(func, timeout = 300) {
	let timer
	return (...args) => {
		clearTimeout(timer)
		timer = setTimeout(() => { func.apply(this, args) }, timeout)
	}
}

function throttle(func, timeout = 300) {
	let throttling = false
	return (...args) => {
		if (throttling) return

		throttling = true
		func.apply(this, args)
		setTimeout(() => { throttling = false }, timeout)
	}
}

// Proxy for breves: if an element is not define, check for the dataset
const datasetHandler = {
	get(target, prop, receiver) {
		if (Reflect.has(target, prop)) return Reflect.get(target, prop, receiver)

		const datasetResult = target.el.dataset[prop]
		return isNaN(datasetResult) ? datasetResult : +datasetResult
	},
}
const breves = (function() {
	const els = Array.from(document.getElementById("breves").children).slice(1)
	const lastDate = els.at(0).dataset.epoch,
	      firstDate = els.at(-1).dataset.epoch
	const colorForEl = (el) => {
		const date = el.dataset.epoch
		const diff = lastDate - firstDate
		return 320 * ((date - firstDate) / diff)
	}

	return els.map(el =>
		new Proxy({
			el,
			loc: el.dataset.loc.split(',').map(e => +e),
			color: colorForEl(el),
			layer: null,
		}, datasetHandler))
})()

breves.forEach(({ el, color }) => {
	el.children[0].style.color = `hsl(${color}deg, 100%, 90%)`
})

const findBreve = () => {
	let closest = null
	let min = 10000
	const getCurr = window.screen.width < 768 ?
		(breve) => Math.abs(breve.el.getBoundingClientRect().left) :
		(breve) => Math.abs(breve.el.getBoundingClientRect().top - window.screen.height / 16 /* Pixel offset from top */)
	for (let breve of breves) {
		let curr = getCurr(breve)
		if (curr < min) {
			closest = breve
			min = curr
		}
	}
	return closest
}

mapboxgl.accessToken = 'pk.eyJ1IjoiaS1raWxsLXlvdSIsImEiOiJjbDlrcXUwZmgwaWk1M25wbG9lbHNpaDg1In0.Fd9r3AfGHSq3lqanlbWR3A'
const map = new mapboxgl.Map({
	container: 'map',
	style: 'mapbox://styles/mapbox/light-v10', /* Or outdoor v11 for more peps */
	center: findBreve().loc,
	zoom: 5,
	minZoom: 2,
	maxBounds: [[-100.0, -60.0], [9.0, 60.0]],
	projection: 'naturalEarth' // starting projection
})

const anchor = (breve) => {
	window.location.hash = breve.anchor
}

const flyToBreve = (map, breve) => {
	return map.flyTo({
		center: breve.loc,
		zoom: breve.zoom,
		essential: true // this animation is considered essential with respect to prefers-reduced-motion
	})
}

const scrollToBreve = (breve) => {
	const container = document.getElementById('breves')
	container.scrollTo({
		top: breve.el.offsetTop - container.getBoundingClientRect().top - 8,
		left: breve.el.offsetLeft,
		behavior: 'smooth'
	})
}

map.on('load', () => {
	map.addLayer({
		'id': 'route',
		'type': 'line',
		'source': {
			'type': 'geojson',
			'data': {
				'type': 'Feature',
				'properties': {},
				'geometry': {
					'type': 'LineString',
					'coordinates': breves.map(({ loc }) => loc)
				}
			}
		},
		'layout': { 'line-cap': 'round' },
		'paint': {
			'line-color': '#282c34',
			'line-width': 1
		}
	})

	breves.map((breve, i) => {
		const id = `pt${i}`
		breve.layer = id
		map.addSource(id, {
			'type': 'geojson',
			'data': {
				'type': 'Point',
				'coordinates': breve.loc
			}
		})
		map.addLayer({
			'id': id,
			'type': 'circle',
			'source': id,
			'paint': {
				'circle-radius': 4,
				'circle-color': `hsl(${breve.color}deg, 100%, 50%)`,
				'circle-stroke-width': 1,
				'circle-stroke-color': '#282c34'
			}
		})

		breve.el.addEventListener('mouseenter', () => {
			map.setPaintProperty(id, 'circle-stroke-width', 2)
			map.setPaintProperty(id, 'circle-radius', 12)
		})
		breve.el.addEventListener('mouseleave', () => {
			map.setPaintProperty(id, 'circle-stroke-width', 1)
			map.setPaintProperty(id, 'circle-radius', 4)
		})

		map.on('click', id, () => {
			anchor(breve)
			flyToBreve(map, breve)
			scrollToBreve(breve)
		})
		breve.el.addEventListener('click', () => {
			anchor(breve)
			flyToBreve(map, breve)
		})
		map.on('mouseenter', id, () => {
			map.getCanvas().style.cursor = 'pointer'
		})

		// Change it back to a pointer when it leaves.
		map.on('mouseleave', id, () => {
			map.getCanvas().style.cursor = ''
		})
	})

	// On start, check if the URL contains a breve to fly to.
	if (window.location.hash.length > 0) {
		const breve = breves.find(({ anchor }) => anchor === window.location.hash.slice(1))
		flyToBreve(map, breve)
		scrollToBreve(breve)
	}
})

// Fly to breve that is closer in view
document.getElementById('breves').addEventListener('scroll', throttle(() => {
	if (window.location.hash.length > 0) { history.pushState(null, null, ' ') }
	const breve = findBreve()
	flyToBreve(map, breve)
}, 40))

//== Show unread breves
/* DESIGN: When the user loads the page, we show a dot for every unread breves.
 * Then we automatically consider that they have read all of them and hence
 * mark the latest one as read.
 */
;(function() {
	let lastRead = localStorage.getItem('globe:lastRead')
	if (!lastRead) { lastRead = breves[0].anchor }
	for (let breve of breves) {
		if (breve.anchor === lastRead) { break }

		breve.el.classList.add('unread')
		breve.el.title = 'Brève non lue.'
	}
	localStorage.setItem('globe:lastRead', breves[0].anchor)
})()
//== End show unread breves
