import 'package:flutter/foundation.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'dart:math';
import 'package:flutter/services.dart';
import 'package:path/path.dart';

class tifFileTileProvider extends TileProvider {
  final Proj4Crs mycrs;
  int tileSize = 256;
  img.Image? _sourceImage;
  String sourceImPath;
  bool _loaded = false;

bool get loaded => _loaded;

  tifFileTileProvider(
      {super.headers, required this.mycrs, required this.sourceImPath});

  Future init() async {
    print("init tifFileTileProvider by loading source image in memory");
    ByteData data =
        await rootBundle.load(url.join("assets", "BV_FEE_colorP.tif"));
    _sourceImage = img.TiffDecoder().decode(data.buffer.asUint8List())!;
    _loaded = true;
  }

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
    double resolution = 10.0;
    int xOffset = ((nw.x - b.topLeft.x) / resolution).round();
    int yOffset = ((b.bottomRight.y - nw.y) / resolution)
        .round(); //--> soit-disant bottomRigth mais contient le ymax (top dans src donc)

    int zFullIm = 7; // raster avec résolution de 10m/pixel
    if (resolution == 20.0) {
      zFullIm = 6;
    }
    int initImSize = (zFullIm + 1 - coordinates.z) *
        tileSize; // pour les zoom plus petit que 7
    if (coordinates.z > zFullIm) {
      initImSize =
          (tileSize.toDouble() / pow(2, (coordinates.z - zFullIm)).toDouble())
              .round();
    }
    img.Image cropped = img.copyCrop(_sourceImage!,
        x: xOffset, y: yOffset, width: initImSize, height: initImSize);
    img.Image resized = img.copyResize(cropped, width: tileSize);
    return MemoryImage(img.encodePng(resized));
  }
}