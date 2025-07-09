import 'dart:async';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tileProvider/tif_tile_provider.dart';
import 'package:fforestimator/tools/color_tools.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'dart:io';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:go_router/go_router.dart';
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:fforestimator/tools/notification.dart';
import 'dart:convert';
import 'package:flutter_map_location_marker/flutter_map_location_marker.dart';

class MapPage extends StatefulWidget {
  const MapPage({super.key});

  @override
  State<MapPage> createState() => _MapPageState();
}

class _MapPageState extends State<MapPage> {
  final _mapController = MapController();
  LatLng? _pt;
  bool _doingAnaPt = false;

  static int _mapFrameCounter = 0;

  bool _modeAnaPtPreview = true;
  bool _lastPressWasShort = false;

  bool _toolbarExtended = false;
  bool _polygonToolbarExtended = false;

  bool _modeDrawPolygon = false;
  bool _modePolygonList = false;

  bool _modeDrawPolygonAddVertexes = false;
  bool _modeDrawPolygonRemoveVertexes = false;
  bool _modeDrawPolygonMoveVertexes = false;
  bool _modeShowButtonDrawPolygonMoveVertexes = true;
  bool _modeShowButtonDrawPolygonRemoveVertexes = true;
  bool _modeShowButtonDrawPolygonAddVertexes = true;

  bool _modeLayerSwitches = false;
  
  bool _modeSearch = false;
  bool _modeMeasurePath = false;
  bool _modeMoveMeasurePath = false;
  int selectedMeasurePointToMove = -1;

  final List<LatLng> _measurePath = [];

  Color _polygonMenuColorTools(bool choice) =>
      choice ? Colors.lightGreenAccent.withAlpha(128) : Colors.transparent;

  LatLng? _selectedPointToMove;
  double iconSize = 50.0;

  //https://github.com/fleaflet/flutter_map/blob/master/example/lib/pages/custom_crs/custom_crs.dart
  late proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
  // si epsg31370 est dans la db proj 4, on prend, sinon on définit
  proj4.Projection epsg31370 =
      proj4.Projection.get('EPSG:31370') ??
      proj4.Projection.add(
        'EPSG:31370',
        '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs',
      );
  // map extend in BL72.
  final epsg31370Bounds = Rect.fromPoints(
    Offset(42250.0, 21170.0), // lower left
    Offset(295170.0, 167700.0), // upper right
  );

  double tileSize = 256.0;

