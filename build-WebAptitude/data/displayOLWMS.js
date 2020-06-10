apt2  = new ol.layer.Tile({
	extent: extent,
	title: 'MYTITLE',
	source: new ol.source.TileWMS({
		 preload: Infinity,
		title: 'MYTITLE',
		url: 'MYURL',
		crossOrigin: 'null',
		attributions: 'Gembloux ABT',
		params: {
		  'LAYERS': 'MYLAYER',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
});


if (typeof parcellaire !== 'undefined') {
    // the variable is defined
    groupe = new ol.layer.Group({
		'title': 'parcellaire',
		attributions: 'Gembloux Agro-Bio Tech',
		 layers:[apt2, parcellaire, station]});
} else {
groupe = new ol.layer.Group({
		'title': 'aptitude',
		attributions: 'Gembloux Agro-Bio Tech',
		layers:[IGN, apt2, communes, station]});
IGN.setVisible(false);
}
map.setLayerGroup(groupe);
