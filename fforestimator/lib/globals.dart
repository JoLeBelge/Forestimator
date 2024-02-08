library fforestimator.globals;

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:latlong2/latlong.dart';

late dicoAptProvider dico;

String defaultLayer = "IGN";
// list to memorize the keys of selected layer to show in interface.
List<String> interfaceSelectedLayerKeys = [defaultLayer];

String layersAnaPt =
    "ZBIO+CNSWrast+CS_A+MNT+slope+NT+NH+Topo+AE+COMPOALL+MNH2021+brol";

late LatLng currentPositionOnMap;
late double currentZoom;
