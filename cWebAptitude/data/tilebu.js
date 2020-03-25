//var ol=ol
//import TileGrid from 'ol/layer/tilegrid/TileGrid'
var projection = new ol.proj.Projection({
	code: 'EPSG:31370',
	extent: [42247, 21148, 295176, 167719] // full RW
});
var extent=[42250.0001199999969685,21170.0001200000115205,295170.0001199999824166,167700.0001200000115205];
var res_zbio=[40,20,10];
var tileGrid = new ol.tilegrid.TileGrid({
	extent: extent,
	resolutions: res_zbio,
	tileSize: [512, 512]
});

var zbio = new ol.layer.Tile({
	preload: 1,
	title: 'zone bioclimatique',
	source: new ol.source.TileImage({
		crossOrigin: null,
		extent: extent,
		tileGrid: new ol.tilegrid.TileGrid({
			extent: extent,
			origin: [extent[0], extent[1]],
			resolutions: res_zbio,
			tileSize: [512,512]
		}),
		tileUrlFunction: function (coordinate) {

			if (coordinate === null) return undefined;
			// TMS Style URL
			var z = coordinate[0];
			var x = coordinate[1]+1;
			var y = coordinate[2]+1;
			
			/*	Attention Y est inversÃ© sur son axe ! tuiles gÃ©nÃ©rÃ©e depuis coin haut gauche	*/
			//	1+ nombre d'elements en y
			//		z	2,3, 4, 5, 6,  7
			var coor = [5,9,17,33,65,129]
			y = coor[z-2]-y;
						
			var _x = String(x);
			var _y = String(y);
			if(z>=6){ /* number format "001" there is more than 100 elements */
				if(x<10) _x="00"+_x;
				else if(x<100) _x="0"+_x;
				if(y<10) _y="00"+_y;
				else if(y<100) _y="0"+_y;
			}else if(z>2){	/* number format "01" there is more than 10 elements */
				if(x<10) _x="0"+_x;
				if(y<10) _y="0"+_y;
			}
			//var url = 'CS_zbio2/' + z + '/' + y + '/NDVI_NoDataFilled_Masked_Picea(60)_MNH2018(800)_CC_L72_BBOXed_' + _y + '_' + _x + '.tif';
			var url = 'CS_zbio2/2/CS_zbio2_04_09.tif';
			//var url = 'retiled/3/1/SSSM_jj2019_NDVI_NoDataFilled_L72_1_6.tif';
			return url;
		}
	})
});


var parcelles = new ol.layer.Tile({
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

var layers = [
	new ol.layer.Group({
		'title': 'toto',
		layers:[zbio] //
	}),
		new ol.layer.Group({
		'title': 'Limites administratives',
		layers:[parcelles]
	})
];

/*	Overlay clic info	
var container = document.getElementById('popup');
var content = document.getElementById('popup-content');
var closer = document.getElementById('popup-closer');
var overlay = new ol.Overlay({
	element: container,
	autoPan: true,
	autoPanAnimation: {
		duration: 250
	}
});
closer.onclick = function() {
	overlay.setPosition(undefined);
	closer.blur();
	return false;
};
* */


var _view = new ol.View({
	projection: projection,
	//center: [173532, 139254],//fac Gbx
	center: [223993, 86882],//scolyte somewhere
	extent: extent,
	zoom: 2,
	minZoom: 1,
	maxZoom: 3
});


var map = new ol.Map({
	/*controls: ol.control.defaults().extent([
	  new ol.control.ScaleLine()
	]),*/
	layers: layers,
	target: 'map',
	//overlays: [overlay],
	view: _view
});

/*
olgt_zbio = new olGeoTiff(zbio);
olgt_zbio.plotOptions.domain = [1400, 1900];
olgt_zbio.plotOptions.noDataValue = 0;
olgt_zbio.plotOptions.palette = "hsv";//"inferno";greens
olgt_zbio.redraw();
* */
