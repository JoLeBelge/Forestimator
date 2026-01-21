import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:area_polygon/area_polygon.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';

class Attribute {
  String type;
  String name;
  dynamic value;

  Attribute({required this.name, required this.type, required this.value});
}

class PolygonLayer {
  UniqueKey identifier = UniqueKey();
  bool sentToServer = false;
  String type = "";
  String name = "";
  bool visibleOnMap = true;
  List<LatLng> polygonPoints = [];
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

  PolygonLayer({required String polygonName}) {
    name = polygonName;
    Random randomColor = Random();
    setColorInside(
      Color.fromRGBO(
        randomColor.nextInt(256),
        randomColor.nextInt(256),
        randomColor.nextInt(256),
        0.4,
      ),
    );
    setColorLine(
      Color.fromRGBO(
        (colorInside.r * 255).round(),
        (colorInside.g * 255).round(),
        (colorInside.b * 255).round(),
        1.0,
      ),
    );
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
    if (polygonPoints.length == 1) {
      selectedPolyLinePoints[1] = index;
    } else if (polygonPoints.length - 1 == index) {
      selectedPolyLinePoints[1] = 0;
    } else if (polygonPoints.length - 1 > index) {
      selectedPolyLinePoints[1] = index + 1;
    } else {
      gl.print("error 1st index polygon line: $index");
    }
    gl.print(selectedPolyLinePoints);
  }

  bool isSelectedLine(int i) {
    return (i == selectedPolyLinePoints[0] || i == selectedPolyLinePoints[1])
        ? true
        : false;
  }

  void addPoint(LatLng point) {
    polygonPoints.insert(selectedPolyLinePoints[1], point);
    refreshSelectedLinePoints(point);
    _computeArea();
    _computePerimeter();
    gl.saveChangesToPolygoneToPrefs = true;
  }

  List<Point> getPolyPlusOneVertex(LatLng vertex) {
    List<Point> copyOfPoly = [];

    for (LatLng p in polygonPoints) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }

