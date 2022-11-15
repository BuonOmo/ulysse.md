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

const zoom = 7
const breves = Array.from(document.getElementById("breves").children).slice(1).map(el => ({ el, date: el.dataset.date, loc: el.dataset.loc.split(',').map(e => +e), layer: null }))
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

mapboxgl.accessToken = 'pk.eyJ1IjoiaS1raWxsLXlvdSIsImEiOiJjbDlrcXUwZmgwaWk1M25wbG9lbHNpaDg1In0.Fd9r3AfGHSq3lqanlbWR3A'
const map = new mapboxgl.Map({
	container: 'map',
	style: 'mapbox://styles/mapbox/light-v10', /* Or outdoor v11 for more peps */
	center: findBreve().loc,
	zoom,
	minZoom: 2,
	maxBounds: [[-100.0, -60.0], [9.0, 60.0]],
	projection: 'naturalEarth' // starting projection
})

const flyToBreve = (map, breve) => {
	return map.flyTo({
		center: breve.loc,
		zoom,
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
				'circle-color': '#ddd',
				'circle-stroke-width': 1,
				'circle-stroke-color': '#282c34'
			}
		})
		map.on('click', id, () => {
			flyToBreve(map, breve)
			breve.el.scrollIntoView({ behavior: 'smooth' })
		})
		breve.el.addEventListener('click', () => {
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
})

// Fly to breve that is closer in view
document.getElementById('breves').addEventListener('scroll', throttle((e) => {
	const breve = findBreve()
	flyToBreve(map, breve)
}, 40))