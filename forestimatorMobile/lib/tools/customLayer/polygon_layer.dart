import 'package:flutter/material.dart';
import 'package:image/image.dart';
import 'package:latlong2/latlong.dart';

class PolygonLayer {
  UniqueKey identifier = UniqueKey();
  String name = "";
  List<LatLng> polygonPoints = [];
  Color colorInside = ColorRgba8(255, 255, 255, 255);
  double transparencyInside = 0.0;
  Color colorLine = ColorRgba8(255, 255, 255, 255);
  double transparencyLine = 0.0;
  int selectedVertex = -1;

  PolygonLayer({required String name});

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
    colorInside = value;
  }

  void addPoint(LatLng point) {
    polygonPoints.add(point);
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
    polygonPoints.removeAt(i);
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
  }

  Color get colorSurface => colorInside;
  Color get colorPolygon => colorLine;
  UniqueKey get id => identifier;
  List<LatLng> get vertexes => polygonPoints;
  int get numPoints => polygonPoints.length;
}
