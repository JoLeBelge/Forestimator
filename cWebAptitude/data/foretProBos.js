
	var extent = [283621,6359566,713426,6710876];// full RW 3857
	var resolutions = new Array(22);
	var startResolution = (extent[2]-extent[0]) / 512;
	for (var i = 0, ii = resolutions.length; i < ii; ++i) {
		resolutions[i] = startResolution / Math.pow(2, i);
	}
	var tileGrid = new ol.tilegrid.TileGrid({
		extent: extent,
		resolutions: resolutions,
		tileSize: [512, 512]
	});
	
  var osm_plan = new ol.layer.Tile({
	source: new ol.source.OSM(),
	type:'basic',
	title: 'Fond carto OSM'
  });
  
  var osm_topo = new ol.layer.Tile({
    title: 'Fond topo OSM',
	type:'basic',
    /*source: new ol.source.XYZ({
		crossOrigin: 'null',
		attributions: '© OSM',
        url: 'https://{a|b|c}.tile.opentopomap.org/{z}/{x}/{y}.png'
    })*/
	source: new ol.source.OSM({
		url: 'https://{a-c}.tile.opentopomap.org/{z}/{x}/{y}.png'
	})
});

  var source = new ol.source.Vector({wrapX: false});
  var ly_source = new ol.layer.Vector({
	source: source,
	style: polygonStyleFunctionSource
	//title: 'Source',
  });

  var polygons = new ol.source.Vector({wrapX: false});
  var ly_polygons = new ol.layer.Vector({
	source: polygons,
	style: polygonStyleFunction
	//title: 'Polygones existants',
  });

  var o2018 = new ol.layer.Tile({
	extent: extent,
	title: 'Orthophotos SPW Wallonie 2018',
	source: new ol.source.TileWMS({
		url: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/map',
		crossOrigin: 'null',
		attributions: '© CartoWeb.be & Geoportail.wallonie.be',
		params: {
		  'LAYERS': 'RGB_2018 0.25',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
  });
  
  extent=[280525, 6557859, 661237, 6712007];
  var o2018vlanderen = new ol.layer.Tile({
	extent: extent,
	title: 'Orthophotos Vlanderen Flandres 2018',
	source: new ol.source.TileWMS({
		url: 'https://geoservices.informatievlaanderen.be/raadpleegdiensten/OMWRGBMRVL/wms',
		crossOrigin: 'null',
		attributions: '© overheid.vlaanderen.be',
		params: {
		  'LAYERS': 'Ortho',
		  'TILED': true,
		  'FORMAT': 'image/jpeg'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
  });
  
  /* Rasteur cartographie essence – résultat actuel */
  extent = [122311.5713804678089218, 6158248.4607907421886921, 691356.8099105748115107, 6714230.1151348296552896];
  resolutions = new Array(22);
  startResolution = (extent[2]-extent[0]) / 512;
  for (var i = 0, ii = resolutions.length; i < ii; ++i) {
  	resolutions[i] = startResolution / Math.pow(2, i);
  }
  tileGrid = new ol.tilegrid.TileGrid({
	extent: extent,
	resolutions: resolutions,
	tileSize: [512, 512]
  });
  var rasteur = new ol.layer.Tile({
	extent: extent,
	title: 'Résultat cartographie essence - 2018',
	source: new ol.source.TileWMS({
		url: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/probos',
		crossOrigin: 'null',
		attributions: '© CartoWeb.be & Geoportail.wallonie.be',
		params: {
		  'LAYERS': 'ProBos',
		  'TILED': true,
		  'FORMAT': 'image/png'
		},
		tileGrid: tileGrid,
		serverType: 'mapserver',
	})
  });
  
  var resolutions = [];
  var matrixIds = [];
  var proj3857 = ol.proj.get('EPSG:3857');
  var maxResolution = ol.extent.getWidth(proj3857.getExtent()) / 256;

  for (var i = 0; i < 18; i++) {
	matrixIds[i] = i.toString();
	resolutions[i] = maxResolution / Math.pow(2, i);
  }

  var tileGrid = new ol.tilegrid.WMTS({
	origin: [-20037508, 20037508],
	resolutions: resolutions,
	matrixIds: matrixIds
  });

  var key = 'qnco8jgrx5msn6kleivd42sv';

  var ign_source = new ol.source.WMTS({
	url: 'https://wxs.ign.fr/' + key + '/wmts',
	//layer: 'GEOGRAPHICALGRIDSYSTEMS.MAPS',
	layer: 'ORTHOIMAGERY.ORTHOPHOTOS',
	matrixSet: 'PM',
	format: 'image/jpeg',
	projection: 'EPSG:3857',
	tileGrid: tileGrid,
	style: 'normal',
	attributions: '<a href="http://www.geoportail.fr/" target="_blank">' +
		  '<img src="https://api.ign.fr/geoportail/api/js/latest/' +
		  'theme/geoportal/img/logo_gp.gif"></a>'
  });
  var ign_fr = new ol.layer.Tile({
	title: 'Orthophotos IGN France 2017-2019',
	source: ign_source
  });

  var map = new ol.Map({
	layers: [osm_plan, osm_topo, ign_fr, o2018vlanderen, o2018, rasteur, ly_polygons, ly_source],
	target: 'map',
	view: new ol.View({
	  center: [570000, 6350000],
	  zoom: 9,
	  minZoom: 8,
	  maxZoom: 19
	})
  });
  o2018.setVisible(false);
  ign_fr.setVisible(false);
  rasteur.setVisible(false);
  osm_topo.setVisible(false);
  o2018vlanderen.setVisible(false);

  var layerSwitcher = new ol.control.LayerSwitcher();
  /*{
	layers : [{
		layer: o2018,
		config: {
			title: "Ortho 2018",
			description: "Couche Ortho 2018 Belgique",
		}
	},{
		layer: osm_plan,
		config: {
			title: "OSM",
			description: "Couche OpenStreet Map",
		}
	}]
  });*/
  map.addControl(layerSwitcher);

  var draw, snap, snap2, modify; // global so we can remove it later
  function addInteraction() {
    modify = new ol.interaction.Modify({source: source});
    map.addInteraction(modify);
    draw = new ol.interaction.Draw({
	  source: source,
	  type: "Polygon"
    });
    map.addInteraction(draw);
	snap = new ol.interaction.Snap({source: source});
    map.addInteraction(snap);
	snap2 = new ol.interaction.Snap({source: polygons});
    map.addInteraction(snap2);
	
	draw.on('drawend', function(e){
	  updateCountFeatures(1);
	  var lb = document.getElementById("essences");
	  e.feature.set("ess",lb.value);
    });
  }
  map.on('moveend', onMoveEnd);
  
  function onMoveEnd(evt) {
	//var map = evt.map;
	var extent = map.getView().calculateExtent();
	updatePolygons(extent);
  }

  function updatePolygons(extent){
	  // ajax call
	  $.ajax({
	    url: 'ajax.php',
		type: "GET",
		data: {
			extent : JSON.stringify(extent)
		},
		dataType: "JSON",
		success: function(data,status){
			var span=document.getElementById('featuresCount');
			
			// check si data.overload=true si trop de polygons à afficher
			if(data.result==null){
				alert("Erreur: "+data.msg);
			}else if(data.overload){
				span.innerHTML = "Veuillez zoomer plus pour afficher les polygones...";
			}else{
				var fs=polygons.getFeatures();
				// cleaning
				for(i=0;i<fs.length;i++){
					
					polygons.removeFeature(fs[i]);
				}
				// adding
				for(i=0;i<data.result.length;i++){
					// split coord into array
					var t=data.result[i].poly.replace('POLYGON((','').slice(0,-2);
					var coords=t.split(",");
					var coordsa=[];
					for(j=0;j<coords.length;j++){
						var xy=coords[j].split(" ");
						coordsa.push([parseFloat(xy[0]),parseFloat(xy[1])]);
					}
					var f=new ol.Feature();
					var g=new ol.geom.Polygon([coordsa]);
					f.setGeometry(g);
					f.set("ess",data.result[i].ess);
					polygons.addFeature(f);
				}
			}
		},
		error: function(resultat, statut, erreur){
			alert("Erreur lors du chargement des polygons : "+resultat);
		}
	  });
  }


  function updateCountFeatures(plus=0){
	var span=document.getElementById('featuresCount');
	var c=source.getFeatures().length + plus;
	if(c>1)
		span.innerHTML = c + " polygones ajoutés"
	else
		span.innerHTML = c + " polygone ajouté"
  }
  
  function cancelPoint(){
	draw.removeLastPoint();  
  }
  
  function cancelPoly(){
	var f = source.getFeatures();
	if(f.length>0){
		source.removeFeature(f[f.length-1]);
		updateCountFeatures();
	}
  }
  
  function savePolys(){
	if(source.getFeatures().length==0) return;
	
	if(confirm("Etes-vous sur que les polygones ont été correctement digitalisés ?")){
	  var bs = document.getElementById("bSave");
	  bs.disabled=true;
	  bs.innerHTML="<span class='spinner-border spinner-border-sm' role='status' aria-hidden='true'></span>  Sauvegarde en cours...";
	  
	  // get content
	  var fs = source.getFeatures();
	  var polys=[];
	  for(i=0;i<fs.length;i++){
		polys.push({"poly":fs[i].getGeometry().getCoordinates(), "ess":fs[i].get("ess")});
	  }
	  
	  // ajax call
	  $.ajax({
	    url: 'ajax.php',
		type: "POST",
		data: {
			poly : JSON.stringify(polys)
		},
		dataType: "JSON",
		success: function(data,status){
			console.log("status: "+status);
			console.log("poly saved: "+data.result);
			
			
			bs.disabled=false;
			bs.innerHTML="Sauvegarder les changements";
			
			// move source to layer_saved
			for(i=0;i<fs.length;i++){
			  //polys.push({"poly":fs[i].getGeometry().getCoordinates(), "ess":essences[i]});
			  polygons.addFeature(fs[i]);
			  source.removeFeature(fs[i]);
		    }
			ess=[];
			
			var span=document.getElementById('featuresCount');
			span.innerHTML = data.result+" polygones sauvegardés !";
		},
		error: function(resultat, statut, erreur){
			alert("Erreur lors de la sauvegarde : "+erreur);
		}
	  });
	  
	}
  }
  
  var resineux = Array("AB","CY","DO","EP","PI","MZ","TH","TY");
  var createTextStyle = function(feature, resolution) {
	var align = "center";
	var baseline = "middle";
	var size = "10px";
	var offsetX = 0;
	var offsetY = 0;
	var weight = "normal";
	var placement = "point";
	var maxAngle = 0.7853981633974483;
	var overflow = true;
	var rotation = 0;
	var font = weight + ' ' + size + ' Verdana';
	var fillColor = "blue"; // couleur feuillus
	var outlineColor = "#ffffff";
	var outlineWidth = 3;
	var text = feature.get('ess');
	if(resineux.indexOf(text)>-1)
		fillColor="green"; // couleur résineux
	if (resolution > 75) {
	  text = '';
	}

	return new ol.style.Text({
	  textAlign: align == '' ? undefined : align,
	  textBaseline: baseline,
	  font: font,
	  text: text,
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
  
  function polygonStyleFunction(feature, resolution) {
	var fillColor = "blue"; // couleur feuillus
	var text = feature.get('ess');
	if(resineux.indexOf(text)>-1)
		fillColor="green"; // couleur résineux
	return new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: fillColor,
		width: 1
	  }),
	  fill: new ol.style.Fill({
		color: 'rgba(0, 0, 255, 0.1)'
	  }),
	  text: createTextStyle(feature, resolution)
	});
  }
  
  var createTextStyleSource = function(feature, resolution) {
	var align = "center";
	var baseline = "middle";
	var size = "10px";
	var offsetX = 0;
	var offsetY = 0;
	var weight = "normal";
	var placement = "point";
	var maxAngle = 0.7853981633974483;
	var overflow = true;
	var rotation = 0;
	var font = weight + ' ' + size + ' Verdana';
	var fillColor = "darkturquoise";
	var outlineColor = "#ffffff";
	var outlineWidth = 3;
	var text = feature.get('ess');
	if (resolution > 75) {
	  text = '';
	}

	return new ol.style.Text({
	  textAlign: align == '' ? undefined : align,
	  textBaseline: baseline,
	  font: font,
	  text: text,
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
  
  function polygonStyleFunctionSource(feature, resolution) {
	return new ol.style.Style({
	  stroke: new ol.style.Stroke({
		color: 'darkturquoise',
		width: 1
	  }),
	  fill: new ol.style.Fill({ //e7f77d8c
		color: 'rgba(231, 247, 125, 0.1)'
	  }),
	  text: createTextStyleSource(feature, resolution)
	});
  }
  
  var active_draw=-1;
  function toggle_editor(){
	var b=document.getElementById("bToggle");
	if(active_draw==-1){
	  b.innerHTML = "Désactiver le mode digitalisation";
	  b.className="btn btn-success btn-lg btn-block";
	  addInteraction();
	  active_draw=1;
	}else if(active_draw){
	  b.innerHTML = "Activer le mode digitalisation";
	  b.style.backgroundColor="";
	  b.className="btn btn-primary btn-lg btn-block";
	  draw.setActive(false);
	  modify.setActive(false);
	  snap.setActive(false);
	  active_draw=0;
	}else{
	  b.innerHTML = "Désactiver le mode digitalisation";
	  b.className="btn btn-success btn-lg btn-block";
	  draw.setActive(true);
	  modify.setActive(true);
	  snap.setActive(true);
	  active_draw=1;
	}
  }
    
  function toggleHelp(){
	var p=document.getElementById("help_panel");
	if(p.style.display=="" || p.style.display=="block")
		p.style.display="none";
	else
		p.style.display="block";
	return false;
  }
  
  function checkEss(){
	var e=document.getElementById("essences");
	if(e.value=="--")
		e.value="CH";
  }
  
  const saveData = (function () {
    const a = document.createElement("a");
    document.body.appendChild(a);
    a.style = "display: none";
    return function (url, fileName) {
        a.href = url;
        a.download = fileName;
        a.click();
    };
  }());

  function exportPoly(){
	saveData("ajax.php?download","export_probos.csv");
  }
  
