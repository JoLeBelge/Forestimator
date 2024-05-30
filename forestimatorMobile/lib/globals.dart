library fforestimator.globals;

import 'package:fforestimator/dico/dicoApt.dart';
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
List<bool> interfaceSelectedLOffline = [false];

class selectedLayer {
  String mCode;
  bool offline;
  String sourceImagePath;
  selectedLayer(
      {required this.mCode, this.offline = false, this.sourceImagePath = ""});
}

List<selectedLayer> interfaceSelectedLayerKeys = [];

String getFirstSelLayOffline() {
  List<selectedLayer> l =
      interfaceSelectedLayerKeys.where((i) => i.offline).toList();
  return l.length > 0 ? l.first.mCode : "toto";
}

void refreshInterfaceSelectedL() {
  for (int i = 0; i < interfaceSelectedLCode.length; i++) {
    bool offline = false;
    if (interfaceSelectedLOffline.length > i) {
      offline = interfaceSelectedLOffline.elementAt(i);
    }
    addLayerToList(interfaceSelectedLCode.elementAt(i),
        offline: offline, savePref: false);
  }
}

List<String> getInterfaceSelectedLCode() {
  List<String> aRes = [];
  for (selectedLayer l in interfaceSelectedLayerKeys) {
    aRes.insert(aRes.length, l.mCode);
  }
  return aRes;
}

List<String> getInterfaceSelectedLOffline() {
  List<String> aRes = [];
  for (selectedLayer l in interfaceSelectedLayerKeys) {
    aRes.insert(aRes.length, l.offline.toString());
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
Function refreshCurrentThreeLayer = () {};
Function refreshOfflineView = () {};
Function rebuildOfflineView = (void Function() setter) async {};
Function? rebuildNavigatorBar;

Function addToOfflineList = (var x) {};
Function removeFromOfflineList = (var x) {};

int nMaxSelectedLayer = 3;

bool firstTimeUse = true;

//proj4.Point ptCenter = proj4.Point(x: 217200.0, y: 50100.0); // epioux
// WARNING lat =y lon=x
LatLng latlonCenter = const LatLng(49.76, 5.32);
double mapZoom = 7.0;

void removeLayerFromList(String key, {bool offline = false}) async {
  selectedLayer? sL;
  for (var layer in interfaceSelectedLayerKeys) {
    if (layer.mCode == key && layer.offline == offline) {
      sL = layer;
    }
  }
  if (sL != null) {
    interfaceSelectedLayerKeys.remove(sL);
  }
  savePrefSelLay();
}

void savePrefSelLay() async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  await prefs.setStringList(
      'interfaceSelectedLCode', getInterfaceSelectedLCode());
  await prefs.setStringList(
      'interfaceSelectedLOffline', getInterfaceSelectedLOffline());
}

void changeSelectedLayerModeOffline() {
  interfaceSelectedLayerKeys.removeWhere((element) => element.offline == false);
  // check si il y a au moins une carte offline dans la selection
  ///if (interfaceSelectedLayerKeys.where((i) => i.offline).toList().length == 0) {

  // si non on en ajoute une si on en a

  if (dico.getLayersOffline().where((i) => i.mBits == 8).toList().length > 0) {
    // if (interfaceSelectedLayerKeys.length == 3) {
    //   interfaceSelectedLayerKeys.removeLast();
    // }

    interfaceSelectedLayerKeys.insert(
      0,
      selectedLayer(
          mCode: dico
              .getLayersOffline()
              .where((i) => i.mBits == 8)
              .toList()
              .first
              .mCode,
          offline: true),
    );
  }
  //}
  savePrefSelLay();
}

void addLayerToList(String key,
    {bool offline = false, bool savePref = true}) async {
  interfaceSelectedLayerKeys.insert(
      0, selectedLayer(mCode: key, offline: offline));
  if (interfaceSelectedLayerKeys.length > 3) {
    interfaceSelectedLayerKeys.removeLast();
  }
  if (savePref) {
    savePrefSelLay();
  }
}

bool isSelectedLayer(String key, {offline = false}) {
  for (var layer in interfaceSelectedLayerKeys) {
    if (layer.mCode == key && layer.offline == offline) {
      return true;
    }
  }
  return false;
}
