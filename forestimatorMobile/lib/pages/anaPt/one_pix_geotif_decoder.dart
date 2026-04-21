import 'dart:typed_data';
import 'package:archive/archive.dart'; //for ZLibdecoder. But no LzwDecoder in this package
import 'package:fforestimator/globals.dart' as gl;
import 'package:geoimage/geoimage.dart';
import 'package:image/image.dart';
import 'package:image/src/formats/tiff/tiff_lzw_decoder.dart';
import 'package:dart_jts/dart_jts.dart';

// pour debugger
import 'dart:io';

class OnePixGeotifDecoder extends Decoder {
  TiffInfo? info;
  ExifData? exif;
  late InputBuffer p;
  double x;
  double y;

  OnePixGeotifDecoder({required this.x, required this.y});

  int getVal(Uint8List bytes) {
    int aRes = 0;
    // lecture des headers
    startDecode(bytes);
    if (info == null) {
      return aRes;
    }

    TiffImage im = info!.images[0];
    GeoInfo geoi = GeoInfo(im);
    Coordinate uv = Coordinate(0, 0);
    geoi.worldToPixel(Coordinate(x, y), uv); // determiner la position du pixel qui nous intéresse

    // determiner l'offset et le byteCount de la tile qui contient la valeur du pixel que l'on souhaite - fonctionne QUE avec mes tif qui on un Tile par ligne
    // A modifier!!, maintenant j'utlise des rasters tuilés depuis début avril 2026

    int tilesPerRow = (im.width ~/ im.tileWidth) + 1;
    int nTilerow = (uv.y.toInt() ~/ im.tileHeight);
    int nTilecol = (uv.x.toInt() ~/ im.tileWidth);

    Coordinate tileCoord = Coordinate(
      (uv.x - (nTilecol) * im.tileWidth).floor().toDouble(),
      (uv.y - (nTilerow) * im.tileHeight).floor().toDouble(),
    ); // coord du pixel dans la tile
    int tilePixNum = tileCoord.y.toInt() * im.tileWidth + tileCoord.x.toInt(); // numéro du pixel dans la tile

    gl.print(
      "uv ${uv}tile row $nTilerow tile col $nTilecol tile coord $tileCoord tile pix num $tilePixNum compression ${im.compression}",
    );

    final tileIndex = nTilerow * tilesPerRow + nTilecol;

    //final tileIndex = uv.y.toInt() * im.tilesX;
    p.offset = im.tileOffsets![tileIndex];
    final byteCount = im.tileByteCounts![tileIndex];

    //final data = p.toList(0, byteCount);
    //List<int> outData = const ZLibDecoder().decodeBytes(data);
    //List<int> outData;

    //void _decodeTile(InputBuffer p, Image image, int tileX, int tileY) {
    // Read the data, uncompressing as needed. There are four cases:
    // bilevel, palette-RGB, 4-bit grayscale, and everything else.

    var bytesInThisTile = im.tileWidth * im.tileHeight * im.samplesPerPixel;
    if (im.bitsPerSample == 16) {
      bytesInThisTile *= 2;
    } else if (im.bitsPerSample == 32) {
      bytesInThisTile *= 4;
    }

    InputBuffer byteData;
    if (im.bitsPerSample == 8 || im.bitsPerSample == 16 || im.bitsPerSample == 32 || im.bitsPerSample == 64) {
      if (im.compression == TiffCompression.none) {
        byteData = p;
      } else if (im.compression == TiffCompression.lzw) {
        byteData = InputBuffer(Uint8List(bytesInThisTile));
        final decoder = LzwDecoder();
        try {
          decoder.decode(InputBuffer.from(p, length: byteCount), byteData.buffer);
        } catch (e) {
          //print(e);
        }
        aRes = byteData[tilePixNum];
        // Horizontal Differencing Predictor
        /*if (im.predictor == 2) {
          int count;
          for (var j = 0; j < im.tileHeight; j++) {
            count = im.samplesPerPixel * (j * im.tileWidth + 1);
            final len = im.tileWidth * im.samplesPerPixel;
            for (var i = im.samplesPerPixel; i < len; i++) {
              byteData[count] += byteData[count - im.samplesPerPixel];
              count++;
            }
          }
        }*/

        //} else if (im.compression == TiffCompression.packBits) {
        // byteData = InputBuffer(Uint8List(bytesInThisTile));
        //_decodePackBits(p, bytesInThisTile, byteData.buffer);
      } else if (im.compression == TiffCompression.deflate) {
        final data = p.toList(0, byteCount);
        final outData = const ZLibDecoder().decodeBytes(data);

        if (im.bitsPerSample == 16) {
          aRes = (outData[(tilePixNum * 2) - 1] << 8) + outData[tilePixNum * 2];
        } else {
          aRes = outData[tilePixNum];
        }
        //byteData = InputBuffer(outData);
      } else if (im.compression == TiffCompression.zip) {
        final data = p.toList(0, byteCount);
        List<int> outData = const ZLibDecoder().decodeBytes(data);

        File tata = File("${gl.pathExternalStorage}/tile_$tileIndex.csv");
        tata.writeAsString(outData.join(';'));
        tata.writeAsString("\n", mode: FileMode.append);

        gl.print("outData length ${outData.length}");
        if (im.bitsPerSample == 16) {
          aRes = (outData[(tilePixNum * 2) - 1] << 8) + outData[tilePixNum * 2];
        } else {
          gl.print("value is ${outData[tilePixNum]}");
          aRes = outData[tilePixNum];
        }
        //byteData = InputBuffer(outData);
        //} else if (im.compression == TiffCompression.oldJpeg ||
        //  im.compression == TiffCompression.jpeg) {
        //final data = p.toList(0, byteCount);
        //final tile = JpegDecoder().decode(data as Uint8List);
        //if (tile != null) {
        //  _jpegToImage(tile, image, outX, outY, tileWidth, tileHeight);
        //}
        //return;
      }
    } else {
      throw ImageException('Unsupported Compression Type: $im.compression');
    }
    gl.print("value returned is $aRes");
    return aRes;
  }

