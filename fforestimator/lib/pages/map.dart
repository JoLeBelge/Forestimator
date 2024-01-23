import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'package:url_launcher/url_launcher.dart';

class mapPage extends StatefulWidget {
  const mapPage({super.key, required this.title});

  final String title;

  @override
  State<mapPage> createState() => _mapPageState();
}

class _mapPageState extends State<mapPage> {
  int _counter = 0;

//https://github.com/fleaflet/flutter_map/blob/master/example/lib/pages/custom_crs/custom_crs.dart
  late proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
  // si epsg31370 est dans la db proj 4, on prend, sinon on définit
  proj4.Projection epsg31370 = proj4.Projection.get('EPSG:31370') ??
      proj4.Projection.add('EPSG:31370',
          '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs');
// map extend in BL72.
  final epsg31370Bounds = Bounds<double>(
    Point<double>(42250.0, 21170.0), // lower left
    Point<double>(295170.0, 167700.0), // upper right
  );

  double tileSize = 256.0;
  List<double> getResolutions(double maxX, double minX, int zoom,
      [double tileSize = 256.0]) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var size = (maxX - minX) / (tileSize);
    return List.generate(zoom, (z) => size / pow(2, z));
  }

  late var epsg31370CRS = Proj4Crs.fromFactory(
      code: 'EPSG:31370',
      proj4Projection: epsg31370,
      bounds: epsg31370Bounds,
      resolutions: getResolutions(295170.0, 42250.0, 8, 256.0));

  void _incrementCounter() {
    setState(() {
      _counter++;
    });
  }

  @override
  Widget build(BuildContext context) {
    proj4.Point ptEpioux = proj4.Point(x: 217200.0, y: 50100.0);
    proj4.Point ptBotLeft = proj4.Point(
        x: epsg31370Bounds.bottomLeft.x, y: epsg31370Bounds.bottomLeft.y);
    proj4.Point ptTopR = proj4.Point(
        x: epsg31370Bounds.topRight.x, y: epsg31370Bounds.topRight.y);

    // WARNING lat =y lon=x
    LatLng latlonEpioux = LatLng(epsg31370.transform(epsg4326, ptEpioux).y,
        epsg31370.transform(epsg4326, ptEpioux).x);

    // contraindre la vue de la map sur la zone de la Wallonie. ajout d'un peu de marge
    double margeInDegree = 0.1;
    LatLng latlonBL = LatLng(
        epsg31370.transform(epsg4326, ptBotLeft).y + margeInDegree,
        epsg31370.transform(epsg4326, ptBotLeft).x - margeInDegree);
    LatLng latlonTR = LatLng(
        epsg31370.transform(epsg4326, ptTopR).y - margeInDegree,
        epsg31370.transform(epsg4326, ptTopR).x + margeInDegree);

    return FlutterMap(
      options: MapOptions(
        crs: epsg31370CRS,
        initialZoom: 2,
        maxZoom: 7,
        initialCenter: latlonEpioux,
        cameraConstraint: CameraConstraint.contain(
            bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
      ),
      children: [
        TileLayer(
          wmsOptions: WMSTileLayerOptions(
            baseUrl:
                "http://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/forestimator?",
            format: 'image/png',
            layers: const ["MasqueForet"],
            crs: epsg31370CRS,
            transparent: false,
          ),
          //maxNativeZoom: 7,
          tileSize: tileSize,
        ),
        RichAttributionWidget(
          attributions: [
            TextSourceAttribution(
              'OpenStreetMap contributors',
              onTap: () =>
                  launchUrl(Uri.parse('https://openstreetmap.org/copyright')),
            ),
          ],
        ),
        MarkerLayer(
          markers: [
            Marker(
              width: 50.0,
              height: 50.0,
              point: latlonEpioux,
              child: const FlutterLogo(),
            ),
          ],
        ),
      ],
    );
  }
}
