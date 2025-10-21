import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart' as pol;
import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:memory_info/memory_info.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';
import 'package:shared_preferences/shared_preferences.dart';

const String forestimatorMobileVersion = "2.0.0 - build 21";
const double globalMinZoom = 4.0;
const double globalMaxZoom = 13.0;
const double globalMinOfflineZoom = 8.0;
const double globalMaxOfflineZoom = 13.0;

Color backgroundTransparentBlackBox = Colors.black.withAlpha(180);

bool modeMapFirstTileLayerTransparancy = true;
bool modeMapShowPolygons = true;
bool modeMapShowSearchMarker = true;
bool modeMapShowCustomMarker = true;
bool modeDevelopper = false;

class Display {
  double width = -1;
  double height = -1;
  double dpi = -1;
  double aspect = -1;
  Orientation orientation = Orientation.portrait;
  double equipixel = -1;
  double equiwidth = -1;
  double equiheight = -1;
  static bool keyboardExpanded = false;
  static bool modeSquare = false;
  static bool modeTablet = false;
  static bool overrideModeTablet = false;
  static bool overrideModeSquare = false;
  static bool modeExpert = false;
  static bool modeExpertTools = false;

  Display.empty();

  Display(BuildContext context) {
    width = MediaQuery.of(context).size.width;
    height = MediaQuery.of(context).size.height;
    aspect = MediaQuery.of(context).size.aspectRatio;
    dpi = MediaQuery.of(context).devicePixelRatio;
    orientation = MediaQuery.of(context).orientation;

    _tabletMode();
    _squareMode();
    _enforceEquiWidthHeight();
    if (modeSquare || overrideModeSquare) {
      minEquiPixelsDisplayLandscapeHeight =
          minEquiPixelsDisplayLandscapeWidth * .8;
      minEquiPixelsDisplayPortraitHeight =
          minEquiPixelsDisplayPortraitWidth * .8;
    }
  }

  void _tabletMode() {
    if (dpi < 2.001 && dpi * (width < height ? width : height) > 1800 ||
        dpi < 1.75 && dpi * (width < height ? width : height) > 1320 ||
        dpi < 1.55 && dpi * (width < height ? width : height) > 1000 ||
        overrideModeTablet) {
      if (!modeTablet) {
        minEquiPixelsDisplayPortraitWidth = 150;
        minEquiPixelsDisplayPortraitHeight = 300;
        minEquiPixelsDisplayLandscapeWidth = 300;
        minEquiPixelsDisplayLandscapeHeight = 150;
        modeTablet = true;
      }
    } else {
      if (modeTablet) {
        minEquiPixelsDisplayPortraitWidth = 100;
        minEquiPixelsDisplayPortraitHeight = 200;
        minEquiPixelsDisplayLandscapeWidth = 200;
        minEquiPixelsDisplayLandscapeHeight = 100;
        modeTablet = false;
      }
    }
  }

  void _squareMode() {
    if ((aspect > .8 && aspect < 1 / .8) || modeTablet || overrideModeSquare) {
      modeSquare = true;
      orientation = Orientation.landscape;
    } else {
      modeSquare = false;
    }
  }

  void _enforceEquiWidthHeight() {
    if (orientation == Orientation.portrait) {
      equipixel = height / minEquiPixelsDisplayPortraitHeight;
      equiwidth = width / equipixel;
      while (equiwidth < minEquiPixelsDisplayPortraitWidth) {
        equipixel *= 0.99;
        equiwidth = width / equipixel;
      }
      equiheight = height / equipixel;
    } else {
      equipixel = width / minEquiPixelsDisplayLandscapeWidth;
      equiheight = height / equipixel;
      while (equiheight < minEquiPixelsDisplayLandscapeHeight) {
        equipixel *= 0.99;
        equiheight = height / equipixel;
      }
      equiwidth = width / equipixel;
    }
  }

  @override
  String toString() {
    return "width: $width\nheight: $height\ndpi: $dpi\naspect: $aspect\norientation: ${orientation.name}\nequipixel: $equipixel\nequiwidth: $equiwidth\nequiheight: $equiheight";
  }
}

Display display = Display.empty();

void initializeDisplayInfos(BuildContext context) {
  display = Display(context);
  print(display.toString());
}

// This has to be guaranteed on all displays to ensure correct visibility
double minEquiPixelsDisplayPortraitWidth = 100;
double minEquiPixelsDisplayPortraitHeight = 200;
double minEquiPixelsDisplayLandscapeWidth = 200;
double minEquiPixelsDisplayLandscapeHeight = 100;
// Visual sizes of menus etc. in equipixels
// Top Bar
double topAppInfoBarThickness = 10;
double topAppForestimatorFontHeight = 5;
double topAppForestimatorFontWidth = 60;
// PopupWindows
double popupWindowsPortraitWidth = minEquiPixelsDisplayPortraitWidth - 5;
double popupWindowsPortraitHeight =
    minEquiPixelsDisplayPortraitHeight - topAppInfoBarThickness - 20;
double popupWindowsLandscapeWidth = minEquiPixelsDisplayLandscapeWidth - 5;
double popupWindowsLandscapeHeight =
    minEquiPixelsDisplayLandscapeHeight - topAppInfoBarThickness - 10;
