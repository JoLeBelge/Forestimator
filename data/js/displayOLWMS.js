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
	})
});


if (typeof parcellaire !== 'undefined') {
    // the variable is defined
    groupe = new ol.layer.Group({
		'title': 'parcellaire',
		//attributions: 'Gembloux Agro-Bio Tech',
		 layers:[activeLayer, parcellaire, station]});
} else {
groupe = new ol.layer.Group({
		'title': 'aptitude',
		//attributions: 'Gembloux Agro-Bio Tech',
		layers:[activeLayer, communes, station]});

}
map.setLayerGroup(groupe);
