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
  bool _modeDrawPolygon = false;
  bool _modeDrawPolygonAddVertexes = false;
  bool _modeDrawPolygonRemoveVertexes = false;
  bool _modeDrawPolygonMoveVertexes = false;

  bool _modeLayerProperties = false;

  bool _modeProjectProperties = false;
  bool _modeSearch = false;

  Color _polygonMenuColor(bool choice) => choice ? Colors.orange : Colors.brown;
  Color _polygonMenuColorTools(bool choice) =>
      choice ? Colors.lightGreenAccent : Colors.grey;

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
                    _modeDrawPolygonAddVertexes
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
                          }),
                        }
                        : _modeDrawPolygon
                        ? (tapPosition, point) async => {}
                        : (tapPosition, point) async => {
                          _lastPressWasShort = true,
                          _updatePtMarker(point),
                        },
                onPositionChanged: (position, e) async {
                  if (!e) return;
                  updateLocation();
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
                  gl.interfaceSelectedLayerKeys.reversed.map<Widget>((
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
                        _provider?.init();
                      }
                      return _provider!.loaded
                          ? TileLayer(
                            tileDisplay:
                                gl.modeMapFirstTileLayerTrancparancy &&
                                        i > 1 &&
                                        gl.interfaceSelectedLayerKeys.length ==
                                            i
                                    ? TileDisplay.instantaneous(opacity: 0.5)
                                    : TileDisplay.instantaneous(opacity: 1.0),
                            tileProvider: _provider,
                            maxZoom: gl.globalMaxOfflineZoom,
                            minZoom:
                                gl.globalMinOfflineZoom, // si minZoom de la map est moins restrictif (moins élevé) que celui-ci, la carte ne s'affiche juste pas (écran blanc)
                          )
                          : Container();
                    } else if (selLayer.offline) {
                      // deuxième carte offline ; on ne fait rien avec, un seul provider
                      return Container();
                    } else {
                      LayerBase l = gl.dico.getLayerBase(selLayer.mCode);
                      i++;
                      return TileLayer(
                        userAgentPackageName: "com.forestimator",
                        tileDisplay:
                            gl.modeMapFirstTileLayerTrancparancy &&
                                    i > 1 &&
                                    gl.interfaceSelectedLayerKeys.length == i
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
                        accuracy: 1.0,
                      ),
                    ),
                  ] +
                  (_modeDrawPolygon
                      ? <Widget>[
                        CircleLayer(circles: _drawnLayerPointsCircleMarker()),
                        PolygonLayer(polygons: _getPolygonesToDraw()),
                        MarkerLayer(markers: _drawnLayerPointsMarker()),
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
                  ],
            ),
            if (_modeDrawPolygon && gl.polygonLayers.isNotEmpty)
              Column(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Container(
                    constraints: BoxConstraints(
                      minHeight: MediaQuery.of(context).size.height * .04,
                      maxHeight: MediaQuery.of(context).size.height * .1,
                    ),
                    child: TextButton(
                      onLongPress:
                          () => PopupDrawnLayerMenu(
                            gl.notificationContext!,
                            gl.polygonLayers[gl.selectedPolygonLayer].name,
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
                                _modeProjectProperties = false;
                              });
                            },
                          ),
                      onPressed: () {
                        setState(() {
                          if (gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .center
                                      .longitude !=
                                  0.0 &&
                              gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .center
                                      .latitude !=
                                  0.0) {
                            _mapController.move(
                              gl.polygonLayers[gl.selectedPolygonLayer].center,
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
                          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                          children: [
                            Text(
                              gl.polygonLayers[gl.selectedPolygonLayer].name,
                              style: TextStyle(
                                color: getColorTextFromBackground(
                                  gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .colorInside
                                      .withAlpha(255),
                                ),
                                fontSize:
                                    MediaQuery.of(context).size.height * .02,
                              ),
                            ),
                            Text(
                              "${(gl.polygonLayers[gl.selectedPolygonLayer].area / 100).round() / 100} Ha",
                              style: TextStyle(
                                color: getColorTextFromBackground(
                                  gl
                                      .polygonLayers[gl.selectedPolygonLayer]
                                      .colorInside
                                      .withAlpha(255),
                                ),
                                fontSize:
                                    MediaQuery.of(context).size.height * .02,
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            _toolbarGuard(),
            gl.position != null
                ? Row(
                  mainAxisAlignment: MainAxisAlignment.end,
                  children: [
                    Column(
                      mainAxisAlignment: MainAxisAlignment.end,
                      children: [
                        IconButton(
                          iconSize: iconSize,
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
                          iconSize: iconSize,
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
                          icon: const Icon(Icons.gps_fixed),
                        ),
                      ],
                    ),
                  ],
                )
                : Row(
                  mainAxisAlignment: MainAxisAlignment.end,
                  children: [
                    Column(
                      mainAxisAlignment: MainAxisAlignment.end,
                      children: [
                        IconButton(
                          iconSize: iconSize,
                          color: Colors.black,
                          onPressed: () async {
                            if (gl.position != null) {
                              refreshView(() {});
                            }
                          },
                          icon: const Icon(Icons.gps_fixed),
                        ),
                      ],
                    ),
                  ],
                ),
          ],
        ),
      ),
    );
  }

  Widget _toolbarGuard() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        Column(
          mainAxisAlignment: MainAxisAlignment.end,
          children: [
            _toolbarExtended ? _toolbar() : Container(),
            IconButton(
              iconSize: iconSize,
              color: _toolbarExtended ? gl.colorAgroBioTech : Colors.black,
              onPressed: () async {
                refreshView(() {
                  _toolbarExtended = !_toolbarExtended;
                });
                if (_toolbarExtended == false) {
                  _closeLayerPropertiesMenu();
                  _closePolygonDrawMenu();
                  _closeProjectMenu();
                }
              },
              icon: const Icon(Icons.forest),
            ),
          ],
        ),
      ],
    );
  }

  Widget _toolbar() {
    return Column(
      children: [
        _searchButton(),
        _layerSwitcherButton(),
        _modeDrawPolygon ? _polygonToolbar() : Container(),
        _drawPolygonButton(),
      ],
    );
  }

  void _stopMovingSelectedPoint() {
    _selectedPointToMove = null;
  }

  Widget _polygonToolbar() {
    return Column(
      children: [
        _layerPolygonButton(),
        IconButton(
          iconSize: iconSize,
          color: _polygonMenuColorTools(_modeDrawPolygonMoveVertexes),
          onPressed: () async {
            refreshView(() {
              _modeDrawPolygonMoveVertexes = !_modeDrawPolygonMoveVertexes;
            });
            if (_modeDrawPolygonMoveVertexes == true) {
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
        IconButton(
          iconSize: iconSize,
          color: _polygonMenuColorTools(_modeDrawPolygonRemoveVertexes),

          onPressed: () async {
            refreshView(() {
              _modeDrawPolygonRemoveVertexes = !_modeDrawPolygonRemoveVertexes;
            });
            if (_modeDrawPolygonRemoveVertexes == true) {
              _modeDrawPolygonMoveVertexes = false;
              _modeDrawPolygonAddVertexes = false;
              refreshView(() {
                _stopMovingSelectedPoint();
              });
            }
          },
          icon: const Icon(Icons.remove_circle),
        ),
        IconButton(
          iconSize: iconSize,
          color: _polygonMenuColorTools(_modeDrawPolygonAddVertexes),
          onPressed: () async {
            refreshView(() {
              _modeDrawPolygonAddVertexes = !_modeDrawPolygonAddVertexes;
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
      ],
    );
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
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i)
                  ? iconSize / 2.7
                  : iconSize / 3,
          color:
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i)
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
          alignment: Alignment(0, -.2),
          width: 275,
          height: 500,
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
    if (_modeDrawPolygonMoveVertexes && _selectedPointToMove != null) {
      ret.add(
        Marker(
          alignment: Alignment.center,
          width: iconSize * 1.1,
          height: iconSize * 1.1,
          point: _mapController.camera.center,
          child: const Icon(Icons.donut_large, color: Colors.red),
        ),
      );
    } else if (_modeDrawPolygonMoveVertexes) {
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
    return ret;
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
              if (_modeDrawPolygonRemoveVertexes) {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer].removePoint(point);
                });
              }
              if (_modeDrawPolygonMoveVertexes) {
                refreshView(() {
                  if (_selectedPointToMove == null) {
                    _selectedPointToMove = point;
                    _mapController.move(point, _mapController.camera.zoom);
                  } else {
                    if (point.latitude == _selectedPointToMove!.latitude &&
                        point.longitude == _selectedPointToMove!.longitude) {
                      _stopMovingSelectedPoint();
                    } else {
                      _selectedPointToMove = point;
                      _mapController.move(point, _mapController.camera.zoom);
                    }
                  }
                });
              }
              if (_modeDrawPolygonAddVertexes) {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer]
                      .refreshSelectedLinePoints(point);
                });
              }
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

  void _closeProjectMenu() {
    refreshView(() {
      _modeProjectProperties = false;
    });
  }

  void _closeSearchMenu() {
    refreshView(() {
      _modeSearch = false;
    });
  }

  void _closePolygonDrawMenu() {
    refreshView(() {
      _modeDrawPolygon = false;
      _modeDrawPolygonAddVertexes = false;
      _modeDrawPolygonRemoveVertexes = false;
      _modeDrawPolygonMoveVertexes = false;
      _stopMovingSelectedPoint();
      _modeAnaPtPreview = true;
    });
  }

  void _closeLayerPropertiesMenu() {
    refreshView(() {
      _modeLayerProperties = false;
    });
  }

  Widget _drawPolygonButton() {
    return IconButton(
      iconSize: iconSize,
      color: _polygonMenuColor(_modeDrawPolygon),
      onPressed: () async {
        refreshView(() {
          _modeDrawPolygon = !_modeDrawPolygon;
        });
        gl.refreshMap(() {
          gl.modeMapShowPolygons = true;
        });
        if (_modeDrawPolygon == false) {
          _closePolygonDrawMenu();
          _modeAnaPtPreview = true;
        } else {
          _modeAnaPtPreview = false;
          _closeLayerPropertiesMenu();
          _closeProjectMenu();
        }
      },
      icon: const Icon(Icons.polyline),
    );
  }

  Widget _layerSwitcherButton() {
    return IconButton(
      iconSize: iconSize,
      color: _polygonMenuColor(_modeLayerProperties),
      onPressed: () async {
        refreshView(() {
          _modeLayerProperties = !_modeLayerProperties;
          if (_modeLayerProperties == false) {
            _closeLayerPropertiesMenu();
          } else {
            _closePolygonDrawMenu();
            _closeProjectMenu();
          }
        });
        PopupLayerSwitcher(gl.notificationContext!, () {
          setState(() {
            _modeLayerProperties = false;
          });
        });
      },
      icon: const Icon(Icons.remove_red_eye),
    );
  }

  Widget _layerPolygonButton() {
    return IconButton(
      iconSize: iconSize,
      color: _polygonMenuColorTools(_modeProjectProperties),
      onPressed: () async {
        refreshView(() {
          _modeProjectProperties = !_modeProjectProperties;
        });
        gl.refreshMap(() {});

        PopupDrawnLayerMenu(
          gl.notificationContext!,
          gl.polygonLayers.isNotEmpty
              ? gl.polygonLayers[gl.selectedPolygonLayer].name
              : "",
          (LatLng pos) {
            if (pos.longitude != 0.0 && pos.latitude != 0.0) {
              _mapController.move(pos, _mapController.camera.zoom);
            }
          },
          () {
            refreshView(() {
              _modeProjectProperties = false;
            });
          },
        );
      },
      icon: const Icon(Icons.hexagon),
    );
  }

  Widget _searchButton() {
    return IconButton(
      iconSize: iconSize,
      color: _polygonMenuColor(_modeSearch),
      onPressed: () async {
        refreshView(() {
          _modeSearch = !_modeSearch;
        });
        gl.refreshMap(() {});
        if (_modeSearch == false) {
          _closeSearchMenu();
        } else {
          _closeLayerPropertiesMenu();
          _closePolygonDrawMenu();
          _closeProjectMenu();
        }
        PopupSearchMenu(
          gl.notificationContext!,
          "",
          (LatLng pos) {
            if (pos.longitude != 0.0 && pos.latitude != 0.0) {
              _mapController.move(pos, _mapController.camera.zoom);
            }
          },
          () {
            refreshView(() {
              _modeSearch = false;
            });
          },
        );
      },
      //icon:const Icon(Icons.search_location),
      icon: FaIcon(FontAwesomeIcons.magnifyingGlassLocation),
    );
  }

  List<Marker> _getPolygonesInfos() {
    return List.generate(gl.polygonLayers.length, (i) {
      return Marker(
        alignment: Alignment.center,
        width:
            iconSize * 3 > gl.polygonLayers[i].name.length * 11
                ? iconSize * 4
                : gl.polygonLayers[i].name.length * 11,
        height: iconSize * 1.5,
        point: gl.polygonLayers[i].center,
        child: Column(
          children: <Widget>[
            Row(
              children: [
                Icon(Icons.layers, color: gl.polygonLayers[i].colorLine),
                ConstrainedBox(
                  constraints: BoxConstraints(minWidth: 0),
                  child: Text(
                    gl.polygonLayers[i].name,
                    overflow: TextOverflow.clip,
                  ),
                ),
              ],
            ),
            Row(
              children: [
                Icon(Icons.area_chart, color: gl.polygonLayers[i].colorLine),
                Text("${(gl.polygonLayers[i].area / 100).round() / 100} Ha"),
              ],
            ),
            Row(
              children: [
                Icon(Icons.crop, color: gl.polygonLayers[i].colorLine),
                Text("${(gl.polygonLayers[i].perimeter).round() / 1000} km"),
              ],
            ),
          ],
        ),
      );
    });
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
          .getLayerBase(gl.interfaceSelectedLCode.first)
          .getValXY(ptBL72);
      setState(() {
        gl.anaPtPreview = LayerAnaPt(
          mCode: gl.interfaceSelectedLCode.first,
          mRastValue: val,
        );
      });
    }
    after();
  }
}
