import 'package:flutter/material.dart';
import 'package:test_fluttermap/globals.dart' as gl;
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import 'package:path/path.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'dart:math';
import 'package:image/image.dart' as img;
import 'package:test_fluttermap/cropImTileProvider.dart';
import 'package:flutter/services.dart';
import 'myPngDecoder.dart';

String out = '/home/jo/Images/chatGPT_crop.png';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final File fileIm =
      File('/home/jo/Documents/carteApt/colorMappingTest/aptitudeFEE_BV.tif');

  // un tif avec une palette de couleur qui correspond au dicocol de la layer base. TifDecoder se charge d'appliquer cette palette.
  // j'ai généré ce tif avec palette avec l'api de forestimator (en local, pas encore pushé sur le server)
  //http://localhost:8085/api/clipRast/layerCode/BV_FEE/BV_FEE_colorP.tif
  /*ByteData data =
      await rootBundle.load(url.join("assets", "BV_FEE_colorP.tif"));*/

  /*myPngDecoder _decoder = myPngDecoder();
  _decoder.startDecode(data.buffer.asUint8List());
  // je pense que c'est la position du début de chacun des chunks du fichier (offset). en tout cas nombre constant entre chacunes des valeurs, un peu plus élevé que le nombre de colonne du raster
  print("decoder idat : " + _decoder.infoI.idat.toString());*/
/*
  if (false) {
    final stopwatch = Stopwatch()..start();
    gl.Fullimage = myPngDecoder(x0: 5000, y0: 2000, wSub: 500, hSub: 500)
        .decode(data.buffer.asUint8List())!;
    print('myPngDecode with 500x500 pix + executed in ${stopwatch.elapsed}');
    File(out).writeAsBytes(img.encodePng(gl.Fullimage));
    stopwatch.start();
    gl.Fullimage = img.PngDecoder().decode(data.buffer.asUint8List())!;
    print('PngDecode executed in ${stopwatch.elapsed}');
  }
  */
  final stopwatch = Stopwatch()..start();

  //gl.Fullimage = img.TiffDecoder().decode(data.buffer.asUint8List())!;
  //print('TifDecode executed in ${stopwatch.elapsed}');

  /*img.Image cropped =
      img.copyCrop(gl.Fullimage, x: 0, y: 0, width: 100, height: 100);
  File(out).writeAsBytes(img.encodePng(cropped));*/

  runApp(const mapPage());
}

class mapPage extends StatefulWidget {
  const mapPage({super.key});

  @override
  State<mapPage> createState() => _MapPageState();
}

class _MapPageState extends State<mapPage> {
  final _mapController = MapController();
  //LatLng? _pt;

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
  /* List<double> getResolutions(double maxX, double minX, int zoom,
      [double tileSize = 256.0]) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var size = (maxX - minX) / (tileSize);
    return List.generate(zoom, (z) => size / pow(2, z));
  }*/

  // création d'un vecteur de résolution qui soit le même que celui définit par gdal2tile lors du tiling de mes rasters, pour compatibilité.
  List<double> getResolutions2(int nbzoom) {
    // résolution numéro 1: une tile pour tout l'extend de la Wallonie
    var maxResolution = 1280;
    return List.generate(nbzoom, (z) => maxResolution / pow(2, z));
  }

  late Proj4Crs epsg31370CRS = Proj4Crs.fromFactory(
      code: 'EPSG:31370',
      proj4Projection: epsg31370,
      bounds: epsg31370Bounds,
      resolutions: getResolutions2(11),
      origins: [
        Point<double>(42250.0, 21170.0)
      ]); // ATTENTION sans l'origine définie ici, les tuiles partent de la position (0,0) ce qui est incompatible avec les tuiles résultant de gdla2tiles

  //img.Image cropped =
  //  img.copyCrop(gl.Fullimage, x: 0, y: 0, width: 256, height: 256);

