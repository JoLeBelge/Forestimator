import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'package:url_launcher/url_launcher.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/locationIndicator/animated_location_indicator.dart';
import 'package:http/http.dart' as http;

import 'dart:convert';

class mapPage extends StatefulWidget {
  const mapPage({super.key, required this.title});

  final String title;

  @override
  State<mapPage> createState() => _mapPageState();
}

class _mapPageState extends State<mapPage> {
  int _counter = 0;

  final _mapController = MapController();

  Position? _position;
  var data;

  void _anaPonctOnline() async {
    if (_position != null) {
      //print("postion x " + _position?.latitude.toString() ?? "0" );// + " " + _position?.longitude.toString());
      print(_position?.latitude.toString() ??
          "empty position"); // + " " + _position?.longitude.toString());
      print("anaPonctOnline");
      // on projete en BL72, seul src de Forestimator web pour le moment
      proj4.Point ptBL72 = epsg4326.transform(
          epsg31370,
          proj4.Point(
              x: _position?.longitude ?? 0.0, y: _position?.latitude ?? 0.0));
      String layersAnaPt = "CNSWrast+Topo+AE+MNT+slope+ZBIO+CS_A+NT+NH";
      // todo : ajouter la couche active - et la matrice d'aptitude je suppose? FEE et CS?
      String url = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/" +
          layersAnaPt +
          "/x/" +
          ptBL72.x.toString() +
          "/y/" +
          ptBL72.y.toString();
      print(url);
      var res = await http.get(Uri.parse(url));
      print(res.body);
      data = jsonDecode(res.body); // à présenter dans la fenetre "ana ponct"
    }
    setState(() {});
  }

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

    return Scaffold(
      appBar: AppBar(
        title: Text("Forestimator"),
        backgroundColor: Colors.red,
      ),
      persistentFooterButtons: <Widget>[
        IconButton(icon: Icon(Icons.map), onPressed: () => _anaPonctOnline()),
      ],
      body: FlutterMap(
        mapController: _mapController,
        options: MapOptions(
          crs: epsg31370CRS,
          initialZoom: 2,
          maxZoom: 7,
          initialCenter: latlonEpioux,
          cameraConstraint: CameraConstraint.contain(
              bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
          onMapReady: () async {
            _position = await acquireUserLocation();
            if (_position != null) {
              // IMPORTANT: rebuild location layer when permissions are granted
              setState(() {
                _mapController.move(
                    LatLng(_position?.latitude ?? 0.0,
                        _position?.longitude ?? 0.0),
                    16);
              });
            }
          },
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
          const AnimatedLocationLayer(
              // cameraTrackingMode: CameraTrackingMode.locationAndOrientation,
              ),
        ],
      ),
    );
  }
}

Future<Position?> acquireUserLocation() async {
  LocationPermission permission = await Geolocator.checkPermission();
  if (permission == LocationPermission.denied) {
    permission = await Geolocator.requestPermission();
    if (permission == LocationPermission.denied) {
      return null;
    }
  }

  if (permission == LocationPermission.deniedForever) {
    return null;
  }

  try {
    return await Geolocator.getCurrentPosition();
  } on LocationServiceDisabledException {
    return null;
  }
}
