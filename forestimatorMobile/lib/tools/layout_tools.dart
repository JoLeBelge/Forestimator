import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/layout_tools.dart' as lt;
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
          Container(height: gl.eqPx * height, width: thickness, color: color),
          SizedBox(width: space),
        ],
      )
      : Column(
        children: [SizedBox(height: space), Container(height: thickness, color: color), SizedBox(height: space)],
      );
}

Widget gridlines() {
  double equiPixelPerLine = gl.eqPx * 4;
  return Stack(
    children:
        List<Widget>.generate((gl.dsp.width / equiPixelPerLine).round(), (i) {
          return Container(
            alignment: AlignmentGeometry.xy((1.0 - (2.0 / (gl.dsp.width / equiPixelPerLine).round() * i)), 0),
            child: Container(height: gl.dsp.height, width: 1, color: Colors.black),
          );
        }) +
        List<Widget>.generate((gl.dsp.height / equiPixelPerLine).round(), (i) {
          return Container(
            alignment: AlignmentGeometry.xy(0, (1.0 - (2.0 / (gl.dsp.height / equiPixelPerLine).round() * i))),
            child: Container(height: 1, width: gl.dsp.width, color: Colors.black),
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

ButtonStyle get trNoPadButtonstyle => ButtonStyle(
  animationDuration: Duration(seconds: 1),
  backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
    WidgetState.any: Colors.transparent,
  }),
  padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
    WidgetState.any: EdgeInsetsGeometry.zero,
  }),
);

ButtonStyle get onOflineButtonstyle => ButtonStyle(
  animationDuration: Duration(seconds: 1),
  backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
    WidgetState.any: gl.offlineMode ? gl.colorAgroBioTech : gl.colorUliege,
  }),
  fixedSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
    WidgetState.any: Size(gl.eqPx * 40, gl.eqPx * 10),
  }),
  padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
    WidgetState.any: EdgeInsetsGeometry.zero,
  }),
);

ButtonStyle get essenceButtonstyle => ButtonStyle(
  animationDuration: Duration(seconds: 1),
  backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
    WidgetState.any: gl.Mode.essence ? gl.colorAgroBioTech : gl.colorUliege,
  }),
  fixedSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
    WidgetState.any: Size(gl.eqPx * 40, gl.eqPx * 10),
  }),
  padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
    WidgetState.any: EdgeInsetsGeometry.zero,
  }),
);

class ForestimatorScrollView extends StatefulWidget {
  const ForestimatorScrollView({
    super.key,
    this.height,
    this.width,
    required this.child,
    this.reverse = false,
    this.horizontal = false,
    this.arrowColor,
    this.sizeArrows,
  });
  final double? height;
  final double? width;
  final Widget child;
  final bool reverse;
  final bool horizontal;
  final Color? arrowColor;
  final double? sizeArrows;

  @override
  State<StatefulWidget> createState() => _ForestimatorScrollView();
}

class _ForestimatorScrollView extends State<ForestimatorScrollView> {
  final ScrollController _controller = ScrollController();

  bool _atTop = true;
  bool _atBottom = true;

  @override
  void initState() {
    super.initState();
    _controller.addListener(() {
      setState(() {
        _atBottom = _atTop = false;
        if (_controller.position.maxScrollExtent < 0) {
          return;
        }
        if (_controller.offset < 10) {
          _atTop = true;
        }
        if (_controller.position.maxScrollExtent - _controller.offset < 10) {
          _atBottom = true;
        }
      });
    });
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _controller.jumpTo(2);
    });
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    double width = widget.width ?? gl.eqPx * 65;
    double height = widget.height ?? gl.eqPx * 20;
    double sizeArrows = widget.sizeArrows ?? width * .1;
    return Container(
      padding: EdgeInsets.zero,
      alignment: Alignment.center,
      width: width,
      height: height,
      child: Stack(
        children: [
          SingleChildScrollView(
            controller: _controller,
            padding: EdgeInsets.zero,
            reverse: widget.reverse,
            scrollDirection: widget.horizontal ? Axis.horizontal : Axis.vertical,
            child: widget.child,
          ),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Container(
                alignment: Alignment.centerLeft,
                height: height,
                width: sizeArrows,
                child:
                    !_atTop
                        ? Icon(Icons.arrow_left_outlined, color: widget.arrowColor ?? Colors.red, size: sizeArrows)
                        : Container(),
              ),
              Container(
                alignment: Alignment.centerRight,
                height: height,
                width: sizeArrows,
                child:
                    !_atBottom
                        ? Icon(Icons.arrow_right_outlined, color: widget.arrowColor ?? Colors.red, size: sizeArrows)
                        : Container(),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

Widget forestimatorBackButton(VoidCallback after, IconData icon) {
  return Stack(
    alignment: AlignmentGeometry.center,
    children: [
      CircleAvatar(
        radius: gl.eqPx * (gl.iconSizeXS - 1.5),
        backgroundColor: Colors.white,
        child: CircleAvatar(radius: gl.eqPx * (gl.iconSizeXS - 1.75), backgroundColor: gl.colorAgroBioTech),
      ),
      IconButton(
        alignment: Alignment.center,
        style: lt.trNoPadButtonstyle,
        onPressed: () {
          after();
        },
        icon: Icon(icon, color: Colors.black, size: gl.eqPx * gl.iconSizeS),
      ),
    ],
  );
}
