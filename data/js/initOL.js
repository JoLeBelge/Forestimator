
/*	Fonctions pour affichage label sur polygon	*/
var getText = function(feature, resolution, dom) {
	var type = dom.text.value;
	var maxResolution = dom.maxreso.value;
	var text = feature.get('nom');
	//console.log("t="+text);
	if (resolution > maxResolution) {
	  text = '';
	} else if (type == 'hide') {
	  text = '';
	} else if (type == 'shorten') {
	  text = text.trunc(12);
	} else if (type == 'wrap' && (!dom.placement || dom.placement.value != 'line')) {
	  text = stringDivider(text, 16, '\n');
	}

	return text;
};

var createTextStyle = function(feature, resolution, dom) {
	var align = dom.align;
	var baseline = dom.baseline;
	var size = dom.size;
	var offsetX = parseInt(dom.offsetX, 10);
	var offsetY = parseInt(dom.offsetY, 10);
	var weight = dom.weight;
	var placement = dom.placement ? dom.placement : undefined;
	var maxAngle = dom.maxangle ? parseFloat(dom.maxangle) : undefined;
	var overflow = dom.overflow ? (dom.overflow == 'true') : undefined;
	var rotation = parseFloat(dom.rotation);
	if (dom.font == '\'Open Sans\'' && !openSansAdded) {
	  var openSans = document.createElement('link');
	  openSans.href = 'https://fonts.googleapis.com/css?family=Open+Sans';
	  openSans.rel = 'stylesheet';
	  document.getElementsByTagName('head')[0].appendChild(openSans);
	  openSansAdded = true;
	}
	var font = weight + ' ' + size + ' ' + dom.font;
	var fillColor = dom.color;
	var outlineColor = dom.outline;
	var outlineWidth = parseInt(dom.outlineWidth, 10);

	return new ol.style.Text({
	  textAlign: align == '' ? undefined : align,
	  textBaseline: baseline,
	  font: font,
	  text: getText(feature, resolution, dom),
	  fill: new ol.style.Fill({color: fillColor}),
	  stroke: new ol.style.Stroke({color: outlineColor, width: outlineWidth}),
	  offsetX: offsetX,
	  offsetY: offsetY,
	  placement: placement,
	  maxAngle: maxAngle,
	  overflow: overflow,
	  rotation: rotation
	});
};

myDom = {
	text: "Normal",
	align: "center",
	baseline: "middle",
	rotation: 0,
	font: "Arial",
	weight: "normal",
	placement: "point",
	maxangle: 2,
	overflow: true,
	size: "16px",
	offsetX: 0,
	offsetY: 0,
	color: "black",
	outline: "blue",
	outlineWidth: "2px",
	maxreso: 1200000
};

// Polygons
function polygonStyleFunction(feature, resolution) {
	return new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'rgba(0, 0, 255, 1.0)',
		width: 2
	  }),
	  /*fill: new Fill({
		color: 'rgba(0, 0, 255, 0.1)'
	  }),*/
	  text: createTextStyle(feature, resolution, myDom)
	});
}

/*_____fin label polygon______*/


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

/*
le tileGrid est une astuce assez lourde à mettre en place pour le chargement de tuile png ou raster stockées sur le server (carte des scolyte) --> nous on a plus besoin de cette approche
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
*/
//console.log(resolutions);

//var res_ndvi=[384.02176,192.01088,96.00544,48.00272,24.00136,12.00068,6.00034,3.00017];
var res_ndvi=[384.0576,192.0288,96.0144,48.0072,24.0036,12.0018,6.0009,3.00045];
//var extend_ndvi=[110345, 15392, 302118, 171369];
var extend_previous=[110345.3375329999980750,-25250.0562959999988379, 301972.3004199999850243, 171369.1921980000042822];
//var extend_ndvi=[38824.4511019999990822,19513.5278680000010354, 296527.3037289999774657,171984.5151900000055321];
var extend_ndvi=[39166.5026720000023488,19513.5278680000010354, 296527.3037289999774657,171984.5151900000055321];


