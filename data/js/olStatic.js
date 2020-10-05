
var Layer1  = new ol.layer.Tile({
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
		  'FORMAT': 'image/png'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
});

var activePolygon1 = new ol.layer.Vector({
    source: new ol.source.Vector({
        format: new ol.format.GeoJSON(),
        url: 'NAME'
    }),
    style:new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'yellow',
		width: 3
	  }),
    fill: new ol.style.Fill({
      color: 'rgba(0, 0, 255, 0.1)'
    })
	  }),
	extent: [MINX,MINY,MAXX,MAXY]
});


var Layers1 = [
	new ol.layer.Group({
		'title': 'Carte',
		layers:[ Layer1 ]
	}),
	new ol.layer.Group({
		'title': 'parcellaire',
		layers:[parcellaire,activePolygon1]//
	})
];

var view1 = new ol.View({
	projection: projection,	
	center: [CENTERX, CENTERY],
	extent: [MINX,MINY,MAXX,MAXY],
	zoom: 8,
	minZoom: 1,
	maxZoom: 10
})


// create map here

var mapStat = new ol.Map({
   	//interaction: [],
interactions: ol.interaction.defaults({
    doubleClickZoom :false,
    dragAndDrop: false,
 	dragPan: false,
    keyboardPan: false,
    keyboardZoom: false,
    mouseWheelZoom: false,
    pointer: false,
    select: false
}),
   	controls: ol.control.defaults({
        attribution : false,
        zoom : false,
    }),
	renderer: 'canvas',
	layers: Layers1,
	target: 'MYID',
	view: view1,
	pixelRatio: 1
});

//mapStat.getView().fit(activePolygon1.getExtent(),mapStat.getSize());
mapStat.getView().fit([MINX,MINY,MAXX,MAXY],mapStat.getSize());

//mapStat.getView().fit(mapStat.getView().get(extent),mapStat.getSize());

// puis je change max and min zoom pour plus que la carte ne puisse bouger - plus utile maintenant que j'ai enlever les boutton zoom
//mapStat.getView().setMaxZoom(mapStat.getView().getZoom());
//mapStat.getView().setMinZoom(mapStat.getView().getZoom());

