import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

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
  List<int> values = [(background.r * 255).round(), (background.g * 255).round(), (background.b * 255).round()];
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

Widget stroke(double space, double thickness, Color color, {bool vertical = false, double height = 10}) {
  return vertical
      ? Row(
        children: [
          SizedBox(width: space),
          Container(height: gl.display.equipixel * height, width: thickness, color: color),
          SizedBox(width: space),
        ],
      )
      : Column(
        children: [SizedBox(height: space), Container(height: thickness, color: color), SizedBox(height: space)],
      );
}

Widget gridlines() {
  double equiPixelPerLine = gl.display.equipixel * 4;
  return Stack(
    children:
        List<Widget>.generate((gl.display.width / equiPixelPerLine).round(), (i) {
          return Container(
            alignment: AlignmentGeometry.xy((1.0 - (2.0 / (gl.display.width / equiPixelPerLine).round() * i)), 0),
            child: Container(height: gl.display.height, width: 1, color: Colors.black),
          );
        }) +
        List<Widget>.generate((gl.display.height / equiPixelPerLine).round(), (i) {
          return Container(
            alignment: AlignmentGeometry.xy(0, (1.0 - (2.0 / (gl.display.height / equiPixelPerLine).round() * i))),
            child: Container(height: 1, width: gl.display.width, color: Colors.black),
          );
        }),
  );
}

ButtonStyle borderlessStyle = ButtonStyle(
  animationDuration: Duration(seconds: 1),
  backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
    WidgetState.any: Colors.transparent,
  }),
  padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
    WidgetState.any: EdgeInsetsGeometry.zero,
  }),
);

class ValidTextField extends StatefulWidget {
  final VoidCallback onTap;
  final VoidCallback onEditingComplete;
  final bool Function(String) validate;
  final ValueChanged<String> onValid;
  final ValueChanged<String> onInvalid;
  final Color validColor;
  final Color invalidColor;
  final double width;
  final double height;
  final int maxLength;
  final String initialValue;

  const ValidTextField({
    super.key,
    required this.width,
    required this.height,
    required this.validate,
    required this.onValid,
    required this.onInvalid,
    required this.onTap,
    required this.onEditingComplete,
    this.validColor = Colors.white,
    this.invalidColor = Colors.red,
    this.maxLength = 255,
    this.initialValue = "",
  });

  @override
  State<StatefulWidget> createState() => _ValidTextField();
}

class _ValidTextField extends State<ValidTextField> {
  bool _valid = true;
  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: widget.width,
      height: widget.height,
      child: TextFormField(
        initialValue: widget.initialValue,
        maxLength: widget.maxLength,
        maxLengthEnforcement: MaxLengthEnforcement.enforced,
        onChanged: (String value) {
          setState(() {
            _valid = widget.validate(value);
          });
          _valid ? widget.onValid(value) : widget.onInvalid(value);
        },
        onEditingComplete: widget.onEditingComplete,
        onTap: () => widget.onTap,
        style: TextStyle(color: _valid ? widget.validColor : widget.invalidColor),
      ),
    );
  }
}
