import 'dart:typed_data';
import 'package:archive/archive.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:geoimage/geoimage.dart';
import 'package:image/image.dart';
import 'package:dart_jts/dart_jts.dart';

class OnePixGeotifDecoder extends Decoder {
  TiffInfo? info;
  ExifData? exif;
  late InputBuffer _input;
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
    geoi.worldToPixel(
      Coordinate(x, y),
      uv,
    ); // determiner la position du pixel qui nous intéresse

    // determiner l'offset et le byteCount de la tile qui contient la valeur du pixel que l'on souhaite - fonctionne QUE avec mes tif qui on un Tile par ligne
    final tileIndex = uv.y.toInt() * im.tilesX;
    _input.offset = im.tileOffsets![tileIndex];
    final byteCount = im.tileByteCounts![tileIndex];

    final data = _input.toList(0, byteCount);
    List<int> outData = const ZLibDecoder().decodeBytes(data);

    if (im.bitsPerSample == 16) {
      aRes = (outData[(uv.x.toInt() * 2) - 1] << 8) + outData[uv.x.toInt() * 2];
    } else {
      aRes = outData[uv.x.toInt()];
    }

    return aRes;
  }

  // Is the given file a valid TIFF image?
  @override
  bool isValidFile(Uint8List data) => _readHeader(InputBuffer(data)) != null;

  // Validate the file is a TIFF image and get information about it.
  // If the file is not a valid TIFF image, null is returned.
  @override
  TiffInfo? startDecode(Uint8List bytes) {
    _input = InputBuffer(bytes);
    info = _readHeader(_input);
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

    final image = info!.images[frame].decode(_input);
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
    _input = InputBuffer(bytes);

    info = _readHeader(_input);
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
