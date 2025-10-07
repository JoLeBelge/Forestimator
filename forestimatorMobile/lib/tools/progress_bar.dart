import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/material.dart';

class ProgressBar extends StatefulWidget {
  const ProgressBar({super.key});

  @override
  State<ProgressBar> createState() => _ProgressBarState();
}

class _ProgressBarState extends State<ProgressBar> {
  @override
  Widget build(BuildContext context) {
    return Container(
      constraints: BoxConstraints(
        minWidth: gl.onCatalogueWidth * gl.display!.equipixel!,
        maxWidth: gl.onCatalogueWidth * gl.display!.equipixel!,
        minHeight: gl.display!.equipixel! * 10,
        maxHeight: gl.display!.equipixel! * 10,
      ),
      child: LinearProgressIndicator(
        value: 0.5,
        semanticsLabel: 'Linear progress indicator',
      ),
    );
  }
}
