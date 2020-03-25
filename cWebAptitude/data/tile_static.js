var extent = [0, 0, 1024, 968];
var projection = new ol.proj.Projection({
  code: 'xkcd-image',
  units: 'pixels',
  extent: extent
});

var map = new ol.Map({
  layers: [
    new ol.layer.Image({
      source: new ol.source.ImageStatic({
        attributions: '© <a href="http://xkcd.com/license.html">xkcd</a>',
        url: 'https://imgs.xkcd.com/comics/online_communities.png',
        projection: projection,
        imageExtent: extent
      })
    })
  ],
  target: 'map',
  view: new ol.View({
    projection: projection,
    center: ol.extent.getCenter(extent),
    zoom: 2,
    maxZoom: 8
  })
});
