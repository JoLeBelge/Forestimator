// la portée d'une variable javascript est "locale" (dans un module donné) si déclaré avec préfix var. elle est globale (tout le document si pas de préfixe var
//https://stackabuse.com/using-global-variables-in-node-js/
//IGN.setVisible(true);

apt2 = new ol.layer.Tile({
	preload: 0,
	title: 'Carte aptitude',
	attributions: 'Gembloux Agro-Bio Tech',
	source: new ol.source.TileImage({
	//source: new ol.source.XYZ({
		crossOrigin: null,
		extent: extent,
		projection: projection,
		tileGrid: new ol.tilegrid.TileGrid({
		//tileGrid: new ol.source.TileDebug({
	origins: [[extent[0], extent[3]],[extent[0], extent[3]],[extent[0], extent[3]],[extent[0], extent[3]],[extent[0], extent[3]],[extent[0], extent[3]]],
	resolutions: res_zbio,
	tileSize: [512, 512],
	minZoom:0,
	map:map
		}),
		tileUrlFunction: function (coordinate) {

			if (coordinate === null) return undefined;
			// TMS Style URL
			var z = coordinate[0];
			//var x = coordinate[1]+1;
			//mais y est négatif, donc je pense que le +1 est préjudiciable, non?
			//var y = coordinate[2]+1;
			var x = coordinate[1]+1;
			var y = -(coordinate[2]);		
			var _x = String(x);
			var _y = String(y);

			// la signification du z chez moi est inversée; plus le z est petit, plus les raster sont résolu.
			var myZ = ["5","4","3","2","1",""];
			var _z=myZ[z];
			
			if(z>=3){
			if(x<10) _x="0"+_x;
			if(y<10) _y="0"+_y;
			}
			// pour l'instant, je ne sais le charger que si il est en local!!
			var url = 'Tuiles/CLASSIF_ST_CODE_ES/'+_z+'/aptitudeCLASSIF_ST_CODE_ES_'+_y+'_'+_x+'.tif.png';
			//var url = '/test/'+_z+'/aptitudeFEE_HE_'+_y+'_'+_x+'.tif.png';
			//console.log(url);			
			return url;
		}
	})
});

/*
var source= apt.getSource();
source.changed();
source.refresh();
*/
// ol n'aime pas que je fasse une liste de plusieur groupe de couche, mais un seul groupe ça fonctionne bien.
groupe = new ol.layer.Group({
		'title': 'aptitude',
		attributions: 'Gembloux Agro-Bio Tech',
		layers:[IGN, apt2, communes]});
IGN.setVisible(false);
map.setLayerGroup(groupe);
