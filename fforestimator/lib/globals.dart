library fforestimator.globals;

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:latlong2/latlong.dart';

late dicoAptProvider dico;

String defaultLayer = "IGN";
// list to memorize the keys of selected layer to show in interface.
List<String> interfaceSelectedLayerKeys = [defaultLayer];

String layersAnaPt = "ZBIO+CNSWrast+MNT+slope+CS_A+NT+NH+Topo+AE+brol";

late LatLng currentPositionOnMap;
late double currentZoom;
