import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/pages/anaPt/anaPtpage.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:geolocator/geolocator.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'dart:io' show Platform;
import 'package:fforestimator/pages/catalogueView/catalogueLayerView.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:http/http.dart' as http;
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'dart:convert';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    sqfliteFfiInit();
    databaseFactory = databaseFactoryFfi;
  }

  gl.dico = dicoAptProvider();
  await gl.dico.init();
  while (!gl.dico.finishedLoading) {}
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});
  @override
  State<MyApp> createState() => _MyApp();
}

class _MyApp extends State<MyApp> {
  int _selectedIndex = 0;
  static const TextStyle optionStyle =
      TextStyle(fontSize: 30, fontWeight: FontWeight.bold);

  late final List<Widget> _widgetOptions;

  var data;

  List<layerAnaPt> requestedLayers = [];

  _MyApp() {
    _widgetOptions = <Widget>[
      mapPage(runAnaPt: _runAnapt, title: 'Flutter Demo Home Page'),
      CatalogueLayerView(),
      anaPtpage(requestedLayers),
      const Text(
        'todo settings',
        style: optionStyle,
      ),
    ];
  }

  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
    });
  }

  Future _runAnapt(proj4.Point ptBL72) async {
    String layers4AnaPt = gl.layersAnaPt;
    for (String lCode in gl.interfaceSelectedLayerKeys) {
      if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
        layers4AnaPt += "+" + lCode;
      }
    }

    String url = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/" +
        layers4AnaPt +
        "/x/" +
        ptBL72.x.toString() +
        "/y/" +
        ptBL72.y.toString();
    print(url);
    var res = await http.get(Uri.parse(url));
    //print(res.body);
    data = jsonDecode(res.body);
//if (data["")
    requestedLayers.clear();
    for (var r in data["RequestedLayers"]) {
      requestedLayers.add(layerAnaPt.fromMap(r));
    }
    _onItemTapped(2);
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        title: 'Mobile Forestimator',
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
          useMaterial3: true,
        ),
        home: Scaffold(
          body: Center(
            child: _widgetOptions.elementAt(_selectedIndex),
          ),
          /*Center(
            child: mapPage(title: 'Flutter Demo Home Page'),
          ),*/
          bottomNavigationBar: BottomNavigationBar(
            items: const <BottomNavigationBarItem>[
              BottomNavigationBarItem(
                icon: Icon(Icons.landslide),
                label: 'Carte',
                backgroundColor: Colors.green,
              ),
              BottomNavigationBarItem(
                icon: Icon(Icons.layers),
                label: 'selection des couches',
                backgroundColor: Colors.green,
              ),
              BottomNavigationBarItem(
                icon: Icon(Icons.analytics),
                label: 'analyse ponctuelle',
                backgroundColor: Colors.green,
              ), /*
              BottomNavigationBarItem(
                icon: Icon(Icons.settings),
                label: 'paramètres',
                backgroundColor: Colors.green,
              ),*/
            ],
            currentIndex: _selectedIndex,
            selectedItemColor: Color.fromARGB(255, 255, 255, 255),
            onTap: _onItemTapped,
          ),
        ));
  }
}
