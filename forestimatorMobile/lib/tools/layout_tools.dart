import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';

Color getColorFromName(String name) {
  List<int> numbers = name.codeUnits;
  int r = 0;
  for (var i in numbers) {
    r += i;
  }
  r = r % 255;
  int g = 1;
  for (var i in numbers) {
    g = g * i;
  }
  g = g % 255;
  int b = 0;
  for (var i in numbers) {
    b = b * i + i;
  }
  b = b % 255;
  return Color.fromRGBO(r, g, b, 0.75);
}

Color getColorTextFromBackground(Color background) {
  List<int> values = [
    (background.r * 255).round(),
    (background.g * 255).round(),
    (background.b * 255).round(),
  ];
  int valueAbove128 = 0;
  for (var value in values) {
    if (value > 127) {
      valueAbove128++;
    }
  }

  if (valueAbove128 < 2) {
    return Colors.white;
  } else {
    return Colors.black;
  }
}

Widget stroke(
  double space,
  double thickness,
  Color color, {
  bool vertical = false,
  double height = 10,
}) {
  return vertical
      ? Row(
        children: [
          SizedBox(width: space),
          Container(
            height: gl.display.equipixel * height,
            width: thickness,
            color: color,
          ),
          SizedBox(width: space),
        ],
      )
      : Column(
        children: [
          SizedBox(height: space),
          Container(height: thickness, color: color),
          SizedBox(height: space),
        ],
      );
}

Widget scanlines() {
  double equiPixelPerLine = gl.display.equipixel * 4;
  return Stack(
    children:
        List<Widget>.generate((gl.display.width / equiPixelPerLine).round(), (
          i,
        ) {
          return Container(
            alignment: AlignmentGeometry.xy(
              (1.0 - (2.0 / (gl.display.width / equiPixelPerLine).round() * i)),
              0,
            ),
            child: Container(
              height: gl.display.height,
              width: 1,
              color: Colors.black,
            ),
          );
        }) +
        List<Widget>.generate((gl.display.height / equiPixelPerLine).round(), (
          i,
        ) {
          return Container(
            alignment: AlignmentGeometry.xy(
              0,
              (1.0 -
                  (2.0 / (gl.display.height / equiPixelPerLine).round() * i)),
            ),
            child: Container(
              height: 1,
              width: gl.display.width,
              color: Colors.black,
            ),
          );
        }),
  );
}
