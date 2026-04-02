import 'dart:async';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/myicons.dart';
import 'package:fforestimator/tileProvider/tif_tile_provider.dart';
import 'package:fforestimator/tools/layout_tools.dart' as lt;
import 'package:fforestimator/tools/geometry/geometry.dart' as ge;
import 'package:fforestimator/tools/geometry_layer.dart';
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

class ForestimatorMap extends StatefulWidget {
  const ForestimatorMap({super.key});

  @override
  State<ForestimatorMap> createState() => _ForestimatorMapState();
}

class _ForestimatorMapState extends State<ForestimatorMap> {
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

  Color _polygonMenuColorTools(bool choice) => choice ? Colors.lightGreenAccent.withAlpha(128) : Colors.transparent;

  LatLng? _selectedPointToMove;
  double iconSize = 50.0;
  ScrollController propertiesTableScrollController = ScrollController();

  Offset get _layToolBoxAnimOnScreenPos =>
      Offset(gl.dsp.alignX(gl.dsp.orientation == Orientation.landscape ? gl.dsp.eqAlignLeft : 0), gl.dsp.alignY(gl.dsp.eqAlignTop));
  Offset get _layToolBoxAnimOffScreenPos =>
      Offset(gl.dsp.alignX(gl.dsp.orientation == Orientation.landscape ? gl.dsp.eqAlignLeft : 0), gl.dsp.alignY(-250));
  Offset get _layToolBoxAnimUnderListPos => Offset(
    gl.dsp.alignX(gl.dsp.orientation == Orientation.landscape ? gl.dsp.eqAlignLeft : 0),
    gl.dsp.alignY(gl.eqPxH / 2.0 + computePolygonTitleHeight() * 3),
  );

  Offset get _mainMenuSettingsAnimOnScreenPos => Offset(gl.dsp.alignX(gl.dsp.eqAlignLeft), gl.dsp.alignY(gl.dsp.eqAlignTop));
  Offset get _mainMenuSettingsAnimOffScreenPos => Offset(gl.dsp.alignX(-gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignTop));

  Offset get _mainMenuEssenceAnimOnScreenPos => Offset(gl.dsp.alignX(gl.dsp.eqAlignRight), gl.dsp.alignY(gl.dsp.eqAlignBottom - 20));
  Offset get _mainMenuEssenceAnimOffScreenPos => Offset(gl.dsp.alignX(gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignBottom - 20));

  Offset get _mainMenuFinishAnimOnScreenPos => Offset(gl.dsp.alignX(gl.dsp.eqAlignRight), gl.dsp.alignY(gl.dsp.eqAlignBottom - 40));
  Offset get _mainMenuFinishAnimOffScreenPos => Offset(gl.dsp.alignX(gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignBottom - 40));

  Offset get _mainMenuWarningsAnimOnScreenPos => Offset(gl.dsp.alignX(gl.dsp.eqAlignRight), gl.dsp.alignY(gl.dsp.eqAlignTop));
  Offset get _mainMenuWarningsAnimOffScreenPos => Offset(gl.dsp.alignX(-gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignTop));

  Offset get _mainMenuOnOfflineAnimOnScreenPos => Offset(gl.dsp.alignX(0), gl.dsp.alignY(gl.dsp.eqAlignTop));
  Offset get _mainMenuOnOfflineAnimOffScreenPos => Offset(gl.dsp.alignX(-2 * gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignTop));

  Offset get _anaToolbarAnimOnScreenPos => Offset(gl.dsp.alignX(gl.dsp.eqAlignLeft), gl.dsp.alignY(gl.dsp.eqAlignBottom - gl.menuBarThickness));
  Offset get _anaToolbarAnimOffScreenPos => Offset(gl.dsp.alignX(-gl.eqPxW), gl.dsp.alignY(gl.dsp.eqAlignBottom - gl.menuBarThickness));

  Offset get _mainmenuBarPos => Offset(gl.dsp.alignX(0), gl.dsp.alignY(gl.dsp.eqAlignBottom));

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

