import 'dart:convert';
import 'dart:typed_data';

import 'package:archive/archive.dart';

import 'package:image/image.dart';
import 'package:image/src/formats/png/png_info.dart';
import 'package:image/src/formats/png/png_frame.dart';
import 'package:image/src/formats/tiff/tiff_image.dart';
import 'package:image/src/formats/tiff/tiff_entry.dart';

import 'package:image/src/formats/tiff/tiff_bit_reader.dart';
import 'package:image/src/formats/tiff/tiff_entry.dart';
import 'package:image/src/formats/tiff/tiff_fax_decoder.dart';
import 'package:image/src/formats/tiff/tiff_lzw_decoder.dart';
import 'package:image/src/util/bit_utils.dart';

/// Decode a PNG encoded image.
class myPngDecoder extends Decoder {
  final _info = InternalPngInfo();
  int x0;
  int y0;
  int wSub;
  int hSub;

  myPngDecoder(
      {required this.x0,
      required this.y0,
      required this.wSub,
      required this.hSub});

  /// Is the given file a valid PNG image?
  @override
  bool isValidFile(Uint8List data) {
    final input = InputBuffer(data, bigEndian: true);
    final bytes = input.readBytes(8);
    const pngHeader = [137, 80, 78, 71, 13, 10, 26, 10];
    for (var i = 0; i < 8; ++i) {
      if (bytes[i] != pngHeader[i]) {
        return false;
      }
    }
    return true;
  }

  PngInfo get info => _info;

  /// Start decoding the data as an animation sequence, but don't actually
  /// process the frames until they are requested with decodeFrame.
  @override
  DecodeInfo? startDecode(Uint8List data) {
    _input = InputBuffer(data, bigEndian: true);

    final pngHeader = _input.readBytes(8);
    const expectedHeader = [137, 80, 78, 71, 13, 10, 26, 10];
    for (var i = 0; i < 8; ++i) {
      if (pngHeader[i] != expectedHeader[i]) {
        return null;
      }
    }

    while (true) {
      final inputPos = _input.position;
      var chunkSize = _input.readUint32();
      final chunkType = _input.readString(4);
      print(
          "chunkType : " + chunkType + ", chunkSize : " + chunkSize.toString());
      switch (chunkType) {
        case 'tEXt':
          final txtData = _input.readBytes(chunkSize).toUint8List();
          final l = txtData.length;
          for (var i = 0; i < l; ++i) {
            if (txtData[i] == 0) {
              final key = latin1.decode(txtData.sublist(0, i));
              final text = latin1.decode(txtData.sublist(i + 1));
              _info.textData[key] = text;
              break;
            }
          }
          _input.skip(4); //crc
          break;
        case 'IHDR':
          final hdr = InputBuffer.from(_input.readBytes(chunkSize));
          final Uint8List hdrBytes = hdr.toUint8List();
          _info.width = hdr.readUint32();
          _info.height = hdr.readUint32();
          _info.bits = hdr.readByte();
          _info.colorType = hdr.readByte();
          _info.compressionMethod = hdr.readByte();
          _info.filterMethod = hdr.readByte();
          _info.interlaceMethod = hdr.readByte();

          // Validate some of the info in the header to make sure we support
          // the proposed image data.
          if (!PngColorType.isValid(_info.colorType)) {
            return null;
          }

          if (_info.filterMethod != 0) {
            return null;
          }

          switch (_info.colorType) {
            case PngColorType.grayscale:
              if (![1, 2, 4, 8, 16].contains(_info.bits)) {
                return null;
              }
              break;
            case PngColorType.rgb:
              if (![8, 16].contains(_info.bits)) {
                return null;
              }
              break;
            case PngColorType.indexed:
              if (![1, 2, 4, 8].contains(_info.bits)) {
                return null;
              }
              break;
            case PngColorType.grayscaleAlpha:
              if (![8, 16].contains(_info.bits)) {
                return null;
              }
              break;
            case PngColorType.rgba:
              if (![8, 16].contains(_info.bits)) {
                return null;
              }
              break;
          }

          final crc = _input.readUint32();
          final computedCrc = _crc(chunkType, hdrBytes);
          if (crc != computedCrc) {
            throw ImageException('Invalid $chunkType checksum');
          }
          break;
        case 'PLTE':
          _info.palette = _input.readBytes(chunkSize).toUint8List();
          final crc = _input.readUint32();
          final computedCrc = _crc(chunkType, _info.palette as List<int>);
          if (crc != computedCrc) {
            throw ImageException('Invalid $chunkType checksum');
          }
          break;
        case 'tRNS':
          _info.transparency = _input.readBytes(chunkSize).toUint8List();
          final crc = _input.readUint32();
          final computedCrc = _crc(chunkType, _info.transparency!);
          if (crc != computedCrc) {
            throw ImageException('Invalid $chunkType checksum');
          }
          break;
        case 'IEND':
          // End of the image.
          _input.skip(4); // CRC
          break;
        /*case 'eXif': // TODO: parse exif
          {
            final exifData = _input.readBytes(chunkSize);
            final exif = ExifData.fromInputBuffer(exifData);
            _input.skip(4); // CRC
            break;
          }*/
        case 'gAMA':
          if (chunkSize != 4) {
            throw ImageException('Invalid gAMA chunk');
          }
          final gammaInt = _input.readUint32();
          _input.skip(4); // CRC
          // A gamma of 1.0 doesn't have any affect, so pretend we didn't get
          // a gamma in that case.
          if (gammaInt != 100000) {
            _info.gamma = gammaInt / 100000.0;
          }
          break;
        case 'IDAT':
          _info.idat.add(inputPos);
          _input.skip(chunkSize);
          _input.skip(4); // CRC
          break;
        case 'acTL': // Animation control chunk
          _info.numFrames = _input.readUint32();
          _info.repeat = _input.readUint32();
          _input.skip(4); // CRC
          break;
        case 'fcTL': // Frame control chunk
          final frame = InternalPngFrame(
              sequenceNumber: _input.readUint32(),
              width: _input.readUint32(),
              height: _input.readUint32(),
              xOffset: _input.readUint32(),
              yOffset: _input.readUint32(),
              delayNum: _input.readUint16(),
              delayDen: _input.readUint16(),
              dispose: PngDisposeMode.values[_input.readByte()],
              blend: PngBlendMode.values[_input.readByte()]);
          _info.frames.add(frame);
          _input.skip(4); // CRC
          break;
        case 'fdAT':
          /*int sequenceNumber =*/ _input.readUint32();
          final frame = _info.frames.last as InternalPngFrame;
          frame.fdat.add(inputPos);
          _input.skip(chunkSize - 4);
          _input.skip(4); // CRC
          break;
        case 'bKGD':
          if (_info.colorType == PngColorType.indexed) {
            final paletteIndex = _input.readByte();
            chunkSize--;
            final p3 = paletteIndex * 3;
            final r = _info.palette![p3]!;
            final g = _info.palette![p3 + 1]!;
            final b = _info.palette![p3 + 2]!;
            if (_info.transparency != null) {
              final isTransparent = _info.transparency!.contains(paletteIndex);
              _info.backgroundColor =
                  ColorRgba8(r, g, b, isTransparent ? 0 : 255);
            } else {
              _info.backgroundColor = ColorRgb8(r, g, b);
            }
          } else if (_info.colorType == PngColorType.grayscale ||
              _info.colorType == PngColorType.grayscaleAlpha) {
            /*int gray =*/ _input.readUint16();
            chunkSize -= 2;
          } else if (_info.colorType == PngColorType.rgb ||
              _info.colorType == PngColorType.rgba) {
            /*int r =*/ _input
              ..readUint16()
              /*int g =*/
              ..readUint16()
              /*int b =*/
              ..readUint16();
            chunkSize -= 24;
          }
          if (chunkSize > 0) {
            _input.skip(chunkSize);
          }
          _input.skip(4); // CRC
          break;
        case 'iCCP':
          _info.iccpName = _input.readString();
          _info.iccpCompression = _input.readByte(); // 0: deflate
          chunkSize -= _info.iccpName.length + 2;
          final profile = _input.readBytes(chunkSize);
          _info.iccpData = profile.toUint8List();
          _input.skip(4); // CRC
          break;
        default:
          //print('Skipping $chunkType');
          _input.skip(chunkSize);
          _input.skip(4); // CRC
          break;
      }

      if (chunkType == 'IEND') {
        break;
      }

      if (_input.isEOS) {
        return null;
      }
    }

    return _info;
  }

