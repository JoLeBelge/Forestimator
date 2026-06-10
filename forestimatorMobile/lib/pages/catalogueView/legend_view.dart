import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;

class LegendView extends StatefulWidget {
  final String layerKey;
  final BoxConstraints constraintsText, constraintsColors;
  final Color color;
  const LegendView({
    required this.layerKey,
    required this.constraintsText,
    required this.constraintsColors,
    required this.color,
    super.key,
  });
  @override
  State<LegendView> createState() => _LegendView();
}

class _LegendView extends State<LegendView> {
  bool _graduatedMode = false;
  BoxConstraints _constraintsLeft = BoxConstraints(), _constraintsRight = BoxConstraints();
  int _magicNumber = 1;

  @override
  Widget build(BuildContext context) {
    _computeBoxContraintsPerColorTile();
    return Container(
      alignment: AlignmentGeometry.topCenter,
      color: Colors.white,
      constraints: BoxConstraints(
        minWidth: widget.constraintsText.minWidth + widget.constraintsColors.minWidth,
        maxWidth: widget.constraintsText.maxWidth + widget.constraintsColors.maxWidth,
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          if (gl.dico.getLayerBase(widget.layerKey).mCategorie != "Externe")
            Container(constraints: BoxConstraints(maxHeight: gl.fontSizeM * gl.eqPx), child: const Text('Légende')),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: List.generate(gl.dico.mLayerBases[widget.layerKey]!.getDicoValForLegend().length, (i) {
                  var key = gl.dico.mLayerBases[widget.layerKey]!.getDicoValForLegend().elementAt(i);
                  if (_graduatedMode) {
                    if (i % _magicNumber == 0 || i == gl.dico.mLayerBases[widget.layerKey]!.mDicoVal.length - 1) {
                      return Container(
                        color: i % 2 == 1 ? Colors.white10 : Color.fromRGBO(220, 220, 220, 1),
                        alignment: AlignmentGeometry.centerLeft,
                        constraints: BoxConstraints(
                          minWidth: 0,
                          maxWidth: _constraintsLeft.maxWidth * 1.2,
                          minHeight: 0,
                          maxHeight: _constraintsLeft.maxHeight * 1.8,
                        ),
                        child: Text(
                          gl.dico.mLayerBases[widget.layerKey]!.mDicoVal[key]!,
                          style: TextStyle(fontSize: gl.eqPx * gl.fontSizeXXS, fontWeight: FontWeight.w500),
                        ),
                      );
                    } else {
                      return SizedBox(height: 0.3);
                    }
                  } else {
                    return Container(
                      alignment: AlignmentGeometry.centerLeft,
                      color: i % 2 == 1 ? Colors.white10 : Color.fromRGBO(220, 220, 220, 1),
                      constraints: BoxConstraints(
                        minWidth: 0,
                        maxWidth: _constraintsLeft.maxWidth * 1.2,
                        minHeight: 0,
                        maxHeight: _constraintsLeft.maxHeight * 1.8,
                      ),
                      child: Text(
                        gl.dico.mLayerBases[widget.layerKey]!.mDicoVal[key]!,
                        style: TextStyle(fontSize: gl.eqPx * gl.fontSizeXXS, fontWeight: FontWeight.w500),
                      ),
                    );
                  }
                }),
              ),
              Column(
                mainAxisAlignment: MainAxisAlignment.end,
                children: List.generate(gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length, (i) {
                  var key = gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.keys.elementAt(i);
                  return Container(
                    constraints: BoxConstraints(
                      minWidth: 0,
                      maxWidth: _constraintsRight.maxWidth * .75,
                      minHeight: 0,
                      maxHeight: _constraintsRight.maxHeight * 1.8,
                    ),
                    color: gl.dico.mLayerBases[widget.layerKey]!.mDicoCol[key]!,
                  );
                }),
              ),
            ],
          ),
        ],
      ),
    );
  }

  void _computeBoxContraintsPerColorTile() {
    if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 300) {
      double heightPerColorTile = gl.eqPx * 0.06;
      double graduatedHeight = gl.eqPx * 2;
      _magicNumber = 37;
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(
        graduatedHeight,
        graduatedHeight,
        widget.constraintsText.maxWidth * 1.15,
        widget.constraintsText.maxWidth * 1.15,
      );
      _constraintsRight = _initConstraints(
        heightPerColorTile,
        heightPerColorTile,
        widget.constraintsColors.maxWidth * 0.85,
        widget.constraintsColors.maxWidth * 0.85,
      );
    } else if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 50) {
      double heightPerColorTile = gl.eqPx * .98;
      double graduatedHeight =
          gl.eqPx * gl.dsp.equiheight * gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 11.0 * .005;
      _magicNumber = (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(
        graduatedHeight,
        graduatedHeight,
        widget.constraintsText.maxWidth,
        widget.constraintsText.maxWidth,
      );
      _constraintsRight = _initConstraints(
        heightPerColorTile,
        heightPerColorTile,
        widget.constraintsColors.maxWidth,
        widget.constraintsColors.maxWidth,
      );
    } else if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 40) {
      double heightPerColorTile = gl.eqPx * 1.5;
      double graduatedHeight =
          gl.eqPx * gl.dsp.equiheight * gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 13.0 * .016;
      _magicNumber = (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(
        graduatedHeight,
        graduatedHeight,
        widget.constraintsText.maxWidth,
        widget.constraintsText.maxWidth,
      );
      _constraintsRight = _initConstraints(
        heightPerColorTile,
        heightPerColorTile,
        widget.constraintsColors.maxWidth,
        widget.constraintsColors.maxWidth,
      );
    } else {
      _graduatedMode = false;
      double heightPerColorTile = gl.eqPx * 4;
      _constraintsLeft = _initConstraints(
        heightPerColorTile,
        heightPerColorTile,
        widget.constraintsText.maxWidth,
        widget.constraintsText.maxWidth,
      );
      _constraintsRight = _initConstraints(
        heightPerColorTile,
        heightPerColorTile,
        widget.constraintsColors.maxWidth,
        widget.constraintsColors.maxWidth,
      );
    }
  }

  BoxConstraints _initConstraints(double x, double x_, double y, double y_) {
    return BoxConstraints(minHeight: x, maxHeight: x_, minWidth: y, maxWidth: y_);
  }

  void refreshView() {
    setState(() {});
  }
}
