
if (typeof parcellaire !== 'undefined') {
    // the variable is defined
    groupe = new ol.layer.Group({
		'title': 'parcellaire',
		attributions: 'Gembloux Agro-Bio Tech',
		 layers:[TOREPLACE, parcellaire, station]});
} else {
groupe = new ol.layer.Group({
		'title': 'aptitude',
		attributions: 'Gembloux Agro-Bio Tech',
		layers:[TOREPLACE, communes, station]});
TOREPLACE.setVisible(true);
}

map.setLayerGroup(groupe);
