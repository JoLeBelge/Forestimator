import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'dart:async';
import 'package:area_polygon/area_polygon.dart';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';
import 'package:uuid/uuid.dart';

class Attribute {
  String type;
  String name;
  dynamic value;
  bool visibleOnMapLabel;

  Attribute({required this.name, this.type = "string", this.value = "", this.visibleOnMapLabel = false});
}

class Geometry {
  String identifier = Uuid().v4();
  bool sentToServer = false;
  String hitValue = Uuid().v4();
  String type = "Point";
  String name = "";
  bool visibleOnMap = true;
  bool labelsVisibleOnMap = true;
  int selectedPointIcon = 0;
  double iconSize = 10;
  List<LatLng> points = [];
  List<int> selectedPolyLinePoints = [0, 0];
  Color colorInside = Color.fromRGBO(255, 128, 164, 80);
  double transparencyInside = 0.0;
  Color colorLine = Color.fromRGBO(255, 128, 164, 80);
  double transparencyLine = 0.0;
  int selectedVertex = -1;
  double area = 0.0;
  double perimeter = 0.0;
  Map<String, dynamic> decodedJson = {};
  List<Attribute> attributes = [];

  Geometry({String polygonName = ""}) {
    name = polygonName;
    Random randomColor = Random();
    setColorInside(Color.fromRGBO(randomColor.nextInt(256), randomColor.nextInt(256), randomColor.nextInt(256), 0.4));
    setColorLine(Color.fromRGBO((colorInside.r * 255).round(), (colorInside.g * 255).round(), (colorInside.b * 255).round(), 1.0));
  }

  Geometry.point({String polygonName = ""}) {
    name = polygonName;
    Random randomColor = Random();
    setColorInside(Color.fromRGBO(randomColor.nextInt(256), randomColor.nextInt(256), randomColor.nextInt(256), 0.4));
    setColorLine(Color.fromRGBO((colorInside.r * 255).round(), (colorInside.g * 255).round(), (colorInside.b * 255).round(), 1.0));
  }

  Geometry.polygon({String polygonName = ""}) {
    type = "Polygon";
    name = polygonName;
    Random randomColor = Random();
    setColorInside(Color.fromRGBO(randomColor.nextInt(256), randomColor.nextInt(256), randomColor.nextInt(256), 0.4));
    setColorLine(Color.fromRGBO((colorInside.r * 255).round(), (colorInside.g * 255).round(), (colorInside.b * 255).round(), 1.0));
  }

  Geometry.essencePoint({String polygonName = ""}) {
    type = "Point-essence";
    attributes.addAll([Attribute(name: "essence", type: "string", value: gl.essenceChoice[0]), Attribute(name: "rmq", type: "string", value: "")]);
    attributes[1].visibleOnMapLabel = true;
    visibleOnMap = true;
    name = polygonName;
    selectedPointIcon = 4;
    setColorInside(gl.dico.mLayerBases["COMPOALL"]!.mDicoCol[gl.essenceChoice.indexOf(gl.essenceChoice[0])] ?? Colors.black);
    setColorLine(colorInside.withAlpha(255));
  }

  void setTransparencyLine(double value) {
    if (value >= 0.0 && value <= 1.0) {
      transparencyLine = value;
    }
  }

  void setTransparencyInside(double value) {
    if (value >= 0.0 && value <= 1.0) {
      transparencyInside = value;
    }
  }

  void setColorInside(Color value) {
    colorInside = value;
  }

  void setColorLine(Color value) {
    colorLine = value;
  }

  void refreshSelectedLinePoints(LatLng? point) {
    if (point == null) {
      selectedPolyLinePoints = [0, 0];
      return;
    }
    int index = pointIndex(point);
    gl.print(selectedPolyLinePoints);
    selectedPolyLinePoints[0] = index;
    if (points.length == 1) {
      selectedPolyLinePoints[1] = index;
    } else if (points.length - 1 == index) {
      selectedPolyLinePoints[1] = 0;
    } else if (points.length - 1 > index) {
      selectedPolyLinePoints[1] = index + 1;
    } else {
      gl.print("error 1st index polygon line: $index");
    }
    gl.print(selectedPolyLinePoints);
  }

