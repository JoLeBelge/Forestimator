import 'package:area_polygon/area_polygon.dart';
import 'package:flutter/material.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:latlong2/latlong.dart';

class PolygonLayer {
  UniqueKey identifier = UniqueKey();
  String name = "";
  List<LatLng> polygonPoints = [];
  Color colorInside = Color.fromRGBO(255, 255, 255, 255);
  double transparencyInside = 0.0;
  Color colorLine = Color.fromRGBO(255, 255, 255, 255);
  double transparencyLine = 0.0;
  int selectedVertex = -1;
  double area = 0.0;
  late proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
  proj4.Projection epsg31370 =
      proj4.Projection.get('EPSG:31370') ??
      proj4.Projection.add(
        'EPSG:31370',
        '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs',
      );

  PolygonLayer({required String polygonName}) {
    name = polygonName;
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

  void addPoint(LatLng point) {
    polygonPoints.add(point);
    _computeArea();
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
    _computeArea();
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

  Offset _sphereToCart(proj4.Point spPoint) {
    return Offset(
      epsg4326.transform(epsg31370, spPoint).x,
      epsg4326.transform(epsg31370, spPoint).y,
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
        _sphereToCart(proj4.Point(x: point.latitude, y: point.longitude)),
      );
    }
    area = calculateArea(poly);
  }
}
