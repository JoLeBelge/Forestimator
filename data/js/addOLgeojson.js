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
	  })
});

// c'est pas correct, car si c'est la carte IGN qui est visible mais que la couche apt2 a été défine auparavent, ça ne va pas afficher la carte IGN.


    // the variable is defined
    groupe = new ol.layer.Group({
		'title': 'parcellaire',
		attributions: 'Gembloux Agro-Bio Tech',
		 layers:[activeLayer, parcellaire, station]});


map.setLayerGroup(groupe);
//lays4select=[parcellaire];