  late tifFileTileProvider _provider;

  @override
  void initState() {
    super.initState();
    gl.refreshMap = setState;
    _provider = tifFileTileProvider(
        mycrs: epsg31370CRS, sourceImPath: "BV_FEE_colorP.tif");
    _provider.init();
  }

  @override
  Widget build(BuildContext context) {
    print(getResolutions2(11));
    //print(getResolutions(295170.0, 42250.0, 15, 256.0));
    proj4.Point ptEpioux = proj4.Point(x: 217200.0, y: 50100.0);
    proj4.Point ptBotLeft = proj4.Point(
        x: epsg31370Bounds.bottomLeft.x, y: epsg31370Bounds.bottomLeft.y);
    proj4.Point ptTopR = proj4.Point(
        x: epsg31370Bounds.topRight.x, y: epsg31370Bounds.topRight.y);

    // WARNING lat =y lon=x
    LatLng latlonEpioux = LatLng(epsg31370.transform(epsg4326, ptEpioux).y,
        epsg31370.transform(epsg4326, ptEpioux).x);

    // contraindre la vue de la map sur la zone de la Wallonie. ajout d'un peu de marge
    double margeInDegree = 0;
    //bottomLeft
    LatLng latlonBL = LatLng(
        epsg31370.transform(epsg4326, ptBotLeft).y + margeInDegree,
        epsg31370.transform(epsg4326, ptBotLeft).x - margeInDegree);
    // topRigth
    LatLng latlonTR = LatLng(
        epsg31370.transform(epsg4326, ptTopR).y - margeInDegree,
        epsg31370.transform(epsg4326, ptTopR).x + margeInDegree);

    LatLngBounds _bound = LatLngBounds.fromPoints([latlonBL, latlonTR]);

    return MaterialApp(
        home: Scaffold(
            appBar: AppBar(
              title: Text(
                "Forestimator",
                textScaler: TextScaler.linear(0.75),
              ),
              toolbarHeight: 20.0,
              backgroundColor: gl.colorAgroBioTech,
            ),
            body: //Image(image: MemoryImage(img.encodePng(cropped))),

                FlutterMap(
              mapController: _mapController,
              options: MapOptions(
                //backgroundColor: Colors.transparent,
                keepAlive: true,
                interactionOptions: const InteractionOptions(
                  enableMultiFingerGestureRace: false,
                  flags: InteractiveFlag.drag |
                      InteractiveFlag.pinchZoom |
                      InteractiveFlag.pinchMove |
                      InteractiveFlag.doubleTapZoom |
                      InteractiveFlag.scrollWheelZoom,
                ),
                crs: epsg31370CRS,
                initialZoom: 7.0,
                maxZoom: 10, //14
                minZoom: 4, //4
                initialCenter: latlonEpioux,
                cameraConstraint: CameraConstraint.contain(
                    bounds: LatLngBounds.fromPoints([latlonBL, latlonTR])),
              ),
              children: [
                if (_provider.loaded)
                  TileLayer(
                    //tileBuilder: mycoordinateDebugTileBuilder, // très utile pour visualiser comment sont réparties les tuiles celon les conventions de flutter_map
                    // tileProvider: gdal2tilesFileTileProvider(mycrs: epsg31370CRS),
                    tileProvider: _provider,
                    //maxNativeZoom: 7,
                    minNativeZoom: 8,
                    minZoom: 8,
                  ),
                if (true)
                  TileLayer(
                    // tileBuilder: mycoordinateDebugTileBuilder,
                    tileProvider: NetworkTileProvider(),
                    wmsOptions: WMSTileLayerOptions(
                      baseUrl:
                          "https://geoservices.wallonie.be/arcgis/services/SOL_SOUS_SOL/CNSW/MapServer/WMSServer?",
                      format: 'image/png',
                      layers: ["1"],
                      crs: epsg31370CRS,
                      transparent: true,
                    ),
                    tileSize: tileSize,
                  ),
                MarkerLayer(
                  markers: [
                    Marker(
                      width: 50.0,
                      height: 50.0,
                      point: latlonEpioux,
                      child: const Icon(Icons.location_on),
                    ),
                    Marker(
                        point: _bound.northEast,
                        child: const Icon(Icons.location_on)),
                    Marker(
                        point: _bound.northWest,
                        child: const Icon(Icons.location_on)),
                    Marker(
                        point: _bound.southEast,
                        child: const Icon(Icons.location_on)),
                    Marker(
                        point: _bound.southWest,
                        child: const Icon(Icons.location_on)),
                  ],
                ),
              ],
            )));
  }
}