  /// The number of frames that can be decoded.
  @override
  int numFrames() => _info.numFrames;

  /// Decode the frame (assuming [startDecode] has already been called).
  @override
  Image? decodeFrame(int frame) {
    Uint8List imageData;

    int? width = _info.width;
    int? height = _info.height;

    // lecture de tout les chunks -> je devrais limiter la lecture aux chunks qui m'intéressent pour gagner du temps.
    // 1) création de datablock ; liste de bloc contenant la liste des valeurs
    // 2) création de imageData : liste des valeurs
    if (!_info.isAnimated || frame == 0) {
      final dataBlocks = <Uint8List>[];
      var totalSize = 0;
      final len = _info.idat.length;
      for (var i = 0; i < len; ++i) {
        _input.offset = _info.idat[i];
        final chunkSize = _input.readUint32();
        final chunkType = _input.readString(4);
        final data = _input.readBytes(chunkSize).toUint8List();
        totalSize += data.length;
        dataBlocks.add(data);
        final crc = _input.readUint32();
        final computedCrc = _crc(chunkType, data);
        if (crc != computedCrc) {
          throw ImageException('Invalid $chunkType checksum');
        }
      }
      print("total imageData size : " + totalSize.toString());
      imageData = Uint8List(totalSize);
      var offset = 0;
      for (var data in dataBlocks) {
        imageData.setAll(offset, data);
        offset += data.length;
      }
    } else {
      if (frame < 0 || frame >= _info.frames.length) {
        throw ImageException('Invalid Frame Number: $frame');
      }

      final f = _info.frames[frame] as InternalPngFrame;
      width = f.width;
      height = f.height;
      var totalSize = 0;
      final dataBlocks = <Uint8List>[];
      for (var i = 0; i < f.fdat.length; ++i) {
        _input.offset = f.fdat[i];
        final chunkSize = _input.readUint32();
        _input
          ..readString(4) // fDat chunk header
          ..skip(4); // sequence number
        final data = _input.readBytes(chunkSize - 4).toUint8List();
        totalSize += data.length;
        dataBlocks.add(data);
      }
      imageData = Uint8List(totalSize);
      var offset = 0;
      for (var data in dataBlocks) {
        imageData.setAll(offset, data);
        offset += data.length;
      }
    }

    var numChannels = _info.colorType == PngColorType.indexed
        ? 1
        : _info.colorType == PngColorType.grayscale
            ? 1
            : _info.colorType == PngColorType.grayscaleAlpha
                ? 2
                : _info.colorType == PngColorType.rgba
                    ? 4
                    : 3;

    List<int> uncompressed;
    try {
      uncompressed = const ZLibDecoder().decodeBytes(imageData);
    } catch (error) {
      //print(error);
      return null;
    }

    // input is the decompressed data.
    final InputBuffer input = InputBuffer(uncompressed, bigEndian: true);
    print("input image data after decompression ; " +
        input.length.toString()); // presque le nombre de pixels de l'image x3
    _resetBits();

    PaletteUint8? palette;

    // Non-indexed PNGs may have a palette, but it only provides a suggested
    // set of colors to which an RGB color can be quantized if not displayed
    // directly. In this case, just ignore the palette.
    if (_info.colorType == PngColorType.indexed) {
      if (_info.palette != null) {
        final p = _info.palette!;
        final numColors = p.length ~/ 3;
        final t = _info.transparency;
        final tl = t != null ? t.length : 0;
        final nc = t != null ? 4 : 3;
        palette = PaletteUint8(numColors, nc);
        for (var i = 0, pi = 0; i < numColors; ++i, pi += 3) {
          var a = 255;
          if (nc == 4 && i < tl) {
            a = t![i];
          }
          palette.setRgba(i, p[pi]!, p[pi + 1]!, p[pi + 2]!, a);
        }
      }
    }

    // grayscale images with no palette but with transparency, get
    // converted to a indexed palette image.
    if (_info.colorType == PngColorType.grayscale &&
        _info.transparency != null &&
        palette == null &&
        _info.bits <= 8) {
      final t = _info.transparency!;
      final nt = t.length;
      final numColors = 1 << _info.bits;
      palette = PaletteUint8(numColors, 4);
      // palette color are 8-bit, so convert the grayscale bit value to the
      // 8-bit palette value.
      final to8bit = _info.bits == 1
          ? 255
          : _info.bits == 2
              ? 85
              : _info.bits == 4
                  ? 17
                  : 1;
      for (var i = 0; i < numColors; ++i) {
        final g = i * to8bit;
        palette.setRgba(i, g, g, g, 255);
      }
      for (var i = 0; i < nt; i += 2) {
        final ti = ((t[i] & 0xff) << 8) | (t[i + 1] & 0xff);
        if (ti < numColors) {
          palette.set(ti, 3, 0);
        }
      }
    }

    final format = _info.bits == 1
        ? Format.uint1
        : _info.bits == 2
            ? Format.uint2
            : _info.bits == 4
                ? Format.uint4
                : _info.bits == 16
                    ? Format.uint16
                    : Format.uint8;

    if (_info.colorType == PngColorType.grayscale &&
        _info.transparency != null &&
        _info.bits > 8) {
      numChannels = 4;
    }

    if (_info.colorType == PngColorType.rgb && _info.transparency != null) {
      numChannels = 4;
    }

    final image = Image(
        //    width: width,
        //    height: height,
        width: wSub,
        height: hSub,
        numChannels: numChannels,
        palette: palette,
        format: format);

    final origW = _info.width;
    final origH = _info.height;
    _info
      ..width = width
      ..height = height;

    final w = width;
    final h = height;
    _progressY = 0;
    if (_info.interlaceMethod != 0) {
      _processPass(input, image, 0, 0, 8, 8, (w + 7) >> 3, (h + 7) >> 3);
      _processPass(input, image, 4, 0, 8, 8, (w + 3) >> 3, (h + 7) >> 3);
      _processPass(input, image, 0, 4, 4, 8, (w + 3) >> 2, (h + 3) >> 3);
      _processPass(input, image, 2, 0, 4, 4, (w + 1) >> 2, (h + 3) >> 2);
      _processPass(input, image, 0, 2, 2, 4, (w + 1) >> 1, (h + 1) >> 2);
      _processPass(input, image, 1, 0, 2, 2, w >> 1, (h + 1) >> 1);
      _processPass(input, image, 0, 1, 1, 2, w, h >> 1);
    } else {
      _process(input, image);
    }

    _info
      ..width = origW
      ..height = origH;

    if (_info.iccpData != null) {
      image.iccProfile = IccProfile(
          _info.iccpName, IccProfileCompression.deflate, _info.iccpData!);
    }

    if (_info.textData.isNotEmpty) {
      image.addTextData(_info.textData);
    }

    return image;
  }

