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

class PolygonLayer {
  UniqueKey identifier = UniqueKey();
  String name = "";
  List<LatLng> polygonPoints = [];
  List<int> selectedPolyLinePoints = [0, 0];
  Color colorInside = Color.fromRGBO(255, 128, 164, 80);
  double transparencyInside = 0.0;
  Color colorLine = Color.fromRGBO(255, 128, 164, 80);
  double transparencyLine = 0.0;
  int selectedVertex = -1;
  double area = 0.0;
  double perimeter = 0.0;
  late proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
  proj4.Projection epsg31370 =
      proj4.Projection.get('EPSG:31370') ??
      proj4.Projection.add(
        'EPSG:31370',
        '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs',
      );
  Map<String, dynamic> decodedJson = {};

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
        proj4.Point tLb72 = epsg4326.transform(
          epsg31370,
          proj4.Point(x: point.longitude, y: point.latitude),
        );
        polygon = "$polygon${tLb72.x} ${tLb72.y},";
      }
      proj4.Point tLb72 = epsg4326.transform(
        epsg31370,
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
          gl.print(correctedBody);
          gl.print(res.body);
          decodedJson = jsonDecode(correctedBody);
          gl.print("Error corrected");
        } catch (e) {
          gl.print("Error cannot correct!");
          gl.print("$e");
        }
      }
      gl.print(decodedJson);
    } else {
      gl.print("Could not make surface analysis, no internet!");
      showDialog(
        context: gl.notificationContext!,
        builder: (BuildContext context) {
          return PopupNoInternet();
        },
      );
      return false;
    }
    return true;
  }
}
