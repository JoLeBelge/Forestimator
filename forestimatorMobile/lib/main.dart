import 'package:downloadsfolder/downloadsfolder.dart' as path;
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/dico/ess.dart';
import 'package:fforestimator/pages/anaPt/ana_ptpage.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:fforestimator/pages/pdf_screen.dart';
import 'package:latlong2/latlong.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'package:go_router/go_router.dart';
import 'dart:async';
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import 'package:flutter/services.dart';
import 'package:fforestimator/scaffold_navigation.dart';
import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:memory_info/memory_info.dart';

void main() async {
  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    sqfliteFfiInit();
    databaseFactory = databaseFactoryFfi;
  } else {
    initDownloader();
  }

  gl.dico = DicoAptProvider();
  await gl.dico.init();

  while (!gl.dico.finishedLoading) {
    sleep(const Duration(seconds: 1));
  }
  try {
    runApp(const MyApp());
  } catch (e) {
    gl.print("$e");
  }
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyApp();
}

class _MyApp extends State<MyApp> {
  bool _initializedPersistentValues = false;

  _MyApp();

  Future<void> getMemoryInfo() async {
    Memory? memory;
    try {
      memory = await MemoryInfoPlugin().memoryInfo;
    } on PlatformException catch (e) {
      gl.print('error $e');
    }

    if (memory != null) {
      setState(() {
        gl.memory = memory;
      });
    }
  }

  Color _getColorFromMemory(String name, SharedPreferences prefs) {
    return Color.fromRGBO(
      prefs.getInt('$name.r')!,
      prefs.getInt('$name.g')!,
      prefs.getInt('$name.b')!,
      prefs.getDouble('$name.a')!,
    );
  }

  List<LatLng> _getPolygonFromMemory(String name, SharedPreferences prefs) {
    List<LatLng> polygon = [];
    int nPoints = prefs.getInt('$name.nPolyPoints')!;
    for (int i = 0; i < nPoints; i++) {
      polygon.add(
        LatLng(
          prefs.getDouble('$name.$i-lat')!,
          prefs.getDouble('$name.$i-lng')!,
        ),
      );
    }

    return polygon;
  }

  Future _readPreference() async {
    final SharedPreferences prefs = await SharedPreferences.getInstance();

    final bool? modeDevelopper = prefs.getBool('modeDevelopper');
    if (modeDevelopper != null) {
      gl.modeDevelopper = modeDevelopper;
    }

    final bool? aOfflineMode = prefs.getBool('offlineMode');
    if (aOfflineMode != null) {
      gl.offlineMode = aOfflineMode;
    }

    final List<String>? aAnaPtSelectedLayerKeys = prefs.getStringList(
      'anaPtSelectedLayerKeys',
    );

    if (aAnaPtSelectedLayerKeys != null) {
      gl.anaPtSelectedLayerKeys = aAnaPtSelectedLayerKeys;
    }

    final bool? firstTimeUse = prefs.getBool('firstTimeUse');
    if (firstTimeUse != null) {
      gl.firstTimeUse = firstTimeUse;
    }

    final List<String>? ainterfaceSelectedLCode = prefs.getStringList(
      'interfaceSelectedLCode',
    );
    if (ainterfaceSelectedLCode != null) {
      gl.interfaceSelectedLCode = ainterfaceSelectedLCode;
    }
    final List<String>? ainterfaceSelectedLOffline = prefs.getStringList(
      'interfaceSelectedLOffline',
    );
    if (ainterfaceSelectedLOffline != null) {
      gl.interfaceSelectedLOffline =
          ainterfaceSelectedLOffline.map<bool>((e) {
            return e == "true";
          }).toList();
    }
    gl.refreshInterfaceSelectedL();

    double? lat = prefs.getDouble('mapCenterLat');
    double? lon = prefs.getDouble('mapCenterLon');
    if (lat != null && lon != null) {
      gl.latlonCenter = LatLng(lat, lon);
    }

    double? aZoom = prefs.getDouble('mapZoom');
    if (aZoom != null) {
      gl.mapZoom = aZoom;
    }

    int? nPolys = prefs.getInt('nPolys');
    if (nPolys != null && nPolys != 0) {
      gl.polygonLayers.clear();
      for (int i = 0; i < nPolys; i++) {
        gl.polygonLayers.add(
          PolygonLayer(polygonName: prefs.getString('poly$i.name')!),
        );
        gl.polygonLayers[i].area = prefs.getDouble('poly$i.area')!;
        gl.polygonLayers[i].perimeter = prefs.getDouble('poly$i.perimeter')!;
        gl.polygonLayers[i].transparencyInside =
            prefs.getDouble('poly$i.transparencyInside')!;
        gl.polygonLayers[i].transparencyLine =
            prefs.getDouble('poly$i.transparencyLine')!;
        gl.polygonLayers[i].colorInside = _getColorFromMemory(
          'poly$i.colorInside',
          prefs,
        );
        gl.polygonLayers[i].colorLine = _getColorFromMemory(
          'poly$i.colorLine',
          prefs,
        );
        gl.polygonLayers[i].polygonPoints = _getPolygonFromMemory(
          'poly$i.poly',
          prefs,
        );
      }
    }

    setState(() {
      _initializedPersistentValues = true;
    });
  }

