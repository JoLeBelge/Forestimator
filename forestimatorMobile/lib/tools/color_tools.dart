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
