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
        minWidth: gl.onCatalogueWidth * gl.eqPx,
        maxWidth: gl.onCatalogueWidth * gl.eqPx,
        minHeight: gl.eqPx * 10,
        maxHeight: gl.eqPx * 10,
      ),
      child: LinearProgressIndicator(value: 0.5, semanticsLabel: 'Linear progress indicator'),
    );
  }
}