  Future<File> fromAsset(String asset, String filename) async {
    // To open from assets, you can copy them to the app storage folder, and the access them "locally"
    Completer<File> completer = Completer();
    try {
      var dir = await getApplicationDocumentsDirectory();
      //var dir = await getExternalStorageDirectory();
      gl.pathExternalStorage = dir.path;
      File file = File("${dir.path}/$filename");
      if (await file.exists() == false) {
        var data = await rootBundle.load(asset);
        var bytes = data.buffer.asUint8List();
        await file.writeAsBytes(bytes, flush: true);
      }
      completer.complete(file);
    } catch (e) {
      gl.print("$e");
    }

    return completer.future;
  }

  Future _listAndCopyPdfassets() async {
    // load as string
    final manifestcontent =
    //await defaultassetbundle.of(context).loadstring('assetmanifest.json');
    await rootBundle.loadString('AssetManifest.json');
    // decode to map
    final Map<String, dynamic> manifestmap = json.decode(manifestcontent);

    // filter by extension
    List<String> list =
        manifestmap.keys.where((path) => path.endsWith('.pdf')).toList();
    for (String f in list) {
      fromAsset(f, path.basename(f));
    }
  }

  @override
  void initState() {
    super.initState();
    // copier tout les pdf de l'asset bundle vers un fichier utilisable par la librairie flutter_pdfviewer
    _listAndCopyPdfassets();
    _readPreference();
  }

  late final _router = GoRouter(
    initialLocation: '/',
    navigatorKey: _rootNavigatorKey,
    routes: [
      StatefulShellRoute.indexedStack(
        builder: (context, state, navigationShell) {
          return ScaffoldWithNestedNavigation(navigationShell: navigationShell);
        },
        branches: [
          // first branch (A)
          StatefulShellBranch(
            navigatorKey: _shellNavigatorAKey,
            routes: [
              // top route inside branch
              GoRoute(
                path: '/',
                pageBuilder:
                    (context, state) =>
                        const NoTransitionPage(child: MapPage()),
                routes: [
                  GoRoute(
                    path: 'anaPt',
                    builder: (context, state) => AnaPtpage(gl.requestedLayers),
                  ),
                  ...gl.dico.getLayersWithDoc().map<GoRoute>((LayerBase item) {
                    return GoRoute(
                      path: "${item.getFicheRoute()}/:currentPage",
                      name: item.mCode,
                      builder:
                          (context, state) =>
                              (Platform.isAndroid || Platform.isIOS)
                                  ? PDFScreen(
                                    path:
                                        "${gl.pathExternalStorage}/${item.mPdfName}",
                                    titre: "documentation", //+ item.mNomCourt,
                                    currentPage: int.parse(
                                      state.pathParameters['currentPage']!,
                                    ),
                                  )
                                  : Scaffold(
                                    appBar: AppBar(title: Text("pdf")),
                                    body: Text("toto"),
                                  ),
                    );
                  }),
                  ...gl.dico.getFEEess().map<GoRoute>((Ess item) {
                    return GoRoute(
                      path: item.getFicheRoute(),
                      builder:
                          (context, state) =>
                              (Platform.isAndroid || Platform.isIOS)
                                  ? PDFScreen(
                                    path:
                                        "${gl.pathExternalStorage}/FEE-${item.mCode}.pdf",
                                    titre: item.mNomFR,
                                  )
                                  : Scaffold(
                                    appBar: AppBar(title: Text("pdf")),
                                    body: Text(
                                      "${gl.pathExternalStorage}/FEE-${item.mCode}.pdf",
                                    ),
                                  ),
                    );
                  }),
                  ...gl.dico.getAllStationFiches().map<GoRoute>((String item) {
                    //print("create go route " + item);
                    return GoRoute(
                      path: "$item/:currentPage",
                      builder:
                          (context, state) =>
                              (Platform.isAndroid || Platform.isIOS)
                                  ? PDFScreen(
                                    path: "${gl.pathExternalStorage}/$item",
                                    titre: item,
                                    currentPage: int.parse(
                                      state.pathParameters['currentPage']!,
                                    ),
                                  )
                                  : Scaffold(
                                    appBar: AppBar(title: Text("pdf")),
                                    body: Text(
                                      "${gl.pathExternalStorage} $item",
                                    ),
                                  ),
                    );
                  }),
                ],
              ),
            ],
          ),
        ],
      ),
    ],
  );

  final _rootNavigatorKey = GlobalKey<NavigatorState>();
  final _shellNavigatorAKey = GlobalKey<NavigatorState>(debugLabel: 'shellA');

  @override
  Widget build(BuildContext context) {
    gl.initializeDisplayInfos(context);
    gl.notificationContext = context;
    if (!_initializedPersistentValues) {
      return const MaterialApp(home: CircularProgressIndicator());
    }
    return MaterialApp.router(
      routerDelegate: _router.routerDelegate,
      routeInformationParser: _router.routeInformationParser,
      routeInformationProvider: _router.routeInformationProvider,
      title: 'Forestimator',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
    );
  }
}
