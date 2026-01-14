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
import 'package:fforestimator/tools/geometry/polygon_utils.dart' as poly;
import 'dart:io';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
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
  bool _mapControllerInit = false;

  bool _toolbarExtended = false;

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
        gl.mainStack.add(popupNoInternet());
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
    gl.refreshMainStack = refreshView;
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
      child: handlePermissionForStorage(
        refreshParentWidgetTree: refreshView,
        child: Scaffold(
          resizeToAvoidBottomInset: true,
          extendBody: true,
          appBar: AppBar(
            toolbarHeight: gl.display.equipixel * gl.topAppInfoBarThickness,
            backgroundColor:
                gl.offlineMode ? gl.colorAgroBioTech : gl.colorUliege,
            title: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                IconButton(
                  iconSize: gl.display.equipixel * gl.iconSizeSettings,
                  color: gl.offlineMode ? Colors.black : Colors.white,
                  onPressed: () {
                    if (!gl.modeSettings) {
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
                    } else {
                      gl.mainStackPopLast();
                      gl.modeSettings = false;
                    }
                    gl.refreshMainStack(() {});
                  },
                  icon: Icon(Icons.settings),
                ),
                SizedBox(width: 1),
                Container(
                  constraints: BoxConstraints(
                    maxWidth:
                        gl.display.equipixel * gl.topAppForestimatorFontWidth,
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
                      gl.rebuildSwitcherCatalogueButtons(() {});
                      gl.rebuildOfflineCatalogue(() {});
                      gl.rebuildSwitcherBox(() {});
                      gl.rebuildLayerSwitcher(() {});
                    },
                    child:
                        gl.offlineMode
                            ? Text(
                              "Forestimator terrain",
                              style: TextStyle(
                                color: Colors.black,
                                fontSize:
                                    gl.display.equipixel *
                                    gl.topAppForestimatorFontHeight,
                              ),
                            )
                            : Text(
                              "Forestimator online",
                              style: TextStyle(
                                color: Colors.white,
                                fontSize:
                                    gl.display.equipixel *
                                    gl.topAppForestimatorFontHeight,
                              ),
                            ),
                  ),
                ),
                _mapControllerInit
                    ? _mapController.camera.zoom.round() <
                                gl.globalMinOfflineZoom.round() &&
                            gl.getIndexForNextLayerOffline() > -1
                        ? IconButton(
                          iconSize: gl.display.equipixel * gl.iconSizeSettings,
                          color: Colors.red,
                          tooltip:
                              "Si vous n'arrivez plus à visualiser les cartes hors ligne c'est que votre zoom est trop large.",
                          onPressed: () {},
                          icon: Icon(Icons.info_rounded),
                        )
                        : SizedBox(
                          width:
                              gl.display.equipixel * gl.iconSizeSettings * 1.5,
                        )
                    : SizedBox(
                      width: gl.display.equipixel * gl.iconSizeSettings * 1.5,
                    ),
              ],
            ),
          ),
          body: OrientationBuilder(
            builder: (context, orientation) {
              return Stack(
                children:
                    [
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
                                  proj4.Point(
                                    x: point.longitude,
                                    y: point.latitude,
                                  ),
                                ),
                              );
                              _pt = point;
                              refreshView(() {
                                _doingAnaPt = false;
                              });
                              PopupAnaResultsMenu(
                                gl.notificationContext!,
                                gl.requestedLayers,
                                () {
                                  refreshView(() {});
                                },
                              );
                              gl.refreshMainStack(() {});
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
                                  : gl.Mode.addVertexesPolygon
                                  ? (tapPosition, point) async => {
                                    if (gl.polygonLayers.isNotEmpty)
                                      {
                                        if (_isPolygonWellDefined(
                                          gl
                                              .polygonLayers[gl
                                                  .selectedPolygonLayer]
                                              .getPolyPlusOneVertex(point),
                                        ))
                                          {
                                            refreshView(() {
                                              gl
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .addPoint(point);
                                            }),
                                          }
                                        else
                                          {
                                            PopupPolygonNotWellDefined(
                                              gl.notificationContext!,
                                            ),
                                          },
                                      },
                                  }
                                  : gl.Mode.moveVertexesPolygon
                                  ? (tapPosition, point) async => {
                                    refreshView(() {
                                      _stopMovingSelectedPoint();
                                      gl.Mode.showButtonAddVertexesPolygon =
                                          false;
                                      gl.Mode.showButtonMoveVertexesPolygon =
                                          true;
                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                          true;
                                      gl.Mode.addVertexesPolygon = false;
                                      gl.Mode.moveVertexesPolygon = false;
                                      gl.Mode.removeVertexesPolygon = false;
                                    }),
                                  }
                                  : gl.Mode.editPolygon && gl.Mode.editPolygon
                                  ? (tapPosition, point) async => {
                                    refreshView(() {
                                      gl.Mode.showButtonAddVertexesPolygon =
                                          true;
                                      gl.Mode.showButtonMoveVertexesPolygon =
                                          false;
                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                          false;
                                      gl.Mode.addVertexesPolygon = false;
                                      gl.Mode.moveVertexesPolygon = false;
                                      gl.Mode.removeVertexesPolygon = false;
                                    }),
                                  }
                                  : gl.Mode.editPolygon && !gl.Mode.editPolygon
                                  ? (tapPosition, point) async => {
                                    refreshView(() {
                                      gl.Mode.showButtonAddVertexesPolygon =
                                          false;
                                      gl.Mode.showButtonMoveVertexesPolygon =
                                          false;
                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                          false;
                                      gl.Mode.addVertexesPolygon = false;
                                      gl.Mode.moveVertexesPolygon = false;
                                      gl.Mode.removeVertexesPolygon = false;
                                    }),
                                  }
                                  : (tapPosition, point) async => {
                                    _lastPressWasShort = true,
                                    _updatePtMarker(point),
                                  },
                          onPositionChanged: (position, e) async {
                            if (!e) return;
                            _mapControllerInit = true;
                            updateLocation();
                            if (gl.selectedpathLayer > -1 &&
                                !gl.pathLayers[gl.selectedpathLayer].finished) {
                              gl.pathLayers[gl.selectedpathLayer].addPosition(
                                LatLng(
                                  position.center.latitude,
                                  position.center.longitude,
                                ),
                              );
                            }
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
                              if (_isPolygonWellDefined(
                                gl.polygonLayers[gl.selectedPolygonLayer]
                                    .getPolyMoveOneVertex(
                                      _selectedPointToMove!,
                                      LatLng(
                                        position.center.latitude,
                                        position.center.longitude,
                                      ),
                                    ),
                              )) {
                                gl.polygonLayers[gl.selectedPolygonLayer]
                                    .replacePoint(
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
                              }
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
                          initialZoom:
                              (gl.globalMinZoom + gl.globalMaxZoom) / 2.0,
                          maxZoom: gl.globalMaxZoom,
                          minZoom: gl.globalMinZoom,
                          initialCenter: gl.latlonCenter,
                          cameraConstraint: CameraConstraint.containCenter(
                            bounds: LatLngBounds.fromPoints([
                              latlonBL,
                              latlonTR,
                            ]),
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
                                  gl.dico
                                      .getLayerBase(selLayer.mCode)
                                      .mOffline &&
                                  (selLayer.mCode ==
                                      gl.getFirstSelLayOffline())) {
                                if (_provider == null ||
                                    _provider?.layerCode != selLayer.mCode) {
                                  _provider = TifFileTileProvider(
                                    refreshView: refreshView,
                                    mycrs: epsg31370CRS,
                                    sourceImPath: gl.dico.getRastPath(
                                      selLayer.mCode,
                                    ),
                                    layerCode: selLayer.mCode,
                                  );
                                  _provider!.init();
                                }
                                return _provider!.loaded
                                    ? TileLayer(
                                      tileDisplay:
                                          gl.modeMapFirstTileLayerTransparancy &&
                                                  i == 2
                                              ? TileDisplay.instantaneous(
                                                opacity: 0.5,
                                              )
                                              : TileDisplay.instantaneous(
                                                opacity: 1.0,
                                              ),
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
                                                gl.display.equipixel *
                                                gl.loadingMapBoxHeight,
                                            minWidth:
                                                gl.display.equipixel *
                                                gl.loadingMapBoxWidth,
                                            maxWidth:
                                                gl.display.equipixel *
                                                gl.loadingMapBoxWidth,
                                          ),
                                          child: Row(
                                            mainAxisAlignment:
                                                MainAxisAlignment.center,
                                            children: [
                                              SizedBox(
                                                width:
                                                    gl.display.equipixel *
                                                    gl.loadingMapBoxWidth *
                                                    .7,
                                                child: Text(
                                                  "La carte choisie est en préparation dans la mémoire.",
                                                  textAlign: TextAlign.center,
                                                  style: TextStyle(
                                                    color: Colors.white,
                                                    fontSize:
                                                        gl.display.equipixel *
                                                        gl.fontSizeS,
                                                  ),
                                                ),
                                              ),
                                              SizedBox(
                                                width:
                                                    gl.display.equipixel *
                                                    gl.fontSizeS,
                                              ),
                                              CircularProgressIndicator(
                                                constraints: BoxConstraints(
                                                  minHeight:
                                                      gl.display.equipixel *
                                                      gl.fontSizeXL,
                                                  minWidth:
                                                      gl.display.equipixel *
                                                      gl.fontSizeXL,
                                                ),
                                                color: gl.colorAgroBioTech,
                                                strokeWidth:
                                                    gl.display.equipixel *
                                                    gl.fontSizeS *
                                                    .5,
                                              ),
                                            ],
                                          ),
                                        ),
                                      ),
                                    );
                              } else {
                                LayerBase l = gl.dico.getLayerBase(
                                  selLayer.mCode,
                                );
                                i++;
                                return TileLayer(
                                  userAgentPackageName: "com.forestimator",
                                  tileDisplay:
                                      gl.modeMapFirstTileLayerTransparancy &&
                                              i > 1 &&
                                              gl
                                                      .getLayersForFlutterMap()
                                                      .length ==
                                                  i
                                          ? TileDisplay.instantaneous(
                                            opacity: 0.5,
                                          )
                                          : TileDisplay.instantaneous(
                                            opacity: 1.0,
                                          ),
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
                            (gl.Mode.editPolygon
                                ? <Widget>[
                                  if (gl.polygonLayers.isNotEmpty &&
                                      gl
                                              .polygonLayers[gl
                                                  .selectedPolygonLayer]
                                              .vertexes
                                              .length >
                                          1 &&
                                      !gl.Mode.showButtonMoveVertexesPolygon &&
                                      gl.Mode.addVertexesPolygon)
                                    PolylineLayer(
                                      polylines: [
                                        Polyline(
                                          points: [
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .vertexes[gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .selectedPolyLinePoints[0]],
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .vertexes[gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .selectedPolyLinePoints[1]],
                                          ],
                                          color:
                                              gl
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .colorLine,
                                          strokeWidth: 5.0,
                                        ),
                                      ],
                                    ),

                                  CircleLayer(
                                    circles: _drawnLayerPointsCircleMarker(),
                                  ),
                                  PolygonLayer(polygons: _getPolygonesToDraw()),
                                  MarkerLayer(
                                    markers: _drawnLayerPointsMarker(),
                                  ),
                                  if (gl.polygonLayers.isNotEmpty &&
                                      gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .vertexes
                                          .isNotEmpty &&
                                      !gl.Mode.showButtonAddVertexesPolygon)
                                    CircleLayer(
                                      circles: [
                                        CircleMarker(
                                          point:
                                              gl
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .vertexes[gl
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .selectedPolyLinePoints[0]],
                                          radius: 15,
                                          color: Colors.red,
                                        ),
                                      ],
                                    ),
                                ]
                                : <Widget>[
                                  if (gl.modeMapShowPolygons)
                                    PolygonLayer(
                                      polygons: _getPolygonesToDraw(),
                                    ),
                                ]) +
                            <Widget>[
                              MarkerLayer(
                                markers:
                                    _placeVertexMovePointer() +
                                    _placeAnaPtMarker(),
                              ),
                              if (gl.modeMapShowPolygons)
                                MarkerLayer(markers: _getPolygonesInfos()),
                              if (gl.modeMapShowCustomMarker)
                                MarkerLayer(markers: []),
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
                      //TODO: polygon moves always if you close the polygonmenu by the menubar
                      (gl.Mode.polygon &&
                              gl.polygonLayers.isNotEmpty &&
                              gl.selectedPolygonLayer > -1)
                          ? Column(
                            mainAxisAlignment: MainAxisAlignment.start,
                            children: [
                              Row(
                                mainAxisAlignment:
                                    gl.display.orientation ==
                                            Orientation.portrait
                                        ? MainAxisAlignment.center
                                        : MainAxisAlignment.start,
                                children: [
                                  Container(
                                    width:
                                        gl.display.equipixel *
                                        gl.chosenPolyBarWidth,
                                    constraints: BoxConstraints(
                                      minHeight:
                                          gl.display.equipixel *
                                          gl.chosenPolyBarHeight,
                                      maxHeight:
                                          gl.display.equipixel *
                                          gl.chosenPolyBarHeight,
                                    ),
                                    child: TextButton(
                                      onPressed: () {
                                        gl.mainStack.add(
                                          popupPolygonListMenu(
                                            gl.notificationContext!,
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .name,
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
                                                if (gl
                                                        .polygonLayers
                                                        .isNotEmpty &&
                                                    gl.Mode.editPolygon) {
                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                      true;
                                                  gl.Mode.showButtonMoveVertexesPolygon =
                                                      false;
                                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                                      false;
                                                }
                                              });
                                            },
                                          ),
                                        );
                                        refreshView(() {});
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
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .center,
                                              _mapController.camera.zoom,
                                            );
                                          }
                                        });
                                        gl.refreshMainStack(() {
                                          gl.modeMapShowPolygons = true;
                                        });
                                      },
                                      child: Card(
                                        surfaceTintColor: Colors.transparent,
                                        shadowColor: Colors.transparent,
                                        color: gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .colorInside
                                            .withAlpha(255),
                                        child: Row(
                                          mainAxisAlignment:
                                              MainAxisAlignment.spaceEvenly,
                                          children: [
                                            IconButton(
                                              onPressed: () {
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
                                                          .polygonLayers[gl
                                                              .selectedPolygonLayer]
                                                          .center,
                                                      _mapController
                                                          .camera
                                                          .zoom,
                                                    );
                                                  }
                                                });
                                                gl.refreshMainStack(() {
                                                  gl.modeMapShowPolygons = true;
                                                });
                                              },
                                              icon: Icon(
                                                gl
                                                            .polygonLayers[gl
                                                                .selectedPolygonLayer]
                                                            .polygonPoints
                                                            .length >
                                                        2
                                                    ? Icons.center_focus_strong
                                                    : Icons
                                                        .center_focus_strong_outlined,
                                                size:
                                                    gl.display.equipixel *
                                                    gl.iconSizeM,
                                                color: getColorTextFromBackground(
                                                  gl
                                                      .polygonLayers[gl
                                                          .selectedPolygonLayer]
                                                      .colorInside
                                                      .withAlpha(255),
                                                ),
                                              ),
                                            ),
                                            IconButton(
                                              onPressed: () {
                                                gl.refreshMainStack(() {
                                                  gl.modeMapShowPolygons = true;
                                                  gl.Mode.editPolygon =
                                                      !gl.Mode.editPolygon;
                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                      gl.Mode.editPolygon;
                                                });
                                              },
                                              icon: Icon(
                                                gl.Mode.editPolygon
                                                    ? Icons.lock_open
                                                    : Icons.lock_outline,
                                                size:
                                                    gl.display.equipixel *
                                                    gl.iconSizeM,
                                                color: getColorTextFromBackground(
                                                  gl
                                                      .polygonLayers[gl
                                                          .selectedPolygonLayer]
                                                      .colorInside
                                                      .withAlpha(255),
                                                ),
                                              ),
                                            ),

                                            SizedBox(
                                              width:
                                                  gl.display.equipixel *
                                                  gl.chosenPolyBarWidth *
                                                  .3,
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
                                                      gl.display.equipixel *
                                                      gl.fontSizeM *
                                                      .75,
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
                                                    gl.display.equipixel *
                                                    gl.fontSizeM *
                                                    .9,
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
                          : gl.Mode.editPolygon
                          ? Column(
                            mainAxisAlignment: MainAxisAlignment.start,
                            children: [
                              Row(
                                mainAxisAlignment:
                                    gl.display.orientation ==
                                            Orientation.portrait
                                        ? MainAxisAlignment.center
                                        : MainAxisAlignment.start,
                                children: [
                                  SizedBox(
                                    height:
                                        gl.display.equipixel *
                                        gl.chosenPolyBarHeight,
                                    width:
                                        gl.display.equipixel *
                                        gl.chosenPolyBarWidth,
                                    child: TextButton(
                                      onPressed: () {
                                        gl.mainStack.add(
                                          popupPolygonListMenu(
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
                                                if (gl
                                                    .polygonLayers
                                                    .isNotEmpty) {
                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                      true;
                                                  gl.Mode.showButtonMoveVertexesPolygon =
                                                      false;
                                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                                      false;
                                                }
                                              });
                                            },
                                          ),
                                        );
                                        refreshView(() {});
                                      },

                                      child: Card(
                                        surfaceTintColor: Colors.transparent,
                                        shadowColor: Colors.transparent,
                                        color: gl.backgroundTransparentBlackBox,
                                        child: Row(
                                          mainAxisAlignment:
                                              MainAxisAlignment.spaceEvenly,
                                          children: [
                                            Icon(
                                              Icons.add,
                                              size:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM,
                                              color: Colors.white,
                                            ),
                                            SizedBox(
                                              width:
                                                  gl.display.equipixel *
                                                  gl.fontSizeS,
                                            ),
                                            SizedBox(
                                              width:
                                                  gl.display.equipixel *
                                                  gl.chosenPolyBarWidth *
                                                  .7,
                                              child: Text(
                                                "Tappez ici pour ajouter un Polygone",
                                                style: TextStyle(
                                                  color: Colors.white,
                                                  fontSize:
                                                      gl.display.equipixel *
                                                      gl.fontSizeM,
                                                ),
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
                          : Column(),

                      _mainMenuBar(),
                      if (_toolbarExtended) _toolBar(),
                      if (gl.Mode.editPolygon && gl.polygonLayers.isNotEmpty)
                        _polygonToolbar(),
                    ] +
                    gl.mainStack,
              );
            },
          ),
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
                      !gl.Mode.showButtonMoveVertexesPolygon &&
                      gl.Mode.addVertexesPolygon
                  ? iconSize / 2.7
                  : iconSize / 3,
          color:
              gl.polygonLayers[gl.selectedPolygonLayer].isSelectedLine(i) &&
                      !gl.Mode.showButtonMoveVertexesPolygon &&
                      gl.Mode.addVertexesPolygon
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
                  ? gl.display.equipixel * gl.anaPtBoxSize
                  : gl.dico
                          .getLayerBase(gl.anaPtPreview!.mCode)
                          .getValLabel(gl.anaPtPreview!.mRastValue)
                          .length >
                      3
                  ? gl.display.equipixel * gl.anaPtBoxSize * 1.5 +
                      gl.dico
                              .getLayerBase(gl.anaPtPreview!.mCode)
                              .getValLabel(gl.anaPtPreview!.mRastValue)
                              .length *
                          8.0
                  : gl.dico.getLayerBase(gl.anaPtPreview!.mCode).mCategorie !=
                      "Externe"
                  ? gl.display.equipixel * gl.anaPtBoxSize * 2.5
                  : 0.0,
          height: gl.display.equipixel * gl.anaPtBoxSize,
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
                PopupAnaResultsMenu(
                  gl.notificationContext!,
                  gl.requestedLayers,
                  () {
                    refreshView(() {});
                  },
                );
                refreshView(() {
                  _doingAnaPt = false;
                });
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
    if ((gl.Mode.moveVertexesPolygon && _selectedPointToMove != null) ||
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
    } else if (gl.Mode.moveVertexesPolygon || _modeMeasurePath) {
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
          width: gl.display.equipixel * gl.anaPtBoxSize * 2,
          height: gl.display.equipixel * gl.anaPtBoxSize,
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

  // Geometry helpers are in polygon_utils.dart

  bool _isPolygonWellDefined(List<Point> polygones) {
    if (polygones.isEmpty || gl.Mode.overrideWellDefinedCheck) return true;
    return poly.isPolygonWellDefined(polygones);
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
              if (gl.Mode.addVertexesPolygon) {
                //select line between points to place next point
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer]
                      .refreshSelectedLinePoints(point);
                });
              } else if (gl.Mode.moveVertexesPolygon) {
                refreshView(() {
                  if (gl
                          .polygonLayers[gl.selectedPolygonLayer]
                          .selectedVertex !=
                      1 + count) {
                    gl.polygonLayers[gl.selectedPolygonLayer].selectedVertex =
                        count;
                  } else {
                    gl.polygonLayers[gl.selectedPolygonLayer].selectedVertex =
                        -1;
                  }
                });
              } else {
                refreshView(() {
                  gl.polygonLayers[gl.selectedPolygonLayer]
                      .refreshSelectedLinePoints(point);
                  gl.Mode.showButtonAddVertexesPolygon = false;
                  gl.Mode.showButtonMoveVertexesPolygon = true;
                  gl.Mode.showButtonRemoveVertexesPolygon = true;
                  gl.Mode.addVertexesPolygon = false;
                  gl.Mode.moveVertexesPolygon = false;
                  gl.Mode.removeVertexesPolygon = false;
                });
              }
              /*if (gl.Mode.removeVertexesPolygon) {
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
    gl.Mode.polygon = false;
    gl.Mode.editPolygon = false;
    _modeSearch = false;
    gl.Mode.removeVertexesPolygon = false;
    gl.Mode.moveVertexesPolygon = false;
    _modeAnaPtPreview = true;
    gl.Mode.showButtonAddVertexesPolygon = false;
    gl.Mode.showButtonMoveVertexesPolygon = false;
    gl.Mode.showButtonRemoveVertexesPolygon = false;
  }

  void _closeToolbarMenu() {
    _toolbarExtended = false;
    _modeSearch = false;
    _modeMeasurePath = false;
  }

  void _closeSwitchesMenu() {
    _modeLayerSwitches = false;
  }

  Widget _mainMenuBar({bool dummy = false, VoidCallback? close}) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment:
              gl.display.orientation == Orientation.portrait
                  ? MainAxisAlignment.center
                  : MainAxisAlignment.end,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(
                maxHeight: gl.display.equipixel * gl.menuBarThickness,
                minHeight: gl.display.equipixel * gl.menuBarThickness,
                maxWidth: gl.display.equipixel * gl.menuBarLength,
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
                        iconSize: gl.display.equipixel * gl.iconSizeM,
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
                              : !gl.Mode.polygon
                              ? Colors.transparent
                              : Colors.yellow.withAlpha(128),
                      child: IconButton(
                        color:
                            dummy
                                ? Colors.transparent
                                : gl.Mode.polygon
                                ? Colors.white
                                : Colors.yellow,
                        iconSize: gl.display.equipixel * gl.iconSizeM,
                        isSelected: gl.Mode.polygon,
                        onPressed: () {
                          setState(() {
                            gl.Mode.polygon = !gl.Mode.polygon;
                            if (gl.Mode.polygon) {
                              if (gl.polygonLayers.isNotEmpty &&
                                  gl.Mode.polygon &&
                                  gl.Mode.editPolygon &&
                                  gl.selectedPolygonLayer > -1) {
                                gl.Mode.showButtonAddVertexesPolygon = true;
                              }
                              _closeSwitchesMenu();
                              _closeToolbarMenu();
                              gl.mainStack.add(
                                popupPolygonListMenu(
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
                                      if (gl.polygonLayers.isNotEmpty &&
                                          gl.Mode.editPolygon) {
                                        gl.Mode.showButtonAddVertexesPolygon =
                                            true;
                                        gl.Mode.showButtonMoveVertexesPolygon =
                                            false;
                                        gl.Mode.showButtonRemoveVertexesPolygon =
                                            false;
                                      }
                                    });
                                  },
                                ),
                              );
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
                          width: gl.display.equipixel * gl.menuBarThickness,
                          height: gl.display.equipixel * gl.menuBarThickness,
                          child: IconButton(
                            color: Colors.transparent,
                            iconSize: gl.display.equipixel * gl.iconSizeM,
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
                            iconSize: gl.display.equipixel * gl.iconSizeM,
                            isSelected: _modeLayerSwitches,
                            onPressed: () {
                              setState(() {
                                _modeLayerSwitches = true;
                                _closePolygonMenu();
                                _closeToolbarMenu();
                                gl.mainStack.add(
                                  popupLayerSwitcher(
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
                                  ),
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

  bool positionMarkerInsideViewRectangle() =>
      gl.position != null
          ? _mapController.camera.visibleBounds.contains(
            LatLng(gl.position!.latitude, gl.position!.longitude),
          )
          : false;

  Widget _toolBar() {
    double toolbarHeight = gl.iconSizeM * 2 + gl.iconSpaceBetween * 2;
    if (gl.modeDevelopper) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    if (positionMarkerInsideViewRectangle()) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(
                maxHeight: gl.display.equipixel * toolbarHeight,
                maxWidth: gl.display.equipixel * gl.menuBarThickness,
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
                              if (positionMarkerInsideViewRectangle())
                                IconButton(
                                  iconSize: gl.display.equipixel * gl.iconSizeM,
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
                                      PopupAnaResultsMenu(
                                        gl.notificationContext!,
                                        gl.requestedLayers,
                                        () {
                                          refreshView(() {});
                                        },
                                      );
                                      refreshView(() {
                                        _doingAnaPt = false;
                                      });
                                    }
                                  },
                                  icon: const Icon(Icons.analytics),
                                ),
                              IconButton(
                                iconSize: gl.display.equipixel * gl.iconSizeM,
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
                              iconSize: gl.display.equipixel * gl.iconSizeM,
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
                    if (gl.Mode.expertTools && gl.modeDevelopper)
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
                          iconSize: gl.display.equipixel * gl.iconSizeM,
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
                    if (gl.Mode.expertTools && gl.modeDevelopper)
                      Container(
                        color:
                            !gl.Mode.recordPath
                                ? Colors.transparent
                                : Colors.yellow.withAlpha(128),
                        child: IconButton(
                          color:
                              gl.Mode.recordPath
                                  ? Colors.white
                                  : Colors.yellow.withAlpha(128),
                          iconSize: gl.display.equipixel * gl.iconSizeM,
                          isSelected: gl.Mode.recordPath,
                          onPressed: () {
                            setState(() {
                              gl.Mode.recordPath = !gl.Mode.recordPath;
                              _modeAnaPtPreview = !gl.Mode.recordPath;
                              if (gl.Mode.recordPath) {
                                gl.mainStack.add(
                                  popupPathListMenu(
                                    gl.notificationContext!,
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .name,
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
                                        gl.Mode.recordPath =
                                            !gl.Mode.recordPath;
                                      });
                                    },
                                  ),
                                );
                                refreshView(() {});
                              }
                            });
                          },
                          icon: Icon(Icons.nordic_walking),
                        ),
                      ),
                    Container(
                      color:
                          !_modeSearch
                              ? Colors.transparent
                              : Colors.blueGrey.withAlpha(128),
                      child: IconButton(
                        iconSize: gl.display.equipixel * gl.iconSizeM,
                        color: _modeSearch ? Colors.white : Colors.blueGrey,
                        onPressed: () {
                          setState(() {
                            _modeSearch = true;
                          });
                          gl.mainStack.add(
                            popupSearchMenu(
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
                                  _modeSearch = false;
                                });
                              },
                            ),
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
        if (gl.display.orientation == Orientation.portrait)
          _placeholder(
            constraints: BoxConstraints(
              maxHeight: gl.display.equipixel * gl.menuBarThickness,
              minHeight: gl.display.equipixel * gl.menuBarThickness,
              maxWidth: gl.display.equipixel * gl.menuBarThickness * .5,
            ),
          ),
      ],
    );
  }

  Widget _polygonToolbar() {
    double toolbarHeight = gl.iconSpaceBetween;
    if (gl.Mode.showButtonMoveVertexesPolygon) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    if (gl.Mode.showButtonRemoveVertexesPolygon) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    if (gl.Mode.showButtonAddVertexesPolygon) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    return Column(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Container(
              alignment: Alignment.center,
              height: gl.display.equipixel * toolbarHeight,
              constraints: BoxConstraints(
                minWidth: gl.display.equipixel * gl.menuBarThickness,
                maxWidth: gl.display.equipixel * gl.menuBarThickness,
              ),
              child: Card(
                color: gl.backgroundTransparentBlackBox,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    if (gl.Mode.showButtonMoveVertexesPolygon)
                      Container(
                        color: _polygonMenuColorTools(
                          gl.Mode.moveVertexesPolygon,
                        ),
                        child: IconButton(
                          color:
                              gl.Mode.moveVertexesPolygon
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          iconSize: gl.display.equipixel * gl.iconSizeM,
                          onPressed: () async {
                            refreshView(() {
                              gl.Mode.moveVertexesPolygon =
                                  !gl.Mode.moveVertexesPolygon;
                            });
                            if (gl.Mode.moveVertexesPolygon == true) {
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
                              gl.Mode.addVertexesPolygon = false;
                              gl.Mode.removeVertexesPolygon = false;
                            } else {
                              refreshView(() {
                                _stopMovingSelectedPoint();
                              });
                            }
                          },
                          icon: const Icon(Icons.open_with_rounded),
                        ),
                      ),
                    if (gl.Mode.showButtonRemoveVertexesPolygon)
                      Container(
                        color: _polygonMenuColorTools(
                          gl.Mode.removeVertexesPolygon,
                        ),
                        child: IconButton(
                          iconSize: gl.display.equipixel * gl.iconSizeM,
                          color:
                              gl.Mode.removeVertexesPolygon
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          onPressed: () async {
                            refreshView(() {
                              gl.Mode.removeVertexesPolygon =
                                  !gl.Mode.removeVertexesPolygon;
                            });
                            if (gl.Mode.removeVertexesPolygon == true) {
                              refreshView(() {
                                if (_isPolygonWellDefined(
                                  gl.polygonLayers[gl.selectedPolygonLayer]
                                      .getPolyRemoveOneVertex(
                                        gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .polygonPoints[gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .selectedPolyLinePoints[0]],
                                      ),
                                )) {
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
                                }
                              });
                              gl.Mode.moveVertexesPolygon = false;
                              gl.Mode.addVertexesPolygon = false;
                              _stopMovingSelectedPoint();
                              refreshView(() {
                                if (gl
                                    .polygonLayers[gl.selectedPolygonLayer]
                                    .polygonPoints
                                    .isEmpty) {
                                  gl.Mode.showButtonAddVertexesPolygon = true;
                                  gl.Mode.showButtonMoveVertexesPolygon = false;
                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                      false;
                                  gl.Mode.addVertexesPolygon = false;
                                  gl.Mode.moveVertexesPolygon = false;
                                  gl.Mode.removeVertexesPolygon = false;
                                } else {
                                  gl.Mode.showButtonAddVertexesPolygon = false;
                                  gl.Mode.showButtonMoveVertexesPolygon = true;
                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                      true;
                                  gl.Mode.addVertexesPolygon = false;
                                  gl.Mode.moveVertexesPolygon = false;
                                  gl.Mode.removeVertexesPolygon = false;
                                }
                              });
                            }
                          },
                          icon: const Icon(Icons.remove_circle),
                        ),
                      ),
                    if (gl.Mode.showButtonAddVertexesPolygon)
                      Container(
                        color: _polygonMenuColorTools(
                          gl.Mode.addVertexesPolygon,
                        ),
                        child: IconButton(
                          iconSize: gl.display.equipixel * gl.iconSizeM,
                          color:
                              gl.Mode.addVertexesPolygon
                                  ? Colors.white
                                  : Colors.lightGreenAccent,
                          onPressed: () async {
                            refreshView(() {
                              gl.Mode.addVertexesPolygon =
                                  !gl.Mode.addVertexesPolygon;
                            });
                            if (gl.Mode.addVertexesPolygon == true) {
                              gl.Mode.removeVertexesPolygon = false;
                              gl.Mode.moveVertexesPolygon = false;
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
        if (gl.display.orientation == Orientation.portrait)
          _placeholder(
            constraints: BoxConstraints(
              maxHeight: gl.display.equipixel * gl.menuBarThickness,
              minHeight: gl.display.equipixel * gl.menuBarThickness,
              maxWidth: gl.display.equipixel * gl.menuBarThickness * .5,
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
                ? gl.display.equipixel * gl.infoBoxPolygon * 2.5 +
                    textArea.length * gl.fontSizeS
                : gl.display.equipixel * gl.infoBoxPolygon * 1.5 +
                    gl.polygonLayers[i].name.length * gl.fontSizeS,
        height: gl.display.equipixel * gl.infoBoxPolygon * 1.5,
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

  void _writeColorToMemory(String name, Color color) async {
    await gl.shared!.setInt('$name.r', (color.r * 255).round());
    await gl.shared!.setInt('$name.g', (color.g * 255).round());
    await gl.shared!.setInt('$name.b', (color.b * 255).round());
    await gl.shared!.setDouble('$name.a', color.a);
  }

  void _writePolygonPointsToMemory(String name, List<LatLng> polygon) async {
    int i = 0;
    for (var point in polygon) {
      await gl.shared!.setDouble('$name.$i-lat', point.latitude);
      await gl.shared!.setDouble('$name.$i-lng', point.longitude);
      i++;
    }
    await gl.shared!.setInt('$name.nPolyPoints', i);
  }

  void _writePositionDataToSharedPreferences(
    double lon,
    double lat,
    double zoom,
  ) async {
    if (_mapFrameCounter % 100 == 0) {
      await gl.shared!.setDouble('mapCenterLat', lat);
      await gl.shared!.setDouble('mapCenterLon', lon);
      await gl.shared!.setDouble('mapZoom', zoom);
      gl.print("position saved to prefs: $lon $lat $zoom");
    }
  }

  void _writeDataToSharedPreferences() async {
    if (gl.saveChangesToPolygoneToPrefs) {
      gl.saveChangesToPolygoneToPrefs = false;
      int i = 0;
      for (var polygon in gl.polygonLayers) {
        await gl.shared!.setString('poly$i.name', polygon.name);
        await gl.shared!.setDouble('poly$i.area', polygon.area);
        await gl.shared!.setDouble('poly$i.perimeter', polygon.perimeter);
        await gl.shared!.setDouble(
          'poly$i.transparencyInside',
          polygon.transparencyInside,
        );
        await gl.shared!.setDouble(
          'poly$i.transparencyLine',
          polygon.transparencyLine,
        );
        _writeColorToMemory('poly$i.colorInside', polygon.colorInside);
        _writeColorToMemory('poly$i.colorLine', polygon.colorLine);
        _writePolygonPointsToMemory('poly$i.poly', polygon.polygonPoints);
        i++;
      }
      await gl.shared!.setInt('nPolys', i);
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
  final VoidCallback after;
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
      return Container(
        alignment: Alignment.centerLeft,
        height: gl.display.equipixel * gl.fontSizeM * 1.1,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(
                minHeight: gl.fontSizeM * gl.display.equipixel,
                minWidth: gl.fontSizeM * gl.display.equipixel,
              ),
              color: Colors.white,
              child: Container(
                alignment: Alignment.center,
                height: (gl.fontSizeM - .5) * gl.display.equipixel,
                width: (gl.fontSizeM - .5) * gl.display.equipixel,
                color: color == Colors.transparent ? Colors.white : color,
              ),
            ),
            Container(
              constraints: BoxConstraints(
                minHeight: gl.fontSizeM * gl.display.equipixel,
                minWidth: gl.fontSizeM * gl.display.equipixel,
              ),
              child: Text(":", textAlign: TextAlign.center),
            ),
            Text(
              textAlign: TextAlign.center,
              text == "" ? "No data" : text,
              style: TextStyle(fontSize: gl.fontSizeS * gl.display.equipixel),
            ),
          ],
        ),
      );
    }
    return CircularProgressIndicator(
      color: gl.colorAgroBioTech,
      constraints: BoxConstraints(
        maxHeight: gl.display.equipixel * gl.anaPtBoxSize * .9,
        minHeight: gl.display.equipixel * gl.anaPtBoxSize * .8,
        maxWidth: gl.display.equipixel * gl.anaPtBoxSize * .9,
        minWidth: gl.display.equipixel * gl.anaPtBoxSize * .8,
      ),
    );
  }

  Future _runAnaPtPreview(proj4.Point ptBL72, VoidCallback after) async {
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
        gl.mainStack.add(popupNoInternet());
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
