import 'dart:async';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tileProvider/tif_tile_provider.dart';
import 'package:fforestimator/tools/geometry/geometry.dart' as pl;
import 'package:fforestimator/tools/layout_tools.dart';
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
  final LayerHitNotifier<String> hitNotifier = ValueNotifier(null);
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
  ScrollController propertiesTableScrollController = ScrollController();

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
                alignment: AlignmentGeometry.bottomCenter,
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
                                    if (gl.geometries.isNotEmpty)
                                      {
                                        if (_isPolygonWellDefined(
                                          gl.geometries[gl.selectedGeometry]
                                              .getPolyPlusOneVertex(point),
                                        ))
                                          {
                                            refreshView(() {
                                              gl.geometries[gl.selectedGeometry]
                                                  .addPoint(point);
                                            }),
                                            if (gl
                                                    .geometries[gl
                                                        .selectedGeometry]
                                                    .type
                                                    .contains("Point") &&
                                                gl
                                                        .geometries[gl
                                                            .selectedGeometry]
                                                        .numPoints ==
                                                    1)
                                              {
                                                refreshView(() {
                                                  gl
                                                      .geometries[gl
                                                          .selectedGeometry]
                                                      .selectedVertex = 0;
                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                      false;
                                                  gl.Mode.showButtonMoveVertexesPolygon =
                                                      true;
                                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                                      true;
                                                  gl.Mode.addVertexesPolygon =
                                                      false;
                                                  gl.Mode.moveVertexesPolygon =
                                                      false;
                                                  gl.Mode.removeVertexesPolygon =
                                                      false;
                                                }),
                                              },
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
                                      if (gl
                                              .geometries[gl.selectedGeometry]
                                              .type
                                              .contains("Point") &&
                                          gl
                                                  .geometries[gl
                                                      .selectedGeometry]
                                                  .numPoints ==
                                              1) {
                                        refreshView(() {
                                          gl
                                              .geometries[gl.selectedGeometry]
                                              .selectedVertex = 0;
                                          gl.Mode.showButtonAddVertexesPolygon =
                                              false;
                                          gl.Mode.showButtonMoveVertexesPolygon =
                                              true;
                                          gl.Mode.showButtonRemoveVertexesPolygon =
                                              true;
                                          gl.Mode.addVertexesPolygon = false;
                                          gl.Mode.moveVertexesPolygon = false;
                                          gl.Mode.removeVertexesPolygon = false;
                                        });
                                      } else {
                                        gl.Mode.showButtonAddVertexesPolygon =
                                            true;
                                        gl.Mode.showButtonMoveVertexesPolygon =
                                            false;
                                        gl.Mode.showButtonRemoveVertexesPolygon =
                                            false;
                                        gl.Mode.addVertexesPolygon = false;
                                        gl.Mode.moveVertexesPolygon = false;
                                        gl.Mode.removeVertexesPolygon = false;
                                      }
                                    }),
                                  }
                                  : gl.Mode.essence
                                  ? (tapPosition, point) async => {
                                    PopupNewEssenceObservationPoint(
                                      context,
                                      point,
                                    ),
                                  }
                                  : gl.Mode.editPolygon
                                  ? (tapPosition, point) async => {}
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
                                gl.geometries[gl.selectedGeometry]
                                    .getPolyMoveOneVertex(
                                      _selectedPointToMove!,
                                      LatLng(
                                        position.center.latitude,
                                        position.center.longitude,
                                      ),
                                    ),
                              )) {
                                gl.geometries[gl.selectedGeometry].replacePoint(
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
                            if (gl.positionInit) {
                              refreshView(() {
                                _mapController.move(
                                  LatLng(
                                    gl.position.latitude,
                                    gl.position.longitude,
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
                                  latitude: gl.position.latitude,
                                  longitude: gl.position.longitude,
                                  accuracy: 10.0,
                                ),
                              ),
                            ] +
                            (gl.Mode.editPolygon
                                ? <Widget>[
                                  if (gl.geometries.isNotEmpty &&
                                      gl
                                              .geometries[gl.selectedGeometry]
                                              .points
                                              .length >
                                          1 &&
                                      !gl.Mode.showButtonMoveVertexesPolygon &&
                                      gl.Mode.addVertexesPolygon)
                                    PolylineLayer(
                                      polylines: [
                                        Polyline(
                                          points: [
                                            gl
                                                .geometries[gl.selectedGeometry]
                                                .points[gl
                                                .geometries[gl.selectedGeometry]
                                                .selectedPolyLinePoints[0]],
                                            gl
                                                .geometries[gl.selectedGeometry]
                                                .points[gl
                                                .geometries[gl.selectedGeometry]
                                                .selectedPolyLinePoints[1]],
                                          ],
                                          color:
                                              gl
                                                  .geometries[gl
                                                      .selectedGeometry]
                                                  .colorLine,
                                          strokeWidth: 5.0,
                                        ),
                                      ],
                                    ),
                                  CircleLayer(
                                    circles: _drawnLayerPointsCircleMarker(),
                                  ),
                                  MarkerLayer(markers: _getPointsToDraw()),
                                  PolygonLayer(polygons: _getPolygonesToDraw()),
                                  MarkerLayer(
                                    markers: _drawnLayerPointsMarker(),
                                  ),
                                  if (gl.geometries.isNotEmpty &&
                                      gl
                                          .geometries[gl.selectedGeometry]
                                          .points
                                          .isNotEmpty &&
                                      !gl.Mode.showButtonAddVertexesPolygon)
                                    CircleLayer(
                                      circles: [
                                        CircleMarker(
                                          point:
                                              gl
                                                  .geometries[gl
                                                      .selectedGeometry]
                                                  .points[gl
                                                  .geometries[gl
                                                      .selectedGeometry]
                                                  .selectedPolyLinePoints[0]],
                                          radius: 15,
                                          color: Colors.red,
                                        ),
                                      ],
                                    ),
                                ]
                                : (gl.modeMapShowPolygons && gl.Mode.polygon)
                                ? <Widget>[
                                  MouseRegion(
                                    hitTestBehavior:
                                        HitTestBehavior.deferToChild,
                                    cursor: SystemMouseCursors.click,
                                    child: GestureDetector(
                                      onTap: () {
                                        final LayerHitResult<String>? result =
                                            hitNotifier.value;
                                        if (result == null) return;
                                        int index = 0;
                                        if (gl.Mode.openToolbox) {
                                          for (var it in gl.geometries) {
                                            if (result.hitValues.first ==
                                                it.identifier) {
                                              setState(() {
                                                gl.selectedGeometry = index;
                                              });
                                            }
                                            index++;
                                          }
                                        }
                                      },
                                      child: PolygonLayer<String>(
                                        hitNotifier: hitNotifier,
                                        polygons: _getPolygonesToDraw(),
                                      ),
                                    ),
                                  ),
                                  MarkerLayer(
                                    markers: _getPointsToDraw(hitButton: true),
                                  ),
                                ]
                                : <Widget>[
                                  PolygonLayer<String>(
                                    polygons: _getPolygonesToDraw(),
                                  ),
                                  MarkerLayer(
                                    markers: _getPointsToDraw(hitButton: false),
                                  ),
                                ]) +
                            <Widget>[
                              MarkerLayer(
                                markers:
                                    _placeVertexMovePointer() +
                                    _placeAnaPtMarker(),
                              ),
                              if (gl.modeMapShowPolygons)
                                MarkerLayer(markers: _getPolygonesLabels()),
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
                      (gl.Mode.polygon &&
                              gl.geometries.isNotEmpty &&
                              gl.selectedGeometry > -1)
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
                                    width:
                                        gl.display.equipixel *
                                        gl.chosenPolyBarWidth,
                                    height:
                                        gl.display.equipixel *
                                        computePolygonTitleHeight(),
                                    child: Card(
                                      shape: RoundedRectangleBorder(
                                        borderRadius:
                                            BorderRadiusGeometry.circular(12.0),
                                        side: BorderSide(
                                          color: Colors.black,
                                          width: 2.0,
                                        ),
                                      ),
                                      surfaceTintColor: Colors.transparent,
                                      shadowColor: Colors.transparent,
                                      color: gl.backgroundTransparentBlackBox,
                                      child: Column(
                                        children: [
                                          Row(
                                            mainAxisAlignment:
                                                MainAxisAlignment.spaceAround,
                                            children: [
                                              SizedBox(
                                                height:
                                                    gl.display.equipixel *
                                                    gl.iconSizeM *
                                                    .9,
                                                child: IconButton(
                                                  style: borderlessStyle,
                                                  onPressed: () {
                                                    refreshView(() {
                                                      if (!gl
                                                          .Mode
                                                          .editPolygon) {
                                                        gl
                                                            .geometries[gl
                                                                .selectedGeometry]
                                                            .visibleOnMap = !gl
                                                                .geometries[gl
                                                                    .selectedGeometry]
                                                                .visibleOnMap;
                                                        gl
                                                            .geometries[gl
                                                                .selectedGeometry]
                                                            .serialize();
                                                      }
                                                    });
                                                  },
                                                  icon:
                                                      gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .visibleOnMap
                                                          ? FaIcon(
                                                            FontAwesomeIcons
                                                                .eyeSlash,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                            color: Colors.white,
                                                          )
                                                          : FaIcon(
                                                            FontAwesomeIcons
                                                                .eye,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                            color: Colors.white,
                                                          ),
                                                ),
                                              ),
                                              SizedBox(
                                                width:
                                                    gl.display.equipixel *
                                                    gl.chosenPolyBarWidth *
                                                    .5,
                                                child: Stack(
                                                  children: [
                                                    Row(
                                                      children: [
                                                        Column(
                                                          mainAxisAlignment:
                                                              MainAxisAlignment
                                                                  .spaceBetween,
                                                          children: [
                                                            gl
                                                                    .geometries[gl
                                                                        .selectedGeometry]
                                                                    .type
                                                                    .contains(
                                                                      "Point",
                                                                    )
                                                                ? Text(
                                                                  "POINT",
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .yellow,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeXS *
                                                                        .9,
                                                                  ),
                                                                )
                                                                : gl
                                                                        .geometries[gl
                                                                            .selectedGeometry]
                                                                        .type ==
                                                                    "Polygon"
                                                                ? Text(
                                                                  "POLY",
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .green,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeXS *
                                                                        .9,
                                                                  ),
                                                                )
                                                                : Text(
                                                                  "OHA?",
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .red,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeXS *
                                                                        .9,
                                                                  ),
                                                                ),
                                                            Icon(
                                                              (gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .type
                                                                      .contains(
                                                                        "Point",
                                                                      )
                                                                  ? gl.selectableIcons[gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .selectedPointIcon]
                                                                  : Icons
                                                                      .hexagon_outlined),
                                                              size:
                                                                  gl.iconSizeXS *
                                                                  gl
                                                                      .display
                                                                      .equipixel,
                                                              color:
                                                                  gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .colorLine,
                                                            ),
                                                          ],
                                                        ),
                                                        SizedBox(
                                                          width:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              2,
                                                        ),
                                                        if (gl
                                                            .geometries[gl
                                                                .selectedGeometry]
                                                            .sentToServer)
                                                          Container(
                                                            alignment:
                                                                Alignment
                                                                    .topRight,
                                                            child: Text(
                                                              "SENT",
                                                              style: TextStyle(
                                                                color:
                                                                    Colors.red,
                                                                fontSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.fontSizeXS *
                                                                    .9,
                                                              ),
                                                            ),
                                                          ),
                                                      ],
                                                    ),
                                                    Container(
                                                      alignment:
                                                          Alignment.center,
                                                      child: SingleChildScrollView(
                                                        scrollDirection:
                                                            Axis.horizontal,
                                                        child: Text(
                                                          gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .name,
                                                          textAlign:
                                                              TextAlign.center,
                                                          style: TextStyle(
                                                            color: Colors.white,
                                                            fontSize:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.fontSizeL,
                                                          ),
                                                        ),
                                                      ),
                                                    ),
                                                  ],
                                                ),
                                              ),
                                              SizedBox(
                                                height:
                                                    gl.display.equipixel *
                                                    gl.iconSizeM *
                                                    .9,
                                                child: IconButton(
                                                  style: borderlessStyle,
                                                  onPressed: () {
                                                    refreshView(() {
                                                      gl.Mode.openToolbox =
                                                          !gl.Mode.openToolbox;
                                                    });
                                                  },
                                                  icon: FaIcon(
                                                    FontAwesomeIcons.toolbox,
                                                    size:
                                                        gl.display.equipixel *
                                                        gl.iconSizeS *
                                                        .9,
                                                    color: Colors.white,
                                                  ),
                                                ),
                                              ),
                                            ],
                                          ),
                                          !gl.Mode.openToolbox
                                              ? Row()
                                              : (gl.Mode.editPolygon &&
                                                  gl.geometries.isNotEmpty)
                                              ? Column(
                                                children: [
                                                  stroke(
                                                    gl.display.equipixel,
                                                    gl.display.equipixel * .5,
                                                    gl.colorAgroBioTech,
                                                  ),
                                                  Row(
                                                    children: [
                                                      SizedBox(
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSizeM *
                                                            .9,
                                                        child: IconButton(
                                                          style:
                                                              borderlessStyle,
                                                          iconSize:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.iconSizeS,
                                                          color:
                                                              Colors
                                                                  .lightGreenAccent,
                                                          onPressed: () {
                                                            refreshView(() {
                                                              _stopMovingSelectedPoint();
                                                              gl.Mode.editPolygon =
                                                                  false;
                                                              gl.Mode.showButtonAddVertexesPolygon =
                                                                  true;
                                                              gl.Mode.showButtonMoveVertexesPolygon =
                                                                  false;
                                                              gl.Mode.showButtonRemoveVertexesPolygon =
                                                                  false;
                                                              gl.Mode.addVertexesPolygon =
                                                                  false;
                                                              gl.Mode.moveVertexesPolygon =
                                                                  false;
                                                              gl.Mode.removeVertexesPolygon =
                                                                  false;
                                                            });
                                                          },
                                                          icon: Icon(
                                                            Icons.arrow_back,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                          ),
                                                        ),
                                                      ),
                                                      Container(
                                                        alignment:
                                                            Alignment
                                                                .centerLeft,
                                                        width:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.chosenPolyBarWidth *
                                                            .5,
                                                        child:
                                                            gl
                                                                        .geometries[gl
                                                                            .selectedGeometry]
                                                                        .type ==
                                                                    "Polygon"
                                                                ? Text(
                                                                  "Modifiez le polygone",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .white,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                )
                                                                : gl
                                                                    .geometries[gl
                                                                        .selectedGeometry]
                                                                    .type
                                                                    .contains(
                                                                      "Point",
                                                                    )
                                                                ? Text(
                                                                  "Modifiez le point",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .white,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                )
                                                                : Text(
                                                                  "Modifiez ceci?",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .orange,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                ),
                                                      ),
                                                    ],
                                                  ),
                                                  stroke(
                                                    gl.display.equipixel,
                                                    gl.display.equipixel * .5,
                                                    gl.colorAgroBioTech,
                                                  ),
                                                  Row(
                                                    mainAxisAlignment:
                                                        MainAxisAlignment
                                                            .spaceAround,
                                                    children: [
                                                      gl.Mode.showButtonRemoveVertexesPolygon
                                                          ? Container(
                                                            color: _polygonMenuColorTools(
                                                              gl
                                                                  .Mode
                                                                  .removeVertexesPolygon,
                                                            ),
                                                            child: SizedBox(
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeM *
                                                                  .9,
                                                              child: IconButton(
                                                                style:
                                                                    borderlessStyle,
                                                                iconSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS,
                                                                color:
                                                                    gl.Mode.removeVertexesPolygon
                                                                        ? Colors
                                                                            .white
                                                                        : Colors
                                                                            .lightGreenAccent,
                                                                onPressed: () async {
                                                                  refreshView(() {
                                                                    gl.Mode.removeVertexesPolygon =
                                                                        !gl
                                                                            .Mode
                                                                            .removeVertexesPolygon;
                                                                  });
                                                                  if (gl
                                                                          .Mode
                                                                          .removeVertexesPolygon ==
                                                                      true) {
                                                                    refreshView(() {
                                                                      if (gl
                                                                              .geometries[gl.selectedGeometry]
                                                                              .points
                                                                              .isNotEmpty &&
                                                                          _isPolygonWellDefined(
                                                                            gl.geometries[gl.selectedGeometry].getPolyRemoveOneVertex(
                                                                              gl.geometries[gl.selectedGeometry].points[gl.geometries[gl.selectedGeometry].selectedPolyLinePoints[0]],
                                                                            ),
                                                                          )) {
                                                                        gl.geometries[gl.selectedGeometry].removePoint(
                                                                          gl.geometries[gl.selectedGeometry].points[gl
                                                                              .geometries[gl.selectedGeometry]
                                                                              .selectedPolyLinePoints[0]],
                                                                        );
                                                                      }
                                                                    });
                                                                    gl.Mode.moveVertexesPolygon =
                                                                        false;
                                                                    gl.Mode.addVertexesPolygon =
                                                                        false;
                                                                    _stopMovingSelectedPoint();
                                                                    refreshView(() {
                                                                      gl.Mode.showButtonAddVertexesPolygon =
                                                                          true;
                                                                      gl.Mode.showButtonMoveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.addVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.moveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.removeVertexesPolygon =
                                                                          false;
                                                                    });
                                                                  }
                                                                },
                                                                icon: const Icon(
                                                                  Icons
                                                                      .remove_circle,
                                                                ),
                                                              ),
                                                            ),
                                                          )
                                                          : SizedBox(
                                                            height:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeM *
                                                                .9,
                                                            child: IconButton(
                                                              style:
                                                                  borderlessStyle,
                                                              iconSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS,
                                                              color:
                                                                  Colors
                                                                      .white24,
                                                              onPressed: () {},
                                                              icon: const Icon(
                                                                Icons
                                                                    .remove_circle,
                                                              ),
                                                            ),
                                                          ),
                                                      (gl.geometries[gl.selectedGeometry].type ==
                                                                      "Polygon" ||
                                                                  gl
                                                                          .geometries[gl
                                                                              .selectedGeometry]
                                                                          .type
                                                                          .contains(
                                                                            "Point",
                                                                          ) &&
                                                                      gl.geometries[gl.selectedGeometry].numPoints <
                                                                          1) &&
                                                              gl
                                                                  .Mode
                                                                  .showButtonAddVertexesPolygon
                                                          ? Container(
                                                            color: _polygonMenuColorTools(
                                                              gl
                                                                  .Mode
                                                                  .addVertexesPolygon,
                                                            ),
                                                            child: SizedBox(
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeM *
                                                                  .9,
                                                              child: IconButton(
                                                                style:
                                                                    borderlessStyle,
                                                                iconSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS,
                                                                color:
                                                                    gl.Mode.addVertexesPolygon
                                                                        ? Colors
                                                                            .white
                                                                        : Colors
                                                                            .lightGreenAccent,
                                                                onPressed: () async {
                                                                  refreshView(() {
                                                                    gl.Mode.addVertexesPolygon =
                                                                        !gl
                                                                            .Mode
                                                                            .addVertexesPolygon;
                                                                  });
                                                                  if (gl
                                                                          .Mode
                                                                          .addVertexesPolygon ==
                                                                      true) {
                                                                    gl.Mode.removeVertexesPolygon =
                                                                        false;
                                                                    gl.Mode.moveVertexesPolygon =
                                                                        false;
                                                                    refreshView(
                                                                      () {
                                                                        _stopMovingSelectedPoint();
                                                                      },
                                                                    );
                                                                  }
                                                                },
                                                                icon: const Icon(
                                                                  Icons
                                                                      .add_circle,
                                                                ),
                                                              ),
                                                            ),
                                                          )
                                                          : SizedBox(
                                                            height:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeM *
                                                                .9,
                                                            child: IconButton(
                                                              style:
                                                                  borderlessStyle,
                                                              iconSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS,
                                                              color:
                                                                  Colors
                                                                      .white24,
                                                              onPressed: () {},
                                                              icon: const Icon(
                                                                Icons
                                                                    .add_circle,
                                                              ),
                                                            ),
                                                          ),
                                                      gl.Mode.showButtonMoveVertexesPolygon
                                                          ? Container(
                                                            color: _polygonMenuColorTools(
                                                              gl
                                                                  .Mode
                                                                  .moveVertexesPolygon,
                                                            ),
                                                            child: SizedBox(
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeM *
                                                                  .9,
                                                              child: IconButton(
                                                                style:
                                                                    borderlessStyle,
                                                                color:
                                                                    gl.Mode.moveVertexesPolygon
                                                                        ? Colors
                                                                            .white
                                                                        : Colors
                                                                            .lightGreenAccent,
                                                                iconSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS,
                                                                onPressed: () async {
                                                                  refreshView(() {
                                                                    gl.Mode.moveVertexesPolygon =
                                                                        !gl
                                                                            .Mode
                                                                            .moveVertexesPolygon;
                                                                  });
                                                                  if (gl
                                                                          .Mode
                                                                          .moveVertexesPolygon ==
                                                                      true) {
                                                                    refreshView(() {
                                                                      LatLng
                                                                      point =
                                                                          gl.geometries[gl.selectedGeometry].points[gl
                                                                              .geometries[gl.selectedGeometry]
                                                                              .selectedPolyLinePoints[0]];
                                                                      if (_selectedPointToMove ==
                                                                          null) {
                                                                        _selectedPointToMove =
                                                                            point;
                                                                        _mapController.move(
                                                                          point,
                                                                          _mapController
                                                                              .camera
                                                                              .zoom,
                                                                        );
                                                                      } else {
                                                                        if (point.latitude ==
                                                                                _selectedPointToMove!.latitude &&
                                                                            point.longitude ==
                                                                                _selectedPointToMove!.longitude) {
                                                                          _stopMovingSelectedPoint();
                                                                        } else {
                                                                          _selectedPointToMove =
                                                                              point;
                                                                          _mapController.move(
                                                                            point,
                                                                            _mapController.camera.zoom,
                                                                          );
                                                                        }
                                                                      }
                                                                    });
                                                                    gl.Mode.addVertexesPolygon =
                                                                        false;
                                                                    gl.Mode.removeVertexesPolygon =
                                                                        false;
                                                                  } else if (gl
                                                                          .Mode
                                                                          .editPolygon &&
                                                                      gl
                                                                          .geometries[gl
                                                                              .selectedGeometry]
                                                                          .type
                                                                          .contains(
                                                                            "Point",
                                                                          )) {
                                                                    refreshView(() {
                                                                      _stopMovingSelectedPoint();
                                                                      gl.Mode.showButtonAddVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.showButtonMoveVertexesPolygon =
                                                                          true;
                                                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                                                          true;
                                                                      gl.Mode.addVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.moveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.removeVertexesPolygon =
                                                                          false;
                                                                    });
                                                                  } else {
                                                                    refreshView(() {
                                                                      _stopMovingSelectedPoint();
                                                                      gl.Mode.showButtonAddVertexesPolygon =
                                                                          true;
                                                                      gl.Mode.showButtonMoveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.showButtonRemoveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.addVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.moveVertexesPolygon =
                                                                          false;
                                                                      gl.Mode.removeVertexesPolygon =
                                                                          false;
                                                                    });
                                                                  }
                                                                },
                                                                icon: const Icon(
                                                                  Icons
                                                                      .open_with_rounded,
                                                                ),
                                                              ),
                                                            ),
                                                          )
                                                          : SizedBox(
                                                            height:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeM *
                                                                .9,
                                                            child: IconButton(
                                                              style:
                                                                  borderlessStyle,
                                                              iconSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS,
                                                              color:
                                                                  Colors
                                                                      .white24,
                                                              onPressed: () {},
                                                              icon: const Icon(
                                                                Icons
                                                                    .open_with_rounded,
                                                              ),
                                                            ),
                                                          ),
                                                    ],
                                                  ),
                                                ],
                                              )
                                              : gl.Mode.editAttributes
                                              ? Column(
                                                mainAxisAlignment:
                                                    MainAxisAlignment
                                                        .spaceBetween,
                                                children: [
                                                  Column(
                                                    children: [
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      Row(
                                                        children: [
                                                          Container(
                                                            alignment:
                                                                Alignment
                                                                    .topLeft,
                                                            child: SizedBox(
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeM *
                                                                  .9,
                                                              child: IconButton(
                                                                style:
                                                                    borderlessStyle,
                                                                iconSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS,
                                                                color:
                                                                    Colors
                                                                        .lightGreenAccent,
                                                                onPressed: () {
                                                                  refreshView(() {
                                                                    gl.Mode.editAttributes =
                                                                        false;
                                                                  });
                                                                },
                                                                icon: Icon(
                                                                  Icons
                                                                      .arrow_back,
                                                                  size:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.iconSizeS *
                                                                      .9,
                                                                ),
                                                              ),
                                                            ),
                                                          ),
                                                          Container(
                                                            alignment:
                                                                Alignment
                                                                    .centerLeft,
                                                            width:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.chosenPolyBarWidth *
                                                                .75,
                                                            child: Text(
                                                              "Table des attributs",
                                                              textAlign:
                                                                  TextAlign
                                                                      .center,
                                                              style: TextStyle(
                                                                color:
                                                                    Colors
                                                                        .white,
                                                                fontSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.fontSizeM *
                                                                    .75,
                                                              ),
                                                            ),
                                                          ),
                                                        ],
                                                      ),
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      Column(
                                                        children: [
                                                          Row(
                                                            mainAxisAlignment:
                                                                MainAxisAlignment
                                                                    .spaceEvenly,
                                                            children: [
                                                              SizedBox(
                                                                width:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    11,
                                                                child: Text(
                                                                  "type",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .white,
                                                                    fontWeight:
                                                                        FontWeight
                                                                            .bold,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                ),
                                                              ),
                                                              stroke(
                                                                vertical: true,
                                                                gl
                                                                    .display
                                                                    .equipixel,
                                                                gl
                                                                        .display
                                                                        .equipixel *
                                                                    0.5,
                                                                gl.colorAgroBioTech,
                                                              ),
                                                              SizedBox(
                                                                width:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    7,
                                                                child: Icon(
                                                                  Icons
                                                                      .remove_red_eye,
                                                                  color:
                                                                      Colors
                                                                          .white,
                                                                  size:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.iconSizeXS,
                                                                ),
                                                              ),
                                                              stroke(
                                                                vertical: true,
                                                                gl
                                                                    .display
                                                                    .equipixel,
                                                                gl
                                                                        .display
                                                                        .equipixel *
                                                                    0.5,
                                                                gl.colorAgroBioTech,
                                                              ),
                                                              SizedBox(
                                                                width:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    33,
                                                                child: Text(
                                                                  "Attribut",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .white,
                                                                    fontWeight:
                                                                        FontWeight
                                                                            .bold,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                ),
                                                              ),
                                                              stroke(
                                                                vertical: true,
                                                                gl
                                                                    .display
                                                                    .equipixel,
                                                                gl
                                                                        .display
                                                                        .equipixel *
                                                                    0.5,
                                                                gl.colorAgroBioTech,
                                                              ),
                                                              SizedBox(
                                                                width:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    33,
                                                                child: Text(
                                                                  "Valeur",
                                                                  textAlign:
                                                                      TextAlign
                                                                          .center,
                                                                  style: TextStyle(
                                                                    color:
                                                                        Colors
                                                                            .white,
                                                                    fontWeight:
                                                                        FontWeight
                                                                            .bold,
                                                                    fontSize:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.fontSizeM *
                                                                        .75,
                                                                  ),
                                                                ),
                                                              ),
                                                            ],
                                                          ),
                                                          stroke(
                                                            gl
                                                                .display
                                                                .equipixel,
                                                            gl
                                                                    .display
                                                                    .equipixel *
                                                                0.5,
                                                            gl.colorAgroBioTech,
                                                          ),
                                                          Scrollbar(
                                                            scrollbarOrientation:
                                                                ScrollbarOrientation
                                                                    .right,
                                                            thickness:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                3,
                                                            controller:
                                                                propertiesTableScrollController,
                                                            child: Container(
                                                              color: gl
                                                                  .backgroundTransparentBlackBox
                                                                  .withAlpha(
                                                                    100,
                                                                  ),
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.attributeTableHeight,
                                                              child: ListView(
                                                                controller:
                                                                    propertiesTableScrollController,
                                                                children:
                                                                    <Widget>[
                                                                      _getFixedAttribute(
                                                                        "type",
                                                                        gl
                                                                            .geometries[gl.selectedGeometry]
                                                                            .type,
                                                                      ),
                                                                      _getFixedAttribute(
                                                                        "nom",
                                                                        gl
                                                                            .geometries[gl.selectedGeometry]
                                                                            .name,
                                                                        checked:
                                                                            true,
                                                                      ),
                                                                      if (gl.geometries[gl.selectedGeometry].type ==
                                                                          "Polygon")
                                                                        _getFixedAttribute(
                                                                          "surface",
                                                                          "${(gl.geometries[gl.selectedGeometry].area / 100).round() / 100}",
                                                                        ),
                                                                      if (gl.geometries[gl.selectedGeometry].type ==
                                                                          "Polygon")
                                                                        _getFixedAttribute(
                                                                          "circonference",
                                                                          "${(gl.geometries[gl.selectedGeometry].perimeter).round() / 1000}",
                                                                        ),

                                                                      _getFixedAttribute(
                                                                        "coordinates",
                                                                        gl.geometries[gl.selectedGeometry]
                                                                            .getPolyPointsString(),
                                                                      ),
                                                                    ] +
                                                                    List<
                                                                      Widget
                                                                    >.generate(
                                                                      gl
                                                                          .geometries[gl
                                                                              .selectedGeometry]
                                                                          .attributes
                                                                          .length,
                                                                      (i) {
                                                                        return Column(
                                                                          children: [
                                                                            Row(
                                                                              mainAxisAlignment:
                                                                                  MainAxisAlignment.spaceEvenly,
                                                                              children: [
                                                                                SizedBox(
                                                                                  width:
                                                                                      gl.display.equipixel *
                                                                                      11,
                                                                                  height:
                                                                                      gl.display.equipixel *
                                                                                      gl.iconSizeS,
                                                                                  child: TextButton(
                                                                                    style: ButtonStyle(
                                                                                      animationDuration: Duration(
                                                                                        seconds:
                                                                                            1,
                                                                                      ),
                                                                                      backgroundColor: WidgetStateProperty<
                                                                                        Color
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          Color
                                                                                        >{
                                                                                          WidgetState.any: Colors.transparent,
                                                                                        },
                                                                                      ),
                                                                                      padding: WidgetStateProperty<
                                                                                        EdgeInsetsGeometry
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          EdgeInsetsGeometry
                                                                                        >{
                                                                                          WidgetState.any: EdgeInsetsGeometry.zero,
                                                                                        },
                                                                                      ),
                                                                                    ),
                                                                                    onPressed:
                                                                                        () {},
                                                                                    onLongPress:
                                                                                        () {},
                                                                                    child: Container(
                                                                                      alignment:
                                                                                          Alignment.center,
                                                                                      child:
                                                                                          gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                  "int"
                                                                                              ? Text(
                                                                                                "INT",
                                                                                                style: TextStyle(
                                                                                                  color:
                                                                                                      Colors.yellow,
                                                                                                  fontSize:
                                                                                                      gl.fontSizeXXS *
                                                                                                      gl.display.equipixel,
                                                                                                ),
                                                                                              )
                                                                                              : gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                  "string"
                                                                                              ? Text(
                                                                                                "STRING",
                                                                                                style: TextStyle(
                                                                                                  color:
                                                                                                      Colors.lightBlue,
                                                                                                  fontSize:
                                                                                                      gl.fontSizeXXS *
                                                                                                      gl.display.equipixel,
                                                                                                ),
                                                                                              )
                                                                                              : gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                  "double"
                                                                                              ? Text(
                                                                                                "DOUBLE",
                                                                                                style: TextStyle(
                                                                                                  color:
                                                                                                      Colors.red,
                                                                                                  fontSize:
                                                                                                      gl.fontSizeXXS *
                                                                                                      gl.display.equipixel,
                                                                                                ),
                                                                                              )
                                                                                              : Text(
                                                                                                "UFO",
                                                                                                style: TextStyle(
                                                                                                  color:
                                                                                                      Colors.green,
                                                                                                  fontSize:
                                                                                                      gl.fontSizeXXS *
                                                                                                      gl.display.equipixel,
                                                                                                ),
                                                                                              ),
                                                                                    ),
                                                                                  ),
                                                                                ),
                                                                                stroke(
                                                                                  vertical:
                                                                                      true,
                                                                                  gl.display.equipixel,
                                                                                  gl.display.equipixel *
                                                                                      0.5,
                                                                                  gl.colorAgroBioTech,
                                                                                ),
                                                                                SizedBox(
                                                                                  width:
                                                                                      gl.display.equipixel *
                                                                                      7,
                                                                                  height:
                                                                                      gl.display.equipixel *
                                                                                      gl.iconSizeM,
                                                                                  child: IconButton(
                                                                                    style: ButtonStyle(
                                                                                      animationDuration: Duration(
                                                                                        seconds:
                                                                                            1,
                                                                                      ),
                                                                                      backgroundColor: WidgetStateProperty<
                                                                                        Color
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          Color
                                                                                        >{
                                                                                          WidgetState.any: Colors.transparent,
                                                                                        },
                                                                                      ),
                                                                                      padding: WidgetStateProperty<
                                                                                        EdgeInsetsGeometry
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          EdgeInsetsGeometry
                                                                                        >{
                                                                                          WidgetState.any: EdgeInsetsGeometry.zero,
                                                                                        },
                                                                                      ),
                                                                                    ),
                                                                                    onPressed:
                                                                                        () {},
                                                                                    onLongPress: () async {
                                                                                      refreshView(
                                                                                        () {
                                                                                          gl.geometries[gl.selectedGeometry].attributes[i].visibleOnMapLabel = !gl.geometries[gl.selectedGeometry].attributes[i].visibleOnMapLabel;
                                                                                        },
                                                                                      );
                                                                                      gl.geometries[gl.selectedGeometry].serialize();
                                                                                    },
                                                                                    icon:
                                                                                        gl.geometries[gl.selectedGeometry].attributes[i].visibleOnMapLabel
                                                                                            ? Icon(
                                                                                              Icons.check_box_outlined,
                                                                                              color:
                                                                                                  Colors.white,
                                                                                              size:
                                                                                                  gl.display.equipixel *
                                                                                                  gl.iconSizeXS,
                                                                                            )
                                                                                            : Icon(
                                                                                              Icons.check_box_outline_blank,
                                                                                              color:
                                                                                                  Colors.white,
                                                                                              size:
                                                                                                  gl.display.equipixel *
                                                                                                  gl.iconSizeXS,
                                                                                            ),
                                                                                  ),
                                                                                ),
                                                                                stroke(
                                                                                  vertical:
                                                                                      true,
                                                                                  gl.display.equipixel,
                                                                                  gl.display.equipixel *
                                                                                      0.5,
                                                                                  gl.colorAgroBioTech,
                                                                                ),
                                                                                SizedBox(
                                                                                  width:
                                                                                      gl.display.equipixel *
                                                                                      33,
                                                                                  height:
                                                                                      gl.display.equipixel *
                                                                                      gl.iconSizeS,
                                                                                  child: TextButton(
                                                                                    style: ButtonStyle(
                                                                                      animationDuration: Duration(
                                                                                        seconds:
                                                                                            1,
                                                                                      ),
                                                                                      backgroundColor: WidgetStateProperty<
                                                                                        Color
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          Color
                                                                                        >{
                                                                                          WidgetState.any: Colors.transparent,
                                                                                        },
                                                                                      ),
                                                                                      padding: WidgetStateProperty<
                                                                                        EdgeInsetsGeometry
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          EdgeInsetsGeometry
                                                                                        >{
                                                                                          WidgetState.any: EdgeInsetsGeometry.zero,
                                                                                        },
                                                                                      ),
                                                                                    ),
                                                                                    onPressed:
                                                                                        () {},
                                                                                    onLongPress: () {
                                                                                      PopupValueChange(
                                                                                        "prop",
                                                                                        gl.geometries[gl.selectedGeometry].attributes[i].name,
                                                                                        (
                                                                                          value,
                                                                                        ) {
                                                                                          gl.geometries[gl.selectedGeometry].attributes[i].name = cleanAttributeName(
                                                                                            value.toString(),
                                                                                          );
                                                                                        },
                                                                                        () {},
                                                                                        () {
                                                                                          String nom =
                                                                                              gl.geometries[gl.selectedGeometry].attributes[i].name;
                                                                                          if (controlDuplicateAttributeName(
                                                                                            gl.geometries[gl.selectedGeometry].attributes[i].name,
                                                                                          )) {
                                                                                            PopupMessage(
                                                                                              "Erreur",
                                                                                              "Le nom $nom existe déja!",
                                                                                            );
                                                                                            return;
                                                                                          } else {
                                                                                            gl.geometries[gl.selectedGeometry].serialize();
                                                                                          }
                                                                                        },
                                                                                      );
                                                                                    },
                                                                                    child: Container(
                                                                                      alignment:
                                                                                          Alignment.centerLeft,
                                                                                      child: SingleChildScrollView(
                                                                                        scrollDirection:
                                                                                            Axis.horizontal,
                                                                                        child: Text(
                                                                                          gl.geometries[gl.selectedGeometry].attributes[i].name,
                                                                                          textAlign:
                                                                                              TextAlign.start,
                                                                                          style: TextStyle(
                                                                                            color:
                                                                                                Colors.white,
                                                                                            fontSize:
                                                                                                gl.display.equipixel *
                                                                                                gl.fontSizeM *
                                                                                                .75,
                                                                                          ),
                                                                                        ),
                                                                                      ),
                                                                                    ),
                                                                                  ),
                                                                                ),
                                                                                stroke(
                                                                                  vertical:
                                                                                      true,
                                                                                  gl.display.equipixel,
                                                                                  gl.display.equipixel *
                                                                                      0.5,
                                                                                  gl.colorAgroBioTech,
                                                                                ),
                                                                                SizedBox(
                                                                                  width:
                                                                                      gl.display.equipixel *
                                                                                      33,
                                                                                  height:
                                                                                      gl.display.equipixel *
                                                                                      gl.iconSizeS,
                                                                                  child: TextButton(
                                                                                    style: ButtonStyle(
                                                                                      animationDuration: Duration(
                                                                                        seconds:
                                                                                            1,
                                                                                      ),
                                                                                      backgroundColor: WidgetStateProperty<
                                                                                        Color
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          Color
                                                                                        >{
                                                                                          WidgetState.any: Colors.transparent,
                                                                                        },
                                                                                      ),
                                                                                      padding: WidgetStateProperty<
                                                                                        EdgeInsetsGeometry
                                                                                      >.fromMap(
                                                                                        <
                                                                                          WidgetStatesConstraint,
                                                                                          EdgeInsetsGeometry
                                                                                        >{
                                                                                          WidgetState.any: EdgeInsetsGeometry.zero,
                                                                                        },
                                                                                      ),
                                                                                    ),
                                                                                    onPressed:
                                                                                        () {},
                                                                                    onLongPress: () {
                                                                                      PopupValueChange(
                                                                                        gl.geometries[gl.selectedGeometry].attributes[i].type,
                                                                                        gl.geometries[gl.selectedGeometry].attributes[i].value,
                                                                                        (
                                                                                          value,
                                                                                        ) {
                                                                                          gl.geometries[gl.selectedGeometry].attributes[i].value = value;
                                                                                        },
                                                                                        () {},
                                                                                        () {
                                                                                          gl.geometries[gl.selectedGeometry].serialize();
                                                                                        },
                                                                                      );
                                                                                    },
                                                                                    child: Container(
                                                                                      alignment:
                                                                                          Alignment.centerLeft,
                                                                                      child: SingleChildScrollView(
                                                                                        scrollDirection:
                                                                                            Axis.horizontal,
                                                                                        child:
                                                                                            gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                    "string"
                                                                                                ? Text(
                                                                                                  gl.geometries[gl.selectedGeometry].attributes[i].value,
                                                                                                  textAlign:
                                                                                                      TextAlign.start,
                                                                                                  style: TextStyle(
                                                                                                    color:
                                                                                                        Colors.white,
                                                                                                    fontSize:
                                                                                                        gl.display.equipixel *
                                                                                                        gl.fontSizeM *
                                                                                                        .75,
                                                                                                  ),
                                                                                                )
                                                                                                : gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                    "int"
                                                                                                ? Text(
                                                                                                  gl.geometries[gl.selectedGeometry].attributes[i].value.toString(),
                                                                                                  textAlign:
                                                                                                      TextAlign.start,
                                                                                                  style: TextStyle(
                                                                                                    color:
                                                                                                        Colors.white,
                                                                                                    fontSize:
                                                                                                        gl.display.equipixel *
                                                                                                        gl.fontSizeM *
                                                                                                        .75,
                                                                                                  ),
                                                                                                )
                                                                                                : gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                    "double"
                                                                                                ? Text(
                                                                                                  gl.geometries[gl.selectedGeometry].attributes[i].value.toString(),
                                                                                                  textAlign:
                                                                                                      TextAlign.start,
                                                                                                  style: TextStyle(
                                                                                                    color:
                                                                                                        Colors.white,
                                                                                                    fontSize:
                                                                                                        gl.display.equipixel *
                                                                                                        gl.fontSizeM *
                                                                                                        .75,
                                                                                                  ),
                                                                                                )
                                                                                                : gl.geometries[gl.selectedGeometry].attributes[i].type ==
                                                                                                    "special"
                                                                                                ? Text(
                                                                                                  "special value",
                                                                                                  style: TextStyle(
                                                                                                    color:
                                                                                                        Colors.white,
                                                                                                    fontSize:
                                                                                                        gl.display.equipixel *
                                                                                                        gl.fontSizeM *
                                                                                                        .75,
                                                                                                  ),
                                                                                                )
                                                                                                : Text(
                                                                                                  "ERROR TYPE ${gl.geometries[gl.selectedGeometry].attributes[i].type}",
                                                                                                  style: TextStyle(
                                                                                                    color:
                                                                                                        Colors.white,
                                                                                                    fontSize:
                                                                                                        gl.display.equipixel *
                                                                                                        gl.fontSizeM *
                                                                                                        .75,
                                                                                                  ),
                                                                                                ),
                                                                                      ),
                                                                                    ),
                                                                                  ),
                                                                                ),
                                                                              ],
                                                                            ),
                                                                            stroke(
                                                                              gl.display.equipixel,
                                                                              gl.display.equipixel *
                                                                                  .5,
                                                                              gl.colorAgroBioTech,
                                                                            ),
                                                                          ],
                                                                        );
                                                                      },
                                                                    ),
                                                              ),
                                                            ),
                                                          ),
                                                        ],
                                                      ),

                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                    ],
                                                  ),
                                                  Row(
                                                    mainAxisAlignment:
                                                        MainAxisAlignment
                                                            .spaceAround,
                                                    children: [
                                                      TextButton(
                                                        onPressed: () {
                                                          gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .attributes
                                                              .add(
                                                                pl.Attribute(
                                                                  name: "",
                                                                  type:
                                                                      "string",
                                                                  value: "",
                                                                ),
                                                              );
                                                          PopupNewAttribute(
                                                            context,
                                                            "",
                                                            gl.colorAgroBioTech,
                                                            (String s) {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .attributes
                                                                  .last
                                                                  .type = s;
                                                            },
                                                            (String s) {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .attributes
                                                                  .last
                                                                  .name = s;
                                                            },
                                                            (dynamic it) {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .attributes
                                                                  .last
                                                                  .value = it;
                                                            },
                                                            () {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .attributes
                                                                  .removeLast();
                                                            },
                                                            () {},
                                                          );
                                                        },
                                                        child: Column(
                                                          children: [
                                                            Icon(
                                                              Icons.add_circle,
                                                              color:
                                                                  gl.colorAgroBioTech,
                                                              size:
                                                                  gl.iconSizeS *
                                                                  gl
                                                                      .display
                                                                      .equipixel,
                                                            ),
                                                            Text(
                                                              "Une seule variable",
                                                              style: TextStyle(
                                                                color:
                                                                    gl.colorAgroBioTech,
                                                                fontSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.fontSizeXS,
                                                              ),
                                                            ),
                                                          ],
                                                        ),
                                                      ),
                                                      TextButton(
                                                        onPressed: () {
                                                          PopupSelectAttributeSet(
                                                            context,
                                                          );
                                                        },
                                                        child: Column(
                                                          children: [
                                                            Icon(
                                                              Icons
                                                                  .add_circle_outline_outlined,
                                                              color:
                                                                  gl.colorAgroBioTech,
                                                              size:
                                                                  gl.iconSizeS *
                                                                  gl
                                                                      .display
                                                                      .equipixel,
                                                            ),
                                                            Text(
                                                              "Un set de variables",
                                                              style: TextStyle(
                                                                color:
                                                                    gl.colorAgroBioTech,
                                                                fontSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.fontSizeXS,
                                                              ),
                                                            ),
                                                          ],
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                ],
                                              )
                                              : gl.Mode.editPointMarker
                                              ? Column(
                                                mainAxisAlignment:
                                                    MainAxisAlignment
                                                        .spaceBetween,
                                                children: [
                                                  Column(
                                                    children: [
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      Row(
                                                        children: [
                                                          Container(
                                                            alignment:
                                                                Alignment
                                                                    .topLeft,
                                                            child: SizedBox(
                                                              height:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeM *
                                                                  .9,
                                                              child: IconButton(
                                                                style:
                                                                    borderlessStyle,
                                                                iconSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS,
                                                                color:
                                                                    Colors
                                                                        .lightGreenAccent,
                                                                onPressed: () {
                                                                  refreshView(() {
                                                                    gl.Mode.editPointMarker =
                                                                        false;
                                                                  });
                                                                  gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .serialize();
                                                                },
                                                                icon: Icon(
                                                                  Icons
                                                                      .arrow_back,
                                                                  size:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.iconSizeS *
                                                                      .9,
                                                                ),
                                                              ),
                                                            ),
                                                          ),
                                                          Container(
                                                            alignment:
                                                                Alignment
                                                                    .topLeft,
                                                            width:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.chosenPolyBarWidth *
                                                                .75,
                                                            child: Text(
                                                              "Changez le symbole du point.",
                                                              textAlign:
                                                                  TextAlign
                                                                      .center,
                                                              style: TextStyle(
                                                                color:
                                                                    Colors
                                                                        .white,
                                                                fontSize:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.fontSizeS,
                                                              ),
                                                            ),
                                                          ),
                                                        ],
                                                      ),
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      SizedBox(
                                                        width:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.chosenPolyBarWidth,

                                                        child: SingleChildScrollView(
                                                          scrollDirection:
                                                              Axis.horizontal,
                                                          child: Row(
                                                            children: List<
                                                              Widget
                                                            >.generate(
                                                              gl
                                                                  .selectableIcons
                                                                  .length,
                                                              (k) {
                                                                return Container(
                                                                  color:
                                                                      gl.geometries[gl.selectedGeometry].selectedPointIcon ==
                                                                              k
                                                                          ? gl.colorAgroBioTech
                                                                          : Colors
                                                                              .transparent,
                                                                  child: SizedBox(
                                                                    height:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.iconSizeL,
                                                                    child: IconButton(
                                                                      onPressed: () {
                                                                        refreshView(() {
                                                                          gl.geometries[gl.selectedGeometry].selectedPointIcon =
                                                                              k;
                                                                        });
                                                                      },
                                                                      icon: Icon(
                                                                        gl.selectableIcons[k],
                                                                        size:
                                                                            gl.iconSizeM *
                                                                            gl.display.equipixel,
                                                                        color:
                                                                            Colors.white,
                                                                      ),
                                                                      color:
                                                                          Colors
                                                                              .white,
                                                                      iconSize:
                                                                          gl.display.equipixel *
                                                                          gl.iconSizeM,
                                                                    ),
                                                                  ),
                                                                );
                                                              },
                                                            ),
                                                          ),
                                                        ),
                                                      ),
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      SizedBox(
                                                        width:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.chosenPolyBarWidth,

                                                        child: SingleChildScrollView(
                                                          scrollDirection:
                                                              Axis.horizontal,
                                                          child: Row(
                                                            mainAxisAlignment:
                                                                MainAxisAlignment
                                                                    .spaceAround,
                                                            children: List<
                                                              TextButton
                                                            >.generate(
                                                              gl
                                                                  .predefinedPointSymbPalette
                                                                  .length,
                                                              (int k) {
                                                                return TextButton(
                                                                  onPressed: () {
                                                                    refreshView(() {
                                                                      gl.geometries[gl.selectedGeometry].setColorInside(
                                                                        gl.predefinedPointSymbPalette[k]
                                                                            .withAlpha(
                                                                              150,
                                                                            ),
                                                                      );
                                                                      gl.geometries[gl.selectedGeometry]
                                                                          .setColorLine(
                                                                            gl.predefinedPointSymbPalette[k],
                                                                          );
                                                                    });
                                                                  },
                                                                  child: CircleAvatar(
                                                                    backgroundColor:
                                                                        gl.geometries[gl.selectedGeometry].colorPolygon ==
                                                                                gl.predefinedPointSymbPalette[k]
                                                                            ? Colors.white
                                                                            : Colors.transparent,
                                                                    radius:
                                                                        gl
                                                                            .display
                                                                            .equipixel *
                                                                        gl.iconSizeXS *
                                                                        .9,
                                                                    child: CircleAvatar(
                                                                      radius:
                                                                          gl.display.equipixel *
                                                                          gl.iconSizeXS *
                                                                          .85,
                                                                      backgroundColor:
                                                                          gl.predefinedPointSymbPalette[k],
                                                                    ),
                                                                  ),
                                                                );
                                                              },
                                                            ),
                                                          ),
                                                        ),
                                                      ),
                                                      stroke(
                                                        gl.display.equipixel,
                                                        gl.display.equipixel *
                                                            .5,
                                                        gl.colorAgroBioTech,
                                                      ),
                                                      SizedBox(
                                                        width:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.chosenPolyBarWidth,
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.chosenPolyBarHeight *
                                                            .8,
                                                        child: Slider(
                                                          min: gl.iconSizeXXS,
                                                          max: gl.iconSizeL,
                                                          value:
                                                              gl.geometries[gl.selectedGeometry].iconSize >
                                                                          gl.iconSizeXXS &&
                                                                      gl.geometries[gl.selectedGeometry].iconSize <
                                                                          gl.iconSizeL
                                                                  ? gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .iconSize
                                                                  : 10.0,
                                                          divisions: 20,
                                                          activeColor:
                                                              gl.colorAgroBioTech,
                                                          onChanged: (
                                                            double value,
                                                          ) {
                                                            refreshView(() {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .iconSize = value;
                                                            });
                                                          },
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                ],
                                              )
                                              : Column(
                                                children: [
                                                  stroke(
                                                    gl.display.equipixel,
                                                    gl.display.equipixel * .5,
                                                    gl.colorAgroBioTech,
                                                  ),
                                                  Row(
                                                    children: [
                                                      SizedBox(
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSizeM *
                                                            .9,
                                                        child: IconButton(
                                                          style:
                                                              borderlessStyle,
                                                          iconSize:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.iconSizeS,
                                                          color:
                                                              Colors
                                                                  .lightGreenAccent,
                                                          onPressed: () {
                                                            refreshView(() {
                                                              gl.Mode.openToolbox =
                                                                  false;
                                                            });
                                                          },
                                                          icon: Icon(
                                                            Icons.arrow_back,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                          ),
                                                        ),
                                                      ),
                                                      SizedBox(
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSizeS *
                                                            .9,
                                                        child: Container(
                                                          alignment:
                                                              Alignment
                                                                  .centerLeft,
                                                          width:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.chosenPolyBarWidth *
                                                              .5,
                                                          child: Text(
                                                            "Boite à outils",
                                                            textAlign:
                                                                TextAlign
                                                                    .center,
                                                            style: TextStyle(
                                                              color:
                                                                  Colors.white,
                                                              fontSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.fontSizeM *
                                                                  .75,
                                                            ),
                                                          ),
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                  stroke(
                                                    gl.display.equipixel,
                                                    gl.display.equipixel * .5,
                                                    gl.colorAgroBioTech,
                                                  ),
                                                  Row(
                                                    mainAxisAlignment:
                                                        MainAxisAlignment
                                                            .spaceEvenly,
                                                    children: [
                                                      !gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .labelsVisibleOnMap
                                                          ? SizedBox(
                                                            height:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeM *
                                                                .9,
                                                            child: IconButton(
                                                              onPressed: () {
                                                                setState(() {
                                                                  gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .labelsVisibleOnMap = true;
                                                                });
                                                                gl
                                                                    .geometries[gl
                                                                        .selectedGeometry]
                                                                    .serialize();
                                                                gl
                                                                    .geometries[gl
                                                                        .selectedGeometry]
                                                                    .serialize();
                                                                gl.refreshMainStack(
                                                                  () {
                                                                    gl.modeMapShowPolygons =
                                                                        true;
                                                                  },
                                                                );
                                                              },
                                                              icon: Icon(
                                                                Icons.label,
                                                                size:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS *
                                                                    .9,
                                                                color:
                                                                    Colors
                                                                        .white,
                                                              ),
                                                            ),
                                                          )
                                                          : SizedBox(
                                                            height:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeM *
                                                                .9,
                                                            child: IconButton(
                                                              onPressed: () {
                                                                setState(() {
                                                                  gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .labelsVisibleOnMap = false;
                                                                });
                                                                gl
                                                                    .geometries[gl
                                                                        .selectedGeometry]
                                                                    .serialize();
                                                              },
                                                              icon: Icon(
                                                                Icons.label_off,
                                                                size:
                                                                    gl
                                                                        .display
                                                                        .equipixel *
                                                                    gl.iconSizeS *
                                                                    .9,
                                                                color:
                                                                    Colors
                                                                        .white,
                                                              ),
                                                            ),
                                                          ),
                                                      if (gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .points
                                                              .isNotEmpty &&
                                                          (!_positionInsideViewRectangle(
                                                                Position(
                                                                  longitude:
                                                                      gl
                                                                          .geometries[gl
                                                                              .selectedGeometry]
                                                                          .center
                                                                          .longitude,
                                                                  latitude:
                                                                      gl
                                                                          .geometries[gl
                                                                              .selectedGeometry]
                                                                          .center
                                                                          .latitude,
                                                                  timestamp:
                                                                      DateTime.now(),
                                                                  accuracy: 0,
                                                                  altitude: 0,
                                                                  altitudeAccuracy:
                                                                      0,
                                                                  heading: 0,
                                                                  headingAccuracy:
                                                                      0,
                                                                  speed: 0,
                                                                  speedAccuracy:
                                                                      0,
                                                                ),
                                                              ) ||
                                                              !gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .visibleOnMap))
                                                        SizedBox(
                                                          height:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.iconSizeM *
                                                              .9,
                                                          child: IconButton(
                                                            onPressed: () {
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .visibleOnMap = true;
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .serialize();
                                                              setState(() {
                                                                if (gl
                                                                            .geometries[gl.selectedGeometry]
                                                                            .center
                                                                            .longitude !=
                                                                        0.0 &&
                                                                    gl
                                                                            .geometries[gl.selectedGeometry]
                                                                            .center
                                                                            .latitude !=
                                                                        0.0) {
                                                                  _mapController.move(
                                                                    gl
                                                                        .geometries[gl
                                                                            .selectedGeometry]
                                                                        .center,
                                                                    _mapController
                                                                        .camera
                                                                        .zoom,
                                                                  );
                                                                }
                                                              });
                                                              gl.refreshMainStack(
                                                                () {
                                                                  gl.modeMapShowPolygons =
                                                                      true;
                                                                },
                                                              );
                                                            },
                                                            icon: Icon(
                                                              Icons.gps_fixed,
                                                              size:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS *
                                                                  .9,
                                                              opticalSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS,
                                                              color:
                                                                  Colors.white,
                                                            ),
                                                          ),
                                                        ),
                                                      if (gl
                                                          .geometries[gl
                                                              .selectedGeometry]
                                                          .type
                                                          .contains("Point"))
                                                        SizedBox(
                                                          height:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.iconSizeM *
                                                              .9,
                                                          child: IconButton(
                                                            onPressed: () {
                                                              setState(() {
                                                                gl.Mode.editPointMarker =
                                                                    !gl
                                                                        .Mode
                                                                        .editPointMarker;
                                                              });
                                                            },
                                                            icon: Icon(
                                                              FontAwesomeIcons
                                                                  .locationPin,
                                                              size:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.iconSizeS *
                                                                  .9,
                                                              color:
                                                                  Colors.white,
                                                            ),
                                                          ),
                                                        ),
                                                      SizedBox(
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSizeM *
                                                            .9,
                                                        child: IconButton(
                                                          onPressed: () {
                                                            gl.refreshMainStack(() {
                                                              gl.modeMapShowPolygons =
                                                                  true;
                                                              gl
                                                                  .geometries[gl
                                                                      .selectedGeometry]
                                                                  .visibleOnMap = true;
                                                              gl.Mode.editPolygon =
                                                                  !gl
                                                                      .Mode
                                                                      .editPolygon;
                                                              if (gl
                                                                      .Mode
                                                                      .editPolygon &&
                                                                  gl
                                                                      .geometries[gl
                                                                          .selectedGeometry]
                                                                      .type
                                                                      .contains(
                                                                        "Point",
                                                                      )) {
                                                                refreshView(() {
                                                                  gl.Mode.editPolygon =
                                                                      true;
                                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.showButtonMoveVertexesPolygon =
                                                                      true;
                                                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                                                      true;
                                                                  gl.Mode.addVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.moveVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.removeVertexesPolygon =
                                                                      false;
                                                                });
                                                              } else {
                                                                refreshView(() {
                                                                  gl.Mode.editPolygon =
                                                                      true;
                                                                  gl.Mode.showButtonAddVertexesPolygon =
                                                                      true;
                                                                  gl.Mode.showButtonMoveVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.showButtonRemoveVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.addVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.moveVertexesPolygon =
                                                                      false;
                                                                  gl.Mode.removeVertexesPolygon =
                                                                      false;
                                                                });
                                                              }
                                                            });
                                                          },
                                                          icon: FaIcon(
                                                            FontAwesomeIcons
                                                                .drawPolygon,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                            color: Colors.white,
                                                          ),
                                                        ),
                                                      ),
                                                      SizedBox(
                                                        height:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSizeM *
                                                            .9,
                                                        child: IconButton(
                                                          onPressed: () {
                                                            setState(() {
                                                              gl.Mode.editAttributes =
                                                                  !gl
                                                                      .Mode
                                                                      .editAttributes;
                                                            });
                                                          },
                                                          icon: FaIcon(
                                                            FontAwesomeIcons
                                                                .tableColumns,
                                                            size:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSizeS *
                                                                .9,
                                                            color: Colors.white,
                                                          ),
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                ],
                                              ),
                                        ],
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
                                          popupLayerListMenu(
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
                                                if (gl.geometries.isNotEmpty) {
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
                    ] +
                    gl.mainStack +
                    [
                      if (gl.modeDevelopper && gl.Mode.debugScanlines)
                        gridlines(),
                    ],
              );
            },
          ),
        ),
      ),
    );
  }

  double computePolygonTitleHeight() {
    double result = gl.chosenPolyBarHeight * .8;
    if (gl.Mode.openToolbox) {
      result += gl.chosenPolyBarHeight * 1.25;
      if (gl.Mode.editAttributes) {
        result += gl.attributeTableHeight * 1.3;
      } else if (gl.Mode.editPointMarker) {
        result += gl.chosenPolyBarHeight * 2;
      }
    }
    return result;
  }

  void _stopMovingSelectedPoint() {
    _selectedPointToMove = null;
  }

  List<Polygon<String>> _getPolygonesToDraw() {
    List<Polygon<String>> that = [];
    for (var layer in gl.geometries) {
      if (layer.numPoints > 2 &&
          layer.visibleOnMap &&
          layer.type == "Polygon") {
        that.add(
          Polygon<String>(
            points: layer.points,
            color: layer.colorInside,
            hitValue: layer.identifier,
          ),
        );
      }
    }
    return that;
  }

  List<Marker> _getPointsToDraw({bool hitButton = false}) {
    List<Marker> that = [];

    for (var layer in gl.geometries) {
      gl.selectableIcons[layer.selectedPointIcon];
      if (layer.visibleOnMap &&
          layer.numPoints > 0 &&
          layer.type.contains("Point")) {
        that.add(
          Marker(
            width: layer.iconSize * gl.display.equipixel,
            height: layer.iconSize * gl.display.equipixel,
            point: LatLng(
              layer.points.first.latitude,
              layer.points.first.longitude,
            ),
            child:
                hitButton
                    ? IconButton(
                      highlightColor: Colors.transparent,
                      alignment: Alignment.center,
                      style: ButtonStyle(
                        shape: WidgetStateProperty.fromMap(
                          <WidgetStatesConstraint, ContinuousRectangleBorder>{
                            WidgetState.any: ContinuousRectangleBorder(),
                          },
                        ),
                        alignment: AlignmentGeometry.center,
                        padding: WidgetStateProperty.fromMap(
                          <WidgetStatesConstraint, EdgeInsetsGeometry>{
                            WidgetState.any: EdgeInsetsGeometry.zero,
                          },
                        ),
                      ),
                      onPressed: () {
                        if (gl.Mode.openToolbox) {
                          setState(() {
                            int index = 0;
                            for (var it in gl.geometries) {
                              if (layer.identifier == it.identifier) {
                                setState(() {
                                  gl.selectedGeometry = index;
                                });
                              }
                              index++;
                            }
                          });
                        }
                      },
                      icon: Icon(
                        gl.selectableIcons[layer.selectedPointIcon],
                        size: layer.iconSize * gl.display.equipixel,
                        color: layer.colorLine,
                      ),
                    )
                    : Icon(
                      gl.selectableIcons[layer.selectedPointIcon],
                      size: layer.iconSize * gl.display.equipixel,
                      color: layer.colorLine,
                    ),
          ),
        );
      }
    }
    return that;
  }

  List<CircleMarker> _drawnLayerPointsCircleMarker() {
    if (gl.geometries.isEmpty) {
      return [];
    }
    List<CircleMarker> all = [];
    int i = 0;
    for (var point in gl.geometries[gl.selectedGeometry].points) {
      all.add(
        CircleMarker(
          point: point,
          radius:
              gl.geometries[gl.selectedGeometry].isSelectedLine(i) &&
                      !gl.Mode.showButtonMoveVertexesPolygon &&
                      gl.Mode.addVertexesPolygon
                  ? iconSize / 2.7
                  : iconSize / 3,
          color:
              gl.geometries[gl.selectedGeometry].isSelectedLine(i) &&
                      !gl.Mode.showButtonMoveVertexesPolygon &&
                      gl.Mode.addVertexesPolygon
                  ? gl.geometries[gl.selectedGeometry].colorLine
                  : gl.geometries[gl.selectedGeometry].colorInside,
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
    if (gl.geometries.isEmpty) {
      return [];
    }
    List<Marker> all = [];
    int count = 0;
    for (var point in gl.geometries[gl.selectedGeometry].points) {
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
                  gl.geometries[gl.selectedGeometry].refreshSelectedLinePoints(
                    point,
                  );
                });
              } else if (gl.Mode.moveVertexesPolygon) {
                refreshView(() {
                  if (gl.geometries[gl.selectedGeometry].selectedVertex !=
                      1 + count) {
                    gl.geometries[gl.selectedGeometry].selectedVertex = count;
                  } else {
                    gl.geometries[gl.selectedGeometry].selectedVertex = -1;
                  }
                });
              } else {
                refreshView(() {
                  gl.geometries[gl.selectedGeometry].refreshSelectedLinePoints(
                    point,
                  );
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
    _stopMovingSelectedPoint();
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
    gl.mainStackPopLast();
  }

  Widget _mainMenuBar({bool dummy = false, VoidCallback? close}) {
    return Container(
      alignment: Alignment.center,
      width: gl.display.equipixel * gl.menuBarLength,
      height: gl.display.equipixel * gl.menuBarThickness,
      child: Card(
        shadowColor: Colors.transparent,
        color: dummy ? Colors.transparent : gl.backgroundTransparentBlackBox,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
            _menuButton(
              gl.display.equipixel * gl.menuBarLength / 4,
              gl.display.equipixel * gl.menuBarThickness,
              _toolbarExtended,
              Colors.green,
              () {
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
              () {
                refreshView(() {
                  _closePolygonMenu();
                });
              },
              Icon(
                Icons.forest,
                size: gl.display.equipixel * gl.menuBarLength / 5,
              ),
            ),
            _menuButton(
              gl.display.equipixel * gl.menuBarLength / 4,
              gl.display.equipixel * gl.menuBarThickness,
              gl.Mode.polygon,
              Colors.yellow,
              () {
                setState(() {
                  gl.Mode.polygon = true;
                  _closeSwitchesMenu();
                  _closeToolbarMenu();
                  if (dummy) {
                    close!();
                  }
                  gl.mainStack.add(
                    popupLayerListMenu(
                      gl.notificationContext!,
                      "",
                      (LatLng pos) {
                        if (pos.longitude != 0.0 && pos.latitude != 0.0) {
                          _mapController.move(pos, _mapController.camera.zoom);
                        }
                      },
                      () {
                        refreshView(() {
                          if (gl.geometries.isNotEmpty && gl.Mode.editPolygon) {
                            if (gl.geometries[gl.selectedGeometry].type ==
                                "Polygon") {
                              gl.Mode.showButtonAddVertexesPolygon = true;
                              gl.Mode.showButtonMoveVertexesPolygon = false;
                              gl.Mode.showButtonRemoveVertexesPolygon = false;
                            } else if (gl.geometries[gl.selectedGeometry].type
                                .contains("Point")) {
                              gl
                                  .geometries[gl.selectedGeometry]
                                  .selectedVertex = 0;
                              gl.Mode.showButtonAddVertexesPolygon = false;
                              gl.Mode.showButtonMoveVertexesPolygon = true;
                              gl.Mode.showButtonRemoveVertexesPolygon = true;
                            }
                          }
                        });
                      },
                    ),
                  );
                });
              },
              () {
                refreshView(() {
                  _closePolygonMenu();
                });
              },
              Icon(
                Icons.hexagon_outlined,
                size: gl.display.equipixel * gl.menuBarLength / 5,
              ),
            ),
            _menuButton(
              gl.display.equipixel * gl.menuBarLength / 4,
              gl.display.equipixel * gl.menuBarThickness,
              _modeLayerSwitches,
              Colors.brown,
              () {
                setState(() {
                  if (!_modeLayerSwitches) {
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
                          return _mainMenuBar(dummy: true, close: close);
                        },
                        (LatLng pos) {
                          if (pos.longitude != 0.0 && pos.latitude != 0.0) {
                            _mapController.move(
                              pos,
                              _mapController.camera.zoom,
                            );
                          }
                        },
                      ),
                    );
                  } else {
                    setState(() {
                      _modeLayerSwitches = false;
                      _closeSwitchesMenu();
                    });
                  }
                });
              },
              () {
                refreshView(() {
                  _closeSwitchesMenu();
                });
              },
              Icon(
                Icons.remove_red_eye,
                size: gl.display.equipixel * gl.menuBarLength / 5,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _menuButton(
    double width,
    double height,
    bool isSelected,
    Color color,
    VoidCallback onPressed,
    VoidCallback onLongPress,
    Icon icon,
  ) {
    return Container(
      width: width,
      height: height,
      color: !isSelected ? Colors.transparent : color.withAlpha(128),
      child: IconButton(
        style: ButtonStyle(
          shape: WidgetStateProperty.fromMap(
            <WidgetStatesConstraint, ContinuousRectangleBorder>{
              WidgetState.any: ContinuousRectangleBorder(),
            },
          ),
          alignment: AlignmentGeometry.center,
          padding: WidgetStateProperty.fromMap(
            <WidgetStatesConstraint, EdgeInsetsGeometry>{
              WidgetState.any: EdgeInsetsGeometry.zero,
            },
          ),
        ),
        color: isSelected ? Colors.white : color,
        onPressed: onPressed,
        onLongPress: onLongPress,
        icon: icon,
      ),
    );
  }

  Widget _toolBar() {
    double toolbarHeight = gl.iconSizeM * 2 + gl.iconSpaceBetween * 2;
    if (gl.modeDevelopper) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    if (_positionInsideViewRectangle(gl.position)) {
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
                    gl.positionInit
                        ? Card(
                          color: Colors.orange.withAlpha(128),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.end,
                            children: [
                              if (_positionInsideViewRectangle(gl.position))
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
                                            x: gl.position.longitude,
                                            y: gl.position.latitude,
                                          ),
                                        ),
                                      );
                                      _updatePtMarker(
                                        LatLng(
                                          gl.position.latitude,
                                          gl.position.longitude,
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
                                  if (gl.positionInit) {
                                    refreshView(() {
                                      _mapController.move(
                                        LatLng(
                                          gl.position.latitude,
                                          gl.position.longitude,
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
                                if (gl.positionInit) {
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
                                    gl.geometries[gl.selectedGeometry].name,
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

  List<Marker> _getPolygonesLabels() {
    return List.generate(gl.geometries.length, (i) {
      String textArea = "${(gl.geometries[i].area / 100).round() / 100} Ha";
      return gl.geometries[i].visibleOnMap &&
              gl.geometries[i].labelsVisibleOnMap
          ? gl.geometries[i].type == "Polygon"
              ? Marker(
                alignment: Alignment.center,
                width:
                    gl.display.equipixel *
                    (gl.Mode.smallLabel
                        ? gl.infoBoxPolygon * .6
                        : gl.infoBoxPolygon),
                height:
                    gl.display.equipixel *
                        (gl.geometries[i].getNCheckedAttributes() + 1) *
                        gl.iconSizeS *
                        .8 +
                    5,
                point: gl.geometries[i].center,
                child: Card(
                  color:
                      gl.Mode.smallLabel
                          ? Colors.white.withAlpha(100)
                          : Colors.black.withAlpha(200),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children:
                        <Widget>[
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                            children: [
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.all(2),
                                  color: Colors.transparent,
                                  height: gl.display.equipixel * gl.iconSizeXS,
                                  width: gl.display.equipixel * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<
                                        Color
                                      >.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<
                                        EdgeInsetsGeometry
                                      >.fromMap(<
                                        WidgetStatesConstraint,
                                        EdgeInsetsGeometry
                                      >{
                                        WidgetState.any:
                                            EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {},
                                    icon: FaIcon(
                                      FontAwesomeIcons.list,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeXS *
                                          .5,
                                      color: Colors.transparent,
                                    ),
                                  ),
                                ),
                              Container(
                                padding: EdgeInsets.all(2),
                                alignment: Alignment.center,
                                color: Colors.transparent,
                                height: gl.display.equipixel * gl.iconSizeXS,
                                width:
                                    gl.display.equipixel *
                                    gl.infoBoxPolygon /
                                    2,

                                child: SingleChildScrollView(
                                  scrollDirection: Axis.horizontal,
                                  child: Text(
                                    gl.geometries[i].name,
                                    textAlign: TextAlign.center,
                                    style: TextStyle(
                                      color:
                                          gl.Mode.smallLabel
                                              ? Colors.black
                                              : Colors.white,
                                      fontSize:
                                          gl.display.equipixel * gl.fontSizeXS,
                                    ),
                                  ),
                                ),
                              ),
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.all(0),
                                  color: Colors.transparent,
                                  height: gl.display.equipixel * gl.iconSizeXS,
                                  width: gl.display.equipixel * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<
                                        Color
                                      >.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<
                                        EdgeInsetsGeometry
                                      >.fromMap(<
                                        WidgetStatesConstraint,
                                        EdgeInsetsGeometry
                                      >{
                                        WidgetState.any:
                                            EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {
                                      refreshView(() {
                                        gl.geometries[i].labelsVisibleOnMap =
                                            false;
                                      });
                                      gl.geometries[i].serialize();
                                    },
                                    icon: FaIcon(
                                      Icons.close,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeXS *
                                          .8,
                                      color: Colors.red,
                                    ),
                                  ),
                                ),
                            ],
                          ),
                          if (!gl.Mode.smallLabel &&
                              gl.geometries[i].getNCheckedAttributes() > 1)
                            stroke(
                              gl.display.equipixel * 0.5,
                              gl.display.equipixel * 0.25,
                              gl.colorAgroBioTech,
                            ),
                        ] +
                        List<Widget>.generate(
                          gl.geometries[i].getNCheckedAttributes(),
                          (j) {
                            return Container(
                              padding: EdgeInsetsDirectional.symmetric(
                                horizontal: gl.display.equipixel * 2,
                              ),
                              color: Colors.transparent,
                              height: gl.display.equipixel * gl.iconSizeXS,
                              child: Row(
                                mainAxisAlignment:
                                    MainAxisAlignment.spaceEvenly,
                                children: [
                                  if (!gl.Mode.smallLabel)
                                    Container(
                                      alignment: Alignment.centerLeft,
                                      width: gl.display.equipixel * 15,
                                      child: SingleChildScrollView(
                                        scrollDirection: Axis.horizontal,
                                        child: Text(
                                          gl.geometries[i].attributes[j].name,
                                          textAlign: TextAlign.center,
                                          style: TextStyle(
                                            color:
                                                gl.Mode.smallLabel
                                                    ? Colors.black
                                                    : Colors.white,
                                            fontSize:
                                                gl.display.equipixel *
                                                gl.fontSizeXS,
                                          ),
                                        ),
                                      ),
                                    ),
                                  if (!gl.Mode.smallLabel)
                                    stroke(
                                      vertical: true,
                                      gl.display.equipixel * 0.5,
                                      gl.display.equipixel * 0.25,
                                      gl.colorAgroBioTech,
                                    ),
                                  Container(
                                    alignment: Alignment.centerLeft,
                                    width: gl.display.equipixel * 15,
                                    child: SingleChildScrollView(
                                      scrollDirection: Axis.horizontal,
                                      child: Text(
                                        gl.geometries[i].attributes[j].value
                                            .toString(),
                                        textAlign: TextAlign.center,
                                        style: TextStyle(
                                          color:
                                              gl.Mode.smallLabel
                                                  ? Colors.black
                                                  : Colors.white,
                                          fontSize:
                                              gl.display.equipixel *
                                              gl.fontSizeXS,
                                        ),
                                      ),
                                    ),
                                  ),
                                ],
                              ),
                            );
                          },
                        ),
                  ),
                ),
              )
              : gl.geometries[i].type.contains("Point")
              ? Marker(
                alignment: Alignment.bottomLeft,
                width:
                    gl.display.equipixel *
                    (gl.Mode.smallLabel
                        ? gl.infoBoxPolygon * .6
                        : gl.infoBoxPolygon),
                height:
                    gl.display.equipixel *
                        (gl.geometries[i].getNCheckedAttributes() +
                            ((!gl.geometries[i].type.contains("essence"))
                                ? 1
                                : 0)) *
                        gl.iconSizeS *
                        .8 +
                    5,
                point: gl.geometries[i].center,
                child: Card(
                  color:
                      gl.Mode.smallLabel
                          ? Colors.white.withAlpha(100)
                          : Colors.black.withAlpha(200),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children:
                        <Widget>[
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                            children: [
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.all(2),
                                  color: Colors.transparent,
                                  height: gl.display.equipixel * gl.iconSizeXS,
                                  width: gl.display.equipixel * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<
                                        Color
                                      >.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<
                                        EdgeInsetsGeometry
                                      >.fromMap(<
                                        WidgetStatesConstraint,
                                        EdgeInsetsGeometry
                                      >{
                                        WidgetState.any:
                                            EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {},
                                    icon: FaIcon(
                                      FontAwesomeIcons.list,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeXS *
                                          .5,
                                      color: Colors.transparent,
                                    ),
                                  ),
                                ),
                              if (!gl.geometries[i].type.contains("essence"))
                                Container(
                                  padding: EdgeInsets.all(2),
                                  alignment: Alignment.center,
                                  color: Colors.transparent,
                                  height: gl.display.equipixel * gl.iconSizeXS,
                                  width:
                                      gl.display.equipixel *
                                      gl.infoBoxPolygon /
                                      2,
                                  child: SingleChildScrollView(
                                    scrollDirection: Axis.horizontal,
                                    child: Text(
                                      gl.geometries[i].name,
                                      textAlign: TextAlign.center,
                                      style: TextStyle(
                                        color:
                                            gl.Mode.smallLabel
                                                ? Colors.black
                                                : Colors.white,
                                        fontSize:
                                            gl.display.equipixel *
                                            gl.fontSizeXS,
                                      ),
                                    ),
                                  ),
                                ),
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.all(0),
                                  color: Colors.transparent,
                                  height: gl.display.equipixel * gl.iconSizeXS,
                                  width: gl.display.equipixel * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<
                                        Color
                                      >.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<
                                        EdgeInsetsGeometry
                                      >.fromMap(<
                                        WidgetStatesConstraint,
                                        EdgeInsetsGeometry
                                      >{
                                        WidgetState.any:
                                            EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {
                                      refreshView(() {
                                        gl.geometries[i].labelsVisibleOnMap =
                                            false;
                                      });
                                      gl.geometries[i].serialize();
                                    },
                                    icon: FaIcon(
                                      Icons.close,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeXS *
                                          .8,
                                      color: Colors.red,
                                    ),
                                  ),
                                ),
                            ],
                          ),
                          if (gl.geometries[i].getNCheckedAttributes() > 1)
                            stroke(
                              gl.display.equipixel * 0.5,
                              gl.display.equipixel * 0.25,
                              gl.colorAgroBioTech,
                            ),
                        ] +
                        List<Widget>.generate(
                          gl.geometries[i].getNCheckedAttributes(),
                          (j) {
                            return Container(
                              padding: EdgeInsetsDirectional.symmetric(
                                horizontal: gl.display.equipixel * 2,
                              ),
                              color: Colors.transparent,
                              height: gl.display.equipixel * gl.iconSizeXS,
                              child: Row(
                                mainAxisAlignment:
                                    MainAxisAlignment.spaceEvenly,
                                children: [
                                  if (!gl.Mode.smallLabel)
                                    Container(
                                      alignment: Alignment.centerLeft,
                                      width: gl.display.equipixel * 15,
                                      child: SingleChildScrollView(
                                        scrollDirection: Axis.horizontal,
                                        child: Text(
                                          gl.geometries[i].attributes[j].name,
                                          textAlign: TextAlign.center,
                                          style: TextStyle(
                                            color: Colors.white,
                                            fontSize:
                                                gl.display.equipixel *
                                                gl.fontSizeXS,
                                          ),
                                        ),
                                      ),
                                    ),
                                  if (!gl.Mode.smallLabel)
                                    stroke(
                                      vertical: true,
                                      gl.display.equipixel * 0.5,
                                      gl.display.equipixel * 0.25,
                                      gl.colorAgroBioTech,
                                    ),
                                  Container(
                                    alignment: Alignment.centerLeft,
                                    width: gl.display.equipixel * 15,
                                    child: SingleChildScrollView(
                                      scrollDirection: Axis.horizontal,
                                      child: Text(
                                        gl.geometries[i].attributes[j].value
                                            .toString(),
                                        textAlign: TextAlign.center,
                                        style: TextStyle(
                                          color:
                                              gl.Mode.smallLabel
                                                  ? Colors.black
                                                  : Colors.white,
                                          fontSize:
                                              gl.display.equipixel *
                                              gl.fontSizeXS,
                                        ),
                                      ),
                                    ),
                                  ),
                                ],
                              ),
                            );
                          },
                        ),
                  ),
                ),
              )
              : Marker(
                alignment: Alignment.center,
                width:
                    textArea.length > gl.geometries[i].name.length
                        ? gl.display.equipixel * gl.infoBoxPolygon * 2.5 +
                            textArea.length * gl.fontSizeS
                        : gl.display.equipixel * gl.infoBoxPolygon * 1.5 +
                            gl.geometries[i].name.length * gl.fontSizeS,
                height: gl.display.equipixel * gl.infoBoxPolygon * 1.5,
                point: gl.geometries[i].center,
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
                                  color: gl.geometries[i].colorLine,
                                ),
                                Text(
                                  "Unknown Geometry",
                                  overflow: TextOverflow.clip,
                                ),
                              ],
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
              )
          : Marker(
            alignment: Alignment.center,
            width: 1,
            height: 1,
            point: gl.geometries[i].center,
            child: SizedBox(),
          );
    });
  }

  Widget _placeholder({BoxConstraints? constraints, Alignment? alignement}) {
    return Container(constraints: constraints, alignment: alignement);
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
          return gl.position;
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

  Column _getFixedAttribute(
    String name,
    String values, {
    bool checked = false,
  }) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            Container(
              alignment: Alignment.center,
              width: gl.display.equipixel * 11,
              child: Text(
                "FIXED",
                style: TextStyle(
                  color: Colors.grey,
                  fontSize: gl.fontSizeXXS * gl.display.equipixel,
                ),
              ),
            ),
            stroke(
              vertical: true,
              gl.display.equipixel,
              gl.display.equipixel * 0.5,
              gl.colorAgroBioTech,
            ),
            SizedBox(
              width: gl.display.equipixel * 7,
              child:
                  checked
                      ? Icon(
                        Icons.check_box,
                        color: Colors.white12,
                        size: gl.display.equipixel * gl.iconSizeXS,
                      )
                      : Icon(
                        Icons.check_box_outline_blank,
                        color: Colors.white12,
                        size: gl.display.equipixel * gl.iconSizeXS,
                      ),
            ),
            stroke(
              vertical: true,
              gl.display.equipixel,
              gl.display.equipixel * 0.5,
              gl.colorAgroBioTech,
            ),
            Container(
              alignment: Alignment.centerLeft,
              width: gl.display.equipixel * 33,
              child: Text(
                name,
                style: TextStyle(
                  color: Colors.white,
                  fontSize: gl.display.equipixel * gl.fontSizeM * .75,
                ),
              ),
            ),
            stroke(
              vertical: true,
              gl.display.equipixel,
              gl.display.equipixel * 0.5,
              gl.colorAgroBioTech,
            ),
            Container(
              alignment: Alignment.centerLeft,
              width: gl.display.equipixel * 33,
              child: SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: Text(
                  values,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: gl.display.equipixel * gl.fontSizeM * .75,
                  ),
                ),
              ),
            ),
          ],
        ),
        stroke(
          gl.display.equipixel,
          gl.display.equipixel * 0.5,
          gl.colorAgroBioTech,
        ),
      ],
    );
  }

  bool _positionInsideViewRectangle(Position p) => _mapController
      .camera
      .visibleBounds
      .contains(LatLng(p.latitude, p.longitude));
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
