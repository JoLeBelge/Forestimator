parcellaire = new ol.layer.Vector({
    source: new ol.source.Vector({
        format: new ol.format.GeoJSON(),
        url: 'NAME'
    }),
    style:new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'blue',
		width: 2
	  })//,
// plus de fill car on utilise cette couche dans le rapport d'analyse surfacique, on ne veux pas de couleur au dessus de la couleur des cartes
    //fill: new ol.style.Fill({
    //  color: 'rgba(0, 0, 255, 0.1)'
    //})
		  }),
	extent: [MINX,MINY,MAXX,MAXY],
});

updateGroupeLayers();
map.getView().fit(parcellaire.getExtent());
