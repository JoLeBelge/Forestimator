import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/dico/ess.dart';
import 'package:fforestimator/pages/anaPt/anaPtpage.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:fforestimator/pages/pdfScreen.dart';
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

  _MyApp() {}

  Future<File> fromAsset(String asset, String filename) async {
    // To open from assets, you can copy them to the app storage folder, and the access them "locally"
    Completer<File> completer = Completer();
    try {
      var dir = await getApplicationDocumentsDirectory();
      //var dir = await getExternalStorageDirectory();
      _pathExternalStorage = dir!.path;
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

  // voir void checkLayerBaseOfflineRessource() async de cDicoAptProvider qui fait la mm chose
  /*void _lookForDownloadedFiles() async {
    // Downloaded Layers
    for (var layerCode in gl.dico.mLayerBases.keys) {
      final File file = File(gl.dico.docDir.path +
          "/" +
          gl.dico.getLayerBase(layerCode).mNomRaster);
      gl.dico.getLayerBase(layerCode).mOffline = await file.exists();
    }
  }*/

  @override
  void initState() {
    super.initState();
    // copier tout les pdf de l'asset bundle vers un fichier utilisable par la librairie flutter_pdfviewer
    _listAndCopyPdfassets();
    //_lookForDownloadedFiles();
  }

  late final _router = GoRouter(
    initialLocation: '/',
    navigatorKey: _rootNavigatorKey,
    routes: [
      StatefulShellRoute.indexedStack(
        builder: (context, state, navigationShell) {
          // the UI shell
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
        ],
      ),
    ],
  );

// private navigators
  final _rootNavigatorKey = GlobalKey<NavigatorState>();
  final _shellNavigatorAKey = GlobalKey<NavigatorState>(debugLabel: 'shellA');
  final _shellNavigatorBKey = GlobalKey<NavigatorState>(debugLabel: 'shellB');

  @override
  Widget build(BuildContext context) {
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
