import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/pages/anaPt/anaPtpage.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'dart:io' show Platform;
import 'package:fforestimator/pages/catalogueView/catalogueLayerView.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'dart:convert';
import 'package:go_router/go_router.dart';

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

  _MyApp() {}

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