double popupReturnButtonHeight = 16;
double popupReturnButtonWidth = 52;
// Menus
double menuBarThickness = 20;
double menuBarLength = 65;
double iconSizeXS = 6;
double iconSizeS = 9;
double iconSizeM = 12;
double iconSizeSettings = 8;
double iconSpaceBetween = 8;
// Offline loading box
double loadingMapBoxWidth = 70;
double loadingMapBoxHeight = 15;
// General Fonts
double fontSizeXS = 3;
double fontSizeS = 4;
double fontSizeM = 5;
double fontSizeL = 6;
double fontSizeXL = 7;
// Polygons
double chosenPolyBarWidth = 95;
double chosenPolyBarHeight = 25;
double infoBoxPolygon = 30;
// AnaPtPreview
double anaPtBoxSize = 12;
// Popup Poly List
double polyListCardHeight = 22;
double polyListSelectedCardHeight = 44;
double polyListCardWidth = 90;
double polyListSelectedCardWidth = 95;
double polyNewPolygonButtonHeight = 12;
// search location
double searchBarHeight = 13;
double searchBarWidth = 75;
// Layer Switcher
double layerSwitcherTileHeight = 12;
double layerswitcherBoxWidth = 80;
double layerSwitcherBoxHeightPortrait = 5.5 * layerSwitcherTileHeight;
double layerSwitcherBoxHeightPortraitOffline = 2.5 * layerSwitcherTileHeight;
double layerSwitcherBoxHeightLandscape = 66;
double layerswitcherButtonsBoxHeight = 30;
double layerswitcherControlBoxHeight =
    layerSwitcherTileHeight + fontSizeM * 1.2;
// Do you really dialogue
double dyrDialogWidth = 60;
double dyrDialogHeight = 60;
double dyrButtonsize = 20;
// Online Catalogue
double onCatalogueWidth = 98;
double onCatalogueSearchBoxHeight = 12;
double onCatalogueCategoryHeight = 20;
double onCatalogueMapHeight = 20;
double onCatalogueIconSize = 7;
double onCatalogueLayerSelectionButton = 10;
// Legend view
double legendHeightColorTile = 0.01;

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

List<pol.PolygonLayer> polygonLayers = [];
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
Function rebuildSwitcherCatalogueButtons = (void Function() setter) async {};
Function refreshSearch = (void Function() setter) async {};
Function refreshSettingsMenu = (void Function() setter) async {};
Function refreshCurrentThreeLayer = () {};
Function rebuildOfflineCatalogue = (void Function() setter) async {};
Function rebuildSwitcherBox = (void Function() setter) async {};
Function rebuildLayerSwitcher = (void Function() setter) async {};
Function rebuildStatusSymbols = (void Function() setter) async {};
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
      selectedLayerForMap.insert(
        index,
        SelectedLayer(mCode: '${index + 1}', offline: offline),
      );
    }
  }
  if (index > -1) {
    selectedLayerForMap.removeAt(index);
    selectedLayerForMap.insert(
      index,
      SelectedLayer(mCode: '${index + 1}', offline: offline),
    );
  }
}

void replaceLayerFromList(
  String replacement, {
  String key = "",
  int index = -1,
  bool offline = false,
}) {
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
  } else if (index > -1) {
    selectedLayerForMap.removeAt(index);
    selectedLayerForMap.insert(
      index,
      SelectedLayer(mCode: replacement, offline: offline),
    );
  } else if (getCountOfSelectedLayersForMap() == 3) {
    selectedLayerForMap.removeAt(2);
    selectedLayerForMap.insert(
      0,
      SelectedLayer(mCode: replacement, offline: offline),
    );
  } else if (getCountOfSelectedLayersForMap() == 0) {
    selectedLayerForMap.removeAt(0);
    selectedLayerForMap.insert(
      0,
      SelectedLayer(mCode: replacement, offline: offline),
    );
  } else {
    selectedLayerForMap.removeAt(getIndexForEmptySlot());
    selectedLayerForMap.insert(
      0,
      SelectedLayer(mCode: replacement, offline: offline),
    );
  }
}

int getCountOfSelectedLayersForMap() {
  int count = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.mCode.length > 1) {
      count++;
    }
  }
  return count;
}

int getIndexForEmptySlot() {
  int count = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.mCode.length < 2) {
      return count;
    }
    count++;
  }
  return offlineMode
      ? 0
      : count > 3
      ? 0
      : count;
}

int getCountOfflineLayerSelected() {
  int count = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.offline) {
      count++;
    }
  }
  return count;
}

int getIndexForLayer(String key, bool offline) {
  int index = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.mCode == key && layer.offline == offline) {
      return index;
    }
    index++;
  }
  return -1;
}

int getIndexForNextLayerOffline() {
  int index = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.offline) {
      return index;
    }
    index++;
  }
  return -1;
}

int sameOnlineAsOfflineLayer(String key, bool offline) {
  int index = 0;
  for (SelectedLayer layer in selectedLayerForMap) {
    if (layer.mCode == key && layer.offline != offline) {
      return index;
    }
    index++;
  }
  return -1;
}

void savePrefSelLayOnline() async {
  interfaceSelectedLCode.clear();
  List<String> offlineLayer = [];
  for (SelectedLayer sL in selectedLayerForMap) {
    interfaceSelectedLCode.add(sL.mCode);
    offlineLayer.add(dico.getLayerBase(sL.mCode).mOffline ? "t" : "n");
  }
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList('interfaceSelectedLCode', interfaceSelectedLCode);
  await prefs.setStringList('interfaceSelectedLCodeOfflineFlag', offlineLayer);
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
  List<String> offlineLayer =
      prefs.getStringList('interfaceSelectedLCodeOfflineFlag')!;
  selectedLayerForMap.clear();
  int index = 0;
  for (String key in interfaceSelectedLCode) {
    selectedLayerForMap.add(
      SelectedLayer(
        mCode: key,
        offline: offlineLayer[index] == "t" ? true : false,
      ),
    );
    index++;
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

List<Widget> mainStack = [];

void mainStackPopLast() {
  mainStack.removeLast();
}