  bool isSelectedLine(int i) {
    return (i == selectedPolyLinePoints[0] || i == selectedPolyLinePoints[1]) ? true : false;
  }

  void addPoint(LatLng point) {
    points.insert(selectedPolyLinePoints[1], point);
    refreshSelectedLinePoints(point);
    _computeArea();
    _computePerimeter();
    serialize();
  }

  List<Point> getPolyPlusOneVertex(LatLng vertex) {
    List<Point> copyOfPoly = [];

    for (LatLng p in points) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }

    copyOfPoly.insert(selectedPolyLinePoints[1], Point(vertex.latitude, vertex.longitude));
    return copyOfPoly;
  }

  List<Point> getPolyMoveOneVertex(LatLng old, LatLng index) {
    List<Point> copyOfPoly = [];

    for (LatLng p in points) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }
    int i = 0;
    for (LatLng point in points) {
      if (old.latitude == point.latitude && old.longitude == point.longitude) {
        break;
      }
      i++;
    }

    copyOfPoly.removeAt(i);
    copyOfPoly.insert(i, Point(index.latitude, index.longitude));

    return copyOfPoly;
  }

  List<Point> getPolyRemoveOneVertex(LatLng index) {
    List<Point> copyOfPoly = [];

    for (LatLng p in points) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }
    int i = 0;
    for (var point in points) {
      if (index.latitude == point.latitude && index.longitude == point.longitude) {
        break;
      }
      i++;
    }
    copyOfPoly.removeAt(i);

    return copyOfPoly;
  }

  void removePoint(LatLng index) {
    int i = 0;
    for (var point in points) {
      if (index.latitude == point.latitude && index.longitude == point.longitude) {
        break;
      }
      i++;
    }
    int pointIsSelected = -1;
    if (i == selectedPolyLinePoints[0] && points.length > 2) {
      pointIsSelected = selectedPolyLinePoints[1];
    } else if (i == selectedPolyLinePoints[1] && points.length > 2) {
      pointIsSelected = selectedPolyLinePoints[0];
    }
    points.removeAt(i);
    if (pointIsSelected > -1 && pointIsSelected < points.length) {
      refreshSelectedLinePoints(points[pointIsSelected]);
    } else {
      if (points.length == 1) {
        refreshSelectedLinePoints(points[0]);
      } else if (points.isEmpty) {
        refreshSelectedLinePoints(null);
      } else {
        int selection = (selectedPolyLinePoints[0] > selectedPolyLinePoints[1]) ? selectedPolyLinePoints[1] : selectedPolyLinePoints[0];
        refreshSelectedLinePoints(points[selection > points.length - 1 ? points.length - 1 : selection]);
      }
    }

    _computeArea();
    _computePerimeter();
    serialize();
  }

  void replacePoint(LatLng old, LatLng index) {
    int i = 0;
    for (var point in points) {
      if (old.latitude == point.latitude && old.longitude == point.longitude) {
        break;
      }
      i++;
    }
    points.removeAt(i);
    points.insert(i, index);
    _computeArea();
    _computePerimeter();
    serialize();
  }

  Color get colorSurface => colorInside;
  Color get colorPolygon => colorLine;
  String get id => identifier;
  int get numPoints => points.length;
  LatLng get center {
    double x = 0.0, y = 0.0;
    int i = 0;
    for (var point in points) {
      y += point.longitude;
      x += point.latitude;
      i++;
    }
    return LatLng(x / (i > 0 ? i : 1), y / (i > 0 ? i : 1));
  }

  int pointIndex(LatLng searchedPoint) {
    int i = 0;
    for (var point in points) {
      if (searchedPoint.latitude == point.latitude && searchedPoint.longitude == point.longitude) {
        break;
      }
      i++;
    }
    return i;
  }

  Offset _epsg4326ToEpsg31370(proj4.Point spPoint) {
    return Offset(gl.epsg4326.transform(gl.epsg31370, spPoint).x, gl.epsg4326.transform(gl.epsg31370, spPoint).y);
  }

  void _computeArea() {
    if (points.length < 2) {
      area = 0.0;
      return;
    }

    List<Offset> poly = [];
    for (LatLng point in points) {
      poly.add(_epsg4326ToEpsg31370(proj4.Point(y: point.latitude, x: point.longitude)));
    }
    area = calculateArea(poly);
  }

  void _computePerimeter() {
    if (points.length < 2) {
      area = 0.0;
      return;
    }
    List<Offset> poly = [];
    for (LatLng point in points) {
      poly.add(_epsg4326ToEpsg31370(proj4.Point(y: point.latitude, x: point.longitude)));
    }
    perimeter = 0.0;
    Offset currentPoint = poly.last;
    for (Offset point in poly) {
      perimeter = perimeter + (currentPoint - point).distance;
      currentPoint = point;
    }
  }

  String getPolyPointsString() {
    String coordinates = "";
    for (LatLng point in points) {
      proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: point.longitude, y: point.latitude));
      coordinates = "$coordinates[${tLb72.x}, ${tLb72.y}],";
    }
    if (type == "Polygon" && points.isNotEmpty) {
      proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: points.first.longitude, y: points.first.latitude));
      coordinates = "$coordinates[${tLb72.x}, ${tLb72.y}]";
    } else {
      if (coordinates.length > 1) {
        coordinates = coordinates.substring(0, coordinates.length - 1);
      }
    }
    if (coordinates.isEmpty) {
      return "[]";
    } else {
      return coordinates;
    }
  }

  Future<bool> onlineSurfaceAnalysis() async {
    if (points.length < 2) return false;
    bool internet = await InternetConnection().hasInternetAccess;
    if (internet) {
      String layersAnaSurf = "";
      for (String lCode in gl.anaSurfSelectedLayerKeys) {
        if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
          layersAnaSurf += "$lCode+";
        }
      }
      layersAnaSurf = layersAnaSurf.substring(0, layersAnaSurf.length - 1);

      String polygon = "POLYGON ((";
      for (LatLng point in points) {
        proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: point.longitude, y: point.latitude));
        polygon = "$polygon${tLb72.x} ${tLb72.y},";
      }
      proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: points.first.longitude, y: points.first.latitude));
      polygon = "$polygon${tLb72.x} ${tLb72.y},";
      polygon = "${polygon.substring(0, polygon.length - 1)}))";

      String request = "https://forestimator.gembloux.ulg.ac.be/api/anaSurf/layers/$layersAnaSurf/polygon/$polygon";
      http.Response? res;
      try {
        res = await http.get(Uri.parse(request));
        if (res.statusCode != 200) throw HttpException('${res.statusCode}');
      } catch (e) {
        gl.print("Error surface analysing request");
        gl.print("$e");
      }
      try {
        decodedJson.clear();
        decodedJson = jsonDecode(res!.body);
      } catch (e) {
        gl.print("Error decoding json surface analysing request");
        gl.print("$e");
        try {
          String correctedBody = res!.body.replaceAll('",\n}', '"\n}');
          correctedBody = correctedBody.replaceAll('-', '0.0');
          gl.print(correctedBody);
          gl.print(res.body);
          decodedJson = jsonDecode(correctedBody);
          gl.print("Error corrected");
        } catch (e) {
          gl.print("Error cannot be correct!");
          gl.print("$e");
        }
      }
      gl.print(decodedJson);
    } else {
      gl.print("Could not make surface analysis, no internet!");
      gl.mainStack.add(popupNoInternet());
      return false;
    }
    return true;
  }

  static void sendEssencePointsInBackground() async {
    if (!gl.Mode.essencePointsToSync) {
      gl.Mode.essencePointsToSync = true;
      gl.startTimer(sendInBackground, () => !gl.Mode.essencePointsToSync, 1, 300);
    }
  }

  static Future<bool> sendInBackground() async {
    bool allFinished = true;
    if (gl.selectedGeoLayer < 0) return true;
    for (int i = 0; i < gl.selLay.geometries.length; i++) {
      if (gl.selLay.geometries[i].type == "Point-essence" && !gl.selLay.geometries[i].sentToServer) {
        if (!await gl.selLay.geometries[i].sendGeometryToServer()) allFinished = false;
      }
    }
    gl.Mode.essencePointsToSync = !allFinished;
    return allFinished;
  }

  Future<bool> sendGeometryToServer() async {
    if (sentToServer) {
      gl.print("Geometry $name already sent once!");
      gl.mainStack.add(popupGeometryAlreadySent());
      return false;
    }
    bool internet = await InternetConnection().hasInternetAccess;
    if (internet) {
      String coordinates = "";
      for (LatLng point in points) {
        proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: point.longitude, y: point.latitude));
        coordinates = "$coordinates[${tLb72.x}, ${tLb72.y}],";
      }
      if (type == "Polygon") {
        proj4.Point tLb72 = gl.epsg4326.transform(gl.epsg31370, proj4.Point(x: points.first.longitude, y: points.first.latitude));
        coordinates = "[[$coordinates[${tLb72.x}, ${tLb72.y}]]]";
      } else {
        if (coordinates.length > 1) {
          coordinates = coordinates.substring(0, coordinates.length - 1);
        }
      }
      String properties = "";
      properties = "$properties\"name\":\"$name\",";
      properties = "$properties\"area\":\"$area\",";
      properties = "$properties\"perimeter\":\"$perimeter\",";
      properties = "$properties\"nom_contact\":\"${gl.UserData.name} ${gl.UserData.forename}\",";
      properties = "$properties\"contact\":\"${gl.UserData.mail}\",";
      for (Attribute attribute in attributes) {
        properties = "$properties\"${attribute.name}\":\"${attribute.value.toString()}\",";
      }
      properties = properties.substring(0, properties.length - 1);
      String geometry =
          "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"${getProperTypeForCarto(type)}\",\"coordinates\":$coordinates},\"properties\":{$properties}}]}";

      String request = "https://forestimator.gembloux.ulg.ac.be/api/polygFromMobile/$geometry";
      gl.print(request);
      http.Response? response;
      try {
        response = await http.get(Uri.parse(request));
        if (response.statusCode != 200) {
          throw HttpException('${response.statusCode}');
        }
      } catch (e) {
        gl.print("Server error sending geometry to server");
        gl.print("$e");
        return false;
      }
      try {
        if (!(response.bodyBytes[0] == 79 && response.bodyBytes[1] == 75)) {
          throw Exception("server said not OK to geometry");
        }
      } catch (e) {
        gl.print("Error with answer after sending geometry to server");
        gl.print("$e");
        return false;
      }
    } else {
      gl.print("Could not send Poly to Server, no internet!");
      return false;
    }
    gl.print("Success sending Geometry $name");
    sentToServer = true;
    serialize();
    return true;
  }

  String getProperTypeForCarto(String s) {
    if (s.contains("Point")) return "Point";
    return s;
  }

  int getNCheckedAttributes() {
    int nChecked = 0;
    for (Attribute a in attributes) {
      if (a.visibleOnMapLabel) nChecked++;
    }
    return nChecked;
  }

  bool containsAttribute(String name) {
    for (Attribute a in attributes) {
      if (a.name == name) return true;
    }
    return false;
  }

  void serialize({String layerId = ""}) async {
    String prefix = identifier;
    await gl.shared!.setBool('$prefix.sent', sentToServer);
    await gl.shared!.setString('$prefix.name', name);
    await gl.shared!.setString('$prefix.type', type);
    await gl.shared!.setBool('$prefix.visibleOnMap', visibleOnMap);
    await gl.shared!.setBool('$prefix.labelsVisibleOnMap', labelsVisibleOnMap);
    await gl.shared!.setInt('$prefix.selectedPointIcon', selectedPointIcon);
    await gl.shared!.setDouble('$prefix.iconSize', iconSize);
    await gl.shared!.setDouble('$prefix.area', area);
    await gl.shared!.setDouble('$prefix.perimeter', perimeter);
    await gl.shared!.setDouble('$prefix.transparencyInside', transparencyInside);
    await gl.shared!.setDouble('$prefix.transparencyLine', transparencyLine);
    _writeColorToMemory('$prefix.colorInside', colorInside);
    _writeColorToMemory('$prefix.colorLine', colorLine);
    _writePolygonPointsToMemory('$prefix.poly', points);
    _writePropertiesToMemory('$prefix.prop', attributes);

    List<String> polykeys = gl.shared!.getStringList('$layerId.polyKeys') ?? <String>[];
    if (!polykeys.contains(prefix)) {
      polykeys.add(prefix);
      await gl.shared!.setStringList('$layerId.polyKeys', polykeys);
    }

    gl.print("polygone $name saved to prefs");
  }

  void _writeColorToMemory(String prefix, Color color) async {
    await gl.shared!.setInt('$prefix.r', (color.r * 255).round());
    await gl.shared!.setInt('$prefix.g', (color.g * 255).round());
    await gl.shared!.setInt('$prefix.b', (color.b * 255).round());
    await gl.shared!.setDouble('$prefix.a', color.a);
  }

  void _writePolygonPointsToMemory(String prefix, List<LatLng> polygon) async {
    int i = 0;
    for (var point in polygon) {
      await gl.shared!.setDouble('$prefix.$i-lat', point.latitude);
      await gl.shared!.setDouble('$prefix.$i-lng', point.longitude);
      i++;
    }
    await gl.shared!.setInt('$prefix.nPolyPoints', i);
  }

  void _writePropertiesToMemory(String name, List<Attribute> attributes) async {
    int i = 0;
    for (var attribute in attributes) {
      await gl.shared!.setString('$name.$i.name', attribute.name);
      await gl.shared!.setString('$name.$i.type', attribute.type);
      await gl.shared!.setBool('$name.$i.visibleOnMapLabel', attribute.visibleOnMapLabel);
      if (attribute.type == "string") {
        await gl.shared!.setString('$name.$i.val', attribute.value);
      } else if (attribute.type == "int") {
        await gl.shared!.setInt('$name.$i.val', attribute.value);
      } else if (attribute.type == "double") {
        await gl.shared!.setDouble('$name.$i.val', attribute.value);
      }
      i++;
    }
    await gl.shared!.setInt('$name.nAttributes', i);
  }

  void deserialze(String id) async {
    name = gl.shared!.getString('$id.name')!;
    identifier = id;
    type = gl.shared!.getString('$id.type') ?? "Polygon";
    sentToServer = gl.shared!.getBool('$id.sent') ?? false;
    visibleOnMap = gl.shared!.getBool('$id.visibleOnMap') ?? true;
    labelsVisibleOnMap = gl.shared!.getBool('$id.labelsVisibleOnMap') ?? true;
    selectedPointIcon = gl.shared!.getInt('$id.selectedPointIcon') ?? 0;
    iconSize = gl.shared!.getDouble('$id.iconSize') ?? 10.0;
    area = gl.shared!.getDouble('$id.area')!;
    perimeter = gl.shared!.getDouble('$id.perimeter')!;
    transparencyInside = gl.shared!.getDouble('$id.transparencyInside')!;
    transparencyLine = gl.shared!.getDouble('$id.transparencyLine')!;
    colorInside = _getColorFromMemory('$id.colorInside');
    colorLine = _getColorFromMemory('$id.colorLine');
    points = _getPolygonFromMemory('$id.poly');
    attributes = _getAttributesFromMemory('$id.prop');
  }

  Color _getColorFromMemory(String name) {
    return Color.fromRGBO(
      gl.shared!.getInt('$name.r')!,
      gl.shared!.getInt('$name.g')!,
      gl.shared!.getInt('$name.b')!,
      gl.shared!.getDouble('$name.a')!,
    );
  }

  List<LatLng> _getPolygonFromMemory(String name) {
    List<LatLng> polygon = [];
    int nPoints = gl.shared!.getInt('$name.nPolyPoints')!;
    for (int i = 0; i < nPoints; i++) {
      polygon.add(LatLng(gl.shared!.getDouble('$name.$i-lat')!, gl.shared!.getDouble('$name.$i-lng')!));
    }
    return polygon;
  }

  List<Attribute> _getAttributesFromMemory(String name) {
    List<Attribute> attributes = [];
    int nAttributes = gl.shared!.getInt('$name.nAttributes') ?? 0;
    for (int i = 0; i < nAttributes; i++) {
      String type = gl.shared!.getString('$name.$i.type')!;
      attributes.add(
        Attribute(
          name: gl.shared!.getString('$name.$i.name')!,
          type: type,
          value:
              type == "string"
                  ? gl.shared!.getString('$name.$i.val')!
                  : type == "int"
                  ? gl.shared!.getInt('$name.$i.val')!
                  : type == "double"
                  ? gl.shared!.getDouble('$name.$i.val')!
                  : "unknown",
          visibleOnMapLabel: gl.shared!.getBool('$name.$i.visibleOnMapLabel') ?? false,
        ),
      );
    }
    return attributes;
  }

  static void deserializAllPolys({String layerId = ""}) {
    List<String> polykeys = gl.shared!.getStringList('$layerId.polyKeys') ?? <String>[];
    for (String key in polykeys) {
      gl.selLay.geometries.add(Geometry());
      gl.selLay.geometries.last.deserialze(key);
    }
  }

  static void removePolyFromShared(String id, {String layerId = ""}) async {
    List<String> polykeys = gl.shared!.getStringList('$layerId.polyKeys') ?? <String>[];
    if (!polykeys.contains(id)) {
      polykeys.remove(id);
      await gl.shared!.setStringList('$layerId.polyKeys', polykeys);
    }
  }

  static int countEssenceObservations() {
    int count = 0;
    for (Geometry form in gl.selLay.geometries) {
      if (form.type.contains("essence")) count++;
    }
    return count;
  }

  Future runAnaPt() async {
    proj4.Point ptBL72 = epsg4326.transform(epsg31370, proj4.Point(x: points.first.longitude, y: points.first.latitude));
    gl.requestedLayers.clear();
    Map data;

    gl.pt = ptBL72;

    bool internet = await InternetConnection().hasInternetAccess;
    if (!gl.offlineMode) {
      if (internet) {
        String layersAnaPt = "";
        for (String lCode in gl.anaPtSelectedLayerKeys) {
          if (gl.dico.getLayerBase(lCode).mCategorie != "Externe") {
            layersAnaPt += "+$lCode";
          }
        }

        String request = "https://forestimator.gembloux.ulg.ac.be/api/anaPt/layers/$layersAnaPt/x/${ptBL72.x}/y/${ptBL72.y}";
        try {
          var res = await http.get(Uri.parse(request));
          if (res.statusCode != 200) throw HttpException('${res.statusCode}');
          data = jsonDecode(res.body);

          // si pas de connexion internet, va tenter de lire data comme une map alors que c'est vide, erreur. donc dans le bloc try catch aussi
          for (var r in data["RequestedLayers"]) {
            gl.requestedLayers.add(LayerAnaPt.fromMap(r));
          }
        } catch (e) {
          gl.print(request);
          gl.print("$e");
        }
        gl.requestedLayers.removeWhere((element) => element.mFoundLayer == false);
      } else {
        gl.mainStack.add(popupNoInternet());
      }
    } else {
      if (gl.dico.getLayersOffline().isEmpty) {
        return;
      }
      for (LayerBase l in gl.dico.getLayersOffline()) {
        int val = await l.getValXY(ptBL72);
        gl.requestedLayers.add(LayerAnaPt(mCode: l.mCode, mRastValue: val));
      }
    }

    // un peu radical mais me fait bugger mon affichage par la suite donc je retire
    gl.requestedLayers.removeWhere((element) => element.mRastValue == 0);

    // on les trie sur base des catégories de couches
    gl.requestedLayers.sort((a, b) => gl.dico.getLayerBase(a.mCode).mGroupe.compareTo(gl.dico.getLayerBase(b.mCode).mGroupe));
  }
}
