import 'package:flutter/foundation.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'dart:math';
import 'package:flutter/services.dart';
import 'dart:io';
import 'dart:convert';

class tifFileTileProvider extends TileProvider {
  final Proj4Crs mycrs;
  int tileSize = 256;
  img.Image? _sourceImage;
  String sourceImPath;
  bool _loaded = false;
  String layerCode;

  bool get loaded => _loaded;

  Function refreshView;

  tifFileTileProvider(
      {super.headers,
      required this.mycrs,
      required this.sourceImPath,
      required this.layerCode,
      required this.refreshView});

  void init() async {
    //print("init tifFileTileProvider by loading source image in memory");
    final File fileIm = File(sourceImPath);
    bool e = await fileIm.exists();
    //print("file exist " + e.toString());
    if (e) {
      Uint8List? bytes = await fileIm.readAsBytes();

      img.TiffInfo _tiffInfo = img.TiffDecoder().startDecode(bytes!)!;
      img.TiffImage tifIm = _tiffInfo!.images[0];
      int bps = tifIm.bitsPerSample;
      // le décodage d'un tif 16 bits avec ColorMap sera effectif pour la prochaine sortie du package image (flutter)
      // testé avec image 4.2, imageDecoder (android graphic) ; Input was incomplete-> il faut probablement encore convertir en 8bit apres lecture de la 16 bits avec colormap.
      if (bps <= 8) {
        _sourceImage = img.TiffDecoder().decode(bytes!);
        refreshView(() {
          _loaded = true;
        });
      }
    }
  }

  @override
  ImageProvider getImage(TileCoordinates coordinates, TileLayer options) {
    int tileSize = 256;
    final tileSizePoint = Point(tileSize, tileSize);

    // final nwPoint = coordinates.scaleBy(tileSizePoint) - daptation for flutter_map 7.0.0 (scaleBy depreciated)

    final Point nwPoint =
        Point(coordinates.x * tileSize, coordinates.y * tileSize);

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

    int initImSize = (pow(2, (zFullIm - coordinates.z)) * tileSize).toInt();
    if (_sourceImage != null) {
      img.Image cropped = img.copyCrop(_sourceImage!,
          x: xOffset, y: yOffset, width: initImSize, height: initImSize);
      img.Image resized = img.copyResize(cropped, width: tileSize);
      return MemoryImage(img.encodePng(resized, singleFrame: true));
    } else {
      Uint8List blankBytes = Base64Codec()
          .decode("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7");
      return MemoryImage(
        blankBytes,
      );
    }
  }

  /*@override
  void dispose() {
    //bytes!.clear();
    _sourceImage = img.Image.empty();

    super.dispose(); // This will free the memory space allocated to the page
  }*/
}
