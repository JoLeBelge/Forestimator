import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:flutter/material.dart';
import 'dart:math';
import 'package:url_launcher/url_launcher.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/locationIndicator/animated_location_indicator.dart';
import 'package:fforestimator/globals.dart' as gl;

class mapPage extends StatefulWidget {
  final Function runAnaPt;
  const mapPage({required this.runAnaPt, super.key, required this.title});

  final String title;

  @override
  State<mapPage> createState() => _mapPageState();
}

class _mapPageState extends State<mapPage> {
  final _mapController = MapController();

  Position? _position;

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
        body: Stack(children: <Widget>[
          FlutterMap(
            mapController: _mapController,
            options: MapOptions(
              backgroundColor: Colors.transparent,
              keepAlive: true,
              interactionOptions: const InteractionOptions(
                  flags: InteractiveFlag.drag |
                      InteractiveFlag.pinchZoom |
                      InteractiveFlag.pinchMove |
                      InteractiveFlag.doubleTapZoom),
              crs: epsg31370CRS,
              initialZoom: 2,
              maxZoom: 7,
              initialCenter: latlonEpioux,
              cameraConstraint: CameraConstraint.contain(
                  bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
              onMapReady: () async {
                _position = await acquireUserLocation();
                /*if (_position != null) {//TODO: Ceci tue l'appli!
              // IMPORTANT: rebuild location layer when permissions are granted
              setState(() {
                _mapController.move(
                    LatLng(_position?.latitude ?? 0.0,
                        _position?.longitude ?? 0.0),
                    16);
              });
            }*/
              },
            ),
            children: gl.interfaceSelectedLayerKeys.isNotEmpty
                ? List<TileLayer>.generate(
                    gl.interfaceSelectedLayerKeys.length,
                    (i) => TileLayer(
                      wmsOptions: WMSTileLayerOptions(
                        baseUrl: gl
                                .dico
                                .mLayerBases[gl.interfaceSelectedLayerKeys[gl.interfaceSelectedLayerKeys.length - i - 1]]!
                                .mUrl! +
                            "?",
                        format: 'image/png',
                        layers: [
                          gl.dico.mLayerBases[gl.interfaceSelectedLayerKeys[gl.interfaceSelectedLayerKeys.length - i - 1]]!
                              .mWMSLayerName!,
                        ],
                        crs: epsg31370CRS,
                        transparent: true,
                      ),
                      //maxNativeZoom: 7,
                      tileSize: tileSize,
                    ),
                  )
                : [
                      TileLayer(
                        wmsOptions: WMSTileLayerOptions(
                          baseUrl: (gl.interfaceSelectedLayerKeys.isNotEmpty &&
                                      gl.dico.mLayerBases.keys.contains(
                                          gl.interfaceSelectedLayerKeys[0])
                                  ? gl
                                      .dico
                                      .mLayerBases[
                                          gl.interfaceSelectedLayerKeys[0]]!
                                      .mUrl!
                                  : gl.dico.mLayerBases[gl.defaultLayer]!
                                      .mUrl!) +
                              "?",
                          format: 'image/png',
                          layers: [
                            gl.dico.mLayerBases[gl.defaultLayer]!
                                .mWMSLayerName!,
                          ],
                          crs: epsg31370CRS,
                          transparent: true,
                        ),
                        //maxNativeZoom: 7,
                        tileSize: tileSize,
                      )
                    ] +
                    [
                      /*RichAttributionWidget(
                attributions: [
                  TextSourceAttribution(
                    'OpenStreetMap contributors',
                    onTap: () => launchUrl(
                        Uri.parse('https://openstreetmap.org/copyright')),
                  ),
                ],
              ),*/
                      /*MarkerLayer(
                markers: [
                  Marker(
                    width: 50.0,
                    height: 50.0,
                    point: latlonEpioux,
                    child: const FlutterLogo(),
                  ),
                ],
              ),*/
                      /*const AnimatedLocationLayer(
                  // cameraTrackingMode: CameraTrackingMode.locationAndOrientation,
                  ),*/
                    ],
          ),

          /*Align(
            alignment: Alignment.bottomRight,
            // add your floating action button
            child: FloatingActionButton(
              onPressed: () {},
              child: Icon(Icons.map),
            ),
          ),*/
          Align(
            alignment: Alignment.bottomCenter,
            // add your floating action button
            child: FloatingActionButton(
              onPressed: () {
                //_anaPonctOnline();
                if (_position != null) {
                  //print("postion x " + _position?.latitude.toString() ?? "0" );// + " " + _position?.longitude.toString());
                  print(_position?.latitude.toString() ??
                      "empty position"); // + " " + _position?.longitude.toString());
                  print("anaPonctOnline");
                  // on projete en BL72, seul src de Forestimator web pour le moment
                  proj4.Point ptBL72 = epsg4326.transform(
                      epsg31370,
                      proj4.Point(
                          x: _position?.longitude ?? 0.0,
                          y: _position?.latitude ?? 0.0));
                  widget.runAnaPt(ptBL72);
                }
              },
              child: Icon(Icons.gps_fixed),
            ),
          ),
        ]));
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