  // Is the given file a valid TIFF image?
  @override
  bool isValidFile(Uint8List data) => _readHeader(InputBuffer(data)) != null;

  // Validate the file is a TIFF image and get information about it.
  // If the file is not a valid TIFF image, null is returned.
  @override
  TiffInfo? startDecode(Uint8List bytes) {
    p = InputBuffer(bytes);
    info = _readHeader(p);
    if (info != null) {
      exif = ExifData.fromInputBuffer(InputBuffer(bytes));
    }
    return info;
  }

  // How many frames are available to be decoded. [startDecode] should have
  // been called first. Non animated image files will have a single frame.
  @override
  int numFrames() => info != null ? info!.images.length : 0;

  // Decode a single frame from the data stat was set with [startDecode].
  // If [frame] is out of the range of available frames, null is returned.
  // Non animated image files will only have [frame] 0.
  @override
  Image? decodeFrame(int frame) {
    if (info == null) {
      return null;
    }

    final image = info!.images[frame].decode(p);
    if (exif != null) {
      image.exif = exif!;
    }
    return image;
  }

  /// Decode the file and extract a single image from it. If the file is
  /// animated, the specified [frame] will be decoded. If there was a problem
  /// decoding the file, null is returned.
  @override
  Image? decode(Uint8List bytes, {int? frame}) {
    p = InputBuffer(bytes);

    info = _readHeader(p);
    if (info == null) {
      return null;
    }

    final len = numFrames();
    if (len == 1 || frame != 0) {
      return decodeFrame(frame ?? 0);
    }

    final image = decodeFrame(0);
    if (image == null) {
      return null;
    }
    image
      ..exif = ExifData.fromInputBuffer(InputBuffer(bytes))
      ..frameType = FrameType.page;

    for (var i = 1; i < len; ++i) {
      final frame = decodeFrame(i);
      image.addFrame(frame);
    }

    return image;
  }

  // Read the TIFF header and IFD blocks.
  TiffInfo? _readHeader(InputBuffer p) {
    final info = TiffInfo();
    final byteOrder = p.readUint16();
    if (byteOrder != tiffLittleEndian && byteOrder != tiffBigEndian) {
      return null;
    }

    if (byteOrder == tiffBigEndian) {
      p.bigEndian = true;
      info.bigEndian = true;
    } else {
      p.bigEndian = false;
      info.bigEndian = false;
    }

    info.signature = p.readUint16();
    if (info.signature != tiffSignature) {
      return null;
    }

    var offset = p.readUint32();
    info.ifdOffset = offset;

    final p2 = InputBuffer.from(p)..offset = offset;

    while (offset != 0) {
      TiffImage img;
      try {
        img = TiffImage(p2);
        if (!img.isValid) {
          break;
        }
      } catch (e) {
        gl.print("$e");
        break;
      }
      info.images.add(img);
      if (info.images.length == 1) {
        info
          ..width = info.images[0].width
          ..height = info.images[0].height;
      }

      offset = p2.readUint32();
      if (offset != 0) {
        p2.offset = offset;
      }
    }

    return info.images.isNotEmpty ? info : null;
  }

  static const tiffSignature = 42;
  static const tiffLittleEndian = 0x4949;
  static const tiffBigEndian = 0x4d4d;
}
