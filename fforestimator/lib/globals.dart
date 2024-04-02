library fforestimator.globals;

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/scaffoldNavigation.dart';
import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';
import 'package:shared_preferences/shared_preferences.dart';

late dicoAptProvider dico;

String basePathbranchB = "catalogue";
String basePathbranchC = "offline";

bool offlineMode = false;

// ajouter le code le la couche à la fin de cette requete. fonctionne que pour layerbase avec mRes <= 10m sinon je considère que c'est trop volumineux
String queryApiRastDownload =
    "https://forestimator.gembloux.ulg.ac.be/api/rastPColor/layerCode";

String defaultLayer = "IGN";
List<String> interfaceSelectedLCode = ["IGN"];
// list to memorize the keys of selected layer to show in interface.

class selectedLayer {
  String mCode;
  bool offline;
  String sourceImagePath;
  selectedLayer(
      {required this.mCode, this.offline = false, this.sourceImagePath = ""});
}

List<selectedLayer> interfaceSelectedLayerKeys = [];

void refreshInterfaceSelectedL() {
  interfaceSelectedLayerKeys =
      interfaceSelectedLCode.map<selectedLayer>((String aCode) {
    return selectedLayer(mCode: aCode);
  }).toList();
}

List<String> getInterfaceSelectedLCode() {
  List<String> aRes = [];
  for (selectedLayer l in interfaceSelectedLayerKeys) {
    aRes.insert(aRes.length, l.mCode);
  }
  return aRes;
}

List<layerAnaPt> requestedLayers = [];

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

Function rebuildWholeWidgetTree = () {};
Function refreshMap = () {};
Function refreshCatalogueView = () {};
Function refreshCurrentThreeLayer = () {};

Function? rebuildNavigatorBar;

int nOnlineLayer = 3;
int nOfflineLayer = 1;

//proj4.Point ptCenter = proj4.Point(x: 217200.0, y: 50100.0); // epioux
// WARNING lat =y lon=x
LatLng latlonCenter = const LatLng(49.76, 5.32);
double mapZoom = 7.0;

void removeLayerFromList(String key) async {
  selectedLayer? sL;
  for (var layer in interfaceSelectedLayerKeys) {
    if (layer.mCode == key) {
      sL = layer;
    }
  }
  if (sL != null) {
    interfaceSelectedLayerKeys.remove(sL);
  }
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList(
      'interfaceSelectedLCode', getInterfaceSelectedLCode());
}

void addLayerToList(String key) async {
  interfaceSelectedLayerKeys.insert(0, selectedLayer(mCode: key));
  //sauver dans shared_preference
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList(
      'interfaceSelectedLCode', getInterfaceSelectedLCode());
}
