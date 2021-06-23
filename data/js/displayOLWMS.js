activeLayer  = new ol.layer.Tile({
	extent: extent,
	title: 'MYTITLE',
	source: new ol.source.TileWMS({
		 preload: Infinity,
		title: 'MYTITLE',
		url: 'MYURL',
		crossOrigin: 'null',
		attributions: 'MYATTRIBUTION',
		params: {
		  'LAYERS': 'MYLAYER',
		  'TILED': false, // avant était à true mais ça faisait bugger cartoweb_topo
		  'FORMAT': 'image/png'
		},
		//tileGrid: tileGrid,
		projection: 'EPSG:31370',
		//serverType: 'mapserver',
	}),
	opacity: 0.5
});
activeLayers["MYCODE"] = activeLayer;

updateGroupeLayers();