  @override
  Image? decode(Uint8List bytes, {int? frame}) {
    if (startDecode(bytes) == null) {
      return null;
    }

    if (!_info.isAnimated || frame != null) {
      return decodeFrame(frame ?? 0)!;
    }

    Image? firstImage;
    Image? lastImage;
    for (var i = 0; i < _info.numFrames; ++i) {
      final frame = _info.frames[i];
      final image = decodeFrame(i);
      if (image == null) {
        continue;
      }

      if (firstImage == null || lastImage == null) {
        firstImage = image;
        lastImage = image
          // Convert to MS
          ..frameDuration = (frame.delay * 1000).toInt();
        continue;
      }

      if (image.width == lastImage.width &&
          image.height == lastImage.height &&
          frame.xOffset == 0 &&
          frame.yOffset == 0 &&
          frame.blend == PngBlendMode.source) {
        lastImage = image
          // Convert to MS
          ..frameDuration = (frame.delay * 1000).toInt();
        firstImage.addFrame(lastImage);
        continue;
      }

      final dispose = frame.dispose;
      if (dispose == PngDisposeMode.background) {
        lastImage = Image.from(lastImage)..clear(_info.backgroundColor);
      } else if (dispose == PngDisposeMode.previous) {
        lastImage = Image.from(lastImage);
      } else {
        lastImage = Image.from(lastImage);
      }

      // Convert to MS
      lastImage.frameDuration = (frame.delay * 1000).toInt();

      compositeImage(lastImage, image,
          dstX: frame.xOffset,
          dstY: frame.yOffset,
          blend: frame.blend == PngBlendMode.over
              ? BlendMode.alpha
              : BlendMode.direct);

      firstImage.addFrame(lastImage);
    }

    return firstImage;
  }

  // Process a pass of an interlaced image.
  void _processPass(InputBuffer input, Image image, int xOffset, int yOffset,
      int xStep, int yStep, int passWidth, int passHeight) {
    final channels = (_info.colorType == PngColorType.grayscaleAlpha)
        ? 2
        : (_info.colorType == PngColorType.rgb)
            ? 3
            : (_info.colorType == PngColorType.rgba)
                ? 4
                : 1;

    final pixelDepth = channels * _info.bits;
    final bpp = (pixelDepth + 7) >> 3;
    final rowBytes = (pixelDepth * passWidth + 7) >> 3;

    final inData = <Uint8List?>[null, null];

    final pixel = [0, 0, 0, 0];

    for (var srcY = 0, dstY = yOffset, ri = 0;
        srcY < passHeight;
        ++srcY, dstY += yStep, ri = 1 - ri, _progressY++) {
      final filterType = PngFilterType.values[input.readByte()];
      inData[ri] = input.readBytes(rowBytes).toUint8List();

      final row = inData[ri];
      final prevRow = inData[1 - ri];

      // Before the image is compressed, it was filtered to improve compression.
      // Reverse the filter now.
      _unfilter(filterType, bpp, row!, prevRow);

      // Scanlines are always on byte boundaries, so for bit depths < 8,
      // reset the bit stream counter.
      _resetBits();

      final rowInput = InputBuffer(row, bigEndian: true);

      final blockHeight = xStep;
      final blockWidth = xStep - xOffset;

      for (var srcX = 0, dstX = xOffset;
          srcX < passWidth;
          ++srcX, dstX += xStep) {
        _readPixel(rowInput, pixel);
        _setPixel(image.getPixel(dstX, dstY), pixel);

        if (blockWidth > 1 || blockHeight > 1) {
          for (var i = 0; i < blockHeight; ++i) {
            for (var j = 0; j < blockWidth; ++j) {
              _setPixel(image.getPixelSafe(dstX + j, dstY + i), pixel);
            }
          }
        }
      }
    }
  }

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
      _unfilter(filterType, bpp, row, prevRow);

