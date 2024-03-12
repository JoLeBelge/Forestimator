library fforestimator.globals;

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:go_router/go_router.dart';

late dicoAptProvider dico;

String basePathbranchB = "catalogue";

// ajouter le code le la couche à la fin de cette requete. fonctionne que pour layerbase avec mRes <= 10m sinon je considère que c'est trop volumineux
String queryApiRastDownload =
    "https://forestimator.gembloux.ulg.ac.be/api/rastPColor/layerCode/";

String defaultLayer = "IGN";
// list to memorize the keys of selected layer to show in interface.
List<String> interfaceSelectedLayerKeys = [defaultLayer];

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
  "MNH2021"
];

Position? position;

const Color colorAgroBioTech = Color.fromRGBO(185, 205, 118, 1.0);
const Color colorDeselected = Color.fromARGB(255, 46, 46, 46);
const Color colorUliege = Color.fromRGBO(00, 112, 127, 1.0);
const Color colorBack = Color.fromRGBO(255, 120, 30, 1);
const Color colorBackground = Color.fromRGBO(202, 202, 202, 1);
const Color colorBackgroundSecondary = Color.fromRGBO(243, 243, 243, 1);

Function refreshMap = () {};
Function refreshCatalogueView = () {};
Function refreshCurrentThreeLayer = () {};
