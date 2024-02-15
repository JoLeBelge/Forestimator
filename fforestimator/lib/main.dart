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
  //int _selectedIndex = 0;
  bool _showCompleteLayerSelectionScreen = false;
  bool _showAnalysisResultScreen = false;

  static const TextStyle optionStyle =
      TextStyle(fontSize: 30, fontWeight: FontWeight.bold);

  var data;

  List<layerAnaPt> requestedLayers = [];

  void _switchLayerViewPage() {
    setState(() {
      _showCompleteLayerSelectionScreen = !_showCompleteLayerSelectionScreen;
      _showAnalysisResultScreen = false;
    });
  }

  void _switchAnalysisViewPage() {
    setState(() {
      _showAnalysisResultScreen = !_showAnalysisResultScreen;
      _showCompleteLayerSelectionScreen = false;
    });
  }

  _MyApp() {}

  Future _runAnapt(proj4.Point ptBL72) async {
    String layersAnaPt = "";
    for (String lCode in gl.anaPtSelectedLayerKeys) {
      if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
        layersAnaPt += "+" + lCode;
      }
    }

    String url = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/" +
        layersAnaPt +
        "/x/" +
        ptBL72.x.toString() +
        "/y/" +
        ptBL72.y.toString();
    print(url);
    var res = await http.get(Uri.parse(url));
    print(res.body);
    data = jsonDecode(res.body);
    requestedLayers.clear();
    for (var r in data["RequestedLayers"]) {
      requestedLayers.add(layerAnaPt.fromMap(r));
    }
    requestedLayers.removeWhere((element) => element.mFoundLayer == 0);
    // un peu radical mais me fait bugger mon affichage par la suite donc je retire
    requestedLayers.removeWhere((element) => element.mRastValue == 0);

    // on les trie sur base des catégories de couches
    requestedLayers.sort((a, b) => gl.dico
        .getLayerBase(a.mCode)
        .mGroupe
        .compareTo(gl.dico.getLayerBase(b.mCode).mGroupe));

    _switchAnalysisViewPage();
  }

  late final _router = GoRouter(
    routes: [
      GoRoute(
        name: 'map',
        path: '/',
        pageBuilder: (context, state) => MaterialPage(
          key: state.pageKey,
          child: Stack(children: <Widget>[
            mapPage(runAnaPt: _runAnapt),
            Container(
              constraints: BoxConstraints(
                  minHeight: MediaQuery.of(context).size.height * .075),
              child: Row(children: [
                FloatingActionButton(
                    backgroundColor: gl.colorAgroBioTech,
                    onPressed: () {
                      return context.go("/catalogue");
                    },
                    child: const Icon(Icons.arrow_back, color: gl.colorBack))
              ]),
            )
          ]),
        ),
      ),
      GoRoute(
        name: 'catalogue',
        path: '/catalogue',
        pageBuilder: (context, state) => MaterialPage(
          key: state.pageKey,
          child: Stack(children: <Widget>[
            CatalogueLayerView(),
            Container(
              constraints: BoxConstraints(
                  minHeight: MediaQuery.of(context).size.height * .075),
              child: Row(children: [
                FloatingActionButton(
                    backgroundColor: gl.colorAgroBioTech,
                    onPressed: () {
                      return context.go("/");
                    },
                    child: const Icon(Icons.arrow_back, color: gl.colorBack))
              ]),
            )
          ]),
        ),
      ),
    ],
    initialLocation: "/",
  );

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
                        minHeight: MediaQuery.of(context).size.height * .925,
                        maxHeight: MediaQuery.of(context).size.height * .925,
                        minWidth: MediaQuery.of(context).size.width,
                        maxWidth: MediaQuery.of(context).size.width),
                    child: _showCompleteLayerSelectionScreen
                        ? const CatalogueLayerView()
                        : _showAnalysisResultScreen
                            ? anaPtpage(requestedLayers)
                            : null),
                Container(
                    constraints: BoxConstraints(
                        minHeight: MediaQuery.of(context).size.height * .075),
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
