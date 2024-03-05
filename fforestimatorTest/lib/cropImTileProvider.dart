import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'dart:io';
import 'package:test_fluttermap/globals.dart' as gl;
import 'package:image/image.dart' as img;
import 'dart:math';

class cropImFileTileProvider extends TileProvider {
  final Proj4Crs mycrs;
  int tileSize = 256;
  cropImFileTileProvider({super.headers, required this.mycrs});

  @override
  ImageProvider getImage(TileCoordinates coordinates, TileLayer options) {
    int tileSize = 256;
    final tileSizePoint = Point(tileSize, tileSize);
    final nwPoint = coordinates.scaleBy(tileSizePoint);
    final nwCoords = mycrs.pointToLatLng(nwPoint, coordinates.z.toDouble());
    final nw = mycrs.projection.project(nwCoords);
    Bounds<double> b = mycrs.projection.bounds!;

    /*print("nord w " + nw.toString());
    print("se " + se.toString());

    print("btopleft " +
        b.topLeft
            .toString()); // --> est en fait le point sud ouest en BL72, donc xmin ymin
    print("b rigth " + b.bottomRight.toString());

    //print("x y offset : " + xOffset.toString() + " , " + yOffset.toString());
    */

    int xOffset = ((nw.x - b.topLeft.x) / 10.0).round();
    int yOffset = ((b.bottomRight.y - nw.y) / 10.0)
        .round(); //--> soit-disant bottomRigth mais contient le ymax (top dans src donc)

    int initImSize =
        (8 - coordinates.z) * tileSize; // pour les zoom plus petit que 7
    if (coordinates.z > 7) {
      initImSize =
          (tileSize.toDouble() / pow(2, (coordinates.z - 7)).toDouble())
              .round();
    }

    img.Image cropped = img.copyCrop(gl.Fullimage,
        x: xOffset, y: yOffset, width: initImSize, height: initImSize);
    img.Image resized = img.copyResize(cropped, width: tileSize);
    return MemoryImage(img.encodePng(resized));
  
  }
}
