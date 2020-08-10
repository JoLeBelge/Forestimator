
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
//ol.proj.proj4.register(proj4);

projection = new ol.proj.Projection({
	code: 'EPSG:31370',
	extent: [42247, 21148, 295176, 167719] // full RW
});

extent = [42247, 21148, 295176, 167719];// full RW
ol.proj.get('EPSG:31370').setExtent(extent);

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
//console.log(resolutions);

//var res_ndvi=[384.02176,192.01088,96.00544,48.00272,24.00136,12.00068,6.00034,3.00017];
var res_ndvi=[384.0576,192.0288,96.0144,48.0072,24.0036,12.0018,6.0009,3.00045];
//var extend_ndvi=[110345, 15392, 302118, 171369];
var extend_previous=[110345.3375329999980750,-25250.0562959999988379, 301972.3004199999850243, 171369.1921980000042822];
//var extend_ndvi=[38824.4511019999990822,19513.5278680000010354, 296527.3037289999774657,171984.5151900000055321];
var extend_ndvi=[39166.5026720000023488,19513.5278680000010354, 296527.3037289999774657,171984.5151900000055321];

var o2018 = new ol.layer.Tile({
	extent: extent,
	title: 'Orthophoto 2018',
	source: new ol.source.TileWMS({
		 preload: Infinity,
		title: 'Ortho 2018',
		url: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/map',
		crossOrigin: 'null',
		attributions: 'Â© CartoWeb.be & Geoportail.wallonie.be',
		params: {
		  'LAYERS': 'RGB_2018 0.25',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
});

activeLayer = new ol.layer.Tile({
	extent: extent,
	title: 'Fond topographique IGN',
	source: new ol.source.TileWMS({
		title: 'IGN 20.000',
		url: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/ign',
		crossOrigin: 'null',
		attributions: 'Â© CartoWeb.be & Geoportail.wallonie.be',
		params: {
		  'LAYERS': 'IGN 20.000',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
});


// pour la couche vectorielle de point qui permet d'afficher un point ou l'utilisateur double click pour avoir le descriptif
source = new ol.source.Vector({
format: new ol.format.GeoJSON(),
projection: projection});

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

/*
// apt2 il affiche le numéro du tile en fonction du paramétrage e tileGrid (origin, origins, resolutions ect) --> bon pour le debugging
var apt2 = new ol.layer.Tile({
	preload: 0,
	title: 'Carte aptitude',
	source: new ol.source.TileDebug({
		crossOrigin: null,
		extent: extent,
		projection:projection,
		tileGrid: new ol.tilegrid.TileGrid({
		//tileGrid: new ol.source.TileDebug({
	extent: extent,
	resolutions: res_zbio,
	origin: [extent[0], extent[3]],
	tileSize: [512, 512],
	map:map
		})
	})
});
*/

parcelles = new ol.layer.Tile({
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
		tileGrid: tileGrid
	})
});


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

/*
communes = new ol.layer.Vector({
    title: 'Communes',
	extent: extent,
	source: commune_src,
    style:new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'blue',
		width: 20
	  }),
    fill: new ol.style.Fill({
      color: 'rgba(0, 0, 255, 0.1)'
    })
	  })
});
*/

layers = [
	new ol.layer.Group({
		'title': 'Fonds de carte',
		layers:[ activeLayer ] //
	}),
	new ol.layer.Group({
		'title': 'Limites administratives',
		layers:[ station, parcelles, communes ]
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
	zoom: 7,
	minZoom: 0,
	maxZoom: 10
})


// create map here

map = new ol.Map({
	controls: ol.control.defaults().extend([
	  new ol.control.ScaleLine()
	]),
 	interactions : ol.interaction.defaults({doubleClickZoom :false}),
	renderer: 'canvas',
	//renderer: 'webgl',
	layers: layers,
	target: 'map',
	overlays: [overlay],
	view: _view,
	pixelRatio: 1
});


activeLayer.setVisible(true);
parcelles.setVisible(false);

// pas implémenté dans ol3! bien ben c'est ce que je dois implémenter moi-meme dans wt? oui, avec les checksbox, une pour chaque essences forestières
//var layerSwitcher = new ol.control.LayerSwitcher();
//map.addControl(layerSwitcher);

/*	Tab for communes coordinates  sam a créer un menu déroulant ou on choisi le nom d'une commune et ol zoom sur la commune */
var _tab_com = [[213307,134406,219526,142795],[271055,109761,286544,123145],[192119,126523,206451,135879],[175471,107744,187622,116852],[225866,128104,234085,135299],[244517,35390,261321,47255],[187489,113769,202086,123238],[244294,43676,255513,54881],[246491,26138,258803,34239],[239112,121310,251176,135456],[260720,135695,275517,150084],[240993,67310,257521,90520],[135469,94181,148352,109328],[182232,78685,200917,95050],[233756,78049,247609,90299],[206806,51451,221882,69314],[239627,145269,243741,149707],[190370,59222,203919,78502],[193168,44478,209131,63816],[281283,112389,295157,134097],[275025,121965,287195,136119],[266977,92442,280248,107246],[149488,90082,160696,103296],[236397,138445,242896,145685],[137698,70049,155508,90877],[217770,38360,230359,53747],[195158,99972,213709,114598],[210586,117525,225368,128338],[232816,128264,239916,134190],[151218,69545,166953,92102],[193295,72458,206128,81745],[184675,96705,198445,110565],[254014,143955,259376,148132],[165273,86773,180769,99945],[219292,110410,240218,124948],[218364,136222,225947,142999],[226378,104678,241591,115297],[230040,133147,238432,141829],[233619,35113,246103,45159],[265368,138977,284496,152741],[239614,57288,251735,70936],[231727,117946,245330,128450],[221601,139052,229308,147097],[240273,142981,245357,149091],[173194,120321,182758,129076],[159683,98549,179944,112473],[206122,33657,226282,52628],[165305,116402,180608,124197],[143800,85725,152347,103592],[180236,67439,195972,82396],[156321,111522,166055,118706],[191952,118281,205203,128194],[251322,89903,268902,107845],[231613,42568,246844,53319],[230686,122464,239139,128972],[196664,109191,211957,119466],[177422,93103,187914,104170],[204124,109590,220798,123282],[210944,50125,224009,58223],[197184,132821,206972,141203],[221343,101138,230903,112518],[241702,84443,259194,103682],[184549,91430,203637,104695],[203950,128835,217733,136853],[256988,129582,270851,143814],[227965,87247,248218,103043],[225104,47604,246201,66277],[204138,66658,219024,81919],[216300,63102,235017,79899],[231653,139597,242353,153732],[244990,105806,257819,118002],[258225,141786,264282,150338],[263710,116811,274093,135601],[235945,101752,248272,118669],[209788,94445,228918,108033],[208806,124395,215034,133351],[244771,52056,249951,61614],[222775,28580,234168,37042],[248297,29953,259200,39718],[165173,105630,178384,118845],[211496,123363,219942,135346],[133271,71487,146119,83495],[240233,26093,248840,33756],[175140,119752,193673,135734],[218517,131343,228582,137546],[212476,85760,228445,99238],[218002,52440,233443,66379],[224677,134969,234073,140714],[200262,121134,211704,130117],[240114,138262,249534,143585],[176196,100478,187420,111214],[222088,122436,232959,130785],[198440,57411,209848,76047],[248006,137381,255339,144594],[157706,86667,175773,102272],[176477,114724,189140,122324],[268083,144821,284793,158435],[226578,95311,238741,106638],[199049,87141,215428,103322],[227618,21163,234297,29852],[224909,72250,240360,85784],[217788,139343,222625,147436],[211656,74758,228221,87636],[240128,31411,249293,38034],[230477,145330,233531,149601],[264625,103667,288164,117105],[227979,138002,234743,147035],[132739,89901,145394,100504],[210228,103527,223549,120214],[243356,144078,249110,151519],[252735,125763,260953,134564],[236191,130295,248869,139203],[255391,115138,268216,133000],[242603,116974,260925,129344],[206071,79774,217852,89159],[223117,82339,237892,93947],[247200,125763,260001,140786],[216683,126703,226396,133334],[225561,34838,235725,52954],[248988,113137,266270,121084],[243800,140099,249371,145272],[230993,62393,246499,78595],[252193,138653,260256,147198],[245675,101831,267516,116342],[161296,74925,175263,89237],[231896,22325,242251,37136],[184428,52797,194742,68975],[270136,119482,279677,141517],[144095,100546,162585,113120],[204091,133385,213565,141677],[196030,78541,207969,90446],[184530,107892,196710,117160]];

function centrer_com(){
	var selek = document.getElementById('selek_com');
	if(!selek)return;
	var pt=_tab_com[selek.selectedIndex];
	
	_view.fit(_tab_com[selek.selectedIndex], {padding: [10, 10, 10, 10],constrainResolution: false});

}

featuresSelect = new ol.Collection();
var selectAltClick = new ol.interaction.Select({
  condition: function(mapBrowserEvent) {
    return ol.events.condition.click(mapBrowserEvent) && ol.events.condition.shiftKeyOnly(mapBrowserEvent);
  },
 toggleCondition: ol.events.condition.never,
 //layers: lays4select,
 features: featuresSelect
});

map.addInteraction(selectAltClick);
//selectAltClick.on('select', function (e) {console.log(featuresSelect.item(0).getId());});


/**
 * Add a click handler to hide the popup.
 * @return {boolean} Don't follow the href.
 */
closer.onclick = function() {
  overlay.setPosition(undefined);
  closer.blur();
  return false;
};


map.addEventListener("touchstart",  handleStart);
function handleStart(evt) {
  evt.preventDefault();
  touch = evt.originalEvent.changedTouches[0];	   
}



