import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart' as pol;
import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:memory_info/memory_info.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';
import 'package:shared_preferences/shared_preferences.dart';

const String forestimatorMobileVersion = "1.0.2-14";
const double globalMinZoom = 4.0;
const double globalMaxZoom = 13.0;
const double globalMinOfflineZoom = 8.0;
const double globalMaxOfflineZoom = 13.0;

bool modeMapFirstTileLayerTransparancy = true;
bool modeMapShowPolygons = true;
bool modeMapShowSearchMarker = true;
bool modeMapShowCustomMarker = true;
bool modeDevelopper = false;

class Display {
  double? width;
  double? height;
  double? dpi;
  double? aspect;
  Orientation? orientation;
  Display(BuildContext context) {
    width = MediaQuery.of(context).size.width;
    height = MediaQuery.of(context).size.height;
    aspect = MediaQuery.of(context).size.aspectRatio;
    dpi = MediaQuery.of(context).devicePixelRatio;
    orientation = MediaQuery.of(context).orientation;
  }

  @override
  String toString() {
    return "width: ${display?.width}\nheight: ${display!.height}\ndpi: ${display!.dpi}\naspect: ${display!.aspect}\norientation: ${display!.orientation!.name}";
  }
}

Display? display;

void initializeDisplayInfos(context) {
  display = Display(context);
  print(display.toString());
}

proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
proj4.Projection epsg31370 =
    proj4.Projection.get('EPSG:31370') ??
    proj4.Projection.add(
      'EPSG:31370',
      '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs',
    );

proj4.Point epsg4326ToEpsg31370(proj4.Point spPoint) {
  return epsg4326.transform(epsg31370, spPoint);
}

late DicoAptProvider dico;

Memory? memory;

BuildContext? notificationContext;
BuildContext? anaPtPageContext;
bool offlineMode = false;
bool debug = false;
int currentPage = 0;

List<String> onboardLog = [forestimatorMobileVersion];
int lengthLog = 1;
@override
void print(dynamic it) {
  onboardLog.add("${DateTime.now().toString()}\n${it.toString()}");
}

// -1 means that no marker is selected
int selectedSearchMarker = -1;

List<PoiMarker> poiMarkerList = [];

bool saveChangesToPolygoneToPrefs = false;

class PoiMarker {
  final int index;
  final LatLng position;
  final String name;
  final String address;
  final String city;
  final String postcode;
  PoiMarker({
    required this.index,
    required this.position,
    required this.name,
    required this.address,
    required this.city,
    required this.postcode,
  });
}

List<pol.PolygonLayer> polygonLayers = [
  pol.PolygonLayer(polygonName: "Terrain"),
];
int selectedPolygonLayer = 0;

// ajouter le code le la couche à la fin de cette requete. fonctionne que pour layerbase avec mRes <= 10m sinon je considère que c'est trop volumineux
String queryApiRastDownload =
    "https://forestimator.gembloux.ulg.ac.be/api/rastPColor/layerCode";

String defaultLayer = "IGN";
List<String> interfaceSelectedLCode = ["IGN"];
List<bool> interfaceSelectedLOffline = [false];

String? pathExternalStorage;

class SelectedLayer {
  String mCode;
  bool offline;
  String sourceImagePath;
  SelectedLayer({
    required this.mCode,
    this.offline = false,
    this.sourceImagePath = "",
  });
}

List<SelectedLayer> selectedLayerForMap = [
  SelectedLayer(mCode: "3", offline: false),
  SelectedLayer(mCode: "2", offline: false),
  SelectedLayer(mCode: "1", offline: false),
];

String getFirstSelLayOffline() {
  List<SelectedLayer> l = selectedLayerForMap.where((i) => i.offline).toList();
  return l.isNotEmpty ? l.first.mCode : "_empty_";
}

void initializeSelectedLayerForFlutterMap() {
  if (!firstTimeUse) {
    for (int i = 0; i < interfaceSelectedLCode.length; i++) {
      replaceLayerFromList(
        interfaceSelectedLCode.elementAt(i),
        index: i,
        offline: false,
      );
    }
  } else {
    replaceLayerFromList(defaultLayer, index: 0, offline: false);
    removeLayerFromList(index: 1);
    removeLayerFromList(index: 2);
  }
}

