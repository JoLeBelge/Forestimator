import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'package:url_launcher/url_launcher.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/locationIndicator/animated_location_indicator.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:go_router/go_router.dart';

import 'dart:convert';

class mapPage extends StatefulWidget {
  const mapPage({super.key});

  @override
  State<mapPage> createState() => _mapPageState();
}

class _mapPageState extends State<mapPage> {
  final _mapController = MapController();
  LatLng? _pt;
  var data;

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
      resolutions: getResolutions(295170.0, 42250.0, 15, 256.0));

  Future _runAnaPt(proj4.Point ptBL72) async {
    String layersAnaPt = "";
    for (String lCode in gl.anaPtSelectedLayerKeys) {
      if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
        layersAnaPt += "+" + lCode;
      }
    }

    String url = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/" +
        layersAnaPt +
        "/x/" +
        ptBL72.x.toString() +
        "/y/" +
        ptBL72.y.toString();
    print(url);
    var res = await http.get(Uri.parse(url));
    print(res.body);
    data = jsonDecode(res.body);
    gl.requestedLayers.clear();
    for (var r in data["RequestedLayers"]) {
      gl.requestedLayers.add(layerAnaPt.fromMap(r));
    }
    gl.requestedLayers.removeWhere((element) => element.mFoundLayer == 0);
    // un peu radical mais me fait bugger mon affichage par la suite donc je retire
    gl.requestedLayers.removeWhere((element) => element.mRastValue == 0);

    // on les trie sur base des catégories de couches
    gl.requestedLayers.sort((a, b) => gl.dico
        .getLayerBase(a.mCode)
        .mGroupe
        .compareTo(gl.dico.getLayerBase(b.mCode).mGroupe));
    //return _router.go("/anaPt");
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
          title: Text(
            "Forestimator",
            textScaler: TextScaler.linear(0.75),
          ),
          toolbarHeight: 20.0,
          backgroundColor: gl.colorAgroBioTech,
        ),
        body: Stack(children: <Widget>[
          FlutterMap(
              mapController: _mapController,
              options: MapOptions(
                backgroundColor: Colors.transparent,
                keepAlive: true,
                interactionOptions: const InteractionOptions(
                  enableMultiFingerGestureRace: false,
                  flags: InteractiveFlag.drag |
                      InteractiveFlag.pinchZoom |
                      InteractiveFlag.pinchMove |
                      InteractiveFlag.doubleTapZoom,
                ),
                onLongPress: (tapPosition, point) => {
                  //proj4.Point ptBL72 = epsg4326.transform(epsg31370,proj4.Point(x: point.longitude, y: point.latitude))
                  _runAnaPt(epsg4326.transform(epsg31370,
                      proj4.Point(x: point.longitude, y: point.latitude))),
                  _updatePtMarker(point),
                  context.go("/anaPt"),
                },
                crs: epsg31370CRS,
                initialZoom: 4.0,
                maxZoom: 15,
                initialCenter: latlonEpioux,
                cameraConstraint: CameraConstraint.contain(
                    bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
                onMapReady: () async {
                  gl.position = await acquireUserLocation();
                  //await refreshAnalysisPosition();
                  /* if (gl.position != null) {
                    //TODO: Ceci tue l'appli!
                    // IMPORTANT: rebuild location layer when permissions are granted
                    setState(() {
                      _mapController.move(
                          LatLng(gl.position?.latitude ?? 0.0,
                              gl.position?.longitude ?? 0.0),
                          16);
                    });
                  }*/
                },
              ),
              children: List<Widget>.generate(
                    gl.interfaceSelectedLayerKeys.length,
                    (i) => TileLayer(
                      wmsOptions: WMSTileLayerOptions(
                        baseUrl: gl
                                .dico
                                .mLayerBases[gl.interfaceSelectedLayerKeys[
                                    gl.interfaceSelectedLayerKeys.length -
                                        i -
                                        1]]!
                                .mUrl! +
                            "?",
                        format: 'image/png',
                        layers: [
                          gl
                              .dico
                              .mLayerBases[gl.interfaceSelectedLayerKeys[
                                  gl.interfaceSelectedLayerKeys.length -
                                      i -
                                      1]]!
                              .mWMSLayerName!,
                        ],
                        crs: epsg31370CRS,
                        transparent: true,
                      ),
                      //maxNativeZoom: 7,
                      tileSize: tileSize,
                    ),
                  ) +
                  <Widget>[
                    MarkerLayer(
                      markers: [
                        Marker(
                          width: 50.0,
                          height: 50.0,
                          point: _pt ?? LatLng(0.0, 0.0),
                          child: const Icon(Icons.location_on),
                        ),
                      ],
                    ),
                    const AnimatedLocationLayer(
                        // cameraTrackingMode: CameraTrackingMode.locationAndOrientation,
                        ),
                  ]),
        ]));
  }

  Future<void> refreshAnalysisPosition() async {
    if (gl.position != null) {
      //print("postion x " + _position?.latitude.toString() ?? "0" );// + " " + _position?.longitude.toString());
      print(gl.position?.latitude.toString() ??
          "empty position"); // + " " + _position?.longitude.toString());
      print("anaPonctOnline");
      // on projete en BL72, seul src de Forestimator web pour le moment
      proj4.Point ptBL72 = epsg4326.transform(
          epsg31370,
          proj4.Point(
              x: gl.position?.longitude ?? 0.0,
              y: gl.position?.latitude ?? 0.0));
      _runAnaPt(ptBL72);
    }
  }

  void _updatePtMarker(LatLng pt) {
    setState(() {
      _pt = pt;
    });
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