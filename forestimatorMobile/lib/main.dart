import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/dico/ess.dart';
import 'package:fforestimator/pages/anaPt/anaPtpage.dart';
import 'package:fforestimator/pages/offlinePage/offlineView.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:fforestimator/pages/pdfScreen.dart';
import 'package:latlong2/latlong.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'package:fforestimator/pages/catalogueView/catalogueLayerView.dart';
import 'package:go_router/go_router.dart';
import 'dart:async';
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import 'package:flutter/services.dart';
import 'package:fforestimator/scaffoldNavigation.dart';
import 'dart:convert';
import 'package:path/path.dart' as path;
import 'package:flutter_downloader/flutter_downloader.dart';
import 'package:flutter_logs/flutter_logs.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    sqfliteFfiInit();
    databaseFactory = databaseFactoryFfi;
  } else {
    await FlutterDownloader.initialize(
        debug:
            true, // optional: set to false to disable printing logs to console (default: true)
        ignoreSsl:
            true // option: set to false to disable working with http links (default: false)
        );

    //Initialize Logging
    await FlutterLogs.initLogs(
        logLevelsEnabled: [
          LogLevel.INFO,
          LogLevel.WARNING,
          LogLevel.ERROR,
          LogLevel.SEVERE
        ],
        timeStampFormat: TimeStampFormat.TIME_FORMAT_READABLE,
        directoryStructure: DirectoryStructure.FOR_DATE,
        logTypesEnabled: ["device", "network", "errors"],
        logFileExtension: LogFileExtension.TXT,
        logsWriteDirectoryName: "MyLogs",
        logsExportDirectoryName: "MyLogs/Exported",
        debugFileOperations: true,
        isDebuggable: true);
  }
  gl.dico = dicoAptProvider();
  await gl.dico.init();

  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});
  @override
  State<MyApp> createState() => _MyApp();
}

class _MyApp extends State<MyApp> {
  static const TextStyle optionStyle =
      TextStyle(fontSize: 30, fontWeight: FontWeight.bold);

  late String _pathExternalStorage;
  bool _initializedPersistentValues = false;

  _MyApp() {}

