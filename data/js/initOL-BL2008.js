if (typeof extent == 'undefined') {
console.log("initOL.js");

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


proj4.defs('EPSG:3812',
'+proj=lcc +lat_0=50.797815 +lon_0=4.35921583333333 +lat_1=49.8333333333333 +lat_2=51.1666666666667 +x_0=649328 +y_0=665262 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs +type=crs');
ol.proj.proj4.register(proj4);

projection = new ol.proj.Projection({
	code: 'EPSG:3812',
	extent: [542250,521160, 795190,667720]

});

extent = [542250,521160, 795190,667720];
ol.proj.get('EPSG:3812').setExtent(extent);


activeLayer=1;
activeLayers={};

// pour la couche vectorielle de point qui permet d'afficher un point ou l'utilisateur double click pour avoir le descriptif
source = new ol.source.Vector({
	format: new ol.format.GeoJSON(),
	projection: projection
});


station = new ol.layer.Vector({
  	source: source,
	extent: extent,
	style: new ol.style.Style({
      	image: new ol.style.Circle({
        radius: 7,
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
		'title': 'couches vectorielles',
		layers:[ station ]//, communes ]
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
	center: [700000, 600000],
	extent: extent,
	zoom: 9,
	minZoom: 0,
	maxZoom: 10
})


map = new ol.Map({
	controls: ol.control.defaults.defaults().extend([
	  new ol.control.ScaleLine()
	]),
 	interactions : ol.interaction.defaults.defaults({doubleClickZoom :false, shiftDragZoom: false}),
	renderer: 'canvas',
	layers: layers,
	target: 'mapDiv',
	overlays: [overlay],
	view: _view,
	pixelRatio: 1
});


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

/***
 *	Redéfini les couches à afficher
 *
 **/
updateGroupeLayers = function(){
	let l = [];
	for (const [key, value] of Object.entries(activeLayers)){
		l.push(value);
	};	
	
	if (typeof parcellaire !== 'undefined') {
		l.push(parcellaire);
		l.push(station);
		groupe = new ol.layer.Group({
			'title': 'parcellaire',
			 layers:l
		});
	} else {
		l.push(station);
		groupe = new ol.layer.Group({
			'title': 'mes layers',
			layers:l
		});

	}
	map.setLayerGroup(groupe);
}

moveLayerDown = function(code){
	let a={};
	let tmpv,tmpk;
	let loop=true;
	for (const [key, value] of Object.entries(activeLayers)){
		if(!loop){
			a[key]=value
			a[tmpk]=tmpv;
		}
		if(code==key)loop=false;
		else a[key]=value;

		tmpk=key;
		tmpv=value;		
	};	
	activeLayers=a;
	updateGroupeLayers();
}

moveLayerUp = function(code){
	let a={};
	let tmpv,tmpk;
	let loop=true;
	let first=true;
	for (const [key, value] of Object.entries(activeLayers)){

		if(!loop){
			loop=true;
		}else if(code==key){			
			a[key]=value
			a[tmpk]=tmpv;
			loop=false;
		}
		else{
			if(!first)a[tmpk]=tmpv; // skip first element
		}

		tmpk=key;
		tmpv=value;
		if(first)first=false;
	};	
	// add last elem if any
	if(loop) a[tmpk]=tmpv;
	activeLayers=a;	
	updateGroupeLayers();
}

} else { console.log("initOl already done")}