      // Scanlines are always on byte boundaries, so for bit depths < 8,
      // reset the bit stream counter.
      _resetBits();
      late final rowInput;
      if (y >= y0 && y0 + hSub > y) {
        rowInput = InputBuffer(inData[ri], bigEndian: true);

        for (var x = 0; x < w; ++x) {
          _readPixel(rowInput, pixel);
          if (y >= y0 && y0 + hSub > y) {
            if (x >= x0 && x0 + wSub > x) {
              _setPixel(pIter.current, pixel);

              pIter.moveNext();
            }
          }
        }
      }
    }
  }

  void _unfilter(
      PngFilterType filterType, int bpp, List<int> row, List<int>? prevRow) {
    final rowBytes = row.length;

    switch (filterType) {
      case PngFilterType.none:
        break;
      case PngFilterType.sub:
        for (var x = bpp; x < rowBytes; ++x) {
          row[x] = (row[x] + row[x - bpp]) & 0xff;
        }
        break;
      case PngFilterType.up:
        for (var x = 0; x < rowBytes; ++x) {
          final b = prevRow != null ? prevRow[x] : 0;
          row[x] = (row[x] + b) & 0xff;
        }
        break;
      case PngFilterType.average:
        for (var x = 0; x < rowBytes; ++x) {
          final a = x < bpp ? 0 : row[x - bpp];
          final b = prevRow != null ? prevRow[x] : 0;
          row[x] = (row[x] + ((a + b) >> 1)) & 0xff;
        }
        break;
      case PngFilterType.paeth:
        for (var x = 0; x < rowBytes; ++x) {
          final a = x < bpp ? 0 : row[x - bpp];
          final b = prevRow != null ? prevRow[x] : 0;
          final c = x < bpp || prevRow == null ? 0 : prevRow[x - bpp];

          final p = a + b - c;

          final pa = (p - a).abs();
          final pb = (p - b).abs();
          final pc = (p - c).abs();

          var paeth = 0;
          if (pa <= pb && pa <= pc) {
            paeth = a;
          } else if (pb <= pc) {
            paeth = b;
          } else {
            paeth = c;
          }

          row[x] = (row[x] + paeth) & 0xff;
        }
        break;
      default:
        throw ImageException('Invalid filter value: $filterType');
    }
  }

  // Return the CRC of the bytes
  int _crc(String type, List<int> bytes) {
    final crc = getCrc32(type.codeUnits);
    return getCrc32(bytes, crc);
  }

  int _bitBuffer = 0;
  int _bitBufferLen = 0;

  void _resetBits() {
    _bitBuffer = 0;
    _bitBufferLen = 0;
  }

  // Read a number of bits from the input stream.
  int _readBits(InputBuffer input, int numBits) {
    if (numBits == 0) {
      return 0;
    }

    if (numBits == 8) {
      return input.readByte();
    }

    if (numBits == 16) {
      return input.readUint16();
    }

    // not enough buffer
    while (_bitBufferLen < numBits) {
      if (input.isEOS) {
        throw ImageException('Invalid PNG data.');
      }

      // input byte
      final octet = input.readByte();

      // concat octet
      _bitBuffer = octet << _bitBufferLen;
      _bitBufferLen += 8;
    }

    // output byte
    final mask = (numBits == 1)
        ? 1
        : (numBits == 2)
            ? 3
            : (numBits == 4)
                ? 0xf
                : (numBits == 8)
                    ? 0xff
                    : (numBits == 16)
                        ? 0xffff
                        : 0;

    final octet = (_bitBuffer >> (_bitBufferLen - numBits)) & mask;

    _bitBufferLen -= numBits;

    return octet;
  }

  // Read the next pixel from the input stream.
  void _readPixel(InputBuffer input, List<int> pixel) {
    switch (_info.colorType) {
      case PngColorType.grayscale:
        pixel[0] = _readBits(input, _info.bits);
        return;
      case PngColorType.rgb:
        pixel[0] = _readBits(input, _info.bits);
        pixel[1] = _readBits(input, _info.bits);
        pixel[2] = _readBits(input, _info.bits);
        return;
      case PngColorType.indexed:
        pixel[0] = _readBits(input, _info.bits);
        return;
      case PngColorType.grayscaleAlpha:
        pixel[0] = _readBits(input, _info.bits);
        pixel[1] = _readBits(input, _info.bits);
        return;
      case PngColorType.rgba:
        pixel[0] = _readBits(input, _info.bits);
        pixel[1] = _readBits(input, _info.bits);
        pixel[2] = _readBits(input, _info.bits);
        pixel[3] = _readBits(input, _info.bits);
        return;
    }

    throw ImageException('Invalid color type: ${_info.colorType}.');
  }

  // Get the color with the list of components.
  void _setPixel(Pixel p, List<int> raw) {
    switch (_info.colorType) {
      case PngColorType.grayscale:
        if (_info.transparency != null && _info.bits > 8) {
          final t = _info.transparency!;
          final a = ((t[0] & 0xff) << 24) | (t[1] & 0xff);
          final g = raw[0];
          p.setRgba(g, g, g, g != a ? p.maxChannelValue : 0);
          return;
        }
        p.setRgb(raw[0], 0, 0);
        return;
      case PngColorType.rgb:
        final r = raw[0];
        final g = raw[1];
        final b = raw[2];

        if (_info.transparency != null) {
          final t = _info.transparency!;
          final tr = ((t[0] & 0xff) << 8) | (t[1] & 0xff);
          final tg = ((t[2] & 0xff) << 8) | (t[3] & 0xff);
          final tb = ((t[4] & 0xff) << 8) | (t[5] & 0xff);
          if (raw[0] != tr || raw[1] != tg || raw[2] != tb) {
            p.setRgba(r, g, b, p.maxChannelValue);
            return;
          }
        }

        p.setRgb(r, g, b);
        return;
      case PngColorType.indexed:
        p.index = raw[0];
        return;
      case PngColorType.grayscaleAlpha:
        p.setRgb(raw[0], raw[1], 0);
        return;
      case PngColorType.rgba:
        p.setRgba(raw[0], raw[1], raw[2], raw[3]);
        return;
    }

    throw ImageException('Invalid color type: ${_info.colorType}.');
  }

  late InputBuffer _input;
  int _progressY = 0;
}