  Future readPreference() async {
    final SharedPreferences prefs = await SharedPreferences.getInstance();

    final bool? aOfflineMode = prefs.getBool('offlineMode');
    if (aOfflineMode != null) {
      gl.offlineMode = aOfflineMode;
    }

    final List<String>? aAnaPtSelectedLayerKeys =
        prefs.getStringList('anaPtSelectedLayerKeys');

    if (aAnaPtSelectedLayerKeys != null) {
      gl.anaPtSelectedLayerKeys = aAnaPtSelectedLayerKeys;
    }

    final bool? firstTimeUse = prefs.getBool('firstTimeUse');
    if (firstTimeUse != null) {
      gl.firstTimeUse = firstTimeUse;
    }

    final List<String>? ainterfaceSelectedLCode =
        prefs.getStringList('interfaceSelectedLCode');
    if (ainterfaceSelectedLCode != null) {
      gl.interfaceSelectedLCode = ainterfaceSelectedLCode;
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
      _pathExternalStorage = dir.path;
      File file = File("${dir?.path}/$filename");
      if (await file.exists() == false) {
        var data = await rootBundle.load(asset);
        var bytes = data.buffer.asUint8List();
        await file.writeAsBytes(bytes, flush: true);
      }
      completer.complete(file);
    } catch (e) {
      throw Exception('Error parsing asset file!');
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
    readPreference();
    gl.rebuildWholeWidgetTree = setState;
    
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
                pageBuilder: (context, state) =>
                    const NoTransitionPage(child: mapPage()),
                routes: [
                  GoRoute(
                    path: 'anaPt',
                    builder: (context, state) => anaPtpage(gl.requestedLayers),
                  ),
                ],
              )
            ],
          ),
          // second branch (B)
          StatefulShellBranch(
            navigatorKey: _shellNavigatorBKey,
            routes: [
              // top route inside branch
              GoRoute(
                path: "/" + gl.basePathbranchB,
                pageBuilder: (context, state) => const NoTransitionPage(
                  child: CatalogueLayerView(),
                ),
                routes: [
                  ...gl.dico.getLayersWithDoc().map<GoRoute>((layerBase item) {
                    return GoRoute(
                      path: item.getFicheRoute() + "/:currentPage",
                      name: item.mCode,
                      builder: (context, state) => (Platform.isAndroid ||
                              Platform.isIOS)
                          ? PDFScreen(
                              path: _pathExternalStorage + "/" + item.mPdfName,
                              titre: "documentation", //+ item.mNomCourt,
                              currentPage: int.parse(
                                  state.pathParameters['currentPage']!),
                            )
                          : Scaffold(
                              appBar: AppBar(
                                title: Text("view pdf"),
                              ),
                              body: Text("toto"),
                            ),
                    );
                  }).toList(),
                  ...gl.dico.getFEEess().map<GoRoute>((Ess item) {
                    return GoRoute(
                      path: item.getFicheRoute(),
                      builder: (context, state) =>
                          (Platform.isAndroid || Platform.isIOS)
                              ? PDFScreen(
                                  path: _pathExternalStorage +
                                      "/FEE-" +
                                      item.mCode +
                                      ".pdf",
                                  titre: item.mNomFR,
                                )
                              : Scaffold(
                                  appBar: AppBar(
                                    title: Text("view pdf"),
                                  ),
                                  body: Text(_pathExternalStorage +
                                      "/FEE-" +
                                      item.mCode +
                                      ".pdf"),
                                ),
                    );
                  }).toList(),
                ],
              ),
            ],
          ),
          StatefulShellBranch(navigatorKey: _shellNavigatorCKey, routes: [
            // top route inside branch
            GoRoute(
              path: "/" + gl.basePathbranchC,
              pageBuilder: (context, state) => const NoTransitionPage(
                child: OfflineView(),
              ),
            )
          ]),
        ],
      ),
    ],
  );

// private navigators
  final _rootNavigatorKey = GlobalKey<NavigatorState>();
  final _shellNavigatorAKey = GlobalKey<NavigatorState>(debugLabel: 'shellA');
  final _shellNavigatorBKey = GlobalKey<NavigatorState>(debugLabel: 'shellB');
  final _shellNavigatorCKey = GlobalKey<NavigatorState>(debugLabel: 'shellC');

  @override
  Widget build(BuildContext context) {
    if (!_initializedPersistentValues) {
      return const MaterialApp(home: CircularProgressIndicator());
    } else if (gl.firstTimeUse) {
      return MaterialApp(
          home: PopupNotification(
        title: "FirstTimeUse",
        accept: "oui",
        onAccept: () async {
          setState(() {
            gl.firstTimeUse = false;
          });
          final SharedPreferences prefs = await SharedPreferences.getInstance();
          await prefs.setBool('firstTimeUse', gl.firstTimeUse);
          for (var key in gl.downloadableLayerKeys) {
            if (!(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
              FlutterDownloader.enqueue(
                url: gl.queryApiRastDownload +
                    "/" +
                    gl.dico.getLayerBase(key).mCode,
                fileName: gl.dico.getLayerBase(key).mNomRaster,
                savedDir: gl.dico.docDir.path,
                showNotification: false,
                openFileFromNotification: false,
                timeout: 15000,
              );
            }
          }
        },
        decline: "non",
        onDecline: () async {
          setState(() {
            gl.firstTimeUse = false;
          });
          final SharedPreferences prefs = await SharedPreferences.getInstance();
          await prefs.setBool('firstTimeUse', gl.firstTimeUse);
        },
        dialog:
            "Autorisez vous l'aplication à télécharger un jeu de couches pour l'utilisation de l'analyse hors ligne?",
      ));
    }
    return MaterialApp.router(
      routerDelegate: _router.routerDelegate,
      routeInformationParser: _router.routeInformationParser,
      routeInformationProvider: _router.routeInformationProvider,
      title: 'Mobile Forestimator',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
    );
  }
}