  List<double> getResolutions2(int nbZoom) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var maxResolution = 1280;
    return List.generate(nbZoom, (z) => maxResolution / pow(2, z));
  }

  late var epsg31370CRS = Proj4Crs.fromFactory(
    code: 'EPSG:31370',
    proj4Projection: epsg31370,
    bounds: epsg31370Bounds,
    resolutions: getResolutions2(15),
  );

  void _updatePtMarker(LatLng pt) {
    refreshView(() {
      _pt = pt;
      gl.anaPtPreview = null;
    });
  }

  Future _runAnaPt(proj4.Point ptBL72) async {
    gl.requestedLayers.clear();
    Map data;

    gl.pt = ptBL72;

    bool internet = await InternetConnection().hasInternetAccess;
    if (!gl.offlineMode) {
      if (internet) {
        String layersAnaPt = "";
        for (String lCode in gl.anaPtSelectedLayerKeys) {
          if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
            layersAnaPt += "+$lCode";
          }
        }

        String request =
            "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/$layersAnaPt/x/${ptBL72.x}/y/${ptBL72.y}";
        try {
          var res = await http.get(Uri.parse(request));
          if (res.statusCode != 200) throw HttpException('${res.statusCode}');
          data = jsonDecode(res.body);

          // si pas de connexion internet, va tenter de lire data comme une map alors que c'est vide, erreur. donc dans le bloc try catch aussi
          for (var r in data["RequestedLayers"]) {
            gl.requestedLayers.add(LayerAnaPt.fromMap(r));
          }
        } catch (e) {
          gl.print(request);
          gl.print("$e");
        }
        gl.requestedLayers.removeWhere(
          (element) => element.mFoundLayer == false,
        );
      } else {
        showDialog(
          context: gl.notificationContext!,
          builder: (BuildContext context) {
            return PopupNoInternet();
          },
        );
      }
    } else {
      if (gl.dico.getLayersOffline().isEmpty) {
        return;
      }
      for (LayerBase l in gl.dico.getLayersOffline()) {
        int val = await l.getValXY(ptBL72);
        gl.requestedLayers.add(LayerAnaPt(mCode: l.mCode, mRastValue: val));
      }
    }

    // un peu radical mais me fait bugger mon affichage par la suite donc je retire
    gl.requestedLayers.removeWhere((element) => element.mRastValue == 0);

    // on les trie sur base des catégories de couches
    gl.requestedLayers.sort(
      (a, b) => gl.dico
          .getLayerBase(a.mCode)
          .mGroupe
          .compareTo(gl.dico.getLayerBase(b.mCode).mGroupe),
    );
  }

  TifFileTileProvider? _provider;

  @override
  void initState() {
    super.initState();
    initPermissions();
    gl.refreshMap = refreshView;
    initOtherValuesOnce();
  }

  void refreshView(void Function() f) async {
    _mapFrameCounter++;
    setState(f);
  }

  LatLng latlonBL = LatLng(0.0, 0.0);
  LatLng latlonTR = LatLng(0.0, 0.0);

  void initOtherValuesOnce() {
    proj4.Point ptBotLeft = proj4.Point(
      x: epsg31370Bounds.bottomLeft.dx,
      y: epsg31370Bounds.bottomLeft.dy,
    );
    proj4.Point ptTopR = proj4.Point(
      x: epsg31370Bounds.topRight.dx,
      y: epsg31370Bounds.topRight.dy,
    );

    // contraindre la vue de la map sur la zone de la Wallonie. ajout d'un peu de marge
    double margeInDegree = 0.1;
    latlonBL = LatLng(
      epsg31370.transform(epsg4326, ptBotLeft).y + margeInDegree,
      epsg31370.transform(epsg4326, ptBotLeft).x - margeInDegree,
    );
    latlonTR = LatLng(
      epsg31370.transform(epsg4326, ptTopR).y - margeInDegree,
      epsg31370.transform(epsg4326, ptTopR).x + margeInDegree,
    );
  }

  @override
  Widget build(BuildContext context) {
    int i = 0;
    gl.notificationContext = context;
    return handlePermissionForLocation(
      refreshParentWidgetTree: refreshView,
      child: Scaffold(
        resizeToAvoidBottomInset: true,
        extendBody: true,
        appBar: AppBar(
          toolbarHeight: MediaQuery.of(context).size.height * .04,
          backgroundColor:
              gl.offlineMode ? gl.colorAgroBioTech : gl.colorUliege,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.start,
            children: [
              IconButton(
                iconSize: MediaQuery.of(context).size.height * .035,
                color: gl.offlineMode ? Colors.black : Colors.white,
                onPressed: () {
                  PopupSettingsMenu(
                    gl.notificationContext!,
                    "",
                    () {
                      refreshView(() {});
                    },
                    () {
                      refreshView(() {});
                    },
                  );
                },
                icon: Icon(Icons.settings),
              ),
              Container(
                constraints: BoxConstraints(
                  maxWidth: MediaQuery.of(context).size.width * .8,
                ),
                alignment: Alignment.center,
                child: TextButton(
                  onPressed: () {
                    setState(() {
                      gl.offlineMode = !gl.offlineMode;
                      if (gl.offlineMode) {
                        gl.changeSelectedLayerModeOffline();
                      } else {
                        gl.changeSelectedLayerModeOnline();
                      }
                    });
                  },
                  child:
                      gl.offlineMode
                          ? Text(
                            "Forestimator offline/terrain",
                            style: TextStyle(
                              color: Colors.black,
                              fontSize:
                                  MediaQuery.of(context).size.height * .02,
                            ),
                          )
                          : Text(
                            "Forestimator online",
                            style: TextStyle(
                              color: Colors.white,
                              fontSize:
                                  MediaQuery.of(context).size.height * .02,
                            ),
                          ),
                ),
              ),
            ],
          ),
        ),
        body: Stack(
          children: <Widget>[
            FlutterMap(
              mapController: _mapController,
              options: MapOptions(
                backgroundColor: Colors.transparent,
                keepAlive: true,
                interactionOptions: const InteractionOptions(
                  enableMultiFingerGestureRace: false,
                  flags:
                      InteractiveFlag.drag |
                      InteractiveFlag.pinchZoom |
                      InteractiveFlag.pinchMove |
                      InteractiveFlag.doubleTapZoom |
                      InteractiveFlag.scrollWheelZoom,
                ),
                onLongPress: (tapPosition, point) async {
                  _lastPressWasShort = false;
                  if (!_doingAnaPt) {
                    refreshView(() {
                      _doingAnaPt = true;
                    });
                    await _runAnaPt(
                      epsg4326.transform(
                        epsg31370,
                        proj4.Point(x: point.longitude, y: point.latitude),
                      ),
                    );
                    _pt = point;
                    refreshView(() {
                      _doingAnaPt = false;
                    });
                    GoRouter.of(gl.notificationContext!).push("/anaPt");
                  }
                },
                onTap:
                    _modeMoveMeasurePath
                        ? (tapPosition, point) async => {
                          setState(() {
                            _modeMoveMeasurePath = false;
                          }),
                        }
                        : _modeMeasurePath
                        ? (tapPosition, point) async => {
                          setState(() {
                            _measurePath.add(point);
                          }),
                        }
                        : _modeDrawPolygonAddVertexes
                        ? (tapPosition, point) async => {
                          if (gl.polygonLayers.isNotEmpty)
                            {
                              refreshView(() {
                                gl.polygonLayers[gl.selectedPolygonLayer]
                                    .addPoint(point);
                              }),
                            },
                        }
                        : _modeDrawPolygonMoveVertexes
                        ? (tapPosition, point) async => {
                          refreshView(() {
                            _stopMovingSelectedPoint();
                            _modeShowButtonDrawPolygonAddVertexes = false;
                            _modeShowButtonDrawPolygonMoveVertexes = true;
                            _modeShowButtonDrawPolygonRemoveVertexes = true;
                            _modeDrawPolygonAddVertexes = false;
                            _modeDrawPolygonMoveVertexes = false;
                            _modeDrawPolygonRemoveVertexes = false;
                          }),
                        }
                        : _modeDrawPolygon
                        ? (tapPosition, point) async => {
                          refreshView(() {
                            _modeShowButtonDrawPolygonAddVertexes = true;
                            _modeShowButtonDrawPolygonMoveVertexes = false;
                            _modeShowButtonDrawPolygonRemoveVertexes = false;
                            _modeDrawPolygonAddVertexes = false;
                            _modeDrawPolygonMoveVertexes = false;
                            _modeDrawPolygonRemoveVertexes = false;
                          }),
                        }
                        : (tapPosition, point) async => {
                          _lastPressWasShort = true,
                          _updatePtMarker(point),
                        },
                onPositionChanged: (position, e) async {
                  if (!e) return;
                  updateLocation();
                  if (_modeMoveMeasurePath) {
                    _measurePath.removeAt(selectedMeasurePointToMove);
                    _measurePath.insert(
                      selectedMeasurePointToMove,
                      LatLng(
                        position.center.latitude,
                        position.center.longitude,
                      ),
                    );
                  }
                  if (_selectedPointToMove != null) {
                    gl.polygonLayers[gl.selectedPolygonLayer].replacePoint(
                      _selectedPointToMove!,
                      LatLng(
                        position.center.latitude,
                        position.center.longitude,
                      ),
                    );

                    refreshView(() {
                      _selectedPointToMove = LatLng(
                        position.center.latitude,
                        position.center.longitude,
                      );
                    });
                  } else {
                    refreshView(() {});
                  }
                  _writePositionDataToSharedPreferences(
                    position.center.longitude,
                    position.center.latitude,
                    position.zoom,
                  );
                  _writeDataToSharedPreferences();
                },
                crs: epsg31370CRS,
                initialZoom: (gl.globalMinZoom + gl.globalMaxZoom) / 2.0,
                maxZoom: gl.globalMaxZoom,
                minZoom: gl.globalMinZoom,
                initialCenter: gl.latlonCenter,
                cameraConstraint: CameraConstraint.containCenter(
                  bounds: LatLngBounds.fromPoints([latlonBL, latlonTR]),
                ),
                onMapReady: () async {
                  updateLocation();
                  if (gl.position != null) {
                    refreshView(() {
                      _mapController.move(
                        LatLng(
                          gl.position?.latitude ?? 0.0,
                          gl.position?.longitude ?? 0.0,
                        ),
                        _mapController.camera.zoom,
                      );
                    });
                    // si on refusait d'allumer le GPS, alors la carte ne s'affichait jamais, c'est pourquoi il y a le else et le code ci-dessous
                  } else {
                    refreshView(() {
                      _mapController.move(
                        gl.latlonCenter,
                        _mapController.camera.zoom,
                      );
                    });
                  }
                },
              ),
              children:
                  gl.getLayersForFlutterMap().map<Widget>((
                    gl.SelectedLayer selLayer,
                  ) {
                    if (selLayer.offline &&
                        gl.dico.getLayerBase(selLayer.mCode).mOffline &&
                        (selLayer.mCode == gl.getFirstSelLayOffline())) {
                      if (_provider == null ||
                          _provider?.layerCode != selLayer.mCode) {
                        _provider = TifFileTileProvider(
                          refreshView: refreshView,
                          mycrs: epsg31370CRS,
                          sourceImPath: gl.dico.getRastPath(selLayer.mCode),
                          layerCode: selLayer.mCode,
                        );
                        _provider!.init();
                      }
                      return _provider!.loaded
                          ? TileLayer(
                            tileDisplay:
                                gl.modeMapFirstTileLayerTransparancy && i == 2
                                    ? TileDisplay.instantaneous(opacity: 0.5)
                                    : TileDisplay.instantaneous(opacity: 1.0),
                            tileProvider: _provider,
                            maxZoom: gl.globalMaxOfflineZoom,
                            minZoom:
                                gl.globalMinOfflineZoom, // si minZoom de la map est moins restrictif (moins élevé) que celui-ci, la carte ne s'affiche juste pas (écran blanc)
                          )
                          : Container(
                            alignment: Alignment.center,

                            child: Card(
                              color: gl.backgroundTransparentBlackBox,
                              child: Container(
                                constraints: BoxConstraints(
                                  minHeight:
                                      MediaQuery.of(context).size.width * .15,
                                  minWidth:
                                      MediaQuery.of(context).size.width * .5,
                                  maxWidth:
                                      MediaQuery.of(context).size.width * .5,
                                ),
                                child: Row(
                                  mainAxisAlignment: MainAxisAlignment.center,
                                  children: [
                                    SizedBox(
                                      width:
                                          MediaQuery.of(context).size.width *
                                          .3,
                                      child: Text(
                                        "La carte choisie est en préparation dans la mémoire.",
                                        textAlign: TextAlign.center,
                                        style: TextStyle(
                                          color: Colors.white,
                                          fontSize:
                                              MediaQuery.of(
                                                context,
                                              ).size.width *
                                              .03,
                                        ),
                                      ),
                                    ),
                                    SizedBox(
                                      width:
                                          MediaQuery.of(context).size.width *
                                          .05,
                                    ),
                                    CircularProgressIndicator(
                                      constraints: BoxConstraints(
                                        minHeight:
                                            MediaQuery.of(context).size.width *
                                            .1,
                                        minWidth:
                                            MediaQuery.of(context).size.width *
                                            .1,
                                      ),
                                      strokeWidth:
                                          MediaQuery.of(context).size.width *
                                          .02,
                                    ),
                                  ],
                                ),
                              ),
                            ),
                          );
                    } else {
                      LayerBase l = gl.dico.getLayerBase(selLayer.mCode);
                      i++;
                      return TileLayer(
                        userAgentPackageName: "com.forestimator",
                        tileDisplay:
                            gl.modeMapFirstTileLayerTransparancy &&
                                    i > 1 &&
                                    gl.getLayersForFlutterMap().length == i
                                ? TileDisplay.instantaneous(opacity: 0.5)
                                : TileDisplay.instantaneous(opacity: 1.0),
                        wmsOptions: WMSTileLayerOptions(
                          baseUrl: "${l.mUrl}?",
                          format: 'image/png',
                          layers: [l.mWMSLayerName],
                          crs: epsg31370CRS,
                          transparent: true,
                        ),
                        tileDimension: tileSize.round(),
                      );
                    }
                  }).toList() +
                  <Widget>[
                    AnimatedLocationMarkerLayer(
                      position: LocationMarkerPosition(
                        latitude: gl.position?.latitude ?? 0.0,
                        longitude: gl.position?.longitude ?? 0.0,
                        accuracy: 10.0,
                      ),
                    ),
                  ] +
                  (_modeDrawPolygon
                      ? <Widget>[
                        if (gl.polygonLayers.isNotEmpty &&
                            gl
                                    .polygonLayers[gl.selectedPolygonLayer]
                                    .vertexes
                                    .length >
                                1 &&
                            !_modeShowButtonDrawPolygonMoveVertexes &&
                            _modeDrawPolygonAddVertexes)
                          PolylineLayer(
                            polylines: [
                              Polyline(
                                points: [
                                  gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .vertexes[gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .selectedPolyLinePoints[0]],
                                  gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .vertexes[gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .selectedPolyLinePoints[1]],
                                ],
                                color:
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .colorLine,
                                strokeWidth: 5.0,
                              ),
                            ],
                          ),

                        CircleLayer(circles: _drawnLayerPointsCircleMarker()),
                        PolygonLayer(polygons: _getPolygonesToDraw()),
                        MarkerLayer(markers: _drawnLayerPointsMarker()),
                        if (gl.polygonLayers.isNotEmpty &&
                            gl
                                .polygonLayers[gl.selectedPolygonLayer]
                                .vertexes
                                .isNotEmpty &&
                            !_modeShowButtonDrawPolygonAddVertexes)
                          CircleLayer(
                            circles: [
                              CircleMarker(
                                point:
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .vertexes[gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .selectedPolyLinePoints[0]],
                                radius: 15,
                                color: Colors.red,
                              ),
                            ],
                          ),
                      ]
                      : <Widget>[
                        if (gl.modeMapShowPolygons)
                          PolygonLayer(polygons: _getPolygonesToDraw()),
                      ]) +
                  <Widget>[
                    MarkerLayer(
                      markers: _placeVertexMovePointer() + _placeAnaPtMarker(),
                    ),
                    if (gl.modeMapShowPolygons)
                      MarkerLayer(markers: _getPolygonesInfos()),
                    if (gl.modeMapShowCustomMarker) MarkerLayer(markers: []),
                    if (gl.modeMapShowSearchMarker)
                      MarkerLayer(markers: _placeSearchMarker()),
                    if (_modeMeasurePath && _measurePath.length > 1)
                      PolylineLayer(
                        polylines: [
                          Polyline(
                            points: _measurePath,
                            color: Colors.blueGrey.withAlpha(200),
                            strokeWidth: 5.0,
                          ),
                        ],
                      ),
                    if (_modeMeasurePath)
                      MarkerLayer(markers: _getPathMeasureMarkers()),
                  ],
            ),
            (_modeDrawPolygon && gl.polygonLayers.isNotEmpty)
                ? Column(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Container(
                          width: MediaQuery.of(context).size.width * .98,
                          constraints: BoxConstraints(
                            minHeight: MediaQuery.of(context).size.height * .1,
                            maxHeight: MediaQuery.of(context).size.height * .1,
                          ),
                          child: TextButton(
                            onPressed: () {
                              refreshView(() {
                                _modePolygonList = true;
                              });
                              PopupDrawnLayerMenu(
                                gl.notificationContext!,
                                gl.polygonLayers[gl.selectedPolygonLayer].name,
                                (LatLng pos) {
                                  if (pos.longitude != 0.0 &&
                                      pos.latitude != 0.0) {
                                    _mapController.move(
                                      pos,
                                      _mapController.camera.zoom,
                                    );
                                  }
                                },
                                () {
                                  refreshView(() {
                                    _modePolygonList = false;
                                  });
                                },
                              );
                            },
                            onLongPress: () {
                              setState(() {
                                if (gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .center
                                            .longitude !=
                                        0.0 &&
                                    gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .center
                                            .latitude !=
                                        0.0) {
                                  _mapController.move(
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .center,
                                    _mapController.camera.zoom,
                                  );
                                }
                              });
                              gl.refreshMap(() {
                                gl.modeMapShowPolygons = true;
                              });
                            },
                            child: Card(
                              surfaceTintColor: Colors.transparent,
                              shadowColor: Colors.transparent,
                              color: gl
                                  .polygonLayers[gl.selectedPolygonLayer]
                                  .colorInside
                                  .withAlpha(255),
                              child: Row(
                                mainAxisAlignment:
                                    MainAxisAlignment.spaceEvenly,
                                children: [
                                  Icon(
                                    Icons.hexagon_outlined,
                                    size:
                                        MediaQuery.of(context).size.width * .1,
                                    color: getColorTextFromBackground(
                                      gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .colorInside
                                          .withAlpha(255),
                                    ),
                                  ),
                                  SizedBox(
                                    width:
                                        MediaQuery.of(context).size.width * .01,
                                  ),
                                  Icon(
                                    Icons.center_focus_strong,
                                    size:
                                        MediaQuery.of(context).size.width * .1,
                                    color: getColorTextFromBackground(
                                      gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .colorInside
                                          .withAlpha(255),
                                    ),
                                  ),
                                  SizedBox(
                                    width:
                                        MediaQuery.of(context).size.width * .01,
                                  ),
                                  SizedBox(
                                    width:
                                        MediaQuery.of(context).size.width * .4,
                                    child: Text(
                                      gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .name,
                                      style: TextStyle(
                                        color: getColorTextFromBackground(
                                          gl
                                              .polygonLayers[gl
                                                  .selectedPolygonLayer]
                                              .colorInside
                                              .withAlpha(255),
                                        ),
                                        fontSize:
                                            MediaQuery.of(context).size.width *
                                            .05,
                                      ),
                                    ),
                                  ),
                                  Text(
                                    "${(gl.polygonLayers[gl.selectedPolygonLayer].area / 100).round() / 100} Ha",
                                    style: TextStyle(
                                      color: getColorTextFromBackground(
                                        gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .colorInside
                                            .withAlpha(255),
                                      ),
                                      fontSize:
                                          MediaQuery.of(context).size.width *
                                          .05,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                )
                : _modeDrawPolygon
                ? Column(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        minHeight: MediaQuery.of(context).size.height * .1,
                        maxHeight: MediaQuery.of(context).size.height * .1,
                      ),
                      child: TextButton(
                        onPressed:
                            () => PopupDrawnLayerMenu(
                              gl.notificationContext!,
                              "",
                              (LatLng pos) {
                                if (pos.longitude != 0.0 &&
                                    pos.latitude != 0.0) {
                                  _mapController.move(
                                    pos,
                                    _mapController.camera.zoom,
                                  );
                                }
                              },
                              () {
                                refreshView(() {
                                  _modePolygonList = false;
                                });
                              },
                            ),
                        child: Card(
                          surfaceTintColor: Colors.transparent,
                          shadowColor: Colors.transparent,
                          color: gl.backgroundTransparentBlackBox,
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                            children: [
                              Icon(
                                Icons.add,
                                size: MediaQuery.of(context).size.width * .1,
                                color: Colors.white,
                              ),
                              SizedBox(
                                width: MediaQuery.of(context).size.width * .03,
                              ),
                              SizedBox(
                                width: MediaQuery.of(context).size.width * .6,
                                child: Text(
                                  "Tappez ici pour ajouter un Polygone",
                                  style: TextStyle(
                                    color: Colors.white,
                                    fontSize:
                                        MediaQuery.of(context).size.width * .04,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  ],
                )
                : Column(),
            _mainMenuBar(),
            if (_toolbarExtended) _toolBar(),
            if (_polygonToolbarExtended && gl.polygonLayers.isNotEmpty)
              _polygonToolbar(),
          ],
        ),
      ),
    );
  }

  void _stopMovingSelectedPoint() {
    _selectedPointToMove = null;
  }

  List<Polygon> _getPolygonesToDraw() {
    List<Polygon> that = [];
    for (var layer in gl.polygonLayers) {
      if (layer.numPoints > 2) {
        that.add(Polygon(points: layer.vertexes, color: layer.colorInside));
      }
    }
    return that;
  }

  List<CircleMarker> _drawnLayerPointsCircleMarker() {
    if (gl.polygonLayers.isEmpty) {
      return [];
    }
    List<CircleMarker> all = [];
    int i = 0;
    for (var point in gl.polygonLayers[gl.selectedPolygonLayer].vertexes) {
      all.add(
        CircleMarker(
          point: point,
          radius:
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i) &&
                      !_modeShowButtonDrawPolygonMoveVertexes &&
                      _modeDrawPolygonAddVertexes
                  ? iconSize / 2.7
                  : iconSize / 3,
          color:
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i) &&
                      !_modeShowButtonDrawPolygonMoveVertexes &&
                      _modeDrawPolygonAddVertexes
                  ? gl.polygonLayers[gl.selectedPolygonLayer].colorLine
                  : gl.polygonLayers[gl.selectedPolygonLayer].colorInside,
        ),
      );
      i++;
    }
    return all;
  }

  List<Marker> _placeSearchMarker() {
    List<Marker> all = [];
    int i = 0;
    for (gl.PoiMarker poi in gl.poiMarkerList) {
      double scale = 1.5;
      all.add(
        Marker(
          alignment: Alignment(0, -.075),
          width: 275,
          height: 1000,
          point: poi.position,
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: <Widget>[
              TextButton(
                style: ButtonStyle(
                  animationDuration: Duration(seconds: 1),
                  backgroundColor:
                      gl.selectedSearchMarker == i
                          ? WidgetStateProperty<Color>.fromMap(
                            <WidgetStatesConstraint, Color>{
                              WidgetState.any: getColorFromName(poi.name),
                            },
                          )
                          : WidgetStateProperty<Color>.fromMap(
                            <WidgetStatesConstraint, Color>{
                              WidgetState.any: getColorFromName(poi.name),
                            },
                          ),
                  foregroundColor:
                      gl.selectedSearchMarker == i
                          ? WidgetStateProperty<Color>.fromMap(
                            <WidgetStatesConstraint, Color>{
                              WidgetState.any: getColorTextFromBackground(
                                getColorFromName(poi.name),
                              ),
                            },
                          )
                          : WidgetStateProperty<Color>.fromMap(
                            <WidgetStatesConstraint, Color>{
                              WidgetState.any: getColorTextFromBackground(
                                getColorFromName(poi.name),
                              ),
                            },
                          ),
                ),
                onPressed: () {
                  refreshView(() {
                    if (gl.selectedSearchMarker == poi.index) {
                      gl.selectedSearchMarker = -1;
                    } else {
                      gl.selectedSearchMarker = poi.index;
                    }
                  });
                },
                child: Container(
                  constraints: BoxConstraints(maxWidth: 300),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children:
                        <Widget>[
                          gl.selectedSearchMarker == i
                              ? Text(
                                poi.name,
                                textScaler: TextScaler.linear(1.0),
                              )
                              : Text(
                                poi.name,
                                textScaler: TextScaler.linear(scale),
                              ),
                        ] +
                        _getSearchInfoBox(poi, i),
                  ),
                ),
              ),
            ],
          ),
        ),
      );
      i++;
    }
    for (gl.PoiMarker poi in gl.poiMarkerList) {
      all.add(
        Marker(
          alignment: Alignment.center,
          width: 50,
          height: 50,
          point: poi.position,
          child: IconButton(
            style: ButtonStyle(
              animationDuration: Duration(seconds: 1),
              backgroundColor: WidgetStateProperty<Color>.fromMap(
                <WidgetStatesConstraint, Color>{
                  WidgetState.any: Colors.transparent,
                },
              ),
            ),
            onPressed: () {
              refreshView(() {
                if (gl.selectedSearchMarker == poi.index) {
                  gl.selectedSearchMarker = -1;
                } else {
                  gl.selectedSearchMarker = poi.index;
                }
                gl.print("Selected searchMarker = ${gl.selectedSearchMarker}");
              });
            },
            icon: Icon(Icons.circle_notifications, color: Colors.red, size: 20),
          ),
        ),
      );
      i++;
    }
    if (gl.selectedSearchMarker > -1) {
      final temp = all[gl.selectedSearchMarker];
      all[gl.selectedSearchMarker] = all[all.length - 1];
      all[all.length - 1] = temp;
    }
    return all;
  }

  List<Widget> _getSearchInfoBox(gl.PoiMarker poi, int i) {
    double scale = 1;
    return gl.selectedSearchMarker == i
        ? <Widget>[
          Row(
            mainAxisAlignment: MainAxisAlignment.center,

            children: [
              Container(
                constraints: BoxConstraints(maxWidth: 200),
                child: Text(
                  poi.address,
                  textScaler: TextScaler.linear(scale),
                  textAlign: TextAlign.center,
                  maxLines: 10,
                ),
              ),
            ],
          ),
        ]
        : <Widget>[];
  }

  List<Marker> _placeAnaPtMarker() {
    if (_pt == null || !_modeAnaPtPreview) {
      return [];
    }
    return <Marker>[
      if (_lastPressWasShort)
        Marker(
          alignment: Alignment(0.0, -3.0),
          width:
              gl.anaPtPreview == null
                  ? MediaQuery.of(context).size.width * .1
                  : gl.dico
                          .getLayerBase(gl.anaPtPreview!.mCode)
                          .getValLabel(gl.anaPtPreview!.mRastValue)
                          .length >
                      3
                  ? MediaQuery.of(context).size.width * .15 +
                      gl.dico
                              .getLayerBase(gl.anaPtPreview!.mCode)
                              .getValLabel(gl.anaPtPreview!.mRastValue)
                              .length *
                          8.0
                  : gl.dico.getLayerBase(gl.anaPtPreview!.mCode).mCategorie !=
                      "Externe"
                  ? MediaQuery.of(context).size.width * .25
                  : MediaQuery.of(context).size.width * .0,
          height: MediaQuery.of(context).size.width * .1,
          point: _pt ?? const LatLng(0.0, 0.0),
          child: FloatingActionButton(
            backgroundColor: Color.fromRGBO(0, 0, 0, 0.8),
            foregroundColor: Colors.white,
            splashColor: gl.colorAgroBioTech,
            tooltip: "Faites une Analye Pontcuelle en ligne",

            onPressed: () async {
              if (!_doingAnaPt) {
                refreshView(() {
                  _doingAnaPt = true;
                });
                await _runAnaPt(
                  epsg4326.transform(
                    epsg31370,
                    proj4.Point(x: _pt!.longitude, y: _pt!.latitude),
                  ),
                );
                refreshView(() {
                  _doingAnaPt = false;
                });
                GoRouter.of(gl.notificationContext!).push("/anaPt");
              }
            },
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    AnaPtPreview(
                      position: _pt!,
                      after: () {
                        setState(() {});
                      },
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      Marker(
        alignment: Alignment.center,
        width: 50,
        height: 50,
        point: _pt ?? const LatLng(0.0, 0.0),
        child: const Icon(
          Icons.donut_small,
          color: Color.fromRGBO(0, 0, 0, .75),
        ),
      ),
      Marker(
        alignment: Alignment.center,
        width: 50,
        height: 50,
        point: _pt ?? const LatLng(0.0, 0.0),
        child: const Icon(
          Icons.donut_large_sharp,
          color: Color.fromRGBO(255, 255, 255, .75),
        ),
      ),
    ];
  }

  List<Marker> _placeVertexMovePointer() {
    List<Marker> ret = [];
    if ((_modeDrawPolygonMoveVertexes && _selectedPointToMove != null) ||
        (_modeMeasurePath && _modeMoveMeasurePath)) {
      ret.add(
        Marker(
          alignment: Alignment.center,
          width: iconSize * 1.1,
          height: iconSize * 1.1,
          point: _mapController.camera.center,
          child: const Icon(Icons.donut_large, color: Colors.red),
        ),
      );
    } else if (_modeDrawPolygonMoveVertexes || _modeMeasurePath) {
      ret.add(
        Marker(
          alignment: Alignment.center,
          width: iconSize,
          height: iconSize,
          point: _mapController.camera.center,
          child: const Icon(Icons.donut_small, color: Colors.blue),
        ),
      );
    }
    if (_modeMeasurePath && _measurePath.length > 1) {
      ret.add(
        Marker(
          alignment: Alignment(0, -2),
          width: 100,
          height: MediaQuery.of(context).size.width * .1,
          point: _mapController.camera.center,
          child: Card(
            color: Colors.black.withAlpha(164),
            child: Container(
              alignment: Alignment.center,
              child: Text(
                _computePathLength().round() < 10000
                    ? _computePathLength().round() < 5000
                        ? "${_computePathLength().round().toString()} m"
                        : "${((_computePathLength() / 1).round() / 1000).toString()} km"
                    : "${((_computePathLength() / 100).round() / 10).toString()} km",
                style: TextStyle(color: Colors.white),
              ),
            ),
          ),
        ),
      );
    }

    return ret;
  }

  Offset _epsg4326ToEpsg31370(proj4.Point spPoint) {
    return Offset(
      gl.epsg4326.transform(gl.epsg31370, spPoint).x,
      gl.epsg4326.transform(gl.epsg31370, spPoint).y,
    );
  }

  double _computePathLength() {
    if (_measurePath.length < 2) {
      return 0.0;
    }
    List<Offset> path = [];
    for (LatLng point in _measurePath) {
      path.add(
        _epsg4326ToEpsg31370(
          proj4.Point(y: point.latitude, x: point.longitude),
        ),
      );
    }
    double length = 0.0;
    Offset currentPoint = path.removeAt(0);
    for (Offset point in path) {
      length = length + (currentPoint - point).distance;
      currentPoint = point;
    }
    return length;
  }

  List<Marker> _getPathMeasureMarkers() {
    List<Marker> all = [];
    all.addAll(
      List<Marker>.generate(_measurePath.length, (i) {
        return Marker(
          alignment: Alignment.center,
          point: _measurePath[i],
          child: CircleAvatar(
            radius: 10.0,
            backgroundColor: Colors.blueGrey.withAlpha(164),
            child: TextButton(
              onPressed: () {
                setState(() {
                  _modeMoveMeasurePath = !_modeMoveMeasurePath;
                  if (_modeMoveMeasurePath ||
                      (selectedMeasurePointToMove > -1 &&
                          selectedMeasurePointToMove != i)) {
                    selectedMeasurePointToMove = i;
                    _mapController.move(
                      _measurePath[i],
                      _mapController.camera.zoom,
                    );
                  } else {
                    selectedMeasurePointToMove = -1;
                    _mapController.move(
                      _measurePath[i],
                      _mapController.camera.zoom,
                    );
                  }
                });
              },
              onLongPress: () {
                setState(() {
                  _measurePath.removeAt(i);
                });
              },
              child: SizedBox(width: 10, height: 10),
            ),
          ),
        );
      }),
    );
    return all;
  }

  List<Marker> _drawnLayerPointsMarker() {
    if (gl.polygonLayers.isEmpty) {
      return [];
    }
    List<Marker> all = [];
    int count = 0;
    for (var point in gl.polygonLayers[gl.selectedPolygonLayer].vertexes) {
      all.add(
        Marker(
          alignment: Alignment.center,
          width: iconSize,
          height: iconSize,
          point: point,
          child: TextButton(
            onPressed: () {
              if (_modeDrawPolygonAddVertexes) {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer]
                      .refreshSelectedLinePoints(point);
                });
              } else {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer]
                      .refreshSelectedLinePoints(point);
                  _modeShowButtonDrawPolygonAddVertexes = false;
                  _modeShowButtonDrawPolygonMoveVertexes = true;
                  _modeShowButtonDrawPolygonRemoveVertexes = true;
                  _modeDrawPolygonAddVertexes = false;
                  _modeDrawPolygonMoveVertexes = false;
                  _modeDrawPolygonRemoveVertexes = false;
                });
              }
              /*if (_modeDrawPolygonRemoveVertexes) {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer].removePoint(point);
                });
              }*/
            },
            child: Text(
              overflow: TextOverflow.visible,
              "$count",
              maxLines: 1,
              style: TextStyle(color: Colors.black, fontSize: iconSize / 3),
            ),
          ),
        ),
      );
      count++;
    }
    return all;
  }

  void _closePolygonMenu() {
    _polygonToolbarExtended = false;
    _modeDrawPolygon = false;
    _modeSearch = false;
    _modeDrawPolygonRemoveVertexes = false;
    _modeDrawPolygonMoveVertexes = false;
    _modeAnaPtPreview = true;
    _modeShowButtonDrawPolygonAddVertexes = false;
    _modeShowButtonDrawPolygonMoveVertexes = false;
    _modeShowButtonDrawPolygonRemoveVertexes = false;
  }

  void _closeToolbarMenu() {
    _toolbarExtended = false;
    _modeSearch = false;
    _modeMeasurePath = false;
  }

  void _closeSwitchesMenu() {
    _modeLayerSwitches = false;
  }

  Widget _mainMenuBar({bool dummy = false, Function? close}) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(
                maxHeight: MediaQuery.of(context).size.width * .2,
                minHeight: MediaQuery.of(context).size.width * .2,
                maxWidth: MediaQuery.of(context).size.width * .65,
              ),
              child: Card(
                shadowColor: Colors.transparent,
                color:
                    dummy
                        ? Colors.transparent
                        : gl.backgroundTransparentBlackBox,
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    Container(
                      color:
                          dummy
                              ? Colors.transparent
                              : !_toolbarExtended
                              ? Colors.transparent
                              : Colors.green.withAlpha(128),
                      child: IconButton(
                        color:
                            dummy
                                ? Colors.transparent
                                : _toolbarExtended
                                ? Colors.white
                                : Colors.green,
                        iconSize: MediaQuery.of(context).size.width * .12,
                        isSelected: _toolbarExtended,
                        onPressed: () {
                          setState(() {
                            _toolbarExtended = !_toolbarExtended;
                            if (_toolbarExtended) {
                              _closePolygonMenu();
                              _closeSwitchesMenu();
                              if (dummy) {
                                close!();
                              }
                            } else {
                              _closeToolbarMenu();
                            }
                          });
                        },
                        icon: Icon(Icons.forest),
                      ),
                    ),
                    Container(
                      color:
                          dummy
                              ? Colors.transparent
                              : !_polygonToolbarExtended
                              ? Colors.transparent
                              : Colors.yellow.withAlpha(128),
                      child: IconButton(
                        color:
                            dummy
                                ? Colors.transparent
                                : _polygonToolbarExtended
                                ? Colors.white
                                : Colors.yellow,
                        iconSize: MediaQuery.of(context).size.width * .12,
                        isSelected: _polygonToolbarExtended,
                        onPressed: () {
                          setState(() {
                            _polygonToolbarExtended = !_polygonToolbarExtended;
                            _modeDrawPolygon = _polygonToolbarExtended;
                            if (_modeDrawPolygon) {
                              if (gl.polygonLayers.isNotEmpty) {
                                _modeShowButtonDrawPolygonAddVertexes = true;
                              }
                              _closeSwitchesMenu();
                              _closeToolbarMenu();
                              if (dummy) {
                                close!();
                              }
                            } else {
                              _closePolygonMenu();
                            }
                          });
                        },
                        icon: Icon(Icons.hexagon_outlined),
                      ),
                    ),
                    dummy
                        ? Container(
                          color: Colors.transparent,
                          width: MediaQuery.of(context).size.width * .12,
                          height: MediaQuery.of(context).size.width * .12,
                          child: IconButton(
                            color: Colors.transparent,
                            iconSize: MediaQuery.of(context).size.width * .12,
                            isSelected: _modeLayerSwitches,
                            onPressed: () {
                              setState(() {
                                _modeLayerSwitches = false;
                                _closeSwitchesMenu();
                                if (dummy) {
                                  close!();
                                }
                              });
                            },
                            icon: Icon(Icons.remove_red_eye),
                          ),
                        )
                        : Container(
                          color:
                              !_modeLayerSwitches
                                  ? Colors.transparent
                                  : Colors.brown.withAlpha(128),
                          child: IconButton(
                            color:
                                _modeLayerSwitches
                                    ? Colors.white
                                    : Colors.brown,
                            iconSize: MediaQuery.of(context).size.width * .12,
                            isSelected: _modeLayerSwitches,
                            onPressed: () {
                              setState(() {
                                _modeLayerSwitches = true;
                                _closePolygonMenu();
                                _closeToolbarMenu();
                                PopupLayerSwitcher(
                                  gl.notificationContext!,
                                  () {
                                    setState(() {
                                      _closeSwitchesMenu();
                                    });
                                  },
                                  (close) {
                                    return _mainMenuBar(
                                      dummy: true,
                                      close: close,
                                    );
                                  },
                                  (LatLng pos) {
                                    if (pos.longitude != 0.0 &&
                                        pos.latitude != 0.0) {
                                      _mapController.move(
                                        pos,
                                        _mapController.camera.zoom,
                                      );
                                    }
                                  },
                                );
                              });
                            },
                            icon: Icon(Icons.remove_red_eye),
                          ),
                        ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _toolBar() {
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(
                maxHeight:
                    gl.modeDevelopper
                        ? gl.position != null
                            ? MediaQuery.of(context).size.width * .75
                            : MediaQuery.of(context).size.width * .6
                        : gl.position != null
                        ? MediaQuery.of(context).size.width * .6
                        : MediaQuery.of(context).size.width * .45,
                maxWidth: MediaQuery.of(context).size.width * .2,
              ),
              child: Card(
                color: gl.backgroundTransparentBlackBox,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    gl.position != null
                        ? Card(
                          color: Colors.orange.withAlpha(128),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.end,
                            children: [
                              IconButton(
                                iconSize:
                                    MediaQuery.of(context).size.width * .12,
                                color: gl.colorAgroBioTech,
                                onPressed: () async {
                                  if (!_doingAnaPt) {
                                    refreshView(() {
                                      _doingAnaPt = true;
                                    });
                                    await _runAnaPt(
                                      epsg4326.transform(
                                        epsg31370,
                                        proj4.Point(
                                          x: gl.position?.longitude ?? 0.0,
                                          y: gl.position?.latitude ?? 0.0,
                                        ),
                                      ),
                                    );
                                    _updatePtMarker(
                                      LatLng(
                                        gl.position?.latitude ?? 0.0,
                                        gl.position?.longitude ?? 0.0,
                                      ),
                                    );
                                    GoRouter.of(
                                      gl.notificationContext!,
                                    ).push("/anaPt");
                                    refreshView(() {
                                      _doingAnaPt = false;
                                    });
                                  }
                                },
                                icon: const Icon(Icons.analytics),
                              ),
                              IconButton(
                                iconSize:
                                    MediaQuery.of(context).size.width * .12,
                                color: Colors.red,
                                onPressed: () async {
                                  if (gl.position != null) {
                                    refreshView(() {
                                      _mapController.move(
                                        LatLng(
                                          gl.position?.latitude ?? 0.0,
                                          gl.position?.longitude ?? 0.0,
                                        ),
                                        8,
                                      );
                                    });
                                  }
                                },
                                icon: Icon(Icons.gps_fixed),
                              ),
                            ],
                          ),
                        )
                        : Column(
                          mainAxisAlignment: MainAxisAlignment.end,
                          children: [
                            IconButton(
                              iconSize: MediaQuery.of(context).size.width * .12,
                              color: Colors.black,
                              onPressed: () async {
                                if (gl.position != null) {
                                  refreshView(() {});
                                }
                              },
                              icon: Icon(Icons.gps_fixed),
                            ),
                          ],
                        ),
                    if (gl.modeDevelopper)
                      Container(
                        color:
                            !_modeMeasurePath
                                ? Colors.transparent
                                : Colors.lightBlue.withAlpha(128),
                        child: IconButton(
                          color:
                              _modeMeasurePath
                                  ? Colors.white
                                  : Colors.lightBlue,
                          iconSize: MediaQuery.of(context).size.width * .12,
                          isSelected: _modeMeasurePath,
                          onPressed: () {
                            setState(() {
                              _modeMeasurePath = !_modeMeasurePath;
                              _modeAnaPtPreview = !_modeMeasurePath;
                              _measurePath.clear();
                            });
                          },
                          icon: Icon(Icons.more_horiz_outlined),
                        ),
                      ),
                    Container(
                      color:
                          !_modeSearch
                              ? Colors.transparent
                              : Colors.blueGrey.withAlpha(128),
                      child: IconButton(
                        iconSize: MediaQuery.of(context).size.width * .12,
                        color: _modeSearch ? Colors.white : Colors.blueGrey,
                        onPressed: () {
                          setState(() {
                            _modeSearch = true;
                          });
                          PopupSearchMenu(
                            gl.notificationContext!,
                            "",
                            (LatLng pos) {
                              if (pos.longitude != 0.0 && pos.latitude != 0.0) {
                                _mapController.move(
                                  pos,
                                  _mapController.camera.zoom,
                                );
                              }
                            },
                            () {
                              refreshView(() {
                                _modeSearch = false;
                              });
                            },
                          );
                        },
                        icon: FaIcon(FontAwesomeIcons.magnifyingGlassLocation),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
        _placeholder(
          constraints: BoxConstraints(
            maxHeight: MediaQuery.of(context).size.width * .2,
            minHeight: MediaQuery.of(context).size.width * .2,
            maxWidth: MediaQuery.of(context).size.width * .1,
          ),
        ),
      ],
    );
  }

  Widget _polygonToolbar() {
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Container(
              alignment: Alignment.center,
              height:
                  MediaQuery.of(context).size.width * .05 +
                  (_modeShowButtonDrawPolygonMoveVertexes
                      ? MediaQuery.of(context).size.width * .2
                      : 0.0) +
                  (_modeShowButtonDrawPolygonRemoveVertexes
                      ? MediaQuery.of(context).size.width * .2
                      : 0.0) +
                  (_modeShowButtonDrawPolygonAddVertexes
                      ? MediaQuery.of(context).size.width * .2
                      : 0.0),
              constraints: BoxConstraints(
                minWidth: MediaQuery.of(context).size.width * .2,
                maxWidth: MediaQuery.of(context).size.width * .2,
              ),
              child: Card(
                color: gl.backgroundTransparentBlackBox,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    if (_modeShowButtonDrawPolygonMoveVertexes)
                      Container(
                        color: _polygonMenuColorTools(
                          _modeDrawPolygonMoveVertexes,
                        ),
                        child: IconButton(
                          color:
                              _modeDrawPolygonMoveVertexes
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          iconSize: iconSize,
                          onPressed: () async {
                            refreshView(() {
                              _modeDrawPolygonMoveVertexes =
                                  !_modeDrawPolygonMoveVertexes;
                            });
                            if (_modeDrawPolygonMoveVertexes == true) {
                              refreshView(() {
                                LatLng point =
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .polygonPoints[gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .selectedPolyLinePoints[0]];
                                if (_selectedPointToMove == null) {
                                  _selectedPointToMove = point;
                                  _mapController.move(
                                    point,
                                    _mapController.camera.zoom,
                                  );
                                } else {
                                  if (point.latitude ==
                                          _selectedPointToMove!.latitude &&
                                      point.longitude ==
                                          _selectedPointToMove!.longitude) {
                                    _stopMovingSelectedPoint();
                                  } else {
                                    _selectedPointToMove = point;
                                    _mapController.move(
                                      point,
                                      _mapController.camera.zoom,
                                    );
                                  }
                                }
                              });
                              _modeDrawPolygonAddVertexes = false;
                              _modeDrawPolygonRemoveVertexes = false;
                            } else {
                              refreshView(() {
                                _stopMovingSelectedPoint();
                              });
                            }
                          },
                          icon: const Icon(Icons.open_with_rounded),
                        ),
                      ),
                    if (_modeShowButtonDrawPolygonRemoveVertexes)
                      Container(
                        color: _polygonMenuColorTools(
                          _modeDrawPolygonRemoveVertexes,
                        ),
                        child: IconButton(
                          iconSize: iconSize,
                          color:
                              _modeDrawPolygonRemoveVertexes
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          onPressed: () async {
                            refreshView(() {
                              _modeDrawPolygonRemoveVertexes =
                                  !_modeDrawPolygonRemoveVertexes;
                            });
                            if (_modeDrawPolygonRemoveVertexes == true) {
                              refreshView(() {
                                gl.polygonLayers[gl.selectedPolygonLayer]
                                    .removePoint(
                                      gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .polygonPoints[gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .selectedPolyLinePoints[0]],
                                    );
                              });
                              _modeDrawPolygonMoveVertexes = false;
                              _modeDrawPolygonAddVertexes = false;
                              _stopMovingSelectedPoint();
                              refreshView(() {
                                if (gl
                                    .polygonLayers[gl.selectedPolygonLayer]
                                    .polygonPoints
                                    .isEmpty) {
                                  _modeShowButtonDrawPolygonAddVertexes = true;
                                  _modeShowButtonDrawPolygonMoveVertexes =
                                      false;
                                  _modeShowButtonDrawPolygonRemoveVertexes =
                                      false;
                                  _modeDrawPolygonAddVertexes = false;
                                  _modeDrawPolygonMoveVertexes = false;
                                  _modeDrawPolygonRemoveVertexes = false;
                                } else {
                                  _modeShowButtonDrawPolygonAddVertexes = false;
                                  _modeShowButtonDrawPolygonMoveVertexes = true;
                                  _modeShowButtonDrawPolygonRemoveVertexes =
                                      true;
                                  _modeDrawPolygonAddVertexes = false;
                                  _modeDrawPolygonMoveVertexes = false;
                                  _modeDrawPolygonRemoveVertexes = false;
                                }
                              });
                            }
                          },
                          icon: const Icon(Icons.remove_circle),
                        ),
                      ),
                    if (_modeShowButtonDrawPolygonAddVertexes)
                      Container(
                        color: _polygonMenuColorTools(
                          _modeDrawPolygonAddVertexes,
                        ),
                        child: IconButton(
                          iconSize: iconSize,
                          color:
                              _modeDrawPolygonAddVertexes
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          onPressed: () async {
                            refreshView(() {
                              _modeDrawPolygonAddVertexes =
                                  !_modeDrawPolygonAddVertexes;
                            });
                            if (_modeDrawPolygonAddVertexes == true) {
                              _modeDrawPolygonRemoveVertexes = false;
                              _modeDrawPolygonMoveVertexes = false;
                              refreshView(() {
                                _stopMovingSelectedPoint();
                              });
                            }
                          },
                          icon: const Icon(Icons.add_circle),
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ],
        ),
        _placeholder(
          constraints: BoxConstraints(
            maxHeight: MediaQuery.of(context).size.width * .2,
            minHeight: MediaQuery.of(context).size.width * .2,
            maxWidth: MediaQuery.of(context).size.width * .1,
          ),
        ),
      ],
    );
  }

  List<Marker> _getPolygonesInfos() {
    return List.generate(gl.polygonLayers.length, (i) {
      String textArea = "${(gl.polygonLayers[i].area / 100).round() / 100} Ha";
      return Marker(
        alignment: Alignment.center,

        width:
            textArea.length > gl.polygonLayers[i].name.length
                ? MediaQuery.of(context).size.width * .15 +
                    textArea.length * MediaQuery.of(context).size.width * .02
                : MediaQuery.of(context).size.width * .15 +
                    gl.polygonLayers[i].name.length *
                        MediaQuery.of(context).size.width *
                        .02,
        height: MediaQuery.of(context).size.width * .15,
        point: gl.polygonLayers[i].center,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        Icon(
                          Icons.layers,
                          color: gl.polygonLayers[i].colorLine,
                        ),
                        Text(
                          gl.polygonLayers[i].name,
                          overflow: TextOverflow.clip,
                        ),
                      ],
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        Icon(
                          Icons.square_foot,
                          color: gl.polygonLayers[i].colorLine,
                        ),
                        Text(textArea),
                      ],
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
      );
    });
  }

  Widget _placeholder({BoxConstraints? constraints, Alignment? alignement}) {
    return Container(constraints: constraints, alignment: alignement);
  }

  void _writeColorToMemory(
    String name,
    SharedPreferences prefs,
    Color color,
  ) async {
    await prefs.setInt('$name.r', (color.r * 255).round());
    await prefs.setInt('$name.g', (color.g * 255).round());
    await prefs.setInt('$name.b', (color.b * 255).round());
    await prefs.setDouble('$name.a', color.a);
  }

  void _writePolygonPointsToMemory(
    String name,
    SharedPreferences prefs,
    List<LatLng> polygon,
  ) async {
    int i = 0;
    for (var point in polygon) {
      await prefs.setDouble('$name.$i-lat', point.latitude);
      await prefs.setDouble('$name.$i-lng', point.longitude);
      i++;
    }
    await prefs.setInt('$name.nPolyPoints', i);
  }

  void _writePositionDataToSharedPreferences(
    double lon,
    double lat,
    double zoom,
  ) async {
    if (_mapFrameCounter % 100 == 0) {
      final SharedPreferences prefs = await SharedPreferences.getInstance();
      await prefs.setDouble('mapCenterLat', lat);
      await prefs.setDouble('mapCenterLon', lon);
      await prefs.setDouble('mapZoom', zoom);
      gl.print("position saved to prefs: $lon $lat $zoom");
    }
  }

  void _writeDataToSharedPreferences() async {
    if (gl.saveChangesToPolygoneToPrefs) {
      final SharedPreferences prefs = await SharedPreferences.getInstance();
      gl.saveChangesToPolygoneToPrefs = false;
      int i = 0;
      for (var polygon in gl.polygonLayers) {
        await prefs.setString('poly$i.name', polygon.name);
        await prefs.setDouble('poly$i.area', polygon.area);
        await prefs.setDouble('poly$i.perimeter', polygon.perimeter);
        await prefs.setDouble(
          'poly$i.transparencyInside',
          polygon.transparencyInside,
        );
        await prefs.setDouble(
          'poly$i.transparencyLine',
          polygon.transparencyLine,
        );
        _writeColorToMemory('poly$i.colorInside', prefs, polygon.colorInside);
        _writeColorToMemory('poly$i.colorLine', prefs, polygon.colorLine);
        _writePolygonPointsToMemory(
          'poly$i.poly',
          prefs,
          polygon.polygonPoints,
        );
        i++;
      }
      await prefs.setInt('nPolys', i);
      gl.print("polygones saved to prefs");
    }
  }

  static bool _refreshLocation = false;

  void updateLocation() async {
    try {
      if (_refreshLocation) {
        return;
      } else {
        _refreshLocation = true;
      }
      Position newPosition = await Geolocator.getCurrentPosition().timeout(
        Duration(seconds: 5),
        onTimeout: () {
          gl.print("Geolocator timeout reached!");
          return gl.position!;
        },
      );

      refreshView(() {
        gl.position = newPosition;
      });
      _refreshLocation = false;
    } catch (e) {
      gl.print("Exception getting position!");
      gl.print("$e");
      _refreshLocation = false;
    }
  }
}

Future<Position?> acquireUserLocation() async {
  if (locationGranted()) {
    try {
      return await Geolocator.getCurrentPosition();
    } on LocationServiceDisabledException {
      return null;
    } catch (e) {
      gl.print("$e");
      return null;
    }
  } else {
    return null;
  }
}

class AnaPtPreview extends StatefulWidget {
  final LatLng position;
  final Function after;
  const AnaPtPreview({super.key, required this.position, required this.after});

  @override
  State<StatefulWidget> createState() => _AnaPtPreview();
}

class _AnaPtPreview extends State<AnaPtPreview> {
  static LatLng? lastRequested;

  @override
  Widget build(BuildContext context) {
    if (lastRequested == null || lastRequested != widget.position) {
      lastRequested = widget.position;
      _runAnaPtPreview(
        gl.epsg4326ToEpsg31370(
          proj4.Point(
            x: widget.position.longitude,
            y: widget.position.latitude,
          ),
        ),
        widget.after,
      );
    }
    if (gl.anaPtPreview != null) {
      Color color = gl.dico
          .getLayerBase(gl.anaPtPreview!.mCode)
          .getValColor(gl.anaPtPreview!.mRastValue);
      String text = gl.dico
          .getLayerBase(gl.anaPtPreview!.mCode)
          .getValLabel(gl.anaPtPreview!.mRastValue);
      return Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Container(
                constraints: BoxConstraints(minHeight: 20, minWidth: 20),
                color: color == Colors.transparent ? Colors.white : color,
              ),
              Container(
                alignment: Alignment.center,
                constraints: BoxConstraints(minHeight: 20, minWidth: 20),
                child: Text(":"),
              ),
              Text(text == "" ? "No data" : text),
            ],
          ),
        ],
      );
    }
    return const CircularProgressIndicator(color: gl.colorAgroBioTech);
  }

  Future _runAnaPtPreview(proj4.Point ptBL72, Function after) async {
    Map data;
    bool internet = await InternetConnection().hasInternetAccess;
    if (!gl.offlineMode) {
      if (internet) {
        String request =
            "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/${gl.getInterfaceSelectedLCode().first}/x/${ptBL72.x}/y/${ptBL72.y}";
        try {
          var res = await http.get(Uri.parse(request));
          if (res.statusCode != 200) throw HttpException('${res.statusCode}');
          data = jsonDecode(res.body);
          setState(() {
            gl.anaPtPreview = LayerAnaPt.fromMap(data["RequestedLayers"].first);
          });
        } catch (e) {
          gl.print(request);
          gl.print("$e");
        }
      } else {
        showDialog(
          context: gl.notificationContext!,
          builder: (BuildContext context) {
            return PopupNoInternet();
          },
        );
      }
    } else {
      int val = await gl.dico
          .getLayerBase(gl.selectedLayerForMap.first.mCode)
          .getValXY(ptBL72);
      setState(() {
        gl.anaPtPreview = LayerAnaPt(
          mCode: gl.selectedLayerForMap.first.mCode,
          mRastValue: val,
        );
      });
    }
    after();
  }
}