orthoLayer = new ol.layer.Tile({
	extent: extent,
	title: 'Fond orthophotos',
	source: new ol.source.TileWMS({
		title: 'Orthophotos',
		url: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/map',
		crossOrigin: 'null',
		attributions: 'Â© CartoWeb.be & Geoportail.wallonie.be',
		params: {
		  'LAYERS': 'RGB_2020 0.25',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		//tileGrid: tileGrid,
		projection: 'EPSG:31370',
		serverType: 'mapserver',
	})
});


IGNLayer = new ol.layer.Tile({
	extent: extent,
	title: 'Fond topographique IGN',
	source: new ol.source.TileWMS({
		title: 'carto Web',
		url: 'https://wms.ngi.be/cartoweb/service',
		crossOrigin: 'null',
		attributions: 'CartoWeb.be',
		params: {
		  'LAYERS': 'topo',
		  'TILED': false,
		  'FORMAT': 'image/png'
		},
		//tileGrid: tileGrid,
		projection: 'EPSG:31370',
		//serverType: 'mapserver',
	})
});

activeLayer=1;
activeLayers={};

// pour la couche vectorielle de point qui permet d'afficher un point ou l'utilisateur double click pour avoir le descriptif
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

//var selectElement = document.getElementById('type');

/*parcelles = new ol.layer.Tile({
	title: 'Cadastre',
	extent: extent,
	opacity: 0.5,
	source: new ol.source.TileWMS({
		url: 'http://ccff02.minfin.fgov.be/geoservices/arcgis/services/WMS/Cadastral_Layers/MapServer/WMSServer',
		crossOrigin: 'null',
		attributions: 'Â© CartoWeb.be & Geoportail.wallonie.be',
		params: {
			'LAYERS': 'Cadastral Parcel',
			'TILED': true,
			'FORMAT': 'image/jpeg'},
		serverType: 'mapserver',
		//tileGrid: tileGrid
		projection: 'EPSG:31370',
	})
});*/


var commune_src = new ol.source.Vector({
	format: new ol.format.GeoJSON(),
	url: function(extent) {
	  return 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/scolyte_shp?' +
		  'SERVICE=WFS&version=2.0.0&request=GetFeature&typename=communes_rw&' +
		  'outputFormat=geojson&srsname=EPSG:31370&' +
		  'bbox=' + extent.join(',') + ',EPSG:31370';
	},
	strategy: ol.loadingstrategy.bbox
});

 /*
 var commune_src = new ol.source.Vector({
	format: new ol.format.GeoJSON(),
	url: 'tmp/wt2Kjmcd-epioux_parcellaire.geojson'
});
*  */

style = new ol.style.Style({
	text: new Text({
		font: 'bold 11px "Open Sans", "Arial Unicode MS", "sans-serif"',
		placement: 'line',
		fill: new ol.style.Fill({ color: 'white' })
	}),
	stroke: new ol.style.Stroke({
		color: 'rgba(0, 0, 255, 1.0)',
		width: 2
	  })
});


communes = new ol.layer.Vector({
	crossOrigin: 'null',
	title: 'Communes',
	extent: extent,
	source: commune_src,
	style: polygonStyleFunction
});


layers = [
	new ol.layer.Group({
		'title': 'Fonds de carte',
		layers:[ orthoLayer , IGNLayer ]
	}),
	new ol.layer.Group({
		'title': 'Limites administratives',
		layers:[ station, communes ]
	})
];

/*	Overlay clic info	*/
var container = document.getElementById('popup');
content = document.getElementById('popup-content');
var closer = document.getElementById('popup-closer');

overlay = new ol.Overlay({
	element: container,
	autoPan: true,
	autoPanAnimation: {
		duration: 250
	}
});


var _view = new ol.View({
	projection: projection,	
	center: [217200, 50100],//epioux
	extent: extent,
	zoom: 9,
	minZoom: 0,
	maxZoom: 10
})

// create map here

map = new ol.Map({
	controls: ol.control.defaults().extend([
	  new ol.control.ScaleLine()
	]),
 	interactions : ol.interaction.defaults({doubleClickZoom :false, shiftDragZoom: false}),
	renderer: 'canvas',
	//renderer: 'webgl',
	layers: layers,
	target: 'map',
	overlays: [overlay],
	view: _view,
	pixelRatio: 1
});


//activeLayer.setVisible(true);
orthoLayer.setVisible(false);


/**
 * Add a click handler to hide the popup.
 * @return {boolean} Don't follow the href.
 */ 
// pas bonne compatibilité tablette car le touch sur le closer va questionner la carte sur la station ou on clique, mm si en dessous du popup.
closer.onclick = function() {
  overlay.setPosition(undefined);
  closer.blur();
  return false;
};

closer.ontouch = function() {
  overlay.setPosition(undefined);
  closer.blur();
  return false;
};


refreshLayers = function (){
	orthoLayer.getSource().changed();
	IGNLayer.getSource().changed();
	
	//activeLayer.getSource().changed();
}




