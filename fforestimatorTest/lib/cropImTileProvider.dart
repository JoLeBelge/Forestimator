import 'package:flutter/foundation.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter/material.dart';
import 'dart:io';
import 'package:test_fluttermap/globals.dart' as gl;
import 'package:image/image.dart';
import 'dart:math';

import 'package:image/src/formats/png_decoder.dart';
import 'package:image/src/formats/png/png_info.dart';

import 'package:archive/archive.dart';

import 'package:image/src/color/color_uint8.dart';
import 'package:image/src/color/format.dart';
import 'package:image/src/draw/blend_mode.dart';
import 'package:image/src/draw/composite_image.dart';
import 'package:image/src/image/icc_profile.dart';
import 'package:image/src/image/image.dart';
import 'package:image/src/image/palette_uint8.dart';
import 'package:image/src/image/pixel.dart';
import 'package:image/src/util/image_exception.dart';
import 'package:image/src/util/input_buffer.dart';
import 'package:image/src/formats/decode_info.dart';
import 'package:image/src/formats/decoder.dart';
import 'package:image/src/formats/png/png_frame.dart';
import 'package:image/src/formats/png//png_info.dart';

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
    double resolution = 20.0;
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

    img.Image cropped = img.copyCrop(gl.Fullimage,
        x: xOffset, y: yOffset, width: initImSize, height: initImSize);
    img.Image resized = img.copyResize(cropped, width: tileSize);
    return MemoryImage(img.encodePng(resized));
    //return FileImage(File('/home/jo/Images/chatGPT_crop.png'));
  }
}

/// Decode a PNG encoded image.
class myPngDecoder extends PngDecoder {
  //@override
  //final _info = InternalPngInfo();
  @override
  void _process(InputBuffer input, Image image) {
    final channels = (_info.colorType == PngColorType.grayscaleAlpha)
        ? 2
        : (_info.colorType == PngColorType.rgb)
            ? 3
            : (_info.colorType == PngColorType.rgba)
                ? 4
                : 1;

    final pixelDepth = channels * _info.bits;

    final w = _info.width;
    final h = _info.height;

    final rowBytes = (w * pixelDepth + 7) >> 3;
    final bpp = (pixelDepth + 7) >> 3;

    final line = List<int>.filled(rowBytes, 0);
    final inData = [line, line];

    final pixel = [0, 0, 0, 0];

    final pIter = image.iterator..moveNext();
    for (var y = 0, ri = 0; y < h; ++y, ri = 1 - ri) {
      final filterType = PngFilterType.values[input.readByte()];
      inData[ri] = input.readBytes(rowBytes).toUint8List();

      final row = inData[ri];
      final prevRow = inData[1 - ri];

      // Before the image is compressed, it was filtered to improve compression.
      // Reverse the filter now.
      this._unfilter(filterType, bpp, row, prevRow);

      // Scanlines are always on byte boundaries, so for bit depths < 8,
      // reset the bit stream counter.
      _resetBits();

      final rowInput = InputBuffer(inData[ri], bigEndian: true);

      for (var x = 0; x < w; ++x) {
        _readPixel(rowInput, pixel);
        _setPixel(pIter.current, pixel);
        pIter.moveNext();
      }
    }
  }
}
