import 'dart:math';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';

// Layer to record the path you made on the map with gps.
class PathLayer {
  String identifier = UniqueKey().toString();
  String name = "";
  List<LatLng> pathPoints = [];
  Color colorInside = Color.fromRGBO(255, 128, 164, 80);
  double transparencyInside = 0.0;
  Color colorLine = Color.fromRGBO(255, 128, 164, 80);
  double transparencyLine = 0.0;
  int selectedVertex = -1;
  double length = 0.0;
  bool finished = false;

  PathLayer({required String pathName}) {
    name = pathName;
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

  void addPosition(LatLng point) {
    pathPoints.add(point);
    _computeLength();
    serialize();
  }

  Color get colorSurface => colorInside;
  Color get colorPolygon => colorLine;
  String get id => identifier;
  List<LatLng> get vertexes => pathPoints;
  int get numPoints => pathPoints.length;
  LatLng get center {
    double x = 0.0, y = 0.0;
    int i = 0;
    for (var point in pathPoints) {
      y += point.longitude;
      x += point.latitude;
      i++;
    }
    return LatLng(x / (i > 0 ? i : 1), y / (i > 0 ? i : 1));
  }

  int pointIndex(LatLng searchedPoint) {
    int i = 0;
    for (var point in pathPoints) {
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

  void _computeLength() {
    List<Offset> poly = [];
    for (LatLng point in pathPoints) {
      poly.add(
        _epsg4326ToEpsg31370(
          proj4.Point(y: point.latitude, x: point.longitude),
        ),
      );
    }
    length = 0.0;
    Offset currentPoint = poly.first;
    for (Offset point in poly.sublist(1)) {
      length = length + (currentPoint - point).distance;
      currentPoint = point;
    }
  }

  void serialize() {}
  void deserialze(String id) {}
  static void delete(String id) {}
}
