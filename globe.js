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
	const offset = window.screen.height / 16 /* Pixel offset from top */
	let closest = null
	let min = 10000
	for (let breve of breves) {
		let curr = Math.abs(breve.el.getBoundingClientRect().top - offset)
		if (curr < min) {
			closest = breve
			min = curr
		}
	}
	return closest
}

//== Special audio section

(function() {
	const audio = new Audio("/globe/volare.mp3")
	const volare = breves.find(({ el }) => el.textContent.includes("Volare")).el
	const title = volare.querySelector('h2')
	const playButton = document.createElement('span')
	playButton.textContent = ' ▶'
	audio.addEventListener("canplaythrough", () => {
		title.append(playButton)
		const toggle = () => { audio.paused ? audio.play() : audio.pause() }
		audio.addEventListener('pause', () => { playButton.textContent = ' ▶' })
		audio.addEventListener('play', () => { playButton.textContent = ' ⏸️' })
		playButton.addEventListener('click', toggle)
	})
})()

//= End special audio section

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
			breve.el.scrollIntoView({ behavior: 'smooth' })
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
		breve.el.scrollIntoView({ behavior: 'smooth' })
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