    copyOfPoly.insert(
      selectedPolyLinePoints[1],
      Point(vertex.latitude, vertex.longitude),
    );
    return copyOfPoly;
  }

  List<Point> getPolyMoveOneVertex(LatLng old, LatLng index) {
    List<Point> copyOfPoly = [];

    for (LatLng p in polygonPoints) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }
    int i = 0;
    for (LatLng point in polygonPoints) {
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

    for (LatLng p in polygonPoints) {
      copyOfPoly.add(Point(p.latitude, p.longitude));
    }
    int i = 0;
    for (var point in polygonPoints) {
      if (index.latitude == point.latitude &&
          index.longitude == point.longitude) {
        break;
      }
      i++;
    }
    copyOfPoly.removeAt(i);

    return copyOfPoly;
  }

  void removePoint(LatLng index) {
    int i = 0;
    for (var point in polygonPoints) {
      if (index.latitude == point.latitude &&
          index.longitude == point.longitude) {
        break;
      }
      i++;
    }
    int pointIsSelected = -1;
    if (i == selectedPolyLinePoints[0] && polygonPoints.length > 2) {
      pointIsSelected = selectedPolyLinePoints[1];
    } else if (i == selectedPolyLinePoints[1] && polygonPoints.length > 2) {
      pointIsSelected = selectedPolyLinePoints[0];
    }
    polygonPoints.removeAt(i);
    if (pointIsSelected > -1 && pointIsSelected < polygonPoints.length) {
      refreshSelectedLinePoints(polygonPoints[pointIsSelected]);
    } else {
      if (polygonPoints.length == 1) {
        refreshSelectedLinePoints(polygonPoints[0]);
      } else if (polygonPoints.isEmpty) {
        refreshSelectedLinePoints(null);
      } else {
        int selection =
            (selectedPolyLinePoints[0] > selectedPolyLinePoints[1])
                ? selectedPolyLinePoints[1]
                : selectedPolyLinePoints[0];
        refreshSelectedLinePoints(
          polygonPoints[selection > polygonPoints.length - 1
              ? polygonPoints.length - 1
              : selection],
        );
      }
    }

    _computeArea();
    _computePerimeter();
    gl.saveChangesToPolygoneToPrefs = true;
  }

  void replacePoint(LatLng old, LatLng index) {
    int i = 0;
    for (var point in polygonPoints) {
      if (old.latitude == point.latitude && old.longitude == point.longitude) {
        break;
      }
      i++;
    }
    polygonPoints.removeAt(i);
    polygonPoints.insert(i, index);
    _computeArea();
    _computePerimeter();
    gl.saveChangesToPolygoneToPrefs = true;
  }

  Color get colorSurface => colorInside;
  Color get colorPolygon => colorLine;
  UniqueKey get id => identifier;
  List<LatLng> get vertexes => polygonPoints;
  int get numPoints => polygonPoints.length;
  LatLng get center {
    double x = 0.0, y = 0.0;
    int i = 0;
    for (var point in polygonPoints) {
      y += point.longitude;
      x += point.latitude;
      i++;
    }
    return LatLng(x / (i > 0 ? i : 1), y / (i > 0 ? i : 1));
  }

  int pointIndex(LatLng searchedPoint) {
    int i = 0;
    for (var point in polygonPoints) {
      if (searchedPoint.latitude == point.latitude &&
          searchedPoint.longitude == point.longitude) {
        break;
      }
      i++;
    }
    return i;
  }

  Offset _epsg4326ToEpsg31370(proj4.Point spPoint) {
    return Offset(
      gl.epsg4326.transform(gl.epsg31370, spPoint).x,
      gl.epsg4326.transform(gl.epsg31370, spPoint).y,
    );
  }

  void _computeArea() {
    if (polygonPoints.length < 2) {
      area = 0.0;
      return;
    }

    List<Offset> poly = [];
    for (LatLng point in polygonPoints) {
      poly.add(
        _epsg4326ToEpsg31370(
          proj4.Point(y: point.latitude, x: point.longitude),
        ),
      );
    }
    area = calculateArea(poly);
  }

  void _computePerimeter() {
    if (polygonPoints.length < 2) {
      area = 0.0;
      return;
    }
    List<Offset> poly = [];
    for (LatLng point in polygonPoints) {
      poly.add(
        _epsg4326ToEpsg31370(
          proj4.Point(y: point.latitude, x: point.longitude),
        ),
      );
    }
    perimeter = 0.0;
    Offset currentPoint = poly.last;
    for (Offset point in poly) {
      perimeter = perimeter + (currentPoint - point).distance;
      currentPoint = point;
    }
  }

  Future<bool> onlineSurfaceAnalysis() async {
    if (polygonPoints.length < 2) return false;
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
      for (LatLng point in polygonPoints) {
        proj4.Point tLb72 = gl.epsg4326.transform(
          gl.epsg31370,
          proj4.Point(x: point.longitude, y: point.latitude),
        );
        polygon = "$polygon${tLb72.x} ${tLb72.y},";
      }
      proj4.Point tLb72 = gl.epsg4326.transform(
        gl.epsg31370,
        proj4.Point(
          x: polygonPoints.first.longitude,
          y: polygonPoints.first.latitude,
        ),
      );
      polygon = "$polygon${tLb72.x} ${tLb72.y},";
      polygon = "${polygon.substring(0, polygon.length - 1)}))";

      String request =
          "https://forestimator.gembloux.ulg.ac.be/api/anaSurf/layers/$layersAnaSurf/polygon/$polygon";
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

  Future<bool> sendGeometryToServer() async {
    if (sentToServer) {
      gl.print("Geometry $name already sent once!");
      gl.mainStack.add(popupGeometryAlreadySent());
      return false;
    }
    bool internet = await InternetConnection().hasInternetAccess;
    if (internet) {
      String coordinates = "";
      for (LatLng point in polygonPoints) {
        proj4.Point tLb72 = gl.epsg4326.transform(
          gl.epsg31370,
          proj4.Point(x: point.longitude, y: point.latitude),
        );
        coordinates = "$coordinates[${tLb72.x}, ${tLb72.y}],";
      }
      if (type == "Polygon") {
        proj4.Point tLb72 = gl.epsg4326.transform(
          gl.epsg31370,
          proj4.Point(
            x: polygonPoints.first.longitude,
            y: polygonPoints.first.latitude,
          ),
        );
        coordinates = "[[$coordinates[${tLb72.x}, ${tLb72.y}]]]";
      } else {
        if (coordinates.length > 1) {
          coordinates = coordinates.substring(0, coordinates.length - 1);
        }
      }
      String properties = "";
      for (Attribute attribute in attributes) {
        properties =
            "$properties\"${attribute.name}\":\"${attribute.value.toString()}\",";
      }
      properties = properties.substring(0, properties.length - 1);
      String geometry =
          "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"$type\",\"coordinates\":$coordinates},\"properties\":{$properties}}]}";

      String request =
          "https://forestimator.gembloux.ulg.ac.be/api/polygFromMobile/$geometry";
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
      gl.mainStack.add(popupNoInternet());
      return false;
    }
    gl.print("Success sending Geometry $name");
    sentToServer = true;
    return true;
  }
}