class gdal2tilesFileTileProvider extends TileProvider {
  final Proj4Crs mycrs;
  double tileSize = 256.0;
  gdal2tilesFileTileProvider(
      {super.headers,
      required this.mycrs // sert uniquement à titre d'information (print extent de la tuile)
      });

  @override
  ImageProvider getImage(TileCoordinates coordinates, TileLayer options) {
    final file = File(getTileUrl(coordinates, options));
    print(getTileUrl(coordinates, options));
    final fallbackUrl = getTileFallbackUrl(coordinates, options);

    if (fallbackUrl == null || file.existsSync()) return FileImage(file);
    return FileImage(File(fallbackUrl));
  }

  // copié et adapté de wms_tile_layer_options (getTileUrl). pour avoir les coordonnées de l'extent de chaque tuile (utile si jamais on crée nos tiles depuis l'image "brute" -> image rgb à l'échelle de toute la RW par exemple)
  void printInfoTile(TileCoordinates coordinates, TileLayer options) {
    print("X " +
        coordinates.x.toString() +
        " Y " +
        coordinates.y.toString() +
        " Z " +
        coordinates.z.toString());

    int tileSize = 256;
    final tileSizePoint = Point(tileSize, tileSize);
    final nwPoint = coordinates.scaleBy(tileSizePoint);
    final sePoint = nwPoint + tileSizePoint;
    final nwCoords = mycrs.pointToLatLng(nwPoint, coordinates.z.toDouble());
    final seCoords = mycrs.pointToLatLng(sePoint, coordinates.z.toDouble());
    final nw = mycrs.projection.project(nwCoords);
    final se = mycrs.projection.project(seCoords);
    final bounds = Bounds(nw, se);
    final bbox = [bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y];
    print("bbox :");
    print(bbox);
  }

  @override
  Map<String, String> generateReplacementMap(
    String urlTemplate,
    TileCoordinates coordinates,
    TileLayer options,
  ) {
    final zoom = coordinates.z;

    printInfoTile(coordinates, options);

    return {
      'x': (coordinates.x).toString(),
      'y': (-coordinates.y - 1).toString(),
      'z': (zoom).toString(),
      's': options.subdomains.isEmpty
          ? ''
          : options.subdomains[
              (coordinates.x + coordinates.y) % options.subdomains.length],
      'r': options.resolvedRetinaMode == RetinaMode.server ? '@2x' : '',
      'd': options.tileSize.toString(),
      ...options.additionalOptions,
    };
  }
}

Widget mycoordinateDebugTileBuilder(
  BuildContext context,
  Widget tileWidget,
  TileImage tile,
) {
  final coordinates = tile.coordinates;

  final Y = -coordinates.y - 1;
  final readableKey =
      '${coordinates.x} : ${coordinates.y} -> ${Y}: ${coordinates.z}';

  return DecoratedBox(
    decoration: BoxDecoration(
      border: Border.all(),
    ),
    child: Stack(
      fit: StackFit.passthrough,
      children: [
        tileWidget,
        Center(
          child: Text(
            readableKey,
            style: Theme.of(context).textTheme.headlineSmall,
          ),
        ),
      ],
    ),
  );
}
