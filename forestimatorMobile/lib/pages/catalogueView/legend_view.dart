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
  BoxConstraints _constraintsLeft = BoxConstraints(),
      _constraintsRight = BoxConstraints();
  int _magicNumber = 1;
  @override
  Widget build(BuildContext context) {
    _computeBoxContraintsPerColorTile();
    return Container(
      alignment: AlignmentGeometry.topCenter,
      color: Colors.white,
      constraints: BoxConstraints(
        minWidth:
            widget.constraintsText.minWidth + widget.constraintsColors.minWidth,
        maxWidth:
            widget.constraintsText.maxWidth + widget.constraintsColors.maxWidth,
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          if (gl.dico.getLayerBase(widget.layerKey).mCategorie != "Externe")
            Container(
              constraints: BoxConstraints(
                maxHeight: gl.fontSizeM * gl.display.equipixel,
              ),
              child: const Text('Légende'),
            ),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: List.generate(
                  gl.dico.mLayerBases[widget.layerKey]!
                      .getDicoValForLegend()
                      .length,
                  (i) {
                    var key = gl.dico.mLayerBases[widget.layerKey]!
                        .getDicoValForLegend()
                        .elementAt(i);
                    if (_graduatedMode) {
                      if (i % _magicNumber == 0 ||
                          i ==
                              gl
                                      .dico
                                      .mLayerBases[widget.layerKey]!
                                      .mDicoVal
                                      .length -
                                  1) {
                        return Container(
                          alignment: AlignmentGeometry.topCenter,
                          constraints: BoxConstraints(
                            minWidth: 0,
                            maxWidth: _constraintsLeft.maxWidth,
                            minHeight: 0,
                            maxHeight: gl.fontSizeS * gl.display.equipixel,
                          ),
                          child: Text(
                            gl
                                .dico
                                .mLayerBases[widget.layerKey]!
                                .mDicoVal[key]!,
                            style: TextStyle(
                              fontSize: gl.display.equipixel * gl.fontSizeXS,
                              fontWeight: FontWeight.w300,
                            ),
                          ),
                        );
                      } else {
                        return SizedBox(height: 0.3);
                      }
                    } else {
                      return Container(
                        constraints: _constraintsLeft,
                        child: Text(
                          gl.dico.mLayerBases[widget.layerKey]!.mDicoVal[key]!,
                          style: TextStyle(
                            fontSize: gl.display.equipixel * gl.fontSizeXS,
                            fontWeight: FontWeight.w300,
                          ),
                        ),
                      );
                    }
                  },
                ),
              ),
              Column(
                mainAxisAlignment: MainAxisAlignment.end,
                children: List.generate(
                  gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length,
                  (i) {
                    var key = gl
                        .dico
                        .mLayerBases[widget.layerKey]!
                        .mDicoCol
                        .keys
                        .elementAt(i);
                    return Container(
                      constraints: BoxConstraints(
                        minWidth: 0,
                        maxWidth: _constraintsRight.maxWidth,
                        minHeight: 0,
                        maxHeight: _constraintsRight.maxHeight,
                      ),
                      color:
                          gl.dico.mLayerBases[widget.layerKey]!.mDicoCol[key]!,
                    );
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  void _computeBoxContraintsPerColorTile() {
    if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 300) {
      double heightPerColorTile = gl.display.equipixel * 0.06;
      double graduatedHeight = gl.display.equipixel * 2;
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
      double heightPerColorTile = gl.display.equipixel * 0.5;
      double graduatedHeight =
          gl.display.equipixel *
          gl.display.equiheight *
          gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length /
          11.0 *
          .005;
      _magicNumber =
          (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
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
      double heightPerColorTile = gl.display.equipixel * 1.5;
      double graduatedHeight =
          gl.display.equipixel *
          gl.display.equiheight *
          gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length /
          13.0 *
          .016;
      _magicNumber =
          (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
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
      double heightPerColorTile = gl.display.equipixel * 4;
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

  BoxConstraints _initConstraints(double x, x_, y, y_) {
    return BoxConstraints(
      minHeight: x,
      maxHeight: x_,
      minWidth: y,
      maxWidth: y_,
    );
  }

  void refreshView() {
    setState(() {});
  }
}
