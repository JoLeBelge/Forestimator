parcellaire = new ol.layer.Vector({
    source: new ol.source.Vector({
        format: new ol.format.GeoJSON(),
        url: 'NAME'
    }),
    style:new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'blue',
		width: 2
	  }),
    fill: new ol.style.Fill({
      color: 'rgba(0, 0, 255, 0.1)'
    })
	  }),
	extent: [MINX,MINY,MAXX,MAXY],
});

// the variable is defined
groupe = new ol.layer.Group({
	'title': 'parcellaire',
	attributions: 'Gembloux Agro-Bio Tech',
	layers:[activeLayer, parcellaire, station]
	});

map.setLayerGroup(groupe);
map.getView().fit(parcellaire.getExtent(),map.getSize());