List<String> getInterfaceSelectedLCode() {
  List<String> aRes = [];
  for (SelectedLayer l in selectedLayerForMap) {
    aRes.insert(aRes.length, l.mCode);
  }
  return aRes;
}

List<String> getInterfaceSelectedLOffline() {
  List<String> aRes = [];
  for (SelectedLayer l in selectedLayerForMap) {
    aRes.insert(aRes.length, l.offline.toString());
  }
  return aRes;
}

LayerAnaPt? anaPtPreview;
List<LayerAnaPt> requestedLayers = [];

List<String> anaPtSelectedLayerKeys = [
  "ZBIO",
  "CNSWrast",
  "CS_A",
  "MNT",
  "slope",
  "NT",
  "NH",
  "Topo",
  "AE",
  "COMPOALL",
  "MNH2021",
];

List<String> anaSurfSelectedLayerKeys = [
  "dendro_nha",
  "dendro_gha",
  "dendro_cdom",
  "dendro_hdom",
  "dendro_vha",
  "HE_FEE",
  "COMPOALL",
];

List<String> downloadableLayerKeys = [
  "ZBIO",
  "NT",
  "NH",
  "Topo",
  "CS_A",
  "CNSWrast",
];

//bool doDownload = false;

Position? position;
late proj4.Point pt;

const Color colorAgroBioTech = Color.fromRGBO(185, 205, 118, 1.0);
const Color colorDeselected = Color.fromARGB(255, 46, 46, 46);
const Color colorUliege = Color.fromRGBO(00, 112, 127, 1.0);
const Color colorBack = Color.fromRGBO(255, 120, 30, 1);
const Color colorBackground = Color.fromRGBO(202, 202, 202, 1);
const Color colorBackgroundSecondary = Color.fromRGBO(243, 243, 243, 1);

Function refreshMap = (Function f) {
  f();
};
Function refreshWholeCatalogueView = (void Function() setter) async {};
Function refreshSearch = (void Function() setter) async {};
Function refreshSettingsMenu = (void Function() setter) async {};
Function refreshCurrentThreeLayer = () {};
Function rebuildOfflineView = (void Function() setter) async {};
Function? rebuildNavigatorBar;
Function removeFromOfflineList = (var x) {};

int nMaxSelectedLayer = 3;

bool firstTimeUse = true;

//proj4.Point ptCenter = proj4.Point(x: 217200.0, y: 50100.0); // epioux
// WARNING lat =y lon=x
LatLng latlonCenter = const LatLng(49.76, 5.32);
double mapZoom = 7.0;

void removeLayerFromList({
  bool offline = false,
  int index = -1,
  String key = "",
}) {
  if (key != "" && index > -1) {
    print("Error in removeLayerFromList(): key != '' && index > -1");
    return;
  }
  if (key != "") {
    SelectedLayer? sL;
    for (var layer in selectedLayerForMap) {
      if (layer.mCode == key && layer.offline == offline) {
        sL = layer;
      }
    }
    if (sL != null) {
      int index = selectedLayerForMap.indexOf(sL);
      selectedLayerForMap.removeAt(index);
      selectedLayerForMap.insert(index, SelectedLayer(mCode: '${index + 1}'));
    }
  }
  if (index > -1) {
    selectedLayerForMap.removeAt(index);
    selectedLayerForMap.insert(index, SelectedLayer(mCode: '${index + 1}'));
  }
}

