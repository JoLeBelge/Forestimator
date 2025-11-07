import 'package:downloadsfolder/downloadsfolder.dart' as path;
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/map.dart';
import 'package:latlong2/latlong.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'dart:async';
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import 'package:flutter/services.dart';
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
  GlobalKey<NavigatorState>? _navigatorKey;

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

  Color _getColorFromMemory(String name, SharedPreferences shared) {
    return Color.fromRGBO(
      gl.shared!.getInt('$name.r')!,
      gl.shared!.getInt('$name.g')!,
      gl.shared!.getInt('$name.b')!,
      gl.shared!.getDouble('$name.a')!,
    );
  }

  List<LatLng> _getPolygonFromMemory(String name, SharedPreferences shared) {
    List<LatLng> polygon = [];
    int nPoints = gl.shared!.getInt('$name.nPolyPoints')!;
    for (int i = 0; i < nPoints; i++) {
      polygon.add(
        LatLng(
          gl.shared!.getDouble('$name.$i-lat')!,
          gl.shared!.getDouble('$name.$i-lng')!,
        ),
      );
    }
    return polygon;
  }

  Future _readPreference() async {
    gl.shared = await SharedPreferences.getInstance();

    final bool? modeDevelopper = gl.shared!.getBool('modeDevelopper');
    if (modeDevelopper != null) {
      gl.modeDevelopper = modeDevelopper;
    }

    final bool? aOfflineMode = gl.shared!.getBool('offlineMode');
    if (aOfflineMode != null) {
      gl.offlineMode = aOfflineMode;
    }

    final bool? modeExpert = gl.shared!.getBool('modeExpert');
    if (modeExpert != null) {
      gl.Mode.expert = modeExpert;
    }

    final bool? modeExpertTools = gl.shared!.getBool('modeExpertTools');
    if (modeExpertTools != null) {
      gl.Mode.expertTools = modeExpertTools;
    }

    final List<String>? aAnaPtSelectedLayerKeys = gl.shared!.getStringList(
      'anaPtSelectedLayerKeys',
    );

    if (aAnaPtSelectedLayerKeys != null) {
      gl.anaPtSelectedLayerKeys = aAnaPtSelectedLayerKeys;
    }

    final bool? firstTimeUse = gl.shared!.getBool('firstTimeUse');
    if (firstTimeUse != null) {
      gl.firstTimeUse = firstTimeUse;
    }

    final List<String>? ainterfaceSelectedLCode = gl.shared!.getStringList(
      'interfaceSelectedLCode',
    );
    if (ainterfaceSelectedLCode != null) {
      gl.interfaceSelectedLCode = ainterfaceSelectedLCode;
    }
    final List<String>? ainterfaceSelectedLOffline = gl.shared!.getStringList(
      'interfaceSelectedLOffline',
    );
    if (ainterfaceSelectedLOffline != null) {
      gl.interfaceSelectedLOffline =
          ainterfaceSelectedLOffline.map<bool>((e) {
            return e == "true";
          }).toList();
    }
    gl.initializeSelectedLayerForFlutterMap();

    double? lat = gl.shared!.getDouble('mapCenterLat');
    double? lon = gl.shared!.getDouble('mapCenterLon');
    if (lat != null && lon != null) {
      gl.latlonCenter = LatLng(lat, lon);
    }

    double? aZoom = gl.shared!.getDouble('mapZoom');
    if (aZoom != null) {
      gl.mapZoom = aZoom;
    }

    int? nPolys = gl.shared!.getInt('nPolys');
    if (nPolys != null && nPolys != 0) {
      gl.polygonLayers.clear();
      for (int i = 0; i < nPolys; i++) {
        gl.polygonLayers.add(
          PolygonLayer(polygonName: gl.shared!.getString('poly$i.name')!),
        );
        gl.polygonLayers[i].area = gl.shared!.getDouble('poly$i.area')!;
        gl.polygonLayers[i].perimeter =
            gl.shared!.getDouble('poly$i.perimeter')!;
        gl.polygonLayers[i].transparencyInside =
            gl.shared!.getDouble('poly$i.transparencyInside')!;
        gl.polygonLayers[i].transparencyLine =
            gl.shared!.getDouble('poly$i.transparencyLine')!;
        gl.polygonLayers[i].colorInside = _getColorFromMemory(
          'poly$i.colorInside',
          gl.shared!,
        );
        gl.polygonLayers[i].colorLine = _getColorFromMemory(
          'poly$i.colorLine',
          gl.shared!,
        );
        gl.polygonLayers[i].polygonPoints = _getPolygonFromMemory(
          'poly$i.poly',
          gl.shared!,
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
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _navigatorKey = GlobalKey<NavigatorState>();
      gl.notificationContext = _navigatorKey!.currentContext;
    });
  }

  @override
  Widget build(BuildContext context) {
    gl.initializeDisplayInfos(context);
    if (!_initializedPersistentValues) {
      return const MaterialApp(home: CircularProgressIndicator());
    }
    return MaterialApp(
      navigatorKey: _navigatorKey,
      title: 'Forestimator',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        fontFamily: "Calibri",
        useMaterial3: true,
      ),
      home: MapPage(),
    );
  }
}
