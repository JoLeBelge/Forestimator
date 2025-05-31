import 'dart:async';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tileProvider/tif_tile_provider.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:flutter_map/flutter_map.dart';
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
  var data;

  static int _mapFrameCounter = 0;

  bool _toolbarExtended = false;
  bool _modeDrawPolygon = false;
  bool _modeDrawPolygonAddVertexes = false;
  bool _modeDrawPolygonRemoveVertexes = false;
  bool _modeDrawPolygonMoveVertexes = false;

  bool _modeLayerProperties = false;
  bool _modeLayerPropertiesColors = false;
  bool _modeLayerPropertiesRename = false;

  bool _modeProjectProperties = false;

  LatLng? _selectedPointToMove;
  double _iconSize = 50.0;

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
    resolutions: getResolutions2(12),
  );
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
            layersAnaPt += "+$lCode";
          }
        }

        String url =
            "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/$layersAnaPt/x/${ptBL72.x}/y/${ptBL72.y}";
        try {
          var res = await http.get(Uri.parse(url));
          if (res.statusCode != 200) throw HttpException('${res.statusCode}');
          data = jsonDecode(res.body);

          // si pas de connexion internet, va tenter de lire data comme une map alors que c'est vide, erreur. donc dans le bloc try catch aussi
          for (var r in data["RequestedLayers"]) {
            gl.requestedLayers.add(LayerAnaPt.fromMap(r));
          }
        } catch (e) {
          // handshake et/ou socketExeption
          //print('There was an error: ');
          /*FlutterLogs.logError("anaPt", "online",
              "error while waiting for forestimatorWeb answer. ${e}");*/
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

  /*
  bool _isDownloadableLayer(String key) {
    if (gl.downloadableLayerKeys.contains(key)) {
      return true;
    }
    return false;
  }
*/
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
    gl.notificationContext = context;
    return handlePermissionForLocation(
      refreshParentWidgetTree: refreshView,
      child: Scaffold(
        appBar:
            gl.offlineMode
                ? AppBar(
                  title: const Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        "Forestimator offline/terrain",
                        textScaler: TextScaler.linear(0.75),
                        style: TextStyle(color: Colors.black),
                      ),
                    ],
                  ),
                  toolbarHeight: 20.0,
                  backgroundColor: gl.colorAgroBioTech,
                )
                : AppBar(
                  title: const Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        "Forestimator online",
                        textScaler: TextScaler.linear(0.75),
                        style: TextStyle(color: Colors.white),
                      ),
                    ],
                  ),
                  toolbarHeight: 20.0,
                  backgroundColor: gl.colorUliege,
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
                onTap:
                    _modeDrawPolygonAddVertexes
                        ? (tapPosition, point) async => {
                          refreshView(() {
                            gl.polygonLayers[gl.selectedPolygonLayer].addPoint(
                              point,
                            );
                          }),
                        }
                        : _modeDrawPolygonMoveVertexes
                        ? (tapPosition, point) async => {
                          refreshView(() {
                            _stopMovingSelectedPoint();
                          }),
                        }
                        : (tapPosition, point) async => {},
                onLongPress:
                    _modeDrawPolygon
                        ? (tapPosition, point) async => {}
                        : (tapPosition, point) async => {
                          if (!_doingAnaPt)
                            {
                              refreshView(() {
                                _doingAnaPt = true;
                              }),
                              //proj4.Point ptBL72 = epsg4326.transform(epsg31370,proj4.Point(x: point.longitude, y: point.latitude))
                              await _runAnaPt(
                                epsg4326.transform(
                                  epsg31370,
                                  proj4.Point(
                                    x: point.longitude,
                                    y: point.latitude,
                                  ),
                                ),
                              ),
                              _updatePtMarker(point),
                              refreshView(() {
                                _doingAnaPt = false;
                              }),
                              GoRouter.of(
                                gl.notificationContext!,
                              ).push("/anaPt"),
                            },
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
                  _writeNewPositionToMemory(
                    position.center.longitude,
                    position.center.latitude,
                    position.zoom,
                  );
                },
                crs: epsg31370CRS,
                initialZoom: 8.0,
                maxZoom: 10,
                minZoom:
                    2, // pour les cartes offline, il faudrait informer l'utilisateur du fait que si le zoom est trop peu élevé, la carte ne s'affiche pas
                initialCenter: gl.latlonCenter,
                cameraConstraint: CameraConstraint.containCenter(
                  bounds: LatLngBounds.fromPoints([latlonBL, latlonTR]),
                ),
                onMapReady: () async {
                  updateLocation();
                  if (gl.position != null) {
                    // IMPORTANT: rebuild location layer when permissions are granted
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
                        if (_provider != null) {
                          //_provider?.dispose();
                          _provider =
                              null; // normalement le garbage collector effectue le dispose pour nous.
                        }
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
                            tileProvider: _provider,
                            // minNativeZoom: 8,
                            minZoom:
                                2, // si minZoom de la map est moins restrictif (moins élevé) que celui-ci, la carte ne s'affiche juste pas (écran blanc)
                          )
                          : Container();
                    } else if (selLayer.offline) {
                      // deuxième carte offline ; on ne fait rien avec, un seul provider
                      return Container();
                    } else {
                      LayerBase l = gl.dico.getLayerBase(selLayer.mCode);
                      return TileLayer(
                        userAgentPackageName: "com.forestimator",
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
                    //CurrentLocationLayer(),
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
                        PolygonLayer(polygons: _getPolygonesToDraw()),
                      ]) +
                  <Widget>[
                    MarkerLayer(
                      markers:
                          [
                            !_modeDrawPolygonMoveVertexes
                                ? Marker(
                                  width: 70.0,
                                  height: 70.0,
                                  point: _pt ?? const LatLng(0.0, 0.0),
                                  child: const Icon(Icons.location_on),
                                )
                                : _selectedPointToMove != null
                                ? Marker(
                                  alignment: Alignment.center,
                                  width: _iconSize,
                                  height: _iconSize,
                                  point: _mapController.camera.center,
                                  child: const Icon(
                                    Icons.donut_large,
                                    color: Colors.red,
                                  ),
                                )
                                : Marker(
                                  alignment: Alignment.center,
                                  width: _iconSize,
                                  height: _iconSize,
                                  point: _mapController.camera.center,
                                  child: const Icon(
                                    Icons.donut_large,
                                    color: Colors.blue,
                                  ),
                                ),
                          ] +
                          _getPolygonesInfos(),
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
                          iconSize: _iconSize,
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
                          iconSize: _iconSize,
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
                          iconSize: _iconSize,
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
              iconSize: _iconSize,
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
              icon: const Icon(Icons.square),
            ),
          ],
        ),
      ],
    );
  }

  Widget _toolbar() {
    return Column(
      children: [
        _layerProjectButton(),
        _modeLayerProperties ? _layerPropertiesMenu() : Container(),
        _layerPropertiesButton(),
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
        IconButton(
          iconSize: _iconSize,
          color:
              _modeDrawPolygonMoveVertexes ? gl.colorAgroBioTech : Colors.black,
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
          icon: const Icon(Icons.change_circle),
        ),
        IconButton(
          iconSize: _iconSize,
          color:
              _modeDrawPolygonRemoveVertexes
                  ? gl.colorAgroBioTech
                  : Colors.black,
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
          icon: const Icon(Icons.remove),
        ),
        IconButton(
          iconSize: _iconSize,
          color:
              _modeDrawPolygonAddVertexes ? gl.colorAgroBioTech : Colors.black,
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
          icon: const Icon(Icons.add),
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
    List<CircleMarker> all = [];
    int i = 0;
    for (var point in gl.polygonLayers[gl.selectedPolygonLayer].vertexes) {
      all.add(
        CircleMarker(
          point: point,
          radius:
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i)
                  ? _iconSize / 2.7
                  : _iconSize / 3,
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

  List<Marker> _drawnLayerPointsMarker() {
    List<Marker> all = [];
    int count = 0;
    for (var point in gl.polygonLayers[gl.selectedPolygonLayer].vertexes) {
      all.add(
        Marker(
          alignment: Alignment.center,
          width: _iconSize,
          height: _iconSize,
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
              style: TextStyle(color: Colors.black, fontSize: _iconSize / 3),
            ),
          ),
        ),
      );
      count++;
    }
    return all;
  }

  Widget _layerPropertiesMenu() {
    return Column(
      children: <Widget>[
        IconButton(
          iconSize: _iconSize,
          color:
              _modeLayerPropertiesColors ? gl.colorAgroBioTech : Colors.black,
          onPressed: () {
            refreshView(() {
              _modeLayerPropertiesColors = !_modeLayerPropertiesColors;
              if (_modeLayerPropertiesColors) {
                _modeLayerPropertiesRename = false;
              }
            });
            PopupNameIntroducer(
              gl.notificationContext!,
              gl.polygonLayers[gl.selectedPolygonLayer].name,
              (String nameIt) {
                refreshView(
                  () => gl.polygonLayers[gl.selectedPolygonLayer].name = nameIt,
                );
              },
              () {
                refreshView(() {
                  _modeLayerPropertiesColors = false;
                  if (_modeLayerPropertiesColors) {
                    _modeLayerPropertiesRename = false;
                  }
                });
              },
            );
          },
          icon: Icon(Icons.abc, size: _iconSize),
        ),
        IconButton(
          iconSize: _iconSize,
          color:
              _modeLayerPropertiesRename ? gl.colorAgroBioTech : Colors.black,
          onPressed: () {
            refreshView(() {
              _modeLayerPropertiesRename = !_modeLayerPropertiesRename;
              if (_modeLayerPropertiesRename) {
                _modeLayerPropertiesColors = false;
              }
            });
            PopupColorChooser(
              gl.polygonLayers[gl.selectedPolygonLayer].colorInside,
              gl.notificationContext!,
              (Color col) {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer].setColorInside(col);
                  gl.polygonLayers[gl.selectedPolygonLayer].setColorLine(
                    Color.fromRGBO(
                      (col.r * 255).round(),
                      (col.g * 255).round(),
                      (col.b * 255).round(),
                      0.8,
                    ),
                  );
                });
              },
              () {
                refreshView(() {
                  _modeLayerPropertiesRename = false;
                  if (_modeLayerPropertiesRename) {
                    _modeLayerPropertiesColors = false;
                  }
                });
              },
            );
          },
          icon: Icon(Icons.color_lens, size: _iconSize),
        ),
      ],
    );
  }

  void _closeProjectMenu() {
    refreshView(() {
      _modeProjectProperties = false;
    });
  }

  void _closePolygonDrawMenu() {
    refreshView(() {
      _modeDrawPolygon = false;
      _modeDrawPolygonAddVertexes = false;
      _modeDrawPolygonRemoveVertexes = false;
      _modeDrawPolygonMoveVertexes = false;
      _stopMovingSelectedPoint();
    });
  }

  void _closeLayerPropertiesMenu() {
    refreshView(() {
      _modeLayerPropertiesRename = false;
      _modeLayerPropertiesColors = false;
      _modeLayerProperties = false;
    });
  }

  Widget _drawPolygonButton() {
    return IconButton(
      iconSize: _iconSize,
      color: _modeDrawPolygon ? gl.colorAgroBioTech : Colors.black,
      onPressed: () async {
        refreshView(() {
          _modeDrawPolygon = !_modeDrawPolygon;
        });
        gl.refreshMap(() {});
        if (_modeDrawPolygon == false) {
          _closePolygonDrawMenu();
        } else {
          _closeLayerPropertiesMenu();
          _closeProjectMenu();
        }
      },
      icon: const Icon(Icons.polyline),
    );
  }

  Widget _layerPropertiesButton() {
    return IconButton(
      iconSize: _iconSize,
      color: _modeLayerProperties ? gl.colorAgroBioTech : Colors.black,
      onPressed: () async {
        refreshView(() {
          _modeLayerProperties = !_modeLayerProperties;
        });
        gl.refreshMap(() {});
        if (_modeLayerProperties == false) {
          _closeLayerPropertiesMenu();
        } else {
          _closePolygonDrawMenu();
          _closeProjectMenu();
        }
      },
      icon: const Icon(Icons.settings_display),
    );
  }

  Widget _layerProjectButton() {
    return IconButton(
      iconSize: _iconSize,
      color: _modeProjectProperties ? gl.colorAgroBioTech : Colors.black,
      onPressed: () async {
        refreshView(() {
          _modeProjectProperties = !_modeProjectProperties;
        });
        gl.refreshMap(() {});
        if (_modeProjectProperties == false) {
          _closeProjectMenu();
        } else {
          _closeLayerPropertiesMenu();
          _closePolygonDrawMenu();
        }
        PopupDrawnLayerMenu(
          gl.notificationContext!,
          gl.polygonLayers[gl.selectedPolygonLayer].name,
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
      icon: const Icon(Icons.work),
    );
  }

  List<Marker> _getPolygonesInfos() {
    return List.generate(gl.polygonLayers.length, (i) {
      return Marker(
        alignment: Alignment.center,
        width:
            _iconSize * 3 > gl.polygonLayers[i].name.length * 11
                ? _iconSize * 4
                : gl.polygonLayers[i].name.length * 11,
        height: _iconSize * 1.5,
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
                Text("${(gl.polygonLayers[i].area / 1000).round() / 100} Ha"),
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

  void _writeNewPositionToMemory(double lon, double lat, double zoom) async {
    if (_mapFrameCounter % 100 != 0) {
      return;
    }
    print("Saving references: $lon $lat $zoom");
    final SharedPreferences prefs = await SharedPreferences.getInstance();
    await prefs.setDouble('mapCenterLat', lat);
    await prefs.setDouble('mapCenterLon', lon);
    await prefs.setDouble('mapZoom', zoom);
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
      _writePolygonPointsToMemory('poly$i.poly', prefs, polygon.polygonPoints);
      i++;
    }
    await prefs.setInt('nPolys', i);
  }

  void _updatePtMarker(LatLng pt) {
    refreshView(() {
      _pt = pt;
    });
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
        Duration(seconds: 3),
      );

      refreshView(() {
        gl.position = newPosition;
      });
      _refreshLocation = false;
    } catch (e) {
      // We keep the old position.
      refreshView(() {
        gl.position = gl.position;
      });
      _refreshLocation = false;
      //FlutterLogs.logError("gps", "position", "error while waiting on position. ${e.toString()}");
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
      return null;
    }
  } else {
    return null;
  }
}
