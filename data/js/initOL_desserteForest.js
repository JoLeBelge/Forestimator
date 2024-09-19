proj4.defs('EPSG:31370',
'+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs');
ol.proj.proj4.register(proj4);

projection = new ol.proj.Projection({
	code: 'EPSG:31370',
	extent: [42247, 21148, 295176, 167719] // full RW
});

extent = [42247, 21148, 295176, 167719];// full RW
ol.proj.get('EPSG:31370').setExtent(extent);
// ol 6 va pas. Si si ol 6.4.3 ça roule sans problème, et l'échelle est bonne avec ol.proj.proj4.register(proj4)+ ol.proj.get('EPSG:31370').setExtent(extent);


var resolutions = new Array(22);
var startResolution = (extent[2]-extent[0]) / 512;
for (var i = 0, ii = resolutions.length; i < ii; ++i) {
	resolutions[i] = startResolution / Math.pow(2, i);	
}
tileGrid = new ol.tilegrid.TileGrid({
	extent: extent,
	resolutions: resolutions,
	tileSize: [512, 512]
});

console.log(resolutions);

activeLayer=1;
activeLayers={};

source = new ol.source.Vector({
	format: new ol.format.GeoJSON(),
	projection: projection
});

extent=[42250.0001199999969685,21170.0001200000115205,295170.0001199999824166,167700.0001200000115205];
res_zbio=[320,160,80,40,20,10];

station = new ol.layer.Vector({
  	source: source,
	extent: extent,
	style: new ol.style.Style({
      	image: new ol.style.Circle({
        radius: 7,
        //fill: new ol.style.Fill({color: 'black'}),
        stroke: new ol.style.Stroke({
          color: [128,0,128], width: 4
        })
      })
    })
});


layers = [
	new ol.layer.Group({
		'title': 'Fonds de carte',
		layers:[ ]
	}),
	new ol.layer.Group({
		'title': 'Limites administratives',
		layers:[ station ]
	})
];


var _view = new ol.View({
	projection: projection,	
	center: [217200, 50100],//epioux
	extent: extent,
	zoom: 9,
	minZoom: 0,
	maxZoom: 14
})

// create map here

map = new ol.Map({
	//controls: ol.control.defaults().extend([
	//  new ol.control.ScaleLine()
	//]),
 	//interactions : ol.interaction.defaults({doubleClickZoom :false, shiftDragZoom: false}),
	renderer: 'canvas',
	//renderer: 'webgl',
	layers: layers,
	target: 'map',
	//overlays: [overlay],
	view: _view,
	pixelRatio: 1
});



/**
 * partie "draw"
 */
 
 styleIsValid = new ol.style.Style({
	stroke: new ol.style.Stroke({
		color: 'rgba(0, 255,0, 1.0)',
		width: 5
	  })
});

acr_src = new ol.source.Vector({
	//format: new ol.format.GeoJSON(),
	//url: 'tmp/wt2Kjmcd-epioux_parcellaire.geojson'
	projection : projection
});

acr = new ol.layer.Vector({
  	source: acr_src,
	extent: extent,
	style: styleIsValid,
});

draw = new ol.interaction.Draw({
      source: acr_src,
      type: "LineString",
}); 


/*document.getElementById('undo').addEventListener('click', function () {
  draw.removeLastPoint();
});*/

map.addInteraction(draw);

writer = new ol.format.GeoJSON();


draw.on('drawend', function(event) {
     map.removeInteraction(draw);
});


updateGroupeLayers = function(){
	let l = [];
		l.push(activeLayer)
		l.push(station);
		l.push(acr);
		groupe = new ol.layer.Group({
			layers:l
		});
	map.setLayerGroup(groupe);
}

activeLayer = new ol.layer.Tile({
            extent: extent,
            title: 'Fond topographique IGN',
            source: new ol.source.TileWMS({
                preload: Infinity,
                title: 'Fond topographique IGN',
                url: 'https://cartoweb.wms.ngi.be/service',
                crossOrigin: 'null',
                attributions: '',
                params: {
                    'LAYERS': 'topo',
                    'TILED': false,
                    'FORMAT': 'image/png'
                },
                tileGrid: tileGrid,
                projection: 'EPSG:31370',
            }),
            opacity: 1
        });
activeLayers['IGN'] = activeLayer;updateGroupeLayers();



