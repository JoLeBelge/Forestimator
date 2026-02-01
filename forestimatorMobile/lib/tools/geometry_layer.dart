import 'dart:math';
import 'dart:ui';

import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/geometry/geometry.dart';

class GeometryLayer {
  String? type;
  String? subtype;
  String name = "";

  bool visibleOnMap = true;
  bool labelsVisibleOnMap = true;
  int selectedGeometry = -1;

  Color? defaultColor;
  int defaultPointIcon = 0;
  double defaultIconSize = 10;
  List<Attribute> defaultAttributes = [];

  List<Geometry> geometries = [];

  GeometryLayer.point() {
    type = "Point";
    subtype = "";
    Random rand = Random();
    defaultColor = Color.fromRGBO(
      rand.nextInt(256),
      rand.nextInt(256),
      rand.nextInt(256),
      1.0,
    );
  }

  GeometryLayer.essence() {
    type = "Point";
    subtype = "Essence";
    Random rand = Random();
    defaultColor = Color.fromRGBO(
      rand.nextInt(256),
      rand.nextInt(256),
      rand.nextInt(256),
      1.0,
    );
    defaultAttributes.addAll([
      Attribute(name: "essence", type: "string", value: gl.essenceChoice[0]),
      Attribute(name: "rmq", type: "string", value: ""),
    ]);
  }

  GeometryLayer.polygon() {
    type = "Polygon";
    Random rand = Random();
    defaultColor = Color.fromRGBO(
      rand.nextInt(256),
      rand.nextInt(256),
      rand.nextInt(256),
      1.0,
    );
  }

  void addGeometryToLayer() {
    switch (type) {
      case 'Point':
        subtype == 'Essence'
            ? geometries.add(Geometry.essencePoint())
            : geometries.add(Geometry.point());
        break;
      case 'Polygon':
        subtype == 'Essence'
            ? geometries.add(Geometry.essencePoint())
            : geometries.add(Geometry.point());
        break;
      default:
        gl.print("unknown type $type to create new geometry on layer $name");
        return;
    }
    geometries.last.setColorInside(defaultColor!);
    geometries.last.setColorLine(defaultColor!);
    geometries.last.selectedPointIcon = defaultPointIcon;
    geometries.last.iconSize = defaultIconSize;
    geometries.last.attributes.addAll(defaultAttributes);
  }
}
