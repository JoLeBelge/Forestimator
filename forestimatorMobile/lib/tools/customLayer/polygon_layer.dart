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

  void removePoint(int index) {
    polygonPoints.removeAt(index);
  }

  Color get colorSurface => colorInside;
  Color get colorPolygon => colorLine;
  UniqueKey get id => identifier;
  List<LatLng> get vertexes => polygonPoints;
  int get numPoints => polygonPoints.length;
}