void replaceLayerFromList(
  String replacement, {
  String key = "",
  int index = -1,
  bool offline = false,
}) {
  if (key != "" && index > -1) {
    print("Error in replaceLayerFromList(): key != '' && index > -1");
    return;
  }
  if (key != "") {
    SelectedLayer? sL;
    for (var layer in selectedLayerForMap) {
      if (layer.mCode == key && layer.offline == offline) {
        sL = layer;
      }
    }
    if (sL != null) {
      int index = selectedLayerForMap.indexOf(sL);
      selectedLayerForMap.removeAt(index);
      selectedLayerForMap.insert(
        index,
        SelectedLayer(mCode: replacement, offline: offline),
      );
    }
  }
  if (index > -1) {
    selectedLayerForMap.removeAt(index);
    selectedLayerForMap.insert(
      index,
      SelectedLayer(mCode: replacement, offline: offline),
    );
  }
}

int getIndexForLayer(String key) {
  int index = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.mCode == key) {
      return index;
    }
    index++;
  }
  return -1;
}

void savePrefSelLayOnline() async {
  interfaceSelectedLCode.clear();
  for (SelectedLayer sL in selectedLayerForMap) {
    interfaceSelectedLCode.add(sL.mCode);
  }
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList('interfaceSelectedLCode', interfaceSelectedLCode);
}

void savePrefSelLayOffline() async {
  interfaceSelectedLCode.clear();
  for (SelectedLayer sL in selectedLayerForMap) {
    interfaceSelectedLCode.add(sL.mCode);
  }
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList('interfaceSelectedOffCode', interfaceSelectedLCode);
}

void loadPrefSelLayOnline() async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  interfaceSelectedLCode = prefs.getStringList('interfaceSelectedLCode')!;
  selectedLayerForMap.clear();
  for (String key in interfaceSelectedLCode) {
    selectedLayerForMap.add(SelectedLayer(mCode: key, offline: false));
  }
}

void loadPrefSelLayOffline() async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs.getStringList('interfaceSelectedOffCode') != null) {
    interfaceSelectedLCode = prefs.getStringList('interfaceSelectedOffCode')!;
    selectedLayerForMap.clear();
    for (String key in interfaceSelectedLCode) {
      selectedLayerForMap.add(SelectedLayer(mCode: key, offline: true));
    }
  }
}

void changeSelectedLayerModeOffline() {
  if (dico.getLayersOffline().isEmpty) {
    offlineMode = false;
    return;
  }
  savePrefSelLayOnline();
  loadPrefSelLayOffline();
  selectedLayerForMap.removeWhere((element) => element.offline == false);
  if (dico.getLayersOffline().where((i) => i.mBits == 8).toList().isNotEmpty &&
      selectedLayerForMap.isEmpty) {
    selectedLayerForMap.insert(
      0,
      SelectedLayer(
        mCode:
            dico
                .getLayersOffline()
                .where((i) => i.mBits == 8)
                .toList()
                .first
                .mCode,
        offline: true,
      ),
    );
  } else {
    while (selectedLayerForMap.length > 1) {
      selectedLayerForMap.removeLast();
    }
  }
  if (selectedLayerForMap.isEmpty) {
    selectedLayerForMap.insert(0, SelectedLayer(mCode: '1', offline: true));
  }
}

void changeSelectedLayerModeOnline() {
  savePrefSelLayOffline();
  loadPrefSelLayOnline();
}

bool isSelectedLayer(String key, {offline = false}) {
  for (var layer in selectedLayerForMap) {
    if (layer.mCode == key && layer.offline == offline) {
      return true;
    }
  }
  return false;
}

bool slotContainsLayer(int index, String key) {
  return offlineMode
      ? selectedLayerForMap.first.mCode == key
      : selectedLayerForMap[index].mCode == key;
}

List<SelectedLayer> getLayersForFlutterMap() {
  return selectedLayerForMap
      .where(
        (val) =>
            !(val.mCode.length < 3 &&
                (val.mCode.contains('1') ||
                    val.mCode.contains('2') ||
                    val.mCode.contains('3'))),
      )
      .toList()
      .reversed
      .toList();
}

Map<int, int> lutVulnerabiliteCS = {
  0: 0,
  1: 1,
  2: 1,
  3: 3,
  4: 5,
  5: 2,
  6: 2,
  7: 3,
  8: 6,
  9: 2,
  10: 4,
  11: 4,
  12: 4,
  13: 7,
};
