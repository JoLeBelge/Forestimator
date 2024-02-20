import 'package:fforestimator/dico/dicoApt.dart';
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

// Stateful nested navigation based on:
// https://github.com/flutter/packages/blob/main/packages/go_router/example/lib/stateful_shell_route.dart
class ScaffoldWithNestedNavigation extends StatelessWidget {
  const ScaffoldWithNestedNavigation({Key? key, required this.navigationShell})
      : super(key: key ?? const ValueKey('ScaffoldWithNestedNavigation'));
  final StatefulNavigationShell navigationShell;

  void _goBranch(int index) {
    navigationShell.goBranch(
      index,
      // A common pattern when using bottom navigation bars is to support
      // navigating to the initial location when tapping the item that is
      // already active. This example demonstrates how to support this behavior,
      // using the initialLocation parameter of goBranch.
      initialLocation: index == navigationShell.currentIndex,
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: navigationShell,
      bottomNavigationBar: NavigationBar(
        selectedIndex: navigationShell.currentIndex,
        destinations: const [
          NavigationDestination(label: 'Section A', icon: Icon(Icons.home)),
          NavigationDestination(label: 'Section B', icon: Icon(Icons.settings)),
        ],
        onDestinationSelected: _goBranch,
      ),
    );
  }
}

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    sqfliteFfiInit();
    databaseFactory = databaseFactoryFfi;
  }

  gl.dico = dicoAptProvider();
  await gl.dico.init();

  // copier tout les pdf de l'asset bundle vers un fichier utilisable par la librairie flutter_pdfviewer

  //while (!gl.dico.finishedLoading) {}
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
      //var dir = await getApplicationDocumentsDirectory();
      var dir = await getExternalStorageDirectory();
      //print("tata" + );
      _pathExternalStorage = dir!.path;
      File file = File("${dir?.path}/$filename");
      var data = await rootBundle.load(asset);
      var bytes = data.buffer.asUint8List();
      await file.writeAsBytes(bytes, flush: true);
      completer.complete(file);
    } catch (e) {
      throw Exception('Error parsing asset file!');
    }

    return completer.future;
  }

  @override
  void initState() {
    super.initState();
    fromAsset('assets/pdf/FEE-HE.pdf', 'FEE-HE.pdf').then((f) {});
  }

  late final _router = GoRouter(
    initialLocation: '/',
    navigatorKey: _rootNavigatorKey,
    routes: [
      // Stateful nested navigation based on:
      // https://github.com/flutter/packages/blob/main/packages/go_router/example/lib/stateful_shell_route.dart
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
                //),
                routes: [
                  // child route
                  GoRoute(
                    path: 'anaPt',
                    builder: (context, state) => anaPtpage(gl.requestedLayers),
                  ),
                  GoRoute(
                    path: 'fiche-esssence/HE',
                    builder: (context, state) => PDFScreen(
                        path: _pathExternalStorage + "/FEE-HE.pdf",
                        titre: "fiche-essence " +
                            gl.dico.getEss("HE").getNameAndPrefix()),
                  ),
                ],
              ),
            ],
          ),
          // second branch (B)
          StatefulShellBranch(
            navigatorKey: _shellNavigatorBKey,
            routes: [
              // top route inside branch
              GoRoute(
                path: '/catalogue',
                pageBuilder: (context, state) => const NoTransitionPage(
                  child: CatalogueLayerView(),
                ),
                /* routes: [
                    // child route
                    GoRoute(
                      path: 'details',
                      builder: (context, state) =>
                          const DetailsScreen(label: 'B'),
                    ),
                  ],*/
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
      /* home: Scaffold(
          body: Center(
              child: Stack(
            children: <Widget>[
              mapPage(runAnaPt: _runAnapt),
              Column(children: [
                Container(
                    constraints: BoxConstraints(
                        minHeight: MediaQuery.of(context).size.height - 56,
                        maxHeight: MediaQuery.of(context).size.height - 56,
                        minWidth: MediaQuery.of(context).size.width,
                        maxWidth: MediaQuery.of(context).size.width),
                    child: _showCompleteLayerSelectionScreen
                        ? const CatalogueLayerView()
                        : _showAnalysisResultScreen
                            ? anaPtpage(requestedLayers)
                            : null),
                Container(
                    constraints: BoxConstraints(minHeight: 56),
                    child: Row(children: [
                      _showCompleteLayerSelectionScreen
                          ? FloatingActionButton(
                              backgroundColor: gl.colorAgroBioTech,
                              onPressed: _switchLayerViewPage,
                              child: const Icon(Icons.arrow_back,
                                  color: gl.colorBack))
                          : FloatingActionButton(
                              backgroundColor: gl.colorAgroBioTech,
                              onPressed: _switchLayerViewPage,
                              child: const Icon(
                                Icons.layers_rounded,
                                color: gl.colorUliege,
                              )),
                      _showAnalysisResultScreen
                          ? FloatingActionButton(
                              backgroundColor: gl.colorAgroBioTech,
                              onPressed: _switchAnalysisViewPage,
                              child: const Icon(Icons.arrow_back,
                                  color: gl.colorBack))
                          : FloatingActionButton(
                              backgroundColor: gl.colorAgroBioTech,
                              onPressed: _switchAnalysisViewPage,
                              child: const Icon(
                                Icons.analytics_rounded,
                                color: gl.colorUliege,
                              )),
                    ]))
              ]),
            ],
          )),
        )*/
    );
  }
}
