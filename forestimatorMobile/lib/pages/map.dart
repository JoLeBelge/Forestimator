import 'dart:async';

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/tileProvider/tifTileProvider.dart';
import 'package:flutter_logs/flutter_logs.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'dart:io';
import 'package:geolocator/geolocator.dart';
//import 'package:fforestimator/locationIndicator/animated_location_indicator.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:go_router/go_router.dart';
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:fforestimator/tools/notification.dart';
import 'dart:convert';
import 'package:flutter_map_location_marker/flutter_map_location_marker.dart';

class mapPage extends StatefulWidget {
  const mapPage({super.key});

  @override
  State<mapPage> createState() => _MapPageState();
}

class _MapPageState extends State<mapPage> {
  final _mapController = MapController();
  LatLng? _pt;
  var data;
  Geolocator? _geolocator;
  Position? _position;
  StreamSubscription? _positionStream;

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

  List<double> getResolutions2(int nbzoom) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var maxResolution = 1280;
    return List.generate(nbzoom, (z) => maxResolution / pow(2, z));
  }
  /*List<double> getResolutions(double maxX, double minX, int zoom,
      [double tileSize = 256.0]) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var size = (maxX - minX) / (tileSize);
    return List.generate(zoom, (z) => size / pow(2, z));
  }*/

  late var epsg31370CRS = Proj4Crs.fromFactory(
      code: 'EPSG:31370',
      proj4Projection: epsg31370,
      bounds: epsg31370Bounds,
      resolutions: getResolutions2(11));
  //resolutions: getResolutions(295170.0, 42250.0, 15, 256.0));

  Future _runAnaPt(proj4.Point ptBL72) async {
    gl.requestedLayers.clear();
    data = "";

    gl.pt = ptBL72;

    bool internet = await InternetConnection().hasInternetAccess;
    if (!gl.offlineMode) {
      if (internet) {
        String layersAnaPt = "";
        for (String lCode in gl.anaPtSelectedLayerKeys) {
          if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
            layersAnaPt += "+" + lCode;
          }
        }

        String url =
            "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/" +
                layersAnaPt +
                "/x/" +
                ptBL72.x.toString() +
                "/y/" +
                ptBL72.y.toString();
        try {
          var res = await http.get(Uri.parse(url));
          if (res.statusCode != 200) throw HttpException('${res.statusCode}');
          data = jsonDecode(res.body);

          // si pas de connexion internet, va tenter de lire data comme une map alors que c'est vide, erreur. donc dans le bloc try catch aussi
          for (var r in data["RequestedLayers"]) {
            gl.requestedLayers.add(layerAnaPt.fromMap(r));
          }
        } catch (e) {
          // handshake et/ou socketExeption
          print('There was an error: ');
          FlutterLogs.logError("anaPt", "online",
              "error while waiting for forestimatorWeb answer. ${e}");
        }
        gl.requestedLayers.removeWhere((element) => element.mFoundLayer == 0);
      } else {
        showDialog(
          context: context,
          builder: (BuildContext context) {
            return PopupNoInternet();
          },
        );
      }
    } else {
      for (layerBase l in gl.dico.getLayersOffline()) {
        int val = await l.getValXY(ptBL72);
        gl.requestedLayers.add(layerAnaPt(mCode: l.mCode, mRastValue: val));
      }
    }

    // un peu radical mais me fait bugger mon affichage par la suite donc je retire
    gl.requestedLayers.removeWhere((element) => element.mRastValue == 0);

    // on les trie sur base des catégories de couches
    gl.requestedLayers.sort((a, b) => gl.dico
        .getLayerBase(a.mCode)
        .mGroupe
        .compareTo(gl.dico.getLayerBase(b.mCode).mGroupe));
  }

  bool _isDownloadableLayer(String key) {
    if (gl.downloadableLayerKeys.contains(key)) {
      return true;
    }
    return false;
  }

  tifFileTileProvider? _provider;

  @override
  void initState() {
    super.initState();
    gl.refreshMap = setState;
    _geolocator = Geolocator();
    LocationSettings locationOptions = const LocationSettings(
        accuracy: LocationAccuracy.high, distanceFilter: 1);
    /*Geolocator.getPositionStream(locationSettings: locationOptions)
        .listen((Position? position) {
      gl.position = position;
      setState(() {});
    });*/
  }

  void refreshView(void Function() f) async {
    setState(f);
  }

  @override
  Widget build(BuildContext context) {
    proj4.Point ptBotLeft = proj4.Point(
        x: epsg31370Bounds.bottomLeft.x, y: epsg31370Bounds.bottomLeft.y);
    proj4.Point ptTopR = proj4.Point(
        x: epsg31370Bounds.topRight.x, y: epsg31370Bounds.topRight.y);

    // contraindre la vue de la map sur la zone de la Wallonie. ajout d'un peu de marge
    double margeInDegree = 0.1;
    LatLng latlonBL = LatLng(
        epsg31370.transform(epsg4326, ptBotLeft).y + margeInDegree,
        epsg31370.transform(epsg4326, ptBotLeft).x - margeInDegree);
    LatLng latlonTR = LatLng(
        epsg31370.transform(epsg4326, ptTopR).y - margeInDegree,
        epsg31370.transform(epsg4326, ptTopR).x + margeInDegree);

    return Scaffold(
        appBar: gl.offlineMode
            ? AppBar(
                title: const Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text("Forestimator offline/terrain",
                          textScaler: TextScaler.linear(0.75),
                          style: TextStyle(color: Colors.black)),
                    ]),
                toolbarHeight: 20.0,
                backgroundColor: gl.colorAgroBioTech,
              )
            : AppBar(
                title: const Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text("Forestimator online",
                          textScaler: TextScaler.linear(0.75),
                          style: TextStyle(color: Colors.white)),
                    ]),
                toolbarHeight: 20.0,
                backgroundColor: gl.colorUliege,
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
                      InteractiveFlag.doubleTapZoom |
                      InteractiveFlag.scrollWheelZoom,
                ),
                onLongPress: (tapPosition, point) async => {
                  //proj4.Point ptBL72 = epsg4326.transform(epsg31370,proj4.Point(x: point.longitude, y: point.latitude))
                  await _runAnaPt(epsg4326.transform(epsg31370,
                      proj4.Point(x: point.longitude, y: point.latitude))),
                  _updatePtMarker(point),
                  GoRouter.of(context).push("/anaPt"),
                },
                onPositionChanged: (position, e) async {
                  LatLng c = _mapController.camera.center;
                  final SharedPreferences prefs =
                      await SharedPreferences.getInstance();
                  await prefs.setDouble('mapCenterLat', c.latitude);
                  await prefs.setDouble('mapCenterLon', c.longitude);
                  updateLocation();
                  double aZoom = _mapController.camera.zoom;
                  await prefs.setDouble('mapZoom', aZoom);
                },
                crs: epsg31370CRS,
                initialZoom: 8.0,
                maxZoom: 10,
                minZoom: 0,
                initialCenter: gl.latlonCenter,
                cameraConstraint: CameraConstraint.contain(
                    bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
                onMapReady: () async {
                  Permission.locationWhenInUse.request();
                  if (await Permission.locationWhenInUse.isGranted) {
                    Permission.locationAlways.request();
                  }
                  Permission.manageExternalStorage
                      .request(); // pour nouvelle version de android
                  Permission.storage
                      .request(); // pour ancienne version de android

                  updateLocation();

                  if (gl.position != null) {
                    // IMPORTANT: rebuild location layer when permissions are granted
                    setState(() {
                      _mapController.move(
                          LatLng(gl.position?.latitude ?? 0.0,
                              gl.position?.longitude ?? 0.0),
                          9);
                    });
                    // si on refusait d'allumer le GPS, alors la carte ne s'affichait jamais, c'est pourquoi il y a le else et le code ci-dessous
                  } else {
                    setState(() {
                      _mapController.move(gl.latlonCenter, gl.mapZoom);
                    });
                  }
                },
              ),
              children: gl.interfaceSelectedLayerKeys.reversed
                      .map<Widget>((gl.selectedLayer selLayer) {
                    if (selLayer.offline &&
                        gl.dico.getLayerBase(selLayer.mCode).mOffline &&
                        (selLayer.mCode == gl.getFirstSelLayOffline())) {
                      if (_provider == null ||
                          _provider?.layerCode != selLayer.mCode) {
                        if (_provider != null) {
                          //_provider?.dispose();
                          _provider =
                              null; // normalement le garbage collector effectue le dispose pour nous.
                        }
                        _provider = tifFileTileProvider(
                            refreshView: refreshView,
                            mycrs: epsg31370CRS,
                            sourceImPath: gl.dico.getRastPath(selLayer.mCode),
                            layerCode: selLayer.mCode);
                        _provider?.init();
                      }
                      return _provider!.loaded
                          ? TileLayer(
                              tileProvider: _provider,
                              // minNativeZoom: 8,
                              minZoom: 5,
                            )
                          : Container();
                    } else if (selLayer.offline) {
                      // deuxième carte offline ; on ne fait rien avec, un seul provider
                      return Container();
                    } else {
                      layerBase l = gl.dico.getLayerBase(selLayer.mCode);
                      return TileLayer(
                        userAgentPackageName: "com.forestimator",
                        wmsOptions: WMSTileLayerOptions(
                          baseUrl: l.mUrl + "?",
                          format: 'image/png',
                          layers: [
                            l.mWMSLayerName,
                          ],
                          crs: epsg31370CRS,
                          transparent: true,
                        ),
                        tileSize: tileSize,
                      );
                    }
                  }).toList() +
                  <Widget>[
                    MarkerLayer(
                      markers: [
                        Marker(
                          width: 50.0,
                          height: 50.0,
                          point: _pt ?? const LatLng(0.0, 0.0),
                          child: const Icon(Icons.location_on),
                        ),
                      ],
                    ),
                    CurrentLocationLayer(),
                    //const AnimatedLocationLayer(
                    // cameraTrackingMode: CameraTrackingMode.locationAndOrientation,
                    //),
                  ]),
          gl.position != null
              ? Row(mainAxisAlignment: MainAxisAlignment.end, children: [
                  Column(mainAxisAlignment: MainAxisAlignment.end, children: [
                    IconButton(
                        iconSize: 40.0,
                        color: gl.colorAgroBioTech,
                        onPressed: () async {
                          await _runAnaPt(epsg4326.transform(
                              epsg31370,
                              proj4.Point(
                                  x: gl.position?.longitude ?? 0.0,
                                  y: gl.position?.latitude ?? 0.0)));
                          GoRouter.of(context).push("/anaPt");
                        },
                        icon: const Icon(Icons.analytics)),
                    IconButton(
                        iconSize: 40.0,
                        color: Colors.red,
                        onPressed: () async {
                          if (gl.position != null) {
                            setState(() {
                              _mapController.move(
                                  LatLng(gl.position?.latitude ?? 0.0,
                                      gl.position?.longitude ?? 0.0),
                                  8);
                            });
                          }
                        },
                        icon: const Icon(Icons.gps_fixed)),
                  ])
                ])
              : Row(mainAxisAlignment: MainAxisAlignment.end, children: [
                  Column(mainAxisAlignment: MainAxisAlignment.end, children: [
                    IconButton(
                        iconSize: 40.0,
                        color: Colors.black,
                        onPressed: () async {
                          if (gl.position != null) {
                            setState(() {});
                          }
                        },
                        icon: const Icon(Icons.gps_fixed))
                  ])
                ]),
        ]));
  }

  void _updatePtMarker(LatLng pt) {
    setState(() {
      _pt = pt;
    });
  }

  static bool _refreshLocation = false;

  void updateLocation() async {
    try {
      if (_refreshLocation)
        return;
      else
        _refreshLocation = true;

      Position newPosition = await Geolocator.getCurrentPosition(
              desiredAccuracy: LocationAccuracy.best)
          .timeout(new Duration(seconds: 3));

      setState(() {
        gl.position = newPosition;
      });
      _refreshLocation = false;
    } catch (e) {
      // We keep the old position.
      setState(() {
        gl.position = gl.position;
      });
      _refreshLocation = false;
      FlutterLogs.logError("gps", "position",
          "error while waiting on position. ${e.toString()}");
    }
  }
}

Future<Position?> acquireUserLocation() async {
  if (await Permission.location.request().isGranted) {
    try {
      return await Geolocator.getCurrentPosition();
    } on LocationServiceDisabledException {
      return null;
    } catch (e) {
      return null;
    }
  } else {
    return null;
  }
}
