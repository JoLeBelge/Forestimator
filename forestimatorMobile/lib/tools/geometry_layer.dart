import 'dart:math';

import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/geometry/geometry.dart';
import 'package:flutter/material.dart';
import 'package:uuid/uuid.dart';

class GeometricLayer {
  String id = Uuid().v4();
  String type = "";
  String subtype = "";
  String name = "";

  bool visibleOnMap = true;
  bool labelsVisibleOnMap = true;
  int selectedGeometry = -1;

  Color defaultColor = Colors.black;
  int defaultPointIcon = 0;
  double defaultIconSize = 10;
  List<Attribute> defaultAttributes = [];

  List<Geometry> geometries = [];

  GeometricLayer({String name = ""}) {
    name = "";
    type = "";
    subtype = "";
    Random rand = Random();
    defaultColor = Color.fromRGBO(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256), 1.0);
  }

  GeometricLayer.point() {
    type = "Point";
    subtype = "";
    Random rand = Random();
    defaultColor = Color.fromRGBO(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256), 1.0);
  }

  GeometricLayer.essence() {
    type = "Point";
    subtype = "Essence";
    Random rand = Random();
    defaultColor = Color.fromRGBO(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256), 1.0);
    defaultAttributes.addAll([
      Attribute(name: "essence", type: "string", value: gl.essenceChoice[0]),
      Attribute(name: "rmq", type: "string", value: ""),
    ]);
  }

  GeometricLayer.polygon() {
    type = "Polygon";
    Random rand = Random();
    defaultColor = Color.fromRGBO(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256), 1.0);
  }

  void addGeometry({String name = ""}) {
    switch (type) {
      case 'Point':
        subtype == 'Essence'
            ? geometries.add(Geometry.essencePoint(polygonName: name))
            : geometries.add(Geometry.point(polygonName: name));
        break;
      case 'Polygon':
        geometries.add(Geometry.polygon(polygonName: name));
        break;
      default:
        gl.print("error: unknown type $type to create new geometry on layer $name");
        return;
    }
    geometries.last.setColorInside(defaultColor);
    geometries.last.setColorLine(defaultColor);
    geometries.last.selectedPointIcon = defaultPointIcon;
    geometries.last.iconSize = defaultIconSize;
    geometries.last.attributes.addAll(defaultAttributes);
    geometries.last.serialize(layerId: id);
  }

  void removeGeometry({int? index, String? name, bool? last}) {
    if (geometries.isEmpty) {
      gl.print("error: nothing to remove in geometry list:");
      return;
    } else if (index != null) {
      Geometry.removePolyFromShared(geometries.removeAt(index).id, layerId: id);
    } else if (name != null) {
      String identifier = "";
      geometries.removeWhere((Geometry g) {
        identifier = g.id;
        return g.name == name;
      });
      Geometry.removePolyFromShared(identifier, layerId: id);
    } else if (last != null && last) {
      Geometry.removePolyFromShared(geometries.removeLast().id, layerId: id);
    }
  }

  void visible(bool visibility) {
    for (Geometry g in geometries) {
      g.visibleOnMap = visibility;
    }
  }

  void labelsVisible(bool visibility) {
    for (Geometry g in geometries) {
      g.labelsVisibleOnMap = visibility;
    }
  }

  void serialize() async {
    await gl.shared!.setString('$id.name', name);
    await gl.shared!.setString('$id.type', type);
    await gl.shared!.setString('$id.subtype', subtype);

    await gl.shared!.setBool('$id.visibleOnMap', visibleOnMap);
    await gl.shared!.setBool('$id.labelsVisibleOnMap', labelsVisibleOnMap);
    await gl.shared!.setInt('$id.defaultPointIcon', defaultPointIcon);
    await gl.shared!.setDouble('$id.defaultIconSize', defaultIconSize);

    _writeColorToMemory("$id.col", defaultColor);
    _writeAttributesToMemory("$id.defAttr", defaultAttributes);

    for (Geometry g in geometries) {
      g.serialize();
    }

    List<String> layerKeys = gl.shared!.getStringList('layerKeys') ?? <String>[];
    if (!layerKeys.contains(id)) {
      layerKeys.add(id);
      await gl.shared!.setStringList('layerKeys', layerKeys);
    }

    gl.print("layer $name saved to prefs");
  }

  void deserialize(String id) async {
    name = gl.shared!.getString('$id.name')!;
    type = gl.shared!.getString('$id.type')!;
    subtype = gl.shared!.getString('$id.subtype')!;

    visibleOnMap = gl.shared!.getBool('$id.labelsVisibleOnMap')!;
    labelsVisibleOnMap = gl.shared!.getBool('$id.labelsVisibleOnMap')!;

    defaultPointIcon = gl.shared!.getInt('$id.defaultPointIcon')!;
    defaultIconSize = gl.shared!.getDouble('$id.defaultIconSize')!;

    defaultColor = _getColorFromMemory("$id.col");
    defaultAttributes = _getAttributesFromMemory("$id.defAttr");

    _deserializAllPolys();

    gl.print("layer $name loaded from prefs");
  }

  void _writeAttributesToMemory(String prefix, List<Attribute> attributes) async {
    int i = 0;
    for (var attribute in attributes) {
      await gl.shared!.setString('$prefix.$i.name', attribute.name);
      await gl.shared!.setString('$prefix.$i.type', attribute.type);
      await gl.shared!.setBool('$prefix.$i.visibleOnMapLabel', attribute.visibleOnMapLabel);
      if (attribute.type == "string") {
        await gl.shared!.setString('$prefix.$i.val', attribute.value);
      } else if (attribute.type == "int") {
        await gl.shared!.setInt('$prefix.$i.val', attribute.value);
      } else if (attribute.type == "double") {
        await gl.shared!.setDouble('$prefix.$i.val', attribute.value);
      }
      i++;
    }
    await gl.shared!.setInt('$prefix.nAttributes', i);
  }

  List<Attribute> _getAttributesFromMemory(String prefix) {
    List<Attribute> attributes = [];
    int nAttributes = gl.shared!.getInt('$prefix.nAttributes') ?? 0;
    for (int i = 0; i < nAttributes; i++) {
      String type = gl.shared!.getString('$prefix.$i.type')!;
      attributes.add(
        Attribute(
          name: gl.shared!.getString('$prefix.$i.name')!,
          type: type,
          value:
              type == "string"
                  ? gl.shared!.getString('$prefix.$i.val')!
                  : type == "int"
                  ? gl.shared!.getInt('$prefix.$i.val')!
                  : type == "double"
                  ? gl.shared!.getDouble('$prefix.$i.val')!
                  : "unknown",
          visibleOnMapLabel: gl.shared!.getBool('$prefix.$i.visibleOnMapLabel') ?? false,
        ),
      );
    }
    return attributes;
  }

  void _writeColorToMemory(String prefix, Color color) async {
    await gl.shared!.setInt('$prefix.r', (color.r * 255).round());
    await gl.shared!.setInt('$prefix.g', (color.g * 255).round());
    await gl.shared!.setInt('$prefix.b', (color.b * 255).round());
    await gl.shared!.setDouble('$prefix.a', color.a);
  }

  Color _getColorFromMemory(String prefix) {
    return Color.fromRGBO(
      gl.shared!.getInt('$prefix.r')!,
      gl.shared!.getInt('$prefix.g')!,
      gl.shared!.getInt('$prefix.b')!,
      gl.shared!.getDouble('$prefix.a')!,
    );
  }

  void _deserializAllPolys() {
    List<String> polykeys = gl.shared!.getStringList('$id.polyKeys') ?? <String>[];
    for (String key in polykeys) {
      geometries.add(Geometry());
      geometries.last.deserialze(key);
    }
  }

  static void deserializeLayers() {
    List<String> layerKeys = gl.shared!.getStringList('layerKeys') ?? <String>[];
    for (String key in layerKeys) {
      gl.geoLayers.add(GeometricLayer());
      gl.geoLayers.last.deserialize(key);
    }
  }

  static void removeLayerFromShared(String it) async {
    List<String> layerKeys = gl.shared!.getStringList('layerKeys') ?? <String>[];
    if (!layerKeys.contains(it)) {
      List<String> polykeys = gl.shared!.getStringList('$it.polyKeys') ?? <String>[];
      for (String key in polykeys) {
        Geometry.removePolyFromShared(key);
      }
      layerKeys.remove(it);
      await gl.shared!.setStringList('layerKeys', layerKeys);
    }
  }

  bool allSent() {
    if (geometries.isEmpty) return false;
    for (Geometry g in geometries) {
      if (!g.sentToServer) return false;
    }
    return true;
  }

  bool containsAttribute(String name) {
    for (Attribute a in defaultAttributes) {
      if (a.name == name) return true;
    }
    return false;
  }

  void sendLayerPolys() async {
    //TODO: sendLayer
  }
}
