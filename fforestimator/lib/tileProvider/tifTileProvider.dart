import 'package:flutter/foundation.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'dart:math';
import 'package:flutter/services.dart';
import 'package:path/path.dart';
import 'dart:io';

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
    final File fileIm = File(sourceImPath);
    bool e = await fileIm.exists();

    print("file exist" + e.toString());
    Uint8List bytes = await fileIm.readAsBytes();
    //ByteData data = await rootBundle.load(url.join("assets", "BV_FEE_colorP.tif"));
    //_sourceImage = img.TiffDecoder().decode(data.buffer.asUint8List())!;
    _sourceImage = img.TiffDecoder().decode(bytes)!;
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

    // je devrais lire  GeoInfo geoi = GeoInfo(im); comme dans onePixGeoTifDecoder pour adapter le code à des raster qui ont une autre résolution.
    int zFullIm =
        7; // raster avec résolution de 10m/pixel. fonctionne tant que la map est paramétrée avec scr getResolutions2()

    /* if (resolution == 20.0) { 
      zFullIm = 6;
    }*/
    int initImSize = (pow(2, (zFullIm - coordinates.z)) * tileSize).toInt();

    img.Image cropped = img.copyCrop(_sourceImage!,
        x: xOffset, y: yOffset, width: initImSize, height: initImSize);
    img.Image resized = img.copyResize(cropped, width: tileSize);
    return MemoryImage(img.encodePng(resized));
  }
}
