import 'dart:isolate';

import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'dart:math';
import 'package:flutter/services.dart';
import 'dart:io';
import 'dart:convert';

class TifFileTileProvider extends TileProvider {
  final Proj4Crs mycrs;
  int tileSize = 256;
  img.Image? _sourceImage;
  String sourceImPath;
  bool _loaded = false;
  String layerCode;

  bool get loaded => _loaded;

  Function refreshView;

  TifFileTileProvider({
    super.headers,
    required this.mycrs,
    required this.sourceImPath,
    required this.layerCode,
    required this.refreshView,
  });

  void init() async {
    _loadAndDecodeImage().whenComplete(() => gl.refreshMap(() {}));
  }

  Future _loadAndDecodeImage() async {
    gl.print("init TifFileTileProvider by loading source image in memory");
    final File fileIm = File(sourceImPath);
    bool e = await Isolate.run<bool>(fileIm.exists);
    if (e) {
      Uint8List? bytes = await fileIm.readAsBytes();

      img.TiffInfo tiffInfo = img.TiffDecoder().startDecode(bytes)!;
      img.TiffImage tifIm = tiffInfo.images[0];
      int bps = tifIm.bitsPerSample;
      gl.print("file with $bps loaded in memory $e");
      // le décodage d'un tif 16 bits avec ColorMap sera effectif pour la prochaine sortie du package image (flutter)
      // testé avec image 4.2, imageDecoder (android graphic) ; Input was incomplete-> il faut probablement encore convertir en 8bit apres lecture de la 16 bits avec colormap.
      if (bps <= 8) {
        _sourceImage = await Isolate.run<img.Image?>(() {
          return img.TiffDecoder().decode(bytes);
        });
        _loaded = true;
        gl.print("file decoded in memory $e");
      }
    }
  }

  @override
  ImageProvider getImage(TileCoordinates coordinates, TileLayer options) {
    int tileSize = 256;
    // final tileSizePoint = Point(tileSize, tileSize);

    // final nwPoint = coordinates.scaleBy(tileSizePoint) - daptation for flutter_map 7.0.0 (scaleBy depreciated)

    final Offset nwPoint = Offset(
      coordinates.x.toDouble() * tileSize,
      coordinates.y.toDouble() * tileSize,
    );

    final nwCoords = mycrs.offsetToLatLng(nwPoint, coordinates.z.toDouble());
    final nw = mycrs.projection.project(nwCoords);
    Rect b = mycrs.projection.bounds!;

    double resolution = 10.0;
    int xOffset = ((nw.dx - b.topLeft.dx) / resolution).round();
    int yOffset =
        ((b.bottomRight.dy - nw.dy) / resolution)
            .round(); //--> soit-disant bottomRigth mais contient le ymax (top dans src donc)

    // je devrais lire  GeoInfo geoi = GeoInfo(im); comme dans onePixGeoTifDecoder pour adapter le code à des raster qui ont une autre résolution.
    int zFullIm =
        7; // raster avec résolution de 10m/pixel. fonctionne tant que la map est paramétrée avec scr getResolutions2()

    int initImSize = (pow(2, (zFullIm - coordinates.z)) * tileSize).round();

    if (_sourceImage != null) {
      img.Image cropped = img.copyCrop(
        _sourceImage!,
        x: xOffset,
        y: yOffset,
        width: initImSize,
        height: initImSize,
      );
      img.Image resized = img.copyResize(
        cropped,
        width: tileSize,
        interpolation: img.Interpolation.linear,
      );
      return MemoryImage(img.encodePng(resized, singleFrame: true));
    } else {
      Uint8List blankBytes = Base64Codec().decode(
        "R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7",
      );
      return MemoryImage(blankBytes);
    }
  }
}
