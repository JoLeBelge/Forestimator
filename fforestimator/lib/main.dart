import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:url_launcher/url_launcher.dart';
import 'dart:math';
import 'package:latlong2/latlong.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'dico/dicoApt.dart';
import 'package:provider/provider.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'dart:math' as math;

void main() async {
  sqfliteFfiInit();
  databaseFactory = databaseFactoryFfi;

  dicoAptProvider dico = new dicoAptProvider();
  await dico.init();

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Mobile Forestimator',
      theme: ThemeData(
        // This is the theme of your application.
        //
        // TRY THIS: Try running your application with "flutter run". You'll see
        // the application has a blue toolbar. Then, without quitting the app,
        // try changing the seedColor in the colorScheme below to Colors.green
        // and then invoke "hot reload" (save your changes or press the "hot
        // reload" button in a Flutter-supported IDE, or press "r" if you used
        // the command line to start the app).
        //
        // Notice that the counter didn't reset back to zero; the application
        // state is not lost during the reload. To reset the state, use hot
        // restart instead.
        //
        // This works for code too, not just values: Most code changes can be
        // tested with just a hot reload.
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'Flutter Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  int _counter = 0;

  Point ptEpioux = Point(217200.0, 50100.0);
  //var maxZoom = (256).toDouble();
  var zoomI = (6).toDouble();
  var aproj4string = proj4.Projection.add('EPSG:31370',
      '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs');
  final epsg31370Bounds = Bounds<double>(
    Point<double>(42250.0, 21170.0), // lower left
    Point<double>(295170.0, 167700.0), // upper right
  );

  double tileSize = 256.0;
  List<double> getResolutions(double maxX, double minX, int zoom,
      [double tileSize = 256.0]) {
    var size = (maxX - minX) / tileSize;

    return List.generate(zoom, (z) => size / math.pow(2, z));
  }

  //List<double> resolutions = getResolutions(295170.0, 42250.0, 8,256.0);

  late var epsg31370CRS = Proj4Crs.fromFactory(
      code: 'EPSG:31370',
      proj4Projection: aproj4string,
      origins: [ptEpioux],
      bounds: epsg31370Bounds,
      resolutions: getResolutions(295170.0, 42250.0, 8,
          256.0) /*const <double>[
      32768,
      16384,
      8192,
      4096,
      2048,
      1024,
      512,
      256,
      128,
    ],
    scales: const <double>[
      1024.0,
      512.0,
      256.0,
      128.0,
      64.0,
      32.0,
      16.0,
      8.0,
      4.0,
      2.0
    ],
    transformation: null,*/
      );

  void _incrementCounter() {
    setState(() {
      // This call to setState tells the Flutter framework that something has
      // changed in this State, which causes it to rerun the build method below
      // so that the display can reflect the updated values. If we changed
      // _counter without calling setState(), then the build method would not be
      // called again, and so nothing would appear to happen.
      _counter++;
    });
  }

  @override
  Widget build(BuildContext context) {
    return FlutterMap(
      options: MapOptions(crs: epsg31370CRS, initialZoom: zoomI, maxZoom: 8),
      children: [
        TileLayer(
          //urlTemplate: 'https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/mnh_wms/{z}/{x}/{y}.png',
          //userAgentPackageName: 'com.example.app',
          wmsOptions: WMSTileLayerOptions(
            baseUrl:
                "http://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/forestimator?",
            format: 'image/png',
            layers: const ["MasqueForet"],
            crs: epsg31370CRS,
            transparent: false,
          ),
          tileSize: tileSize,
        ),
        RichAttributionWidget(
          attributions: [
            TextSourceAttribution(
              'OpenStreetMap contributors',
              onTap: () =>
                  launchUrl(Uri.parse('https://openstreetmap.org/copyright')),
            ),
          ],
        ),
        MarkerLayer(
          markers: [
            Marker(
              width: 5.0,
              height: 5.0,
              point: LatLng(5.6,
                  50.0), //epsg31370CRS.pointToLatLng(ptEpioux, 0.0),fonctionne pas , pas bon signe
              child: const FlutterLogo(),
            ),
          ],
        ),
      ],
    );
  }
}
