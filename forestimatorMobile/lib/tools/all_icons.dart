import 'package:flutter/material.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';

class FIcon extends StatelessWidget {
  final dynamic icon;
  final double? size;
  final double? opticalSize;
  final Color? color;
  final double? weight;
  final List<Shadow>? shadows;
  const FIcon(this.icon, {super.key, this.shadows, this.size, this.color, this.weight, this.opticalSize});

  @override
  Widget build(BuildContext context) => icon is FaIconData
      ? FaIcon(icon, size: size, color: color, weight: weight, shadows: shadows, opticalSize: opticalSize)
      : Icon(icon, size: size, color: color, weight: weight, shadows: shadows, opticalSize: opticalSize);
}