        String request = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/$layersAnaPt/x/${ptBL72.x}/y/${ptBL72.y}";
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
        gl.requestedLayers.removeWhere((element) => element.mFoundLayer == false);
      } else {
        gl.stack.add(
          "NoInternet",
          popupNoInternet(() {
            mounted
                ? setState(() {
                  gl.stack.pop("NoInternet");
                })
                : gl.stack.pop("NoInternet");
          }),
          Duration(milliseconds: 400),
          Offset.zero,
          Offset(0, -250),
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
    gl.requestedLayers.sort((a, b) => gl.dico.getLayerBase(a.mCode).mGroupe.compareTo(gl.dico.getLayerBase(b.mCode).mGroupe));
  }

  TifFileTileProvider? _provider;

  @override
  void initState() {
    super.initState();
    initPermissions();
    gl.refreshStack = refreshView;
    initOtherValuesOnce();
  }

  void refreshView(void Function() f) async {
    _mapFrameCounter++;
    mounted ? setState(f) : f();
  }

  LatLng latlonBL = LatLng(0.0, 0.0);
  LatLng latlonTR = LatLng(0.0, 0.0);

  void initOtherValuesOnce() {
    proj4.Point ptBotLeft = proj4.Point(x: epsg31370Bounds.bottomLeft.dx, y: epsg31370Bounds.bottomLeft.dy);
    proj4.Point ptTopR = proj4.Point(x: epsg31370Bounds.topRight.dx, y: epsg31370Bounds.topRight.dy);

    // contraindre la vue de la map sur la zone de la Wallonie. ajout d'un peu de marge
    double margeInDegree = 0.1;
    latlonBL = LatLng(epsg31370.transform(epsg4326, ptBotLeft).y + margeInDegree, epsg31370.transform(epsg4326, ptBotLeft).x - margeInDegree);
    latlonTR = LatLng(epsg31370.transform(epsg4326, ptTopR).y - margeInDegree, epsg31370.transform(epsg4326, ptTopR).x + margeInDegree);
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
                            if (!gl.Mode.polygon) {
                              _lastPressWasShort = false;
                              if (!_doingAnaPt) {
                                refreshView(() {
                                  _doingAnaPt = true;
                                });
                                await _runAnaPt(epsg4326.transform(epsg31370, proj4.Point(x: point.longitude, y: point.latitude)));
                                _pt = point;
                                gl.refreshStack(() {
                                  popupForestimatorWindow(
                                    id: "anaPres",
                                    title: "Resultats de l'analyse",
                                    child: AnaResultsMenu(() {
                                      gl.refreshStack(() {});
                                    }, gl.requestedLayers),
                                  );
                                });
                                refreshView(() {
                                  _doingAnaPt = false;
                                });
                              }
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
                                  : gl.Mode.moveVertexesPolygon
                                  ? (tapPosition, point) async => {
                                    refreshView(() {
                                      _stopMovingSelectedPoint();
                                      if (gl.selGeo.type.contains("Point") && gl.selGeo.numPoints == 1) {
                                        refreshView(() {
                                          gl.selGeo.selectedVertex = 0;
                                          gl.Mode.showButtonAddVertexesPolygon = false;
                                          gl.Mode.showButtonMoveVertexesPolygon = true;
                                          gl.Mode.showButtonRemoveVertexesPolygon = true;
                                          gl.Mode.addVertexesPolygon = false;
                                          gl.Mode.moveVertexesPolygon = false;
                                          gl.Mode.removeVertexesPolygon = false;
                                        });
                                      } else {
                                        gl.Mode.showButtonAddVertexesPolygon = true;
                                        gl.Mode.showButtonMoveVertexesPolygon = false;
                                        gl.Mode.showButtonRemoveVertexesPolygon = false;
                                        gl.Mode.addVertexesPolygon = false;
                                        gl.Mode.moveVertexesPolygon = false;
                                        gl.Mode.removeVertexesPolygon = false;
                                      }
                                    }),
                                  }
                                  : gl.Mode.polygon
                                  ? (tapPosition, point) async => {}
                                  : (tapPosition, point) async => {_lastPressWasShort = true, _updatePtMarker(point)},
                          onPositionChanged: (position, e) async {
                            if (!e) return;
                            _mapControllerInit = true;
                            updateLocation();
                            if (_modeMoveMeasurePath) {
                              _measurePath.removeAt(selectedMeasurePointToMove);
                              _measurePath.insert(selectedMeasurePointToMove, LatLng(position.center.latitude, position.center.longitude));
                            }
                            if (_selectedPointToMove != null) {
                              if (_isPolygonWellDefined(
                                gl.selGeo.getPolyMoveOneVertex(_selectedPointToMove!, LatLng(position.center.latitude, position.center.longitude)),
                              )) {
                                gl.selGeo.replacePoint(_selectedPointToMove!, LatLng(position.center.latitude, position.center.longitude));
                                refreshView(() {
                                  _selectedPointToMove = LatLng(position.center.latitude, position.center.longitude);
                                });
                              }
                            } else {
                              refreshView(() {});
                            }
                            _writePositionDataToSharedPreferences(position.center.longitude, position.center.latitude, position.zoom);
                          },
                          crs: epsg31370CRS,
                          initialZoom: (gl.globalMinZoom + gl.globalMaxZoom) / 2.0,
                          maxZoom: gl.globalMaxZoom,
                          minZoom: gl.globalMinZoom,
                          initialCenter: gl.latlonCenter,
                          cameraConstraint: CameraConstraint.containCenter(bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
                          onMapReady: () async {
                            updateLocation();
                            if (gl.positionInit) {
                              refreshView(() {
                                centerOnLatLng(LatLng(gl.position.latitude, gl.position.longitude));
                              });
                              // si on refusait d'allumer le GPS, alors la carte ne s'affichait jamais, c'est pourquoi il y a le else et le code ci-dessous
                            } else {
                              refreshView(() {
                                centerOnLatLng(gl.latlonCenter);
                              });
                            }
                          },
                        ),
                        children:
                            gl.getLayersForFlutterMap().map<Widget>((gl.SelectedLayer selLayer) {
                              if (selLayer.offline &&
                                  gl.dico.getLayerBase(selLayer.mCode).mOffline &&
                                  (selLayer.mCode == gl.getFirstSelLayOffline())) {
                                if (_provider == null || _provider?.layerCode != selLayer.mCode) {
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
                                            minHeight: gl.eqPx * gl.loadingMapBoxHeight,
                                            minWidth: gl.eqPx * gl.loadingMapBoxWidth,
                                            maxWidth: gl.eqPx * gl.loadingMapBoxWidth,
                                          ),
                                          child: Row(
                                            mainAxisAlignment: MainAxisAlignment.center,
                                            children: [
                                              SizedBox(
                                                width: gl.eqPx * gl.loadingMapBoxWidth * .7,
                                                child: Text(
                                                  "La carte choisie est en préparation dans la mémoire.",
                                                  textAlign: TextAlign.center,
                                                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeS),
                                                ),
                                              ),
                                              SizedBox(width: gl.eqPx * gl.fontSizeS),
                                              CircularProgressIndicator(
                                                constraints: BoxConstraints(minHeight: gl.eqPx * gl.fontSizeXL, minWidth: gl.eqPx * gl.fontSizeXL),
                                                color: gl.colorAgroBioTech,
                                                strokeWidth: gl.eqPx * gl.fontSizeS * .5,
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
                                      gl.modeMapFirstTileLayerTransparancy && i > 1 && gl.getLayersForFlutterMap().length == i
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
                                  latitude: gl.position.latitude,
                                  longitude: gl.position.longitude,
                                  accuracy: gl.position.accuracy,
                                ),
                              ),
                            ] +
                            (gl.Mode.editPolygon
                                ? <Widget>[
                                  if (gl.selLay.geometries.isNotEmpty &&
                                      gl.selGeo.points.length > 1 &&
                                      !gl.Mode.showButtonMoveVertexesPolygon &&
                                      gl.Mode.addVertexesPolygon)
                                    PolylineLayer(
                                      polylines: [
                                        Polyline(
                                          points: [
                                            gl.selGeo.points[gl.selGeo.selectedPolyLinePoints[0]],
                                            gl.selGeo.points[gl.selGeo.selectedPolyLinePoints[1]],
                                          ],
                                          color: gl.selGeo.colorLine,
                                          strokeWidth: 5.0,
                                        ),
                                      ],
                                    ),
                                  MarkerLayer(markers: _getPointsToDraw()),
                                  PolygonLayer(polygons: _getPolygonesToDraw()),
                                ]
                                : (gl.Mode.polygon)
                                ? <Widget>[
                                  MouseRegion(
                                    hitTestBehavior: HitTestBehavior.deferToChild,
                                    cursor: SystemMouseCursors.click,
                                    child: GestureDetector(
                                      onTap: () {
                                        final LayerHitResult<String>? result = hitNotifier.value;
                                        if (result == null) return;
                                        int index = 0;
                                        if (gl.Mode.polygon) {
                                          for (var it in gl.selLay.geometries) {
                                            if (result.hitValues.first == it.identifier) {
                                              setState(() {
                                                gl.selLay.selectedGeometry = index;
                                              });
                                            }
                                            index++;
                                          }
                                        }
                                      },
                                      child: PolygonLayer<String>(hitNotifier: hitNotifier, polygons: _getPolygonesToDraw()),
                                    ),
                                  ),
                                  MarkerLayer(markers: _getPointsToDraw(hitButton: true)),
                                ]
                                : gl.modeMapShowPolygons
                                ? <Widget>[
                                  PolygonLayer<String>(polygons: _getPolygonesToDraw()),
                                  MarkerLayer(markers: _getPointsToDraw(hitButton: false)),
                                ]
                                : <Widget>[]) +
                            <Widget>[
                              MarkerLayer(markers: _placeVertexMovePointer() + _placeAnaPtMarker()),
                              if (gl.modeMapShowPolygons) MarkerLayer(markers: _getPolygonesLabels()),
                              if (gl.modeMapShowCustomMarker) MarkerLayer(markers: []),
                              if (gl.modeMapShowSearchMarker) MarkerLayer(markers: _placeSearchMarker()),
                              if (_modeMeasurePath && _measurePath.length > 1)
                                PolylineLayer(polylines: [Polyline(points: _measurePath, color: Colors.blueGrey.withAlpha(200), strokeWidth: 5.0)]),
                              if (_modeMeasurePath) MarkerLayer(markers: _getPathMeasureMarkers()),
                              if (gl.Mode.editPolygon) CircleLayer(circles: _drawnLayerPointsCircleMarker()),
                              if (gl.Mode.editPolygon) MarkerLayer(markers: _drawnLayerPointsMarker()),
                              if (gl.Mode.editPolygon)
                                if (gl.selLay.geometries.isNotEmpty && gl.selGeo.points.isNotEmpty && !gl.Mode.showButtonAddVertexesPolygon)
                                  CircleLayer(
                                    circles: [
                                      CircleMarker(point: gl.selGeo.points[gl.selGeo.selectedPolyLinePoints[0]], radius: 15, color: Colors.red),
                                    ],
                                  ),
                            ],
                      ),
                      if (!gl.dsp.showKeyboard) forestimatorTopMenuElements,
                      if (!gl.dsp.showKeyboard) forestimatorGeoMenu,
                      if (!gl.dsp.showKeyboard) forestimatorAnalysisToolbar,
                    ] +
                    [if (gl.modeDevelopper && gl.Mode.debugScanlines) lt.gridlines()] +
                    List<Widget>.from(gl.stack.widgets.reversed) +
                    [_forestimatorDebugElements],
              );
            },
          ),
        ),
      ),
    );
  }

  Widget get forestimatorGeoMenu => Stack(
    alignment: AlignmentGeometry.center,
    children: [
      SizedBox(
        height: double.infinity,
        width: double.infinity,
        child: AnimatedContainer(
          alignment:
              gl.dsp.orientation.name == "Portrait"
                  ? gl.Mode.polygon && gl.layerReady
                      ? gl.Mode.polygonList
                          ? AlignmentGeometry.xy(_layToolBoxAnimUnderListPos.dx, _layToolBoxAnimUnderListPos.dy)
                          : AlignmentGeometry.xy(_layToolBoxAnimOnScreenPos.dx, _layToolBoxAnimOnScreenPos.dy)
                      : AlignmentGeometry.xy(_layToolBoxAnimOffScreenPos.dx, _layToolBoxAnimOffScreenPos.dy)
                  : gl.Mode.polygon && gl.layerReady
                  ? gl.Mode.polygonList
                      ? AlignmentGeometry.xy(_layToolBoxAnimUnderListPos.dx, _layToolBoxAnimUnderListPos.dy)
                      : AlignmentGeometry.xy(_layToolBoxAnimOnScreenPos.dx, _layToolBoxAnimOnScreenPos.dy)
                  : AlignmentGeometry.xy(_layToolBoxAnimOffScreenPos.dx, _layToolBoxAnimOffScreenPos.dy),
          curve: Curves.linearToEaseOut,
          duration: Duration(milliseconds: 1500),
          child: forestimatorBuildGeoMenu,
        ),
      ),
    ],
  );

  Widget get forestimatorBuildGeoMenu =>
      (gl.layerReady)
          ? SizedBox(
            width: gl.eqPx * gl.chosenPolyBarWidth,
            height: gl.eqPx * computePolygonTitleHeight(),
            child: Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
                side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
              ),
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color: gl.backgroundTransparentBlackBox,
              child: Column(
                children: [
                  SizedBox(
                    width: gl.eqPx * gl.chosenPolyBarWidth,
                    height: gl.eqPx * gl.chosenPolyBarHeight * 0.6,
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceAround,
                      children: [
                        SizedBox(
                          height: gl.eqPx * gl.iconSizeM,
                          child: IconButton(
                            alignment: Alignment.center,
                            style: lt.borderlessStyle,
                            onPressed: () {
                              refreshView(() {
                                if (!gl.Mode.editPolygon) {
                                  gl.selLay.visible(!gl.selLay.visibleOnMap);
                                  gl.selLay.serialize();
                                }
                              });
                            },
                            icon:
                                gl.selLay.visibleOnMap
                                    ? FaIcon(FontAwesomeIcons.eyeSlash, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white)
                                    : FaIcon(FontAwesomeIcons.eye, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                          ),
                        ),
                        Container(
                          alignment: AlignmentGeometry.center,
                          width: gl.eqPx * 60,
                          height: gl.eqPx * 10,
                          child: DropdownMenuFormField<int>(
                            label: Row(
                              children: [
                                Column(
                                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                  children: [
                                    gl.selLay.type.contains("Point")
                                        ? Text("POINT", style: TextStyle(color: Colors.yellow, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                        : gl.selLay.type == "Polygon"
                                        ? Text("POLY", style: TextStyle(color: Colors.green, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                        : gl.selLay.type == "Path"
                                        ? Text("CHEMIN", style: TextStyle(color: Colors.blue, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                        : Text("OHA?", style: TextStyle(color: Colors.red, fontSize: gl.eqPx * gl.fontSizeXS * .9)),
                                    Icon(
                                      (gl.selLay.type.contains("Point")
                                          ? gl.selectableIcons[gl.selLay.defaultPointIcon]
                                          : gl.selectableIconGeo[gl.selLay.defaultPointIcon]),
                                      size: gl.iconSizeXS * gl.eqPx,
                                      color: gl.selLay.defaultColor,
                                    ),
                                  ],
                                ),
                                lt.ForestimatorScrollView(
                                  width: gl.eqPx * 30,
                                  height: gl.eqPx * 6,
                                  horizontal: true,
                                  child: Text(gl.selLay.name, style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeL)),
                                ),
                              ],
                            ),
                            trailingIcon: Icon(Icons.arrow_drop_down_outlined, size: gl.eqPx * gl.iconSizeXS, color: Colors.white),
                            selectedTrailingIcon: Icon(Icons.arrow_drop_up_outlined, size: gl.eqPx * gl.iconSizeXS, color: Colors.white),
                            expandedInsets: EdgeInsets.zero,
                            textStyle: TextStyle(color: Colors.transparent, fontSize: .01),
                            menuStyle: MenuStyle(
                              backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                WidgetState.any: gl.backgroundTransparentBlackBox,
                              }),
                              shape: WidgetStateProperty<RoundedRectangleBorder>.fromMap(<WidgetStatesConstraint, RoundedRectangleBorder>{
                                WidgetState.any: RoundedRectangleBorder(
                                  borderRadius: BorderRadiusGeometry.circular(12.0),
                                  side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
                                ),
                              }),
                              side: WidgetStateProperty<BorderSide>.fromMap(<WidgetStatesConstraint, BorderSide>{
                                WidgetState.any: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
                              }),
                            ),
                            initialSelection: gl.selectedGeoLayer,
                            onSelected:
                                (int? value) => setState(() {
                                  gl.selectedGeoLayer = value!;
                                }),
                            dropdownMenuEntries: List<DropdownMenuEntry<int>>.generate(gl.geoLayers.length, (int index) {
                              return DropdownMenuEntry<int>(
                                label: "",
                                value: index,
                                labelWidget: Container(
                                  color: gl.selectedGeoLayer == index ? gl.colorAgroBioTech.withAlpha(64) : Colors.transparent,
                                  alignment: Alignment.center,
                                  child: Row(
                                    children: [
                                      Column(
                                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                        children: [
                                          gl.geoLayers[index].type.contains("Point")
                                              ? Text("POINT", style: TextStyle(color: Colors.yellow, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                              : gl.geoLayers[index].type == "Polygon"
                                              ? Text("POLY", style: TextStyle(color: Colors.green, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                              : gl.selLay.type == "Path"
                                              ? Text("CHEMIN", style: TextStyle(color: Colors.blue, fontSize: gl.eqPx * gl.fontSizeXS * .9))
                                              : Text("OHA?", style: TextStyle(color: Colors.red, fontSize: gl.eqPx * gl.fontSizeXS * .9)),
                                          Icon(
                                            (gl.geoLayers[index].type.contains("Point")
                                                ? gl.selectableIcons[gl.geoLayers[index].defaultPointIcon]
                                                : gl.selectableIconGeo[gl.geoLayers[index].defaultPointIcon]),
                                            size: gl.iconSizeXS * gl.eqPx,
                                            color: gl.geoLayers[index].defaultColor,
                                          ),
                                        ],
                                      ),
                                      lt.ForestimatorScrollView(
                                        height: gl.eqPx * 10,
                                        width: gl.eqPx * 50,
                                        horizontal: true,
                                        child: Text(
                                          gl.geoLayers[index].name,
                                          textAlign: TextAlign.center,
                                          style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeL),
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              );
                            }),
                          ),
                        ),
                        Container(
                          padding: EdgeInsets.zero,
                          color: Colors.transparent,
                          height: gl.eqPx * gl.iconSizeL,
                          width: gl.eqPx * gl.iconSizeL,
                          child: lt.forestimatorButton(() {
                            setState(() {
                              gl.Mode.polygonList = true;
                              _polygonMode = true;
                              gl.Mode.editAttributes = false;
                              gl.Mode.editPoint = false;
                              gl.Mode.editPointMarker = false;
                              gl.Mode.editPolyMarker = false;
                            });
                          }, Icons.arrow_drop_down_outlined),
                        ),
                      ],
                    ),
                  ),
                  (gl.Mode.editPolygon && gl.geoReady)
                      ? !gl.selLay.type.contains("Point")
                          ? Column(
                            children: [
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Row(
                                children: [
                                  SizedBox(
                                    height: gl.eqPx * gl.iconSizeM * .9,
                                    child: IconButton(
                                      style: lt.borderlessStyle,
                                      iconSize: gl.eqPx * gl.iconSizeS,
                                      color: Colors.lightGreenAccent,
                                      onPressed: _closeEditingMenu,
                                      icon: Icon(Icons.arrow_back, size: gl.eqPx * gl.iconSizeS * .9),
                                    ),
                                  ),
                                ],
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Row(
                                mainAxisAlignment: MainAxisAlignment.spaceAround,
                                children: [
                                  gl.Mode.showButtonRemoveVertexesPolygon && !gl.selLay.type.contains("Point")
                                      ? CircleAvatar(
                                        radius: gl.iconSizeXS * 0.8 * gl.eqPx,
                                        backgroundColor: _polygonMenuColorTools(gl.Mode.removeVertexesPolygon),
                                        child: SizedBox(
                                          height: gl.eqPx * gl.iconSizeM * .9,
                                          child: IconButton(
                                            style: lt.borderlessStyle,
                                            iconSize: gl.eqPx * gl.iconSizeS,
                                            color: gl.Mode.removeVertexesPolygon ? Colors.white : Colors.lightGreenAccent,
                                            onPressed: () async {
                                              refreshView(() {
                                                gl.Mode.removeVertexesPolygon = !gl.Mode.removeVertexesPolygon;
                                              });
                                              if (gl.Mode.removeVertexesPolygon == true) {
                                                refreshView(() {
                                                  if (gl.selGeo.points.isNotEmpty &&
                                                      _isPolygonWellDefined(
                                                        gl.selGeo.getPolyRemoveOneVertex(
                                                          gl.selGeo.points[gl
                                                              .selLay
                                                              .geometries[gl.selLay.selectedGeometry]
                                                              .selectedPolyLinePoints[0]],
                                                        ),
                                                      )) {
                                                    gl.selGeo.removePoint(
                                                      gl.selGeo.points[gl.selLay.geometries[gl.selLay.selectedGeometry].selectedPolyLinePoints[0]],
                                                    );
                                                  }
                                                });
                                                gl.Mode.moveVertexesPolygon = false;
                                                gl.Mode.addVertexesPolygon = false;
                                                _stopMovingSelectedPoint();
                                                refreshView(() {
                                                  gl.Mode.showButtonAddVertexesPolygon = true;
                                                  gl.Mode.showButtonMoveVertexesPolygon = false;
                                                  gl.Mode.showButtonRemoveVertexesPolygon = false;
                                                  gl.Mode.addVertexesPolygon = false;
                                                  gl.Mode.moveVertexesPolygon = false;
                                                  gl.Mode.removeVertexesPolygon = false;
                                                });
                                              }
                                            },
                                            icon: const Icon(Icons.remove_circle),
                                          ),
                                        ),
                                      )
                                      : SizedBox(
                                        height: gl.eqPx * gl.iconSizeM * .9,
                                        child: IconButton(
                                          style: lt.borderlessStyle,
                                          iconSize: gl.eqPx * gl.iconSizeS,
                                          color: Colors.white24,
                                          onPressed: () {},
                                          icon: const Icon(Icons.remove_circle),
                                        ),
                                      ),
                                  (gl.selGeo.type == "Polygon" ||
                                              gl.selGeo.type == "Path" ||
                                              (gl.selGeo.type.contains("Point") && gl.selGeo.numPoints < 1)) &&
                                          gl.Mode.showButtonAddVertexesPolygon
                                      ? CircleAvatar(
                                        backgroundColor: _polygonMenuColorTools(gl.Mode.addVertexesPolygon),
                                        radius: gl.iconSizeXS * 0.8 * gl.eqPx,
                                        child: SizedBox(
                                          height: gl.eqPx * gl.iconSizeM * .9,
                                          child: IconButton(
                                            style: lt.borderlessStyle,
                                            iconSize: gl.eqPx * gl.iconSizeS,
                                            color: gl.Mode.addVertexesPolygon ? Colors.white : Colors.lightGreenAccent,
                                            onPressed: () async {
                                              refreshView(() {
                                                gl.Mode.addVertexesPolygon = !gl.Mode.addVertexesPolygon;
                                                if (gl.selGeo.type == "Path") {
                                                  gl.Mode.recordPath = gl.Mode.addVertexesPolygon;
                                                  gl.selectedPathLayer = gl.selectedGeoLayer;
                                                  gl.selectedPath = gl.selPathLay.selectedGeometry;
                                                  if (gl.Mode.recordPath == true) {
                                                    gl.startTimer(
                                                      () async {
                                                        updateLocation();
                                                        if (!gl.Mode.gpsTimoutException) {
                                                          if (gl.selPath.points.last.latitude != gl.position.latitude &&
                                                              gl.selPath.points.last.longitude != gl.position.longitude) {
                                                            gl.selPath.addPoint(LatLng(gl.position.latitude, gl.position.longitude));
                                                          }
                                                        }
                                                        return false;
                                                      },
                                                      () => !gl.Mode.recordPath,
                                                      1,
                                                      1,
                                                    );
                                                  }
                                                }
                                              });
                                              if (gl.Mode.addVertexesPolygon == true) {
                                                gl.Mode.removeVertexesPolygon = false;
                                                gl.Mode.moveVertexesPolygon = false;
                                                refreshView(() {
                                                  _stopMovingSelectedPoint();
                                                });
                                              }
                                            },
                                            icon: Icon(
                                              gl.selGeo.type == "Path" && gl.Mode.addVertexesPolygon
                                                  ? Icons.pause_circle
                                                  : gl.selGeo.type == "Path"
                                                  ? Icons.play_circle
                                                  : Icons.add_circle,
                                            ),
                                          ),
                                        ),
                                      )
                                      : SizedBox(
                                        height: gl.eqPx * gl.iconSizeM * .9,
                                        child: IconButton(
                                          style: lt.borderlessStyle,
                                          iconSize: gl.eqPx * gl.iconSizeS,
                                          color: Colors.white24,
                                          onPressed: () {},
                                          icon: const Icon(Icons.add_circle),
                                        ),
                                      ),
                                  gl.Mode.showButtonMoveVertexesPolygon
                                      ? CircleAvatar(
                                        radius: gl.iconSizeXS * 0.8 * gl.eqPx,
                                        backgroundColor: _polygonMenuColorTools(gl.Mode.moveVertexesPolygon),
                                        child: SizedBox(
                                          height: gl.eqPx * gl.iconSizeM * .9,
                                          child: IconButton(
                                            style: lt.borderlessStyle,
                                            color: gl.Mode.moveVertexesPolygon ? Colors.white : Colors.lightGreenAccent,
                                            iconSize: gl.eqPx * gl.iconSizeS,
                                            onPressed: () async {
                                              refreshView(() {
                                                gl.Mode.moveVertexesPolygon = !gl.Mode.moveVertexesPolygon;
                                              });
                                              if (gl.Mode.moveVertexesPolygon == true) {
                                                refreshView(() {
                                                  LatLng point =
                                                      gl.selGeo.points[gl.selLay.geometries[gl.selLay.selectedGeometry].selectedPolyLinePoints[0]];
                                                  if (_selectedPointToMove == null) {
                                                    _selectedPointToMove = point;
                                                    centerOnLatLng(point);
                                                  } else {
                                                    if (point.latitude == _selectedPointToMove!.latitude &&
                                                        point.longitude == _selectedPointToMove!.longitude) {
                                                      _stopMovingSelectedPoint();
                                                    } else {
                                                      _selectedPointToMove = point;
                                                      centerOnLatLng(point);
                                                    }
                                                  }
                                                });
                                                gl.Mode.addVertexesPolygon = false;
                                                gl.Mode.removeVertexesPolygon = false;
                                              } else if (gl.Mode.editPolygon && gl.selGeo.type.contains("Point")) {
                                                refreshView(() {
                                                  _stopMovingSelectedPoint();
                                                  gl.Mode.showButtonAddVertexesPolygon = false;
                                                  gl.Mode.showButtonMoveVertexesPolygon = true;
                                                  gl.Mode.showButtonRemoveVertexesPolygon = true;
                                                  gl.Mode.addVertexesPolygon = false;
                                                  gl.Mode.moveVertexesPolygon = false;
                                                  gl.Mode.removeVertexesPolygon = false;
                                                });
                                              } else {
                                                refreshView(() {
                                                  _stopMovingSelectedPoint();
                                                  gl.Mode.showButtonAddVertexesPolygon = true;
                                                  gl.Mode.showButtonMoveVertexesPolygon = false;
                                                  gl.Mode.showButtonRemoveVertexesPolygon = false;
                                                  gl.Mode.addVertexesPolygon = false;
                                                  gl.Mode.moveVertexesPolygon = false;
                                                  gl.Mode.removeVertexesPolygon = false;
                                                });
                                              }
                                            },
                                            icon: const Icon(Icons.open_with_rounded),
                                          ),
                                        ),
                                      )
                                      : SizedBox(
                                        height: gl.eqPx * gl.iconSizeM * .9,
                                        child: IconButton(
                                          style: lt.borderlessStyle,
                                          iconSize: gl.eqPx * gl.iconSizeS,
                                          color: Colors.white24,
                                          onPressed: () {},
                                          icon: const Icon(Icons.open_with_rounded),
                                        ),
                                      ),
                                ],
                              ),
                            ],
                          )
                          : Column(
                            children: [
                              Row(
                                children: [
                                  SizedBox(
                                    height: gl.eqPx * gl.iconSizeM * .9,
                                    child: IconButton(
                                      style: lt.borderlessStyle,
                                      iconSize: gl.eqPx * gl.iconSizeS,
                                      color: Colors.lightGreenAccent,
                                      onPressed: () {
                                        _closeEditingMenu();
                                      },
                                      icon: Icon(Icons.arrow_back, size: gl.eqPx * gl.iconSizeS * .9),
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          )
                      : gl.Mode.editAttributes
                      ? Column(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Column(
                            children: [
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Row(
                                children: [
                                  Container(
                                    alignment: Alignment.topLeft,
                                    child: SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        style: lt.borderlessStyle,
                                        iconSize: gl.eqPx * gl.iconSizeS,
                                        color: Colors.lightGreenAccent,
                                        onPressed: () {
                                          refreshView(() {
                                            gl.Mode.editAttributes = false;
                                          });
                                        },
                                        icon: Icon(Icons.arrow_back, size: gl.eqPx * gl.iconSizeS * .9),
                                      ),
                                    ),
                                  ),
                                  Container(
                                    alignment: Alignment.centerLeft,
                                    width: gl.eqPx * gl.chosenPolyBarWidth * .75,
                                    child: Text(
                                      "Table des attributs",
                                      textAlign: TextAlign.center,
                                      style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM * .75),
                                    ),
                                  ),
                                ],
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Column(
                                children: [
                                  Row(
                                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                    children: [
                                      SizedBox(
                                        width: gl.eqPx * 11,
                                        child: Text(
                                          "type",
                                          textAlign: TextAlign.center,
                                          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: gl.eqPx * gl.fontSizeM * .75),
                                        ),
                                      ),
                                      lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                      SizedBox(
                                        width: gl.eqPx * 7,
                                        child: Icon(Icons.remove_red_eye, color: Colors.white, size: gl.eqPx * gl.iconSizeXS),
                                      ),
                                      lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                      SizedBox(
                                        width: gl.eqPx * 33,
                                        child: Text(
                                          "Attribut",
                                          textAlign: TextAlign.center,
                                          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: gl.eqPx * gl.fontSizeM * .75),
                                        ),
                                      ),
                                      lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                      SizedBox(
                                        width: gl.eqPx * 33,
                                        child: Text(
                                          "Valeur",
                                          textAlign: TextAlign.center,
                                          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: gl.eqPx * gl.fontSizeM * .75),
                                        ),
                                      ),
                                    ],
                                  ),
                                  lt.stroke(gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                  Scrollbar(
                                    scrollbarOrientation: ScrollbarOrientation.right,
                                    thickness: gl.eqPx * 3,
                                    controller: propertiesTableScrollController,
                                    child: Container(
                                      color: gl.backgroundTransparentBlackBox.withAlpha(100),
                                      height:
                                          gl.dsp.orientation == Orientation.landscape
                                              ? gl.eqPx * computePolygonTitleHeight() / 2.1
                                              : gl.eqPx * gl.attributeTableHeight,
                                      child:
                                          gl.geoReady
                                              ? ListView(
                                                controller: propertiesTableScrollController,
                                                children:
                                                    <Widget>[
                                                      _getFixedAttribute("type", gl.selGeo.type),
                                                      _getFixedAttribute("nom", gl.selGeo.name, checked: true),
                                                      if (gl.selGeo.type == "Polygon")
                                                        _getFixedAttribute("surface", "${(gl.selGeo.area / 100).round() / 100}"),
                                                      if (gl.selGeo.type == "Polygon")
                                                        _getFixedAttribute("circonference", "${(gl.selGeo.perimeter).round() / 1000}"),

                                                      _getFixedAttribute("coordinates", gl.selGeo.getPolyPointsString()),
                                                    ] +
                                                    List<Widget>.generate(gl.selGeo.attributes.length, (i) {
                                                      return Column(
                                                        children: [
                                                          Row(
                                                            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                                            children: [
                                                              SizedBox(
                                                                width: gl.eqPx * 11,
                                                                height: gl.eqPx * gl.iconSizeS,
                                                                child: TextButton(
                                                                  style: ButtonStyle(
                                                                    animationDuration: Duration(seconds: 1),
                                                                    backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                                      <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                                    ),
                                                                    padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                                      <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                                                      },
                                                                    ),
                                                                  ),
                                                                  onPressed: () {},
                                                                  onLongPress: () {},
                                                                  child: Container(
                                                                    alignment: Alignment.center,
                                                                    child:
                                                                        gl.selGeo.attributes[i].type == "int"
                                                                            ? Text(
                                                                              "INT",
                                                                              style: TextStyle(
                                                                                color: Colors.yellow,
                                                                                fontSize: gl.fontSizeXXS * gl.eqPx,
                                                                              ),
                                                                            )
                                                                            : gl.selGeo.attributes[i].type == "string"
                                                                            ? Text(
                                                                              "STRING",
                                                                              style: TextStyle(
                                                                                color: Colors.lightBlue,
                                                                                fontSize: gl.fontSizeXXS * gl.eqPx,
                                                                              ),
                                                                            )
                                                                            : gl.selGeo.attributes[i].type == "double"
                                                                            ? Text(
                                                                              "DOUBLE",
                                                                              style: TextStyle(color: Colors.red, fontSize: gl.fontSizeXXS * gl.eqPx),
                                                                            )
                                                                            : Text(
                                                                              "UFO",
                                                                              style: TextStyle(
                                                                                color: Colors.green,
                                                                                fontSize: gl.fontSizeXXS * gl.eqPx,
                                                                              ),
                                                                            ),
                                                                  ),
                                                                ),
                                                              ),
                                                              lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                                              SizedBox(
                                                                width: gl.eqPx * 7,
                                                                height: gl.eqPx * gl.iconSizeM,
                                                                child: IconButton(
                                                                  style: ButtonStyle(
                                                                    animationDuration: Duration(seconds: 1),
                                                                    backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                                      <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                                    ),
                                                                    padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                                      <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                                                      },
                                                                    ),
                                                                  ),
                                                                  onPressed: () async {
                                                                    refreshView(() {
                                                                      gl
                                                                          .selLay
                                                                          .geometries[gl.selLay.selectedGeometry]
                                                                          .attributes[i]
                                                                          .visibleOnMapLabel = !gl
                                                                              .selLay
                                                                              .geometries[gl.selLay.selectedGeometry]
                                                                              .attributes[i]
                                                                              .visibleOnMapLabel;
                                                                    });
                                                                    gl.selGeo.serialize();
                                                                  },
                                                                  icon:
                                                                      gl.selLay.geometries[gl.selLay.selectedGeometry].attributes[i].visibleOnMapLabel
                                                                          ? Icon(
                                                                            Icons.check_box_outlined,
                                                                            color: Colors.white,
                                                                            size: gl.eqPx * gl.iconSizeXS,
                                                                          )
                                                                          : Icon(
                                                                            Icons.check_box_outline_blank,
                                                                            color: Colors.white,
                                                                            size: gl.eqPx * gl.iconSizeXS,
                                                                          ),
                                                                ),
                                                              ),
                                                              lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                                              SizedBox(
                                                                width: gl.eqPx * 33,
                                                                height: gl.eqPx * gl.iconSizeS,
                                                                child: TextButton(
                                                                  style: ButtonStyle(
                                                                    animationDuration: Duration(seconds: 1),
                                                                    backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                                      <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                                    ),
                                                                    padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                                      <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                                                      },
                                                                    ),
                                                                  ),
                                                                  onPressed: () {},
                                                                  /*onLongPress: () {
                                                                        PopupValueChange(
                                                                          "prop",
                                                                          gl.selGeo.attributes[i].name,
                                                                          (value) {
                                                                            gl
                                                                                .selLay
                                                                                .geometries[gl.selLay.selectedGeometry]
                                                                                .attributes[i]
                                                                                .name = cleanAttributeName(
                                                                              value.toString(),
                                                                            );
                                                                          },
                                                                          () {},
                                                                          () {
                                                                            String nom = gl.selGeo.attributes[i].name;
                                                                            if (controlDuplicateAttributeName(
                                                                              gl.selGeo.attributes[i].name,
                                                                              gl.selGeo.attributes,
                                                                            )) {
                                                                              gl.refreshStack(() {
                                                                                popupForestimatorMessage(
                                                                                  id: "MSGduplicateName",
                                                                                  title: "Erreur",
                                                                                  message: "Le nom $nom existe déja!",
                                                                                );
                                                                              });
                                                                              return;
                                                                            } else {
                                                                              gl.selGeo.serialize();
                                                                            }
                                                                          },
                                                                        );
                                                                      },*/
                                                                  child: Container(
                                                                    alignment: Alignment.centerLeft,
                                                                    child: SingleChildScrollView(
                                                                      scrollDirection: Axis.horizontal,
                                                                      child: Text(
                                                                        gl.selGeo.attributes[i].name,
                                                                        textAlign: TextAlign.start,
                                                                        style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM * .75),
                                                                      ),
                                                                    ),
                                                                  ),
                                                                ),
                                                              ),
                                                              lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                                              SizedBox(
                                                                width: gl.eqPx * 33,
                                                                height: gl.eqPx * gl.iconSizeS,
                                                                child: TextButton(
                                                                  style: ButtonStyle(
                                                                    animationDuration: Duration(seconds: 1),
                                                                    backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                                      <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                                    ),
                                                                    padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                                      <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                                                      },
                                                                    ),
                                                                  ),
                                                                  onPressed: () {
                                                                    PopupValueChange(
                                                                      gl.selGeo.attributes[i].type,
                                                                      gl.selGeo.attributes[i].value,
                                                                      (value) {
                                                                        gl.selGeo.attributes[i].value = value;
                                                                      },
                                                                      () {},
                                                                      () {
                                                                        gl.selGeo.serialize();
                                                                      },
                                                                    );
                                                                  },
                                                                  child: Container(
                                                                    alignment: Alignment.centerLeft,
                                                                    child: SingleChildScrollView(
                                                                      scrollDirection: Axis.horizontal,
                                                                      child:
                                                                          gl.selGeo.attributes[i].type == "string"
                                                                              ? Text(
                                                                                gl.selLay.geometries[gl.selLay.selectedGeometry].attributes[i].value,
                                                                                textAlign: TextAlign.start,
                                                                                style: TextStyle(
                                                                                  color: Colors.white,
                                                                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                                                ),
                                                                              )
                                                                              : gl.selLay.geometries[gl.selLay.selectedGeometry].attributes[i].type ==
                                                                                  "int"
                                                                              ? Text(
                                                                                gl.selGeo.attributes[i].value.toString(),
                                                                                textAlign: TextAlign.start,
                                                                                style: TextStyle(
                                                                                  color: Colors.white,
                                                                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                                                ),
                                                                              )
                                                                              : gl.selLay.geometries[gl.selLay.selectedGeometry].attributes[i].type ==
                                                                                  "double"
                                                                              ? Text(
                                                                                gl.selGeo.attributes[i].value.toString(),
                                                                                textAlign: TextAlign.start,
                                                                                style: TextStyle(
                                                                                  color: Colors.white,
                                                                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                                                ),
                                                                              )
                                                                              : gl.selLay.geometries[gl.selLay.selectedGeometry].attributes[i].type ==
                                                                                  "special"
                                                                              ? Text(
                                                                                "special value",
                                                                                style: TextStyle(
                                                                                  color: Colors.white,
                                                                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                                                ),
                                                                              )
                                                                              : Text(
                                                                                "ERROR TYPE ${gl.selGeo.attributes[i].type}",
                                                                                style: TextStyle(
                                                                                  color: Colors.white,
                                                                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                                                ),
                                                                              ),
                                                                    ),
                                                                  ),
                                                                ),
                                                              ),
                                                            ],
                                                          ),
                                                          lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                                                        ],
                                                      );
                                                    }),
                                              )
                                              : SizedBox(),
                                    ),
                                  ),
                                ],
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                            ],
                          ),
                        ],
                      )
                      : gl.Mode.editPointMarker
                      ? Column(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Column(
                            children: [
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Row(
                                children: [
                                  Container(
                                    alignment: Alignment.topLeft,
                                    child: SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        style: lt.borderlessStyle,
                                        iconSize: gl.eqPx * gl.iconSizeS,
                                        color: Colors.lightGreenAccent,
                                        onPressed: () {
                                          refreshView(() {
                                            gl.Mode.editPointMarker = false;
                                          });
                                          gl.selGeo.serialize();
                                        },
                                        icon: Icon(Icons.arrow_back, size: gl.eqPx * gl.iconSizeS * .9),
                                      ),
                                    ),
                                  ),
                                  Container(
                                    alignment: Alignment.topLeft,
                                    width: gl.eqPx * gl.chosenPolyBarWidth * .75,
                                    child: Text(
                                      "Changez le symbole du point.",
                                      textAlign: TextAlign.center,
                                      style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeS),
                                    ),
                                  ),
                                ],
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              lt.ForestimatorScrollView(
                                width: gl.eqPx * gl.chosenPolyBarWidth,
                                height: gl.eqPx * gl.iconSizeXS * 2.5,
                                sizeArrows: gl.eqPx * gl.iconSizeM,
                                horizontal: true,
                                arrowColor: Colors.black,
                                child: Row(
                                  children: List<Widget>.generate(gl.selectableIcons.length, (k) {
                                    return Container(
                                      color: gl.selGeo.selectedPointIcon == k ? gl.colorAgroBioTech : Colors.transparent,
                                      child: SizedBox(
                                        height: gl.eqPx * gl.iconSizeL,
                                        child: IconButton(
                                          onPressed: () {
                                            refreshView(() {
                                              gl.selGeo.selectedPointIcon = k;
                                            });
                                          },
                                          icon: Icon(gl.selectableIcons[k], size: gl.iconSizeM * gl.eqPx, color: Colors.white),
                                          color: Colors.white,
                                          iconSize: gl.eqPx * gl.iconSizeM,
                                        ),
                                      ),
                                    );
                                  }),
                                ),
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              lt.ForestimatorScrollView(
                                width: gl.eqPx * gl.chosenPolyBarWidth,
                                height: gl.eqPx * gl.iconSizeXS * 2.5,
                                sizeArrows: gl.eqPx * gl.iconSizeM,
                                horizontal: true,
                                child: Row(
                                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                                  children: List<TextButton>.generate(gl.predefinedPointSymbPalette.length, (int k) {
                                    return TextButton(
                                      onPressed: () {
                                        refreshView(() {
                                          gl.selGeo.setColorInside(gl.predefinedPointSymbPalette[k].withAlpha(150));
                                          gl.selGeo.setColorLine(gl.predefinedPointSymbPalette[k]);
                                        });
                                      },
                                      child: CircleAvatar(
                                        backgroundColor:
                                            gl.selGeo.colorPolygon == gl.predefinedPointSymbPalette[k] ? Colors.white : Colors.transparent,
                                        radius: gl.eqPx * gl.iconSizeXS * .9,
                                        child: CircleAvatar(radius: gl.eqPx * gl.iconSizeXS * .85, backgroundColor: gl.predefinedPointSymbPalette[k]),
                                      ),
                                    );
                                  }),
                                ),
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              SizedBox(
                                width: gl.eqPx * gl.chosenPolyBarWidth,
                                height: gl.eqPx * gl.chosenPolyBarHeight * .8,
                                child: Slider(
                                  min: gl.iconSizeXXS,
                                  max: gl.iconSizeL,
                                  value: gl.selGeo.iconSize > gl.iconSizeXXS && gl.selGeo.iconSize < gl.iconSizeL ? gl.selGeo.iconSize : 10.0,
                                  divisions: 20,
                                  activeColor: gl.colorAgroBioTech,
                                  onChanged: (double value) {
                                    refreshView(() {
                                      gl.selGeo.iconSize = value;
                                    });
                                  },
                                ),
                              ),
                            ],
                          ),
                        ],
                      )
                      : gl.Mode.editPolyMarker
                      ? Column(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Column(
                            children: [
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              Row(
                                children: [
                                  Container(
                                    alignment: Alignment.topLeft,
                                    child: SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        style: lt.borderlessStyle,
                                        iconSize: gl.eqPx * gl.iconSizeS,
                                        color: Colors.lightGreenAccent,
                                        onPressed: () {
                                          refreshView(() {
                                            gl.Mode.editPolyMarker = false;
                                          });
                                          gl.selGeo.serialize();
                                        },
                                        icon: Icon(Icons.arrow_back, size: gl.eqPx * gl.iconSizeS * .9),
                                      ),
                                    ),
                                  ),
                                  Container(
                                    alignment: Alignment.topLeft,
                                    width: gl.eqPx * gl.chosenPolyBarWidth * .75,
                                    child: Text(
                                      "Changez la couleur du polygone.",
                                      textAlign: TextAlign.center,
                                      style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeS),
                                    ),
                                  ),
                                ],
                              ),
                              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                              if (gl.geoReady)
                                lt.ForestimatorScrollView(
                                  width: gl.eqPx * gl.chosenPolyBarWidth,
                                  height: gl.eqPx * gl.iconSizeXS * 2.5,
                                  sizeArrows: gl.eqPx * gl.iconSizeM,
                                  horizontal: true,
                                  child: Row(
                                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                                    children:
                                        [
                                          TextButton(
                                            onPressed: () {
                                              refreshView(() {
                                                gl.selGeo.setColorInside(gl.selLay.defaultColor.withAlpha(120));
                                                gl.selGeo.setColorLine(gl.selLay.defaultColor);
                                              });
                                            },
                                            child: CircleAvatar(
                                              backgroundColor: gl.selGeo.colorPolygon == gl.selLay.defaultColor ? Colors.white : Colors.transparent,
                                              radius: gl.eqPx * gl.iconSizeXS * .9,
                                              child: CircleAvatar(
                                                radius: gl.eqPx * gl.iconSizeXS * .85,
                                                backgroundColor: gl.selLay.defaultColor.withAlpha(255),
                                              ),
                                            ),
                                          ),
                                        ] +
                                        List<TextButton>.generate(gl.predefinedPointSymbPalette.length, (int k) {
                                          return TextButton(
                                            onPressed: () {
                                              refreshView(() {
                                                gl.selGeo.setColorInside(gl.predefinedPointSymbPalette[k].withAlpha(120));
                                                gl.selGeo.setColorLine(gl.predefinedPointSymbPalette[k]);
                                              });
                                            },
                                            child: CircleAvatar(
                                              backgroundColor:
                                                  gl.selGeo.colorPolygon == gl.predefinedPointSymbPalette[k] ? Colors.white : Colors.transparent,
                                              radius: gl.eqPx * gl.iconSizeXS * .9,
                                              child: CircleAvatar(
                                                radius: gl.eqPx * gl.iconSizeXS * .85,
                                                backgroundColor: gl.predefinedPointSymbPalette[k],
                                              ),
                                            ),
                                          );
                                        }),
                                  ),
                                ),
                            ],
                          ),
                        ],
                      )
                      : Column(
                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                        children: [
                          lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                            children: [
                              lt.ForestimatorScrollView(
                                id: "geoScrollBar",
                                horizontal: true,
                                height: gl.eqPx * gl.iconSizeM,
                                width: gl.eqPx * gl.chosenPolyBarWidth * .75,
                                child: Row(
                                  children: List<Widget>.generate(gl.selLay.geometries.length, (int index) {
                                    return Container(
                                      height: gl.eqPx * gl.iconSizeM,
                                      width: gl.eqPx * gl.iconSizeM,
                                      alignment: Alignment.center,
                                      padding: EdgeInsets.zero,
                                      color: Colors.transparent,
                                      child: Card(
                                        margin: EdgeInsetsGeometry.zero,
                                        shape: RoundedRectangleBorder(
                                          borderRadius: BorderRadiusGeometry.circular(12.0),
                                          side: BorderSide(
                                            color: gl.selLay.selectedGeometry == index ? gl.colorAgroBioTech : Colors.transparent,
                                            width: gl.eqPx * .75,
                                          ),
                                        ),
                                        surfaceTintColor: Colors.transparent,
                                        shadowColor: Colors.transparent,
                                        color: Colors.transparent,
                                        child: Stack(
                                          children: [
                                            IconButton(
                                              alignment: AlignmentGeometry.center,
                                              style: lt.trNoPadButtonstyle,
                                              onPressed: () {
                                                setState(() {
                                                  if (gl.selLay.selectedGeometry == index) {
                                                    gl.selLay.selectedGeometry = -1;
                                                  } else {
                                                    gl.selLay.selectedGeometry = index;
                                                  }
                                                });
                                              },
                                              icon: Icon(
                                                gl.selLay.type == "Point"
                                                    ? gl.selectableIcons[gl.selLay.geometries[index].selectedPointIcon]
                                                    : gl.selectableIconGeo[gl.selLay.geometries[index].selectedPointIcon],
                                                color: gl.selLay.geometries[index].colorLine,
                                                size: gl.eqPx * gl.iconSizeXS * 1.2,
                                              ),
                                            ),
                                            Container(
                                              alignment: AlignmentGeometry.xy(gl.dsp.alignX(-10 * gl.eqPx), gl.dsp.alignY(-15 * gl.eqPx)),
                                              child: Text("${index + 1}", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS)),
                                            ),
                                          ],
                                        ),
                                      ),
                                    );
                                  }),
                                ),
                              ),
                              Container(
                                alignment: Alignment.center,
                                height: gl.eqPx * gl.iconSizeM,
                                width: gl.eqPx * gl.iconSizeM,
                                child: IconButton(
                                  style: lt.borderlessStyle,
                                  onPressed: () {
                                    setState(() {
                                      if (gl.selLay.subtype != "Essence") {
                                        gl.selLay.addGeometry(
                                          name:
                                              gl.selLay.type == "Point"
                                                  ? "Point${gl.selLay.geometries.length + 1}"
                                                  : gl.selLay.type == "Polygon"
                                                  ? "Polygon${gl.selLay.geometries.length + 1}"
                                                  : gl.selLay.type == "Path"
                                                  ? "Path${gl.selLay.geometries.length + 1}"
                                                  : "UUUPS${gl.selLay.geometries.length + 1}",
                                        );
                                      }
                                      _switchModeEditVertexesOn();
                                      gl.selLay.selectedGeometry = gl.selLay.geometries.length - 1;
                                    });
                                  },
                                  icon: Icon(Icons.add_circle, color: gl.colorAgroBioTech, size: gl.eqPx * gl.iconSizeS),
                                ),
                              ),
                            ],
                          ),
                          if (gl.geoReady) lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                          gl.geoReady
                              ? Row(
                                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                children: [
                                  if (!gl.selGeo.labelsVisibleOnMap && gl.Mode.debugLabel)
                                    SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        onPressed: () {
                                          setState(() {
                                            gl.selGeo.labelsVisibleOnMap = true;
                                          });
                                          gl.selGeo.serialize();
                                          gl.refreshStack(() {
                                            gl.modeMapShowPolygons = true;
                                          });
                                        },
                                        icon: Icon(Icons.label, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                      ),
                                    ),
                                  if (gl.selGeo.visibleOnMap)
                                    SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        onPressed: () {
                                          setState(() {
                                            gl.selGeo.visibleOnMap = false;
                                          });
                                          gl.selGeo.serialize();
                                        },
                                        icon: Icon(FontAwesomeIcons.eyeSlash, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                      ),
                                    ),
                                  if (gl.selGeo.points.isNotEmpty &&
                                      (!_positionInsideViewRectangle(
                                            Position(
                                              longitude: gl.selGeo.center.longitude,
                                              latitude: gl.selGeo.center.latitude,
                                              timestamp: DateTime.now(),
                                              accuracy: 0,
                                              altitude: 0,
                                              altitudeAccuracy: 0,
                                              heading: 0,
                                              headingAccuracy: 0,
                                              speed: 0,
                                              speedAccuracy: 0,
                                            ),
                                          ) ||
                                          !gl.selGeo.visibleOnMap))
                                    SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        onPressed: () {
                                          gl.selGeo.visibleOnMap = true;
                                          gl.selGeo.serialize();
                                          setState(() {
                                            if (gl.selLay.type.contains("Polygon") && gl.selGeo.points.length > 2) {
                                              centerOnPolygon(gl.selGeo);
                                            } else if (gl.selLay.type.contains("Point") &&
                                                gl.selGeo.center.longitude != 0.0 &&
                                                gl.selGeo.center.latitude != 0.0) {
                                              centerOnPoint(gl.selGeo);
                                            }
                                          });
                                          gl.refreshStack(() {
                                            gl.modeMapShowPolygons = true;
                                          });
                                        },
                                        icon: Icon(
                                          Icons.gps_fixed,
                                          size: gl.eqPx * gl.iconSizeS * .9,
                                          opticalSize: gl.eqPx * gl.iconSizeS,
                                          color: Colors.white,
                                        ),
                                      ),
                                    ),
                                  gl.selGeo.type.contains("Point")
                                      ? SizedBox(
                                        height: gl.eqPx * gl.iconSizeM * .9,
                                        child: IconButton(
                                          onPressed: () {
                                            setState(() {
                                              gl.Mode.editPointMarker = !gl.Mode.editPointMarker;
                                            });
                                          },
                                          icon: Icon(FontAwesomeIcons.noteSticky, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                        ),
                                      )
                                      : SizedBox(
                                        height: gl.eqPx * gl.iconSizeM * .9,
                                        child: IconButton(
                                          onPressed: () {
                                            setState(() {
                                              gl.Mode.editPolyMarker = !gl.Mode.editPolyMarker;
                                            });
                                          },
                                          icon: Icon(FontAwesomeIcons.noteSticky, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                        ),
                                      ),
                                  if (!gl.selLay.type.contains("Point"))
                                    SizedBox(
                                      height: gl.eqPx * gl.iconSizeM * .9,
                                      child: IconButton(
                                        onPressed: () {
                                          gl.refreshStack(() {
                                            gl.modeMapShowPolygons = true;
                                            gl.selGeo.visibleOnMap = true;
                                            gl.Mode.editPolygon = !gl.Mode.editPolygon;
                                            if (gl.Mode.editPolygon && gl.selGeo.type.contains("Point") && gl.selGeo.points.isNotEmpty) {
                                              refreshView(() {
                                                gl.Mode.editPolygon = true;
                                                gl.Mode.showButtonAddVertexesPolygon = false;
                                                gl.Mode.showButtonMoveVertexesPolygon = true;
                                                gl.Mode.showButtonRemoveVertexesPolygon = true;
                                                gl.Mode.addVertexesPolygon = false;
                                                gl.Mode.moveVertexesPolygon = false;
                                                gl.Mode.removeVertexesPolygon = false;
                                              });
                                            } else {
                                              refreshView(() {
                                                gl.Mode.editPolygon = true;
                                                gl.Mode.showButtonAddVertexesPolygon = true;
                                                gl.Mode.showButtonMoveVertexesPolygon = false;
                                                gl.Mode.showButtonRemoveVertexesPolygon = false;
                                                gl.Mode.addVertexesPolygon = false;
                                                gl.Mode.moveVertexesPolygon = false;
                                                gl.Mode.removeVertexesPolygon = false;
                                              });
                                            }
                                          });
                                        },
                                        icon: FaIcon(FontAwesomeIcons.drawPolygon, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                      ),
                                    ),
                                  SizedBox(
                                    height: gl.eqPx * gl.iconSizeM * .9,
                                    child: IconButton(
                                      onPressed: () {
                                        setState(() {
                                          gl.Mode.editAttributes = !gl.Mode.editAttributes;
                                        });
                                      },
                                      icon: FaIcon(FontAwesomeIcons.tableColumns, size: gl.eqPx * gl.iconSizeS * .9, color: Colors.white),
                                    ),
                                  ),
                                ],
                              )
                              : SizedBox(),
                        ],
                      ),
                ],
              ),
            ),
          )
          : Column();

  Widget get forestimatorAnalysisToolbar => Stack(
    alignment: AlignmentGeometry.center,
    children: [
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? _toolbarExtended
                    ? AlignmentGeometry.xy(_anaToolbarAnimOnScreenPos.dx, _anaToolbarAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_anaToolbarAnimOffScreenPos.dx, _anaToolbarAnimOffScreenPos.dy)
                : _toolbarExtended
                ? AlignmentGeometry.xy(_anaToolbarAnimOnScreenPos.dx, _anaToolbarAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_anaToolbarAnimOffScreenPos.dx, _anaToolbarAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: toolBar,
      ),
    ],
  );

  void centerOnPolygon(ge.Geometry it) {
    _mapController.fitCamera(CameraFit.bounds(bounds: LatLngBounds(it.boundingBox[0], it.boundingBox[1])));
  }

  void centerOnPoint(ge.Geometry it) {
    _mapController.move(it.center, _mapController.camera.zoom);
  }

  void centerOnLatLng(LatLng it) {
    _mapController.move(it, _mapController.camera.zoom);
  }

  void _switchModeEditVertexesOn() {
    gl.refreshStack(() {
      gl.modeMapShowPolygons = true;
      gl.Mode.editPolygon = true;
      gl.Mode.showButtonAddVertexesPolygon = true;
      gl.Mode.showButtonMoveVertexesPolygon = false;
      gl.Mode.showButtonRemoveVertexesPolygon = false;
      gl.Mode.addVertexesPolygon = true;
      gl.Mode.moveVertexesPolygon = false;
      gl.Mode.removeVertexesPolygon = false;
    });
  }

  void _closeEditingMenu() {
    refreshView(() {
      _stopMovingSelectedPoint();
      gl.Mode.editPolygon = false;
      gl.Mode.showButtonAddVertexesPolygon = true;
      gl.Mode.showButtonMoveVertexesPolygon = false;
      gl.Mode.showButtonRemoveVertexesPolygon = false;
      gl.Mode.addVertexesPolygon = false;
      gl.Mode.moveVertexesPolygon = false;
      gl.Mode.removeVertexesPolygon = false;
    });
    if (gl.geoReady && gl.selLay.subtype != "Essence" && gl.selGeo.type.contains("Point") && gl.selGeo.points.isEmpty) {
      gl.selLay.removeGeometry(last: true);
    } else if (gl.geoReady && gl.selGeo.type.contains("Polygon") && gl.selGeo.points.length < 3) {
      gl.selLay.removeGeometry(last: true);
    }
  }

  Widget get forestimatorTopMenuElements => Stack(
    alignment: AlignmentGeometry.center,
    children: [
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? !gl.Mode.polygon
                    ? AlignmentGeometry.xy(_mainMenuSettingsAnimOnScreenPos.dx, _mainMenuSettingsAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_mainMenuSettingsAnimOffScreenPos.dx, _mainMenuSettingsAnimOffScreenPos.dy)
                : !gl.Mode.polygon
                ? AlignmentGeometry.xy(_mainMenuSettingsAnimOnScreenPos.dx, _mainMenuSettingsAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_mainMenuSettingsAnimOffScreenPos.dx, _mainMenuSettingsAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorSettingButton,
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? AlignmentGeometry.xy(_mainmenuBarPos.dx, _mainmenuBarPos.dy)
                : AlignmentGeometry.xy(_mainmenuBarPos.dx, _mainmenuBarPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _mainMenuBar(),
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? gl.Mode.essence || gl.Mode.addVertexesPolygon || gl.Mode.recordPathAvailable
                    ? AlignmentGeometry.xy(_mainMenuEssenceAnimOnScreenPos.dx, _mainMenuEssenceAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_mainMenuEssenceAnimOffScreenPos.dx, _mainMenuEssenceAnimOffScreenPos.dy)
                : gl.Mode.essence || gl.Mode.addVertexesPolygon || gl.Mode.recordPathAvailable
                ? AlignmentGeometry.xy(_mainMenuEssenceAnimOnScreenPos.dx, _mainMenuEssenceAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_mainMenuEssenceAnimOffScreenPos.dx, _mainMenuEssenceAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorAddVertex,
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? gl.Mode.addVertexesPolygon && gl.layerReady && gl.selLay.type == "Point"
                    ? AlignmentGeometry.xy(_mainMenuFinishAnimOnScreenPos.dx, _mainMenuFinishAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_mainMenuFinishAnimOffScreenPos.dx, _mainMenuFinishAnimOffScreenPos.dy)
                : gl.Mode.addVertexesPolygon && gl.layerReady && gl.selLay.type == "Point"
                ? AlignmentGeometry.xy(_mainMenuFinishAnimOnScreenPos.dx, _mainMenuFinishAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_mainMenuFinishAnimOffScreenPos.dx, _mainMenuFinishAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorFinishEditing,
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? gl.Mode.essence || gl.Mode.addVertexesPolygon || gl.Mode.recordPathAvailable
                    ? AlignmentGeometry.center
                    : AlignmentGeometry.xy(_mainMenuEssenceAnimOffScreenPos.dx, _mainMenuEssenceAnimOffScreenPos.dy)
                : gl.Mode.essence || gl.Mode.addVertexesPolygon || gl.Mode.recordPathAvailable
                ? AlignmentGeometry.center
                : AlignmentGeometry.xy(_mainMenuEssenceAnimOffScreenPos.dx, _mainMenuEssenceAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorCrosshair,
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "Portrait"
                ? !gl.Mode.polygon
                    ? AlignmentGeometry.xy(_mainMenuWarningsAnimOnScreenPos.dx, _mainMenuWarningsAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_mainMenuWarningsAnimOffScreenPos.dx, _mainMenuWarningsAnimOffScreenPos.dy)
                : !gl.Mode.polygon
                ? AlignmentGeometry.xy(_mainMenuWarningsAnimOnScreenPos.dx, _mainMenuWarningsAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_mainMenuWarningsAnimOffScreenPos.dx, _mainMenuWarningsAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorWarnings,
      ),
      AnimatedContainer(
        alignment:
            gl.dsp.orientation.name == "portrait"
                ? !gl.Mode.polygon
                    ? AlignmentGeometry.xy(_mainMenuOnOfflineAnimOnScreenPos.dx, _mainMenuOnOfflineAnimOnScreenPos.dy)
                    : AlignmentGeometry.xy(_mainMenuOnOfflineAnimOffScreenPos.dx, _mainMenuOnOfflineAnimOffScreenPos.dy)
                : !gl.Mode.polygon
                ? AlignmentGeometry.xy(_mainMenuOnOfflineAnimOnScreenPos.dx, _mainMenuOnOfflineAnimOnScreenPos.dy)
                : AlignmentGeometry.xy(_mainMenuOnOfflineAnimOffScreenPos.dx, _mainMenuOnOfflineAnimOffScreenPos.dy),
        curve: Curves.easeInOutBack,
        duration: Duration(milliseconds: 750),
        child: _forestimatorOnOffline,
      ),
    ],
  );

  Widget get _forestimatorDebugElements => Stack(
    children: [
      SizedBox(
        width: double.infinity,
        height: double.infinity,
        child: AnimatedContainer(
          alignment:
              gl.dsp.orientation.name == "portrait"
                  ? gl.Mode.debugInfo
                      ? AlignmentGeometry.xy(gl.Anim.debugOnScreenPos.dx, gl.Anim.debugOnScreenPos.dy)
                      : AlignmentGeometry.xy(gl.Anim.debugOffScreenPos.dx, gl.Anim.debugOffScreenPos.dy)
                  : gl.Mode.debugInfo
                  ? AlignmentGeometry.xy(gl.Anim.debugOnScreenPos.dx, gl.Anim.debugOnScreenPos.dy)
                  : AlignmentGeometry.xy(gl.Anim.debugOffScreenPos.dx, gl.Anim.debugOffScreenPos.dy),
          curve: Curves.easeInOutBack,
          duration: Duration(milliseconds: 750),
          child: _forestimatorDebugInfo,
        ),
      ),
    ],
  );

  Widget get _forestimatorSettingButton => TextButton(
    style: lt.trNoPadButtonstyle,
    onPressed: () {
      if (!gl.modeSettings) {
        setState(() {
          gl.modeSettings = true;
        });
        PopupSettingsMenu();
      } else {
        gl.refreshStack(() {
          gl.stack.pop("SettingsMenu");
          gl.modeSettings = false;
        });
      }
      gl.refreshStack(() {});
    },
    child: Stack(
      alignment: AlignmentGeometry.center,
      children: [
        Icon(Icons.settings, color: gl.modeSettings ? Colors.black : gl.colorAgroBioTech, size: gl.eqPx * (gl.iconSizeM + 2)),
        Icon(Icons.settings, color: gl.modeSettings ? gl.colorAgroBioTech : Colors.black, size: gl.eqPx * gl.iconSizeM),
      ],
    ),
  );

  Widget get _forestimatorAddVertex => Container(
    alignment: Alignment.center,
    width: gl.eqPx * 12,
    height: gl.eqPx * 12,
    child: FloatingActionButton(
      backgroundColor:
          (!(gl.geoReady && gl.selLay.type.contains("Point") && gl.selGeo.points.isNotEmpty)) ||
                  gl.Mode.essence ||
                  (!gl.Mode.essence && gl.selLay.subtype == "Essence") ||
                  gl.geoReady && gl.selLay.type.contains("Polygon")
              ? gl.colorAgroBioTech
              : Colors.grey,
      onPressed: () {
        if ((!(gl.geoReady && gl.selLay.type.contains("Point") && gl.selGeo.points.isNotEmpty)) ||
            gl.Mode.essence ||
            (!gl.Mode.essence && gl.selLay.subtype == "Essence") ||
            gl.geoReady && gl.selLay.type.contains("Polygon")) {
          refreshView(() {
            gl.Mode.recordPathAvailable
                ? PopupRoadChanged(context, _mapController.camera.center)
                : gl.Mode.addVertexesPolygon && gl.selLay.subtype != "Essence"
                ? {
                  _isPolygonWellDefined(gl.selGeo.getPolyPlusOneVertex(_mapController.camera.center))
                      ? {
                        gl.selGeo.addPoint(_mapController.camera.center),
                        if (gl.selGeo.type.contains("Point"))
                          {
                            gl.selLay.addGeometry(
                              name:
                                  gl.selLay.type == "Point" ? "Point${gl.selLay.geometries.length + 1}" : "Polygon${gl.selLay.geometries.length + 1}",
                            ),
                            gl.selLay.selectedGeometry = gl.selLay.geometries.length - 1,
                          },
                      }
                      : PopupPolygonNotWellDefined(),
                }
                : PopupNewEssenceObservationPoint(context, _mapController.camera.center);
          });
        }
      },
      child: SizedBox(
        width: gl.eqPx * gl.iconSizeL,
        height: gl.eqPx * gl.iconSizeL,
        child: Stack(
          alignment: AlignmentGeometry.center,
          children: [
            if (gl.Mode.editPolygon && gl.selLay.type == "Point" && gl.selLay.subtype == "Essence")
              Container(alignment: Alignment.bottomRight, child: Icon(CustomIcons.tree, color: gl.colorBack, size: gl.eqPx * gl.iconSizeXXS))
            else if (gl.Mode.editPolygon && gl.selLay.type == "Point")
              Container(alignment: Alignment.bottomRight, child: Icon(Icons.location_pin, color: gl.colorBack, size: gl.eqPx * gl.iconSizeXXS))
            else if (gl.Mode.editPolygon && gl.selLay.type == "Polygon")
              Container(
                alignment: Alignment.bottomRight,
                child: Icon(FontAwesomeIcons.drawPolygon, color: gl.colorBack, size: gl.eqPx * gl.iconSizeXXS),
              )
            else if (gl.Mode.essence)
              Container(alignment: Alignment.bottomRight, child: Icon(CustomIcons.tree, color: gl.colorBack, size: gl.eqPx * gl.iconSizeXXS))
            else if (gl.Mode.recordPathAvailable)
              Container(alignment: Alignment.bottomRight, child: Icon(FontAwesomeIcons.road, color: gl.colorBack, size: gl.eqPx * gl.iconSizeXXS)),
            Icon(Icons.add, color: gl.Mode.essence ? Colors.black : Colors.white, size: gl.eqPx * gl.iconSizeS),
          ],
        ),
      ),
    ),
  );

  Widget get _forestimatorFinishEditing => Container(
    alignment: Alignment.center,
    width: gl.eqPx * 12,
    height: gl.eqPx * 12,
    child: FloatingActionButton(
      backgroundColor: gl.colorBack,
      onPressed: _closeEditingMenu,
      child: Icon(Icons.verified_outlined, color: gl.Mode.essence ? Colors.black : Colors.white, size: gl.eqPx * gl.iconSizeSettings),
    ),
  );

  Widget get _forestimatorCrosshair => Stack(
    alignment: AlignmentGeometry.center,
    children: [
      Container(
        alignment: Alignment.center,
        width: gl.eqPx * 10,
        height: gl.eqPx * 10,
        child: Icon(FontAwesomeIcons.crosshairs, color: Colors.black.withAlpha(180), size: gl.eqPx * 10),
      ),
      Container(
        alignment: Alignment.center,
        width: gl.eqPx * 10,
        height: gl.eqPx * 10,
        child: Icon(FontAwesomeIcons.crosshairs, color: Colors.white.withAlpha(50), size: gl.eqPx * 8),
      ),
    ],
  );

  Widget get _forestimatorDebugInfo =>
      gl.Mode.keyboardExpanded && !gl.Mode.debugInfo
          ? Container()
          : Container(
            color: Colors.black.withAlpha(100),
            width: gl.eqPx * 40,
            height: gl.eqPx * 40,
            child: Column(
              children: [
                Text("Orientation: ${gl.dsp.orientation.name}", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS)),
                Text("eqPxH: ${gl.dsp.equiheight.toStringAsFixed(1)}", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS)),
                Text("eqPxW: ${gl.dsp.equiwidth.toStringAsFixed(1)}", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS)),
                Text("eqPx: ${gl.dsp.equipixel.toStringAsFixed(1)}", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS)),
                Text(
                  "Padding Top: ${gl.dsp.paddingTop.toStringAsFixed(1)}",
                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS),
                ),
                Text(
                  "Padding Bottom: ${gl.dsp.paddingBot.toStringAsFixed(1)}",
                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS),
                ),
                Text(
                  "Padding Left: ${gl.dsp.paddingBot.toStringAsFixed(1)}",
                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS),
                ),
                Text(
                  "Padding Right: ${gl.dsp.paddingBot.toStringAsFixed(1)}",
                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS),
                ),
                Text(
                  "ViewInset Bottom: ${MediaQuery.of(context).viewInsets.bottom.toStringAsFixed(1)}",
                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXXS),
                ),
              ],
            ),
          );

  Widget get _forestimatorOnOffline => SizedBox(
    width: gl.eqPx * 40,
    height: gl.eqPx * 7,
    child: TextButton(
      style: lt.onOflineButtonstyle,
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
              ? Text("Forestimator terrain", style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS))
              : Text("Forestimator en ligne", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
    ),
  );

  Widget get _forestimatorWarnings =>
      _mapControllerInit
          ? (_mapController.camera.zoom.round() < gl.globalMinOfflineZoom.round() && gl.getIndexForNextLayerOffline() > -1)
              ? IconButton(
                style: lt.trNoPadButtonstyle,
                iconSize: gl.eqPx * gl.iconSizeSettings,
                color: Colors.red,
                tooltip: "Si vous n'arrivez plus à visualiser les cartes hors ligne c'est que votre zoom est trop large.",
                onPressed: () {},
                icon: Icon(Icons.info_rounded),
              )
              : Container(width: gl.eqPx * gl.iconSizeSettings * 1.5)
          : Container(width: gl.eqPx * gl.iconSizeSettings * 1.5);

  double computePolygonTitleHeight() {
    double result = gl.chosenPolyBarHeight * 1.5;
    if (gl.geoReady) result += gl.chosenPolyBarHeight * 0.7;
    if (gl.Mode.editAttributes) {
      result += gl.attributeTableHeight * 1.05;
    } else if (gl.Mode.editPointMarker) {
      result += gl.chosenPolyBarHeight * 2;
    } else if (gl.Mode.editPolyMarker) {
      result += gl.chosenPolyBarHeight * 0.2;
    }
    if (gl.dsp.orientation == Orientation.landscape && result > gl.dsp.eqMaxWindowHeight) {
      result = gl.dsp.eqMaxWindowHeight;
    }
    return result;
  }

  void _stopMovingSelectedPoint() {
    _selectedPointToMove = null;
  }

  List<Polygon<String>> _getPolygonesToDraw() {
    List<Polygon<String>> that = [];
    for (GeometricLayer layer in gl.geoLayers) {
      if (layer.visibleOnMap && layer.type == "Polygon") {
        for (var g in layer.geometries) {
          if (g.numPoints > 2 && g.visibleOnMap) {
            that.add(Polygon<String>(points: g.points, color: g.colorInside, hitValue: g.identifier));
          }
        }
      }
    }
    return that;
  }

  List<Polygon<String>> _getPathsToDraw() {
    List<Polygon<String>> that = [];
    for (GeometricLayer layer in gl.geoLayers) {
      if (layer.visibleOnMap && layer.type == "Polygon") {
        for (var g in layer.geometries) {
          if (g.numPoints > 2 && g.visibleOnMap) {
            that.add(Polygon<String>(points: g.points, color: g.colorInside, hitValue: g.identifier));
          }
        }
      }
    }
    return that;
  }

  List<Marker> _getPointsToDraw({bool hitButton = false}) {
    List<Marker> that = [];
    int index = 0;
    for (GeometricLayer layer in gl.geoLayers) {
      for (ge.Geometry geometry in layer.geometries) {
        gl.selectableIcons[geometry.selectedPointIcon];
        if (geometry.visibleOnMap && geometry.numPoints > 0 && geometry.type.contains("Point")) {
          that.add(
            Marker(
              width: geometry.iconSize * gl.eqPx,
              height: geometry.iconSize * gl.eqPx,
              point: LatLng(geometry.points.first.latitude, geometry.points.first.longitude),
              child:
                  hitButton && gl.selectedGeoLayer != index
                      ? IconButton(
                        highlightColor: Colors.transparent,
                        alignment: Alignment.center,
                        style: ButtonStyle(
                          shape: WidgetStateProperty.fromMap(<WidgetStatesConstraint, ContinuousRectangleBorder>{
                            WidgetState.any: ContinuousRectangleBorder(),
                          }),
                          alignment: AlignmentGeometry.center,
                          padding: WidgetStateProperty.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                            WidgetState.any: EdgeInsetsGeometry.zero,
                          }),
                        ),
                        onPressed: () {
                          if (gl.Mode.polygon) {
                            setState(() {
                              int index = 0;
                              for (ge.Geometry it in gl.selLay.geometries) {
                                if (geometry.identifier == it.identifier) {
                                  setState(() {
                                    gl.selLay.selectedGeometry = index;
                                  });
                                }
                                index++;
                              }
                            });
                          }
                        },
                        icon: Icon(gl.selectableIcons[geometry.selectedPointIcon], size: geometry.iconSize * gl.eqPx, color: geometry.colorLine),
                      )
                      : Icon(gl.selectableIcons[geometry.selectedPointIcon], size: geometry.iconSize * gl.eqPx, color: geometry.colorLine),
            ),
          );
        }
        index++;
      }
    }
    return that;
  }

  List<CircleMarker> _drawnLayerPointsCircleMarker() {
    if (!gl.layerReady || gl.selLay.geometries.isEmpty) {
      return [];
    }
    List<CircleMarker> all = [];
    int i = 0;
    for (var point in gl.selGeo.points) {
      all.add(
        CircleMarker(
          point: point,
          radius: gl.selGeo.isSelectedLine(i) && !gl.Mode.showButtonMoveVertexesPolygon && gl.Mode.addVertexesPolygon ? iconSize / 2.7 : iconSize / 3,
          color:
              gl.selGeo.isSelectedLine(i) && !gl.Mode.showButtonMoveVertexesPolygon && gl.Mode.addVertexesPolygon
                  ? gl.selGeo.colorLine
                  : gl.selGeo.colorInside,
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
                          ? WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{WidgetState.any: lt.getColorFromName(poi.name)})
                          : WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{WidgetState.any: lt.getColorFromName(poi.name)}),
                  foregroundColor:
                      gl.selectedSearchMarker == i
                          ? WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                            WidgetState.any: lt.getColorTextFromBackground(lt.getColorFromName(poi.name)),
                          })
                          : WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                            WidgetState.any: lt.getColorTextFromBackground(lt.getColorFromName(poi.name)),
                          }),
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
                              ? Text(poi.name, textScaler: TextScaler.linear(1.0))
                              : Text(poi.name, textScaler: TextScaler.linear(scale)),
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
              backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent}),
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
                child: Text(poi.address, textScaler: TextScaler.linear(scale), textAlign: TextAlign.center, maxLines: 10),
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
                  ? gl.eqPx * gl.anaPtBoxSize
                  : gl.dico.getLayerBase(gl.anaPtPreview!.mCode).getValLabel(gl.anaPtPreview!.mRastValue).length > 3
                  ? gl.eqPx * gl.anaPtBoxSize * 1.5 +
                      gl.dico.getLayerBase(gl.anaPtPreview!.mCode).getValLabel(gl.anaPtPreview!.mRastValue).length * 8.0
                  : gl.dico.getLayerBase(gl.anaPtPreview!.mCode).mCategorie != "Externe"
                  ? gl.eqPx * gl.anaPtBoxSize * 2.5
                  : 0.0,
          height: gl.eqPx * gl.anaPtBoxSize,
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
                await _runAnaPt(epsg4326.transform(epsg31370, proj4.Point(x: _pt!.longitude, y: _pt!.latitude)));
                gl.refreshStack(() {
                  popupForestimatorWindow(
                    id: "anaPres",
                    title: "Resultats de l'analyse",
                    child: AnaResultsMenu(() {
                      gl.refreshStack(() {});
                    }, gl.requestedLayers),
                  );
                });
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
        child: const Icon(Icons.donut_small, color: Color.fromRGBO(0, 0, 0, .75)),
      ),
      Marker(
        alignment: Alignment.center,
        width: 50,
        height: 50,
        point: _pt ?? const LatLng(0.0, 0.0),
        child: const Icon(Icons.donut_large_sharp, color: Color.fromRGBO(255, 255, 255, .75)),
      ),
    ];
  }

  List<Marker> _placeVertexMovePointer() {
    List<Marker> ret = [];
    if ((gl.Mode.moveVertexesPolygon && _selectedPointToMove != null) || (_modeMeasurePath && _modeMoveMeasurePath)) {
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
          width: gl.eqPx * gl.anaPtBoxSize * 2,
          height: gl.eqPx * gl.anaPtBoxSize,
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
    return Offset(gl.epsg4326.transform(gl.epsg31370, spPoint).x, gl.epsg4326.transform(gl.epsg31370, spPoint).y);
  }

  double _computePathLength() {
    if (_measurePath.length < 2) {
      return 0.0;
    }
    List<Offset> path = [];
    for (LatLng point in _measurePath) {
      path.add(_epsg4326ToEpsg31370(proj4.Point(y: point.latitude, x: point.longitude)));
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
                  if (_modeMoveMeasurePath || (selectedMeasurePointToMove > -1 && selectedMeasurePointToMove != i)) {
                    selectedMeasurePointToMove = i;
                    centerOnLatLng(_measurePath[i]);
                  } else {
                    selectedMeasurePointToMove = -1;
                    centerOnLatLng(_measurePath[i]);
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
    if (gl.selLay.geometries.isEmpty) {
      return [];
    }
    List<Marker> all = [];
    int count = 0;
    for (var point in gl.selGeo.points) {
      all.add(
        Marker(
          alignment: Alignment.center,
          width: iconSize,
          height: iconSize,
          point: point,
          child: TextButton(
            onPressed: () {
              if (gl.Mode.addVertexesPolygon) {
                refreshView(() {
                  gl.selGeo.refreshSelectedLinePoints(point);
                });
              } else if (gl.Mode.moveVertexesPolygon) {
                refreshView(() {
                  if (gl.selGeo.selectedVertex != 1 + count) {
                    gl.selGeo.selectedVertex = count;
                  } else {
                    gl.selGeo.selectedVertex = -1;
                  }
                });
              } else {
                refreshView(() {
                  gl.selGeo.refreshSelectedLinePoints(point);
                  gl.Mode.showButtonAddVertexesPolygon = false;
                  gl.Mode.showButtonMoveVertexesPolygon = true;
                  gl.Mode.showButtonRemoveVertexesPolygon = true;
                  gl.Mode.addVertexesPolygon = false;
                  gl.Mode.moveVertexesPolygon = false;
                  gl.Mode.removeVertexesPolygon = false;
                });
              }
            },
            child: Text(
              overflow: TextOverflow.visible,
              count == gl.selGeo.numPoints - 1 ? "1" : "${count + 2}",
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
    _closeEditingMenu();
    _stopMovingSelectedPoint();
    _polygonMode = false;
    gl.Mode.editPolygon = false;
    gl.Mode.editAttributes = false;
    gl.Mode.editPoint = false;
    gl.Mode.editPointMarker = false;
    gl.Mode.editPolyMarker = false;
    _modeSearch = false;
    gl.Mode.removeVertexesPolygon = false;
    gl.Mode.moveVertexesPolygon = false;
    _modeAnaPtPreview = true;
    gl.Mode.showButtonAddVertexesPolygon = false;
    gl.Mode.showButtonMoveVertexesPolygon = false;
    gl.Mode.showButtonRemoveVertexesPolygon = false;
  }

  set _polygonMode(bool mode) {
    setState(() {
      gl.Mode.polygon = mode;
    });
    if (gl.Mode.polygon) {
      if (gl.Mode.polygonList) {
        popupLayerListMenu(
          (LatLng pos) {
            if (pos.longitude != 0.0 && pos.latitude != 0.0) {
              centerOnLatLng(pos);
            }
          },
          () {
            setState(() {
              gl.Mode.polygonList = false;
              if (!gl.layerReady) {
                _closePolygonMenu();
              } else {
                _polygonMode = true;
              }
            });
          },
        );
      }
    }
  }

  set _toolBarMenu(bool mode) {
    setState(() {
      _toolbarExtended = mode;
    });
  }

  void _closeToolbarMenu() {
    gl.stack.pop("SearchMenu");
    _toolBarMenu = false;
    _modeSearch = false;
    _modeMeasurePath = false;
  }

  void _closeSwitchesMenu() {
    gl.stack.pop("LayerSwitcher");
    _modeLayerSwitches = false;
  }

  Widget _mainMenuBar({bool dummy = false, VoidCallback? close}) {
    return Container(
      alignment: Alignment.center,
      width: gl.eqPx * gl.menuBarLength,
      height: gl.eqPx * gl.menuBarThickness,
      child: Card(
        shadowColor: Colors.transparent,
        color: dummy ? Colors.transparent : gl.backgroundTransparentBlackBox,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
            _menuButton(
              gl.eqPx * gl.menuBarLength / 4,
              gl.eqPx * gl.menuBarThickness,
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
                    _toolBarMenu = true;
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
              Icon(Icons.forest, size: gl.eqPx * gl.menuBarLength / 5),
            ),
            _menuButton(
              gl.eqPx * gl.menuBarLength / 4,
              gl.eqPx * gl.menuBarThickness,
              gl.Mode.polygon,
              Colors.yellow,
              () {
                setState(() {
                  if (!gl.Mode.polygon) {
                    if (!gl.layerReady) {
                      gl.Mode.polygonList = true;
                    }
                    _polygonMode = true;
                    _closeSwitchesMenu();
                    _closeToolbarMenu();
                    if (dummy) {
                      close!();
                    }
                  } else {
                    if (gl.layerReady) {
                      if (gl.Mode.polygonList) {
                        gl.Mode.polygonList = false;
                        _polygonMode = true;
                      } else {
                        _closePolygonMenu();
                      }
                    } else {
                      gl.Mode.polygonList = false;
                      _closePolygonMenu();
                    }
                  }
                });
              },
              () {
                refreshView(() {
                  _closePolygonMenu();
                });
              },
              Icon(Icons.hexagon_outlined, size: gl.eqPx * gl.menuBarLength / 5),
            ),
            _menuButton(
              gl.eqPx * gl.menuBarLength / 4,
              gl.eqPx * gl.menuBarThickness,
              _modeLayerSwitches,
              Colors.brown,
              () {
                setState(() {
                  if (!_modeLayerSwitches) {
                    _modeLayerSwitches = true;
                    _closePolygonMenu();
                    _closeToolbarMenu();
                    gl.stack.add(
                      "LayerSwitcher",
                      LayerSwitcher(
                        (LatLng pos) {
                          if (pos.longitude != 0.0 && pos.latitude != 0.0) {
                            centerOnLatLng(pos);
                          }
                        },
                        () {
                          setState(() {
                            _closeSwitchesMenu();
                          });
                        },
                      ),
                      Duration(milliseconds: 400),
                      gl.Anim.onScreenPosCenter,
                      Offset(0, -500),
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
              Icon(Icons.remove_red_eye, size: gl.eqPx * gl.menuBarLength / 5),
            ),
          ],
        ),
      ),
    );
  }

  Widget _menuButton(double width, double height, bool isSelected, Color color, VoidCallback onPressed, VoidCallback onLongPress, Icon icon) {
    return Container(
      width: width,
      height: height,
      color: !isSelected ? Colors.transparent : color.withAlpha(128),
      child: IconButton(
        style: ButtonStyle(
          shape: WidgetStateProperty.fromMap(<WidgetStatesConstraint, ContinuousRectangleBorder>{WidgetState.any: ContinuousRectangleBorder()}),
          alignment: AlignmentGeometry.center,
          padding: WidgetStateProperty.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{WidgetState.any: EdgeInsetsGeometry.zero}),
        ),
        color: isSelected ? Colors.white : color,
        onPressed: onPressed,
        onLongPress: onLongPress,
        icon: icon,
      ),
    );
  }

  Widget get toolBar {
    double toolbarHeight = gl.iconSizeM * 2 + gl.iconSpaceBetween * 2;
    if (gl.modeDevelopper) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    if (gl.positionInit && _positionInsideViewRectangle(gl.position)) {
      toolbarHeight += gl.iconSizeM + gl.iconSpaceBetween;
    }
    return Container(
      alignment: Alignment.center,
      height: gl.eqPx * toolbarHeight,
      width: gl.eqPx * gl.menuBarThickness,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          SizedBox(
            height: gl.eqPx * toolbarHeight,
            width: gl.eqPx * gl.menuBarThickness,
            child: Card(
              color: gl.backgroundTransparentBlackBox,

              child: Column(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  gl.positionInit
                      ? Card(
                        color: Colors.orange.withAlpha(128),
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.end,
                          children: [
                            if (_positionInsideViewRectangle(gl.position))
                              IconButton(
                                iconSize: gl.eqPx * gl.iconSizeM,
                                color: gl.colorAgroBioTech,
                                onPressed: () async {
                                  if (!_doingAnaPt) {
                                    refreshView(() {
                                      _doingAnaPt = true;
                                    });
                                    await _runAnaPt(epsg4326.transform(epsg31370, proj4.Point(x: gl.position.longitude, y: gl.position.latitude)));
                                    _updatePtMarker(LatLng(gl.position.latitude, gl.position.longitude));
                                    gl.refreshStack(() {
                                      popupForestimatorWindow(
                                        id: "anaPres",
                                        title: "Resultats de l'analyse",
                                        child: AnaResultsMenu(() {
                                          gl.refreshStack(() {});
                                        }, gl.requestedLayers),
                                      );
                                    });
                                    refreshView(() {
                                      _doingAnaPt = false;
                                    });
                                  }
                                },
                                icon: const Icon(Icons.analytics),
                              ),
                            IconButton(
                              iconSize: gl.eqPx * gl.iconSizeM,
                              color: Colors.red,
                              onPressed: () async {
                                if (gl.positionInit) {
                                  refreshView(() {
                                    _mapController.move(LatLng(gl.position.latitude, gl.position.longitude), 8);
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
                            iconSize: gl.eqPx * gl.iconSizeM,
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
                      color: !_modeMeasurePath ? Colors.transparent : Colors.lightBlue.withAlpha(128),
                      child: IconButton(
                        color: _modeMeasurePath ? Colors.white : Colors.lightBlue,
                        iconSize: gl.eqPx * gl.iconSizeM,
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
                    color: !_modeSearch ? Colors.transparent : Colors.blueGrey.withAlpha(128),
                    child: IconButton(
                      iconSize: gl.eqPx * gl.iconSizeM,
                      color: _modeSearch ? Colors.white : Colors.blueGrey,
                      onPressed: () {
                        setState(() {
                          _modeSearch = true;
                        });
                        PopupSearchMenu(
                          (LatLng pos) {
                            if (pos.longitude != 0.0 && pos.latitude != 0.0) {
                              centerOnLatLng(pos);
                            }
                          },
                          () {
                            gl.refreshStack(() {
                              gl.stack.pop("SearchMenu");
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
    );
  }

  List<Marker> _getPolygonesLabels() {
    if (gl.selectedGeoLayer < 0) return [];
    return List.generate(gl.selLay.geometries.length, (i) {
      String textArea = "${(gl.selLay.geometries[i].area / 100).round() / 100} Ha";
      return gl.selLay.geometries[i].visibleOnMap && gl.selLay.geometries[i].labelsVisibleOnMap
          ? gl.selLay.geometries[i].type == "Polygon"
              ? Marker(
                alignment: Alignment.center,
                width: gl.eqPx * (!gl.Mode.debugLabel ? gl.infoBoxPolygon * .6 : gl.infoBoxPolygon),
                height: gl.eqPx * (gl.selLay.geometries[i].getNCheckedAttributes() + 1) * gl.iconSizeS * .8 + 5,
                point: gl.selLay.geometries[i].center,
                child: Card(
                  color: !gl.Mode.debugLabel ? Colors.white.withAlpha(100) : Colors.black.withAlpha(200),
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
                                  height: gl.eqPx * gl.iconSizeXS,
                                  width: gl.eqPx * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {},
                                    icon: FaIcon(FontAwesomeIcons.list, size: gl.eqPx * gl.iconSizeXS * .5, color: Colors.transparent),
                                  ),
                                ),
                              Container(
                                padding: EdgeInsets.all(2),
                                alignment: Alignment.center,
                                color: Colors.transparent,
                                height: gl.eqPx * gl.iconSizeXS,
                                width: gl.eqPx * gl.infoBoxPolygon / 2,

                                child: SingleChildScrollView(
                                  scrollDirection: Axis.horizontal,
                                  child: Text(
                                    gl.selLay.geometries[i].name,
                                    textAlign: TextAlign.center,
                                    style: TextStyle(color: !gl.Mode.debugLabel ? Colors.black : Colors.white, fontSize: gl.eqPx * gl.fontSizeXS),
                                  ),
                                ),
                              ),
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.zero,
                                  color: Colors.transparent,
                                  height: gl.eqPx * gl.iconSizeXS,
                                  width: gl.eqPx * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {
                                      refreshView(() {
                                        gl.selLay.geometries[i].labelsVisibleOnMap = false;
                                      });
                                      gl.selLay.geometries[i].serialize();
                                    },
                                    icon: FaIcon(Icons.close, size: gl.eqPx * gl.iconSizeXS * .8, color: Colors.red),
                                  ),
                                ),
                            ],
                          ),
                          if (gl.Mode.debugLabel && gl.selLay.geometries[i].getNCheckedAttributes() > 1)
                            lt.stroke(gl.eqPx * 0.5, gl.eqPx * 0.25, gl.colorAgroBioTech),
                        ] +
                        List<Widget>.generate(gl.selLay.geometries[i].attributes.length, (j) {
                          return gl.selLay.geometries[i].attributes[j].visibleOnMapLabel
                              ? Container(
                                padding: EdgeInsetsDirectional.symmetric(horizontal: gl.eqPx * 2),
                                color: Colors.transparent,
                                height: gl.eqPx * gl.iconSizeXS,
                                child: Row(
                                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                  children: [
                                    if (gl.Mode.debugLabel)
                                      Container(
                                        alignment: Alignment.centerLeft,
                                        width: gl.eqPx * 15,
                                        child: SingleChildScrollView(
                                          scrollDirection: Axis.horizontal,
                                          child: Text(
                                            gl.selLay.geometries[i].attributes[j].name,
                                            textAlign: TextAlign.center,
                                            style: TextStyle(
                                              color: !gl.Mode.debugLabel ? Colors.black : Colors.white,
                                              fontSize: gl.eqPx * gl.fontSizeXS,
                                            ),
                                          ),
                                        ),
                                      ),
                                    if (!gl.Mode.debugLabel) lt.stroke(vertical: true, gl.eqPx * 0.5, gl.eqPx * 0.25, gl.colorAgroBioTech),
                                    Container(
                                      alignment: Alignment.centerLeft,
                                      width: gl.eqPx * 15,
                                      child: SingleChildScrollView(
                                        scrollDirection: Axis.horizontal,
                                        child: Text(
                                          gl.selLay.geometries[i].attributes[j].value.toString(),
                                          textAlign: TextAlign.center,
                                          style: TextStyle(
                                            color: !gl.Mode.debugLabel ? Colors.black : Colors.white,
                                            fontSize: gl.eqPx * gl.fontSizeXS,
                                          ),
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                              )
                              : SizedBox();
                        }),
                  ),
                ),
              )
              : gl.selLay.geometries[i].type.contains("Point")
              ? Marker(
                alignment: Alignment.bottomLeft,
                width: gl.eqPx * (!gl.Mode.debugLabel ? gl.infoBoxPolygon * .6 : gl.infoBoxPolygon),
                height:
                    gl.eqPx *
                        (gl.selLay.geometries[i].getNCheckedAttributes() + ((!gl.selLay.geometries[i].type.contains("essence")) ? 1 : 0)) *
                        gl.iconSizeS *
                        .8 +
                    5,
                point: gl.selLay.geometries[i].center,
                child: Card(
                  color: !gl.Mode.debugLabel ? Colors.white.withAlpha(100) : Colors.black.withAlpha(200),
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
                                  height: gl.eqPx * gl.iconSizeXS,
                                  width: gl.eqPx * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {},
                                    icon: FaIcon(FontAwesomeIcons.list, size: gl.eqPx * gl.iconSizeXS * .5, color: Colors.transparent),
                                  ),
                                ),
                              if (!gl.selLay.geometries[i].type.contains("essence"))
                                Container(
                                  padding: EdgeInsets.all(2),
                                  alignment: Alignment.center,
                                  color: Colors.transparent,
                                  height: gl.eqPx * gl.iconSizeXS,
                                  width: gl.eqPx * gl.infoBoxPolygon / 2,
                                  child: SingleChildScrollView(
                                    scrollDirection: Axis.horizontal,
                                    child: Text(
                                      gl.selLay.geometries[i].name,
                                      textAlign: TextAlign.center,
                                      style: TextStyle(color: !gl.Mode.debugLabel ? Colors.black : Colors.white, fontSize: gl.eqPx * gl.fontSizeXS),
                                    ),
                                  ),
                                ),
                              if (gl.Mode.labelCross)
                                Container(
                                  padding: EdgeInsets.zero,
                                  color: Colors.transparent,
                                  height: gl.eqPx * gl.iconSizeXS,
                                  width: gl.eqPx * gl.iconSizeXS,
                                  child: IconButton(
                                    style: ButtonStyle(
                                      animationDuration: Duration(seconds: 1),
                                      backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                        WidgetState.any: Colors.transparent,
                                      }),
                                      padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                                        WidgetState.any: EdgeInsetsGeometry.zero,
                                      }),
                                    ),
                                    onPressed: () {
                                      refreshView(() {
                                        gl.selLay.geometries[i].labelsVisibleOnMap = false;
                                      });
                                      gl.selLay.geometries[i].serialize();
                                    },
                                    icon: FaIcon(Icons.close, size: gl.eqPx * gl.iconSizeXS * .8, color: Colors.red),
                                  ),
                                ),
                            ],
                          ),
                          if (gl.selLay.geometries[i].getNCheckedAttributes() > 1) lt.stroke(gl.eqPx * 0.5, gl.eqPx * 0.25, gl.colorAgroBioTech),
                        ] +
                        List<Widget>.generate(gl.selLay.geometries[i].attributes.length, (j) {
                          return gl.selLay.geometries[i].attributes[j].visibleOnMapLabel
                              ? Container(
                                padding: EdgeInsetsDirectional.symmetric(horizontal: gl.eqPx * 2),
                                color: Colors.transparent,
                                height: gl.eqPx * gl.iconSizeXS,
                                child: Row(
                                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                  children: [
                                    if (gl.Mode.debugLabel)
                                      Container(
                                        alignment: Alignment.centerLeft,
                                        width: gl.eqPx * 15,
                                        child: SingleChildScrollView(
                                          scrollDirection: Axis.horizontal,
                                          child: Text(
                                            gl.selLay.geometries[i].attributes[j].name,
                                            textAlign: TextAlign.center,
                                            style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS),
                                          ),
                                        ),
                                      ),
                                    if (gl.Mode.debugLabel) lt.stroke(vertical: true, gl.eqPx * 0.5, gl.eqPx * 0.25, gl.colorAgroBioTech),
                                    Container(
                                      alignment: Alignment.centerLeft,
                                      width: gl.eqPx * 15,
                                      child: SingleChildScrollView(
                                        scrollDirection: Axis.horizontal,
                                        child: Text(
                                          gl.selLay.geometries[i].attributes[j].value.toString(),
                                          textAlign: TextAlign.center,
                                          style: TextStyle(
                                            color: !gl.Mode.debugLabel ? Colors.black : Colors.white,
                                            fontSize: gl.eqPx * gl.fontSizeXS,
                                          ),
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                              )
                              : SizedBox();
                        }),
                  ),
                ),
              )
              : Marker(
                alignment: Alignment.center,
                width:
                    textArea.length > gl.selLay.geometries[i].name.length
                        ? gl.eqPx * gl.infoBoxPolygon * 2.5 + textArea.length * gl.fontSizeS
                        : gl.eqPx * gl.infoBoxPolygon * 1.5 + gl.selLay.geometries[i].name.length * gl.fontSizeS,
                height: gl.eqPx * gl.infoBoxPolygon * 1.5,
                point: gl.selLay.geometries[i].center,
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
                                Icon(Icons.layers, color: gl.selLay.geometries[i].colorLine),
                                Text("Unknown Geometry", overflow: TextOverflow.clip),
                              ],
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
              )
          : Marker(alignment: Alignment.center, width: 1, height: 1, point: gl.selLay.geometries[i].center, child: SizedBox());
    });
  }

  void _writePositionDataToSharedPreferences(double lon, double lat, double zoom) async {
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
        gl.Mode.gpsTimoutException = false;
      }
      Position newPosition = await Geolocator.getCurrentPosition().timeout(
        Duration(seconds: 5),
        onTimeout: () {
          gl.print("Geolocator timeout reached!");
          gl.Mode.gpsTimoutException = true;
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

  Column _getFixedAttribute(String name, String values, {bool checked = false}) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            Container(
              alignment: Alignment.center,
              width: gl.eqPx * 11,
              child: Text("FIXED", style: TextStyle(color: Colors.grey, fontSize: gl.fontSizeXXS * gl.eqPx)),
            ),
            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            SizedBox(
              width: gl.eqPx * 7,
              child:
                  checked
                      ? Icon(Icons.check_box, color: Colors.white12, size: gl.eqPx * gl.iconSizeXS)
                      : Icon(Icons.check_box_outline_blank, color: Colors.white12, size: gl.eqPx * gl.iconSizeXS),
            ),
            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            Container(
              alignment: Alignment.centerLeft,
              width: gl.eqPx * 33,
              child: Text(name, style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM * .75)),
            ),
            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            Container(
              alignment: Alignment.centerLeft,
              width: gl.eqPx * 33,
              child: SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: Text(values, textAlign: TextAlign.center, style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM * .75)),
              ),
            ),
          ],
        ),
        lt.stroke(gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
      ],
    );
  }

  bool _positionInsideViewRectangle(Position p) => _mapController.camera.visibleBounds.contains(LatLng(p.latitude, p.longitude));
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
      _runAnaPtPreview(gl.epsg4326ToEpsg31370(proj4.Point(x: widget.position.longitude, y: widget.position.latitude)), widget.after);
    }
    if (gl.anaPtPreview != null) {
      Color color = gl.dico.getLayerBase(gl.anaPtPreview!.mCode).getValColor(gl.anaPtPreview!.mRastValue);
      String text = gl.dico.getLayerBase(gl.anaPtPreview!.mCode).getValLabel(gl.anaPtPreview!.mRastValue);
      return Container(
        alignment: Alignment.centerLeft,
        height: gl.eqPx * gl.fontSizeM * 1.1,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Container(
              alignment: Alignment.center,
              constraints: BoxConstraints(minHeight: gl.fontSizeM * gl.eqPx, minWidth: gl.fontSizeM * gl.eqPx),
              color: Colors.white,
              child: Container(
                alignment: Alignment.center,
                height: (gl.fontSizeM - .5) * gl.eqPx,
                width: (gl.fontSizeM - .5) * gl.eqPx,
                color: color == Colors.transparent ? Colors.white : color,
              ),
            ),
            Container(
              constraints: BoxConstraints(minHeight: gl.fontSizeM * gl.eqPx, minWidth: gl.fontSizeM * gl.eqPx),
              child: Text(":", textAlign: TextAlign.center),
            ),
            Text(textAlign: TextAlign.center, text == "" ? "No data" : text, style: TextStyle(fontSize: gl.fontSizeS * gl.eqPx)),
          ],
        ),
      );
    }
    return CircularProgressIndicator(
      color: gl.colorAgroBioTech,
      constraints: BoxConstraints(
        maxHeight: gl.eqPx * gl.anaPtBoxSize * .9,
        minHeight: gl.eqPx * gl.anaPtBoxSize * .8,
        maxWidth: gl.eqPx * gl.anaPtBoxSize * .9,
        minWidth: gl.eqPx * gl.anaPtBoxSize * .8,
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
        gl.stack.add(
          "NoInternet",
          popupNoInternet(() {
            mounted
                ? setState(() {
                  gl.stack.pop("NoInternet");
                })
                : gl.stack.pop("NoInternet");
          }),
          Duration(milliseconds: 400),
          Offset.zero,
          Offset(0, -250),
        );
      }
    } else {
      int val = await gl.dico.getLayerBase(gl.switcherMaps.first.mCode).getValXY(ptBL72);
      setState(() {
        gl.anaPtPreview = LayerAnaPt(mCode: gl.switcherMaps.first.mCode, mRastValue: val);
      });
    }
    after();
  }
}
