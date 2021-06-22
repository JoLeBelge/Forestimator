//https://gis.stackexchange.com/questions/68330/how-can-i-use-arcgis-server-10-mapservices-in-openlayers2
// trop vieux, ça c'est récent: https://openlayers.org/en/latest/examples/arcgis-image.html
// c'est du Dynamic Layers apparemment 

activeLayer  = new ol.layer.Tile({
	extent: extent,
	title: 'MYTITLE',
      source: new ol.source.TileArcGISRest({
          attributions: 'MYATTRIBUTION',
        url:
          //'https://geoservices.wallonie.be/arcgis/rest/services/PLAN_REGLEMENT/CADMAP_2020_PARCELLES/MapServer/'
          'MYURL'
      }),	
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