class myTiffDecoder extends Decoder {
  TiffInfo? info;
  ExifData? exif;
  late InputBuffer _input;

  /// Is the given file a valid TIFF image?
  @override
  bool isValidFile(Uint8List data) => _readHeader(InputBuffer(data)) != null;

  /// Validate the file is a TIFF image and get information about it.
  /// If the file is not a valid TIFF image, null is returned.
  @override
  TiffInfo? startDecode(Uint8List bytes) {
    _input = InputBuffer(bytes);
    info = _readHeader(_input);
    if (info != null) {
      exif = ExifData.fromInputBuffer(InputBuffer(bytes));
    }
    return info;
  }

  /// How many frames are available to be decoded. [startDecode] should have
  /// been called first. Non animated image files will have a single frame.
  @override
  int numFrames() => info != null ? info!.images.length : 0;

  /// Decode a single frame from the data stat was set with [startDecode].
  /// If [frame] is out of the range of available frames, null is returned.
  /// Non animated image files will only have [frame] 0.
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
      } catch (error) {
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

class myTiffImage {
  Map<int, TiffEntry> tags = {};
  int width = 0;
  int height = 0;
  TiffPhotometricType photometricType = TiffPhotometricType.unknown;
  int compression = 1;
  int bitsPerSample = 1;
  int samplesPerPixel = 1;
  TiffFormat sampleFormat = TiffFormat.uint;
  TiffImageType imageType = TiffImageType.invalid;
  bool isWhiteZero = false;
  int predictor = 1;
  late int chromaSubH;
  late int chromaSubV;
  bool tiled = false;
  int tileWidth = 0;
  int tileHeight = 0;
  List<int>? tileOffsets;
  List<int>? tileByteCounts;
  late int tilesX;
  late int tilesY;
  int? tileSize;
  int? fillOrder = 1;
  int? t4Options = 0;
  int? t6Options = 0;
  int? extraSamples;
  int colorMapSamples = 0;
  List<int>? colorMap;
  // Starting index in the [colorMap] for the red channel.
  late int colorMapRed;
  // Starting index in the [colorMap] for the green channel.
  late int colorMapGreen;
  // Starting index in the [colorMap] for the blue channel.
  late int colorMapBlue;

  myTiffImage(InputBuffer p) {
    final p3 = InputBuffer.from(p);

    final numDirEntries = p.readUint16();
    for (var i = 0; i < numDirEntries; ++i) {
      final tag = p.readUint16();
      final ti = p.readUint16();
      final type = IfdValueType.values[ti];
      final typeSize = ifdValueTypeSize[ti];
      final count = p.readUint32();
      var valueOffset = 0;
      // The value for the tag is either stored in another location,
      // or within the tag itself (if the size fits in 4 bytes).
      // We're not reading the data here, just storing offsets.
      if (count * typeSize > 4) {
        valueOffset = p.readUint32();
      } else {
        valueOffset = p.offset;
        p.skip(4);
      }

      final entry = TiffEntry(tag, type, count, p3, valueOffset);

      tags[entry.tag] = entry;

      if (tag == exifTagNameToID['ImageWidth']) {
        width = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['ImageLength']) {
        height = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['PhotometricInterpretation']) {
        final v = entry.read();
        final pt = v?.toInt() ?? TiffPhotometricType.values.length;
        if (pt < TiffPhotometricType.values.length) {
          photometricType = TiffPhotometricType.values[pt];
        } else {
          photometricType = TiffPhotometricType.unknown;
        }
      } else if (tag == exifTagNameToID['Compression']) {
        compression = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['BitsPerSample']) {
        bitsPerSample = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['SamplesPerPixel']) {
        samplesPerPixel = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['Predictor']) {
        predictor = entry.read()?.toInt() ?? 0;
      } else if (tag == exifTagNameToID['SampleFormat']) {
        final v = entry.read()?.toInt() ?? 0;
        sampleFormat = TiffFormat.values[v];
      } else if (tag == exifTagNameToID['ColorMap']) {
        final v = entry.read();
        if (v != null) {
          colorMap = v.toData().buffer.asUint16List();
          colorMapRed = 0;
          colorMapGreen = colorMap!.length ~/ 3;
          colorMapBlue = colorMapGreen * 2;
        }
      }
    }

    if (colorMap != null && photometricType == TiffPhotometricType.palette) {
      // Only support RGB palettes.
      colorMapSamples = 3;
      samplesPerPixel = 1;
    }

    if (width == 0 || height == 0) {
      return;
    }

    if (colorMap != null && bitsPerSample == 8) {
      final cm = colorMap!;
      final len = cm.length;
      for (var i = 0; i < len; ++i) {
        cm[i] >>= 8;
      }
    }

    if (photometricType == TiffPhotometricType.whiteIsZero) {
      isWhiteZero = true;
    }

    if (hasTag(exifTagNameToID['TileOffsets']!)) {
      tiled = true;
      // Image is in tiled format
      tileWidth = _readTag(exifTagNameToID['TileWidth']!);
      tileHeight = _readTag(exifTagNameToID['TileLength']!);
      tileOffsets = _readTagList(exifTagNameToID['TileOffsets']!);
      tileByteCounts = _readTagList(exifTagNameToID['TileByteCounts']!);
    } else {
      tiled = false;

      tileWidth = _readTag(exifTagNameToID['TileWidth']!, width);
      if (!hasTag(exifTagNameToID['RowsPerStrip']!)) {
        tileHeight = _readTag(exifTagNameToID['TileLength']!, height);
      } else {
        final l = _readTag(exifTagNameToID['RowsPerStrip']!);
        var infinity = 1;
        infinity = (infinity << 32) - 1;
        if (l == infinity) {
          // 2^32 - 1 (effectively infinity, entire image is 1 strip)
          tileHeight = height;
        } else {
          tileHeight = l;
        }
      }

      tileOffsets = _readTagList(exifTagNameToID['StripOffsets']!);
      tileByteCounts = _readTagList(exifTagNameToID['StripByteCounts']!);
    }

    // Calculate number of tiles and the tileSize in bytes
    tilesX = (width + tileWidth - 1) ~/ tileWidth;
    tilesY = (height + tileHeight - 1) ~/ tileHeight;
    tileSize = tileWidth * tileHeight * samplesPerPixel;

    fillOrder = _readTag(exifTagNameToID['FillOrder']!, 1);
    t4Options = _readTag(exifTagNameToID['T4Options']!);
    t6Options = _readTag(exifTagNameToID['T6Options']!);
    extraSamples = _readTag(exifTagNameToID['ExtraSamples']!);

    // Determine which kind of image we are dealing with.
    switch (photometricType) {
      case TiffPhotometricType.whiteIsZero:
      case TiffPhotometricType.blackIsZero:
        if (bitsPerSample == 1 && samplesPerPixel == 1) {
          imageType = TiffImageType.bilevel;
        } else if (bitsPerSample == 4 && samplesPerPixel == 1) {
          imageType = TiffImageType.gray4bit;
        } else if (bitsPerSample % 8 == 0) {
          if (samplesPerPixel == 1) {
            imageType = TiffImageType.gray;
          } else if (samplesPerPixel == 2) {
            imageType = TiffImageType.grayAlpha;
          } else {
            imageType = TiffImageType.generic;
          }
        }
        break;
      case TiffPhotometricType.rgb:
        if (bitsPerSample % 8 == 0) {
          if (samplesPerPixel == 3) {
            imageType = TiffImageType.rgb;
          } else if (samplesPerPixel == 4) {
            imageType = TiffImageType.rgba;
          } else {
            imageType = TiffImageType.generic;
          }
        }
        break;
      case TiffPhotometricType.palette:
        if (samplesPerPixel == 1 &&
            colorMap != null &&
            (bitsPerSample == 4 || bitsPerSample == 8 || bitsPerSample == 16)) {
          imageType = TiffImageType.palette;
        }
        break;
      case TiffPhotometricType.transparencyMask: // Transparency mask
        if (bitsPerSample == 1 && samplesPerPixel == 1) {
          imageType = TiffImageType.bilevel;
        }
        break;
      case TiffPhotometricType.yCbCr:
        if (compression == TiffCompression.jpeg &&
            bitsPerSample == 8 &&
            samplesPerPixel == 3) {
          imageType = TiffImageType.rgb;
        } else {
          if (hasTag(exifTagNameToID['YCbCrSubSampling']!)) {
            final v = tags[exifTagNameToID['YCbCrSubSampling']!]!.read()!;
            chromaSubH = v.toInt();
            chromaSubV = v.toInt(1);
          } else {
            chromaSubH = 2;
            chromaSubV = 2;
          }

          if (chromaSubH * chromaSubV == 1) {
            imageType = TiffImageType.generic;
          } else if (bitsPerSample == 8 && samplesPerPixel == 3) {
            imageType = TiffImageType.yCbCrSub;
          }
        }
        break;
      default: // Other including CMYK, CIE L*a*b*, unknown.
        if (bitsPerSample % 8 == 0) {
          imageType = TiffImageType.generic;
        }
        break;
    }
  }

  bool get isValid => width != 0 && height != 0;

  Image decode(InputBuffer p) {
    final isFloat = sampleFormat == TiffFormat.float;
    final isInt = sampleFormat == TiffFormat.int;
    final format = bitsPerSample == 1
        ? Format.uint1
        : bitsPerSample == 2
            ? Format.uint2
            : bitsPerSample == 4
                ? Format.uint4
                : isFloat && bitsPerSample == 16
                    ? Format.float16
                    : isFloat && bitsPerSample == 32
                        ? Format.float32
                        : isFloat && bitsPerSample == 64
                            ? Format.float64
                            : isInt && bitsPerSample == 8
                                ? Format.int8
                                : isInt && bitsPerSample == 16
                                    ? Format.int16
                                    : isInt && bitsPerSample == 32
                                        ? Format.int32
                                        : bitsPerSample == 16
                                            ? Format.uint16
                                            : bitsPerSample == 32
                                                ? Format.uint32
                                                : Format.uint8;
    final hasPalette =
        colorMap != null && photometricType == TiffPhotometricType.palette;
    final numChannels = hasPalette ? 3 : samplesPerPixel;

    final image = Image(
        width: width,
        height: height,
        format: format,
        numChannels: numChannels,
        withPalette: hasPalette);

    if (hasPalette) {
      final p = image.palette!;
      final cm = colorMap!;
      const numChannels = 3; // Only support RGB palettes
      final numColors = cm.length ~/ numChannels;
      for (var i = 0; i < numColors; ++i) {
        p.setRgb(i, cm[colorMapRed + i], cm[colorMapGreen + i],
            cm[colorMapBlue + i]);
      }
    }

    for (var tileY = 0, ti = 0; tileY < tilesY; ++tileY) {
      for (var tileX = 0; tileX < tilesX; ++tileX, ++ti) {
        _decodeTile(p, image, tileX, tileY);
      }
    }

    return image;
  }

  bool hasTag(int tag) => tags.containsKey(tag);

  void _decodeTile(InputBuffer p, Image image, int tileX, int tileY) {
    // Read the data, uncompressing as needed. There are four cases:
    // bilevel, palette-RGB, 4-bit grayscale, and everything else.
    if (imageType == TiffImageType.bilevel) {
      _decodeBilevelTile(p, image, tileX, tileY);
      return;
    }

    final tileIndex = tileY * tilesX + tileX;
    p.offset = tileOffsets![tileIndex];

    final outX = tileX * tileWidth;
    final outY = tileY * tileHeight;

    final byteCount = tileByteCounts![tileIndex];
    var bytesInThisTile = tileWidth * tileHeight * samplesPerPixel;
    if (bitsPerSample == 16) {
      bytesInThisTile *= 2;
    } else if (bitsPerSample == 32) {
      bytesInThisTile *= 4;
    }

    InputBuffer byteData;
    if (bitsPerSample == 8 ||
        bitsPerSample == 16 ||
        bitsPerSample == 32 ||
        bitsPerSample == 64) {
      if (compression == TiffCompression.none) {
        byteData = p;
      } else if (compression == TiffCompression.lzw) {
        byteData = InputBuffer(Uint8List(bytesInThisTile));
        final decoder = LzwDecoder();
        try {
          decoder.decode(
              InputBuffer.from(p, length: byteCount), byteData.buffer);
        } catch (e) {
          //print(e);
        }
        // Horizontal Differencing Predictor
        if (predictor == 2) {
          int count;
          for (var j = 0; j < tileHeight; j++) {
            count = samplesPerPixel * (j * tileWidth + 1);
            final len = tileWidth * samplesPerPixel;
            for (var i = samplesPerPixel; i < len; i++) {
              byteData[count] += byteData[count - samplesPerPixel];
              count++;
            }
          }
        }
      } else if (compression == TiffCompression.packBits) {
        byteData = InputBuffer(Uint8List(bytesInThisTile));
        _decodePackBits(p, bytesInThisTile, byteData.buffer);
      } else if (compression == TiffCompression.deflate) {
        final data = p.toList(0, byteCount);
        final outData = Inflate(data).getBytes();
        byteData = InputBuffer(outData);
      } else if (compression == TiffCompression.zip) {
        final data = p.toList(0, byteCount);
        final outData = const ZLibDecoder().decodeBytes(data);
        byteData = InputBuffer(outData);
      } else if (compression == TiffCompression.oldJpeg) {
        final data = p.toList(0, byteCount);
        final tile = JpegDecoder().decode(data as Uint8List);
        if (tile != null) {
          _jpegToImage(tile, image, outX, outY, tileWidth, tileHeight);
        }
        return;
      } else {
        throw ImageException('Unsupported Compression Type: $compression');
      }

      for (var y = 0, py = outY; y < tileHeight && py < height; ++y, ++py) {
        for (var x = 0, px = outX; x < tileWidth && px < width; ++x, ++px) {
          if (samplesPerPixel == 1) {
            if (sampleFormat == TiffFormat.float) {
              num sample = 0;
              if (bitsPerSample == 32) {
                sample = byteData.readFloat32();
              } else if (bitsPerSample == 64) {
                sample = byteData.readFloat64();
              } else if (bitsPerSample == 16) {
                sample = Float16.float16ToDouble(byteData.readUint16());
              }
              image.setPixelR(px, py, sample);
            } else {
              var sample = 0;
              if (bitsPerSample == 8) {
                sample = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
              } else if (bitsPerSample == 16) {
                sample = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
              } else if (bitsPerSample == 32) {
                sample = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
              }

              if (photometricType == TiffPhotometricType.whiteIsZero) {
                final mx = image.maxChannelValue as int;
                sample = mx - sample;
              }

              image.setPixelR(px, py, sample);
            }
          } else if (samplesPerPixel == 2) {
            var gray = 0;
            var alpha = 0;
            if (bitsPerSample == 8) {
              gray = sampleFormat == TiffFormat.int
                  ? byteData.readInt8()
                  : byteData.readByte();
              alpha = sampleFormat == TiffFormat.int
                  ? byteData.readInt8()
                  : byteData.readByte();
            } else if (bitsPerSample == 16) {
              gray = sampleFormat == TiffFormat.int
                  ? byteData.readInt16()
                  : byteData.readUint16();
              alpha = sampleFormat == TiffFormat.int
                  ? byteData.readInt16()
                  : byteData.readUint16();
            } else if (bitsPerSample == 32) {
              gray = sampleFormat == TiffFormat.int
                  ? byteData.readInt32()
                  : byteData.readUint32();
              alpha = sampleFormat == TiffFormat.int
                  ? byteData.readInt32()
                  : byteData.readUint32();
            }

            image.setPixelRgb(px, py, gray, alpha, 0);
          } else if (samplesPerPixel == 3) {
            if (sampleFormat == TiffFormat.float) {
              var r = 0.0;
              var g = 0.0;
              var b = 0.0;
              if (bitsPerSample == 32) {
                r = byteData.readFloat32();
                g = byteData.readFloat32();
                b = byteData.readFloat32();
              } else if (bitsPerSample == 64) {
                r = byteData.readFloat64();
                g = byteData.readFloat64();
                b = byteData.readFloat64();
              } else if (bitsPerSample == 16) {
                r = Float16.float16ToDouble(byteData.readUint16());
                g = Float16.float16ToDouble(byteData.readUint16());
                b = Float16.float16ToDouble(byteData.readUint16());
              }
              image.setPixelRgb(px, py, r, g, b);
            } else {
              var r = 0;
              var g = 0;
              var b = 0;
              if (bitsPerSample == 8) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
              } else if (bitsPerSample == 16) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
              } else if (bitsPerSample == 32) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
              }

              image.setPixelRgb(px, py, r, g, b);
            }
          } else if (samplesPerPixel >= 4) {
            if (sampleFormat == TiffFormat.float) {
              var r = 0.0;
              var g = 0.0;
              var b = 0.0;
              var a = 0.0;
              if (bitsPerSample == 32) {
                r = byteData.readFloat32();
                g = byteData.readFloat32();
                b = byteData.readFloat32();
                a = byteData.readFloat32();
              } else if (bitsPerSample == 64) {
                r = byteData.readFloat64();
                g = byteData.readFloat64();
                b = byteData.readFloat64();
                a = byteData.readFloat64();
              } else if (bitsPerSample == 16) {
                r = Float16.float16ToDouble(byteData.readUint16());
                g = Float16.float16ToDouble(byteData.readUint16());
                b = Float16.float16ToDouble(byteData.readUint16());
                a = Float16.float16ToDouble(byteData.readUint16());
              }
              image.setPixelRgba(px, py, r, g, b, a);
            } else {
              var r = 0;
              var g = 0;
              var b = 0;
              var a = 0;
              if (bitsPerSample == 8) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
                a = sampleFormat == TiffFormat.int
                    ? byteData.readInt8()
                    : byteData.readByte();
              } else if (bitsPerSample == 16) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
                a = sampleFormat == TiffFormat.int
                    ? byteData.readInt16()
                    : byteData.readUint16();
              } else if (bitsPerSample == 32) {
                r = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
                g = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
                b = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
                a = sampleFormat == TiffFormat.int
                    ? byteData.readInt32()
                    : byteData.readUint32();
              }

              if (photometricType == TiffPhotometricType.cmyk) {
                final rgba = cmykToRgb(r, g, b, a);
                r = rgba[0];
                g = rgba[1];
                b = rgba[2];
                a = image.maxChannelValue as int;
              }

              image.setPixelRgba(px, py, r, g, b, a);
            }
          }
        }
      }
    } else {
      throw ImageException('Unsupported bitsPerSample: $bitsPerSample');
    }
  }

  void _jpegToImage(Image tile, Image image, int outX, int outY, int tileWidth,
      int tileHeight) {
    final width = tileWidth;
    final height = tileHeight;
    for (var y = 0; y < height; y++) {
      for (var x = 0; x < width; x++) {
        image.setPixel(x + outX, y + outY, tile.getPixel(x, y));
      }
    }
    /*Uint8List data = jpeg.getData(width, height);
    List components = jpeg.components;

    int i = 0;
    int j = 0;
    switch (components.length) {
      case 1: // Luminance
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            int Y = data[i++];
            image.setPixel(x + outX, y + outY, getColor(Y, Y, Y, 255));
          }
        }
        break;
      case 3: // RGB
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            int R = data[i++];
            int G = data[i++];
            int B = data[i++];

            int c = getColor(R, G, B, 255);
            image.setPixel(x + outX, y + outY, c);
          }
        }
        break;
      case 4: // CMYK
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            int C = data[i++];
            int M = data[i++];
            int Y = data[i++];
            int K = data[i++];

            int R = 255 - _clamp(C * (1 - K ~/ 255) + K);
            int G = 255 - _clamp(M * (1 - K ~/ 255) + K);
            int B = 255 - _clamp(Y * (1 - K ~/ 255) + K);

            image.setPixel(x + outX, y + outY, getColor(R, G, B, 255));
          }
        }
        break;
      default:
        throw 'Unsupported color mode';
    }*/
  }

  void _decodeBilevelTile(InputBuffer p, Image image, int tileX, int tileY) {
    final tileIndex = tileY * tilesX + tileX;
    p.offset = tileOffsets![tileIndex];

    final outX = tileX * tileWidth;
    final outY = tileY * tileHeight;

    final byteCount = tileByteCounts![tileIndex];

    InputBuffer byteData;
    if (compression == TiffCompression.packBits) {
      // Since the decompressed data will still be packed
      // 8 pixels into 1 byte, calculate bytesInThisTile
      int bytesInThisTile;
      if ((tileWidth % 8) == 0) {
        bytesInThisTile = (tileWidth ~/ 8) * tileHeight;
      } else {
        bytesInThisTile = (tileWidth ~/ 8 + 1) * tileHeight;
      }
      byteData = InputBuffer(Uint8List(tileWidth * tileHeight));
      _decodePackBits(p, bytesInThisTile, byteData.buffer);
    } else if (compression == TiffCompression.lzw) {
      byteData = InputBuffer(Uint8List(tileWidth * tileHeight));

      LzwDecoder()
          .decode(InputBuffer.from(p, length: byteCount), byteData.buffer);

      // Horizontal Differencing Predictor
      if (predictor == 2) {
        int count;
        for (var j = 0; j < height; j++) {
          count = samplesPerPixel * (j * width + 1);
          for (var i = samplesPerPixel; i < width * samplesPerPixel; i++) {
            byteData[count] += byteData[count - samplesPerPixel];
            count++;
          }
        }
      }
    } else if (compression == TiffCompression.ccittRle) {
      byteData = InputBuffer(Uint8List(tileWidth * tileHeight));
      try {
        TiffFaxDecoder(fillOrder, tileWidth, tileHeight)
            .decode1D(byteData, p, 0, tileHeight);
      } catch (_) {}
    } else if (compression == TiffCompression.ccittFax3) {
      byteData = InputBuffer(Uint8List(tileWidth * tileHeight));
      try {
        TiffFaxDecoder(fillOrder, tileWidth, tileHeight)
            .decode2D(byteData, p, 0, tileHeight, t4Options!);
      } catch (_) {}
    } else if (compression == TiffCompression.ccittFax4) {
      byteData = InputBuffer(Uint8List(tileWidth * tileHeight));
      try {
        TiffFaxDecoder(fillOrder, tileWidth, tileHeight)
            .decodeT6(byteData, p, 0, tileHeight, t6Options!);
      } catch (_) {}
    } else if (compression == TiffCompression.zip) {
      final data = p.toList(0, byteCount);
      final outData = const ZLibDecoder().decodeBytes(data);
      byteData = InputBuffer(outData);
    } else if (compression == TiffCompression.deflate) {
      final data = p.toList(0, byteCount);
      final outData = Inflate(data).getBytes();
      byteData = InputBuffer(outData);
    } else if (compression == TiffCompression.none) {
      byteData = p;
    } else {
      throw ImageException('Unsupported Compression Type: $compression');
    }

    final br = TiffBitReader(byteData);
    final mx = image.maxChannelValue;
    final black = isWhiteZero ? mx : 0;
    final white = isWhiteZero ? 0 : mx;

    for (var y = 0, py = outY; y < tileHeight; ++y, ++py) {
      for (var x = 0, px = outX; x < tileWidth; ++x, ++px) {
        if (py >= image.height || px >= image.width) {
          break;
        }
        if (br.readBits(1) == 0) {
          image.setPixelRgb(px, py, black, 0, 0);
        } else {
          image.setPixelRgb(px, py, white, 0, 0);
        }
      }
      br.flushByte();
    }
  }

  // Uncompress packBits compressed image data.
  void _decodePackBits(InputBuffer data, int arraySize, List<int> dst) {
    var srcCount = 0;
    var dstCount = 0;

    while (dstCount < arraySize) {
      final b = uint8ToInt8(data[srcCount++]);
      if (b >= 0 && b <= 127) {
        // literal run packet
        for (var i = 0; i < (b + 1); ++i) {
          dst[dstCount++] = data[srcCount++];
        }
      } else if (b <= -1 && b >= -127) {
        // 2 byte encoded run packet
        final repeat = data[srcCount++];
        for (var i = 0; i < (-b + 1); ++i) {
          dst[dstCount++] = repeat;
        }
      } else {
        // no-op packet. Do nothing
        srcCount++;
      }
    }
  }

  int _readTag(int type, [int defaultValue = 0]) {
    if (!hasTag(type)) {
      return defaultValue;
    }
    return tags[type]!.read()?.toInt() ?? 0;
  }

  List<int>? _readTagList(int type) {
    if (!hasTag(type)) {
      return null;
    }
    final tag = tags[type]!;
    final value = tag.read()!;
    return List<int>.generate(tag.count, value.toInt);
  }
}
