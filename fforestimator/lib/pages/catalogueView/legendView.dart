import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:go_router/go_router.dart';

class LegendView extends StatefulWidget {
  final String layerKey;
  final BoxConstraints constraintsText, constraintsColors;
  final Color color;
  const LegendView(
      {required this.layerKey,
      required this.constraintsText,
      required this.constraintsColors,
      required this.color,
      super.key});
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

    return Center(
        child: Container(
      color: widget.color,
      constraints: BoxConstraints(
        minWidth:
            widget.constraintsText.minWidth + widget.constraintsColors.minWidth,
        maxWidth:
            widget.constraintsText.maxWidth + widget.constraintsColors.maxWidth,
      ),
      child: Column(
        children: [
          if (gl.dico.getLayerBase(widget.layerKey).hasDoc())
            ListTile(
              title: Text(
                  "Consulter la documentation relative à la cette couche cartographique"),
              leading: IconButton(
                  onPressed: () {
                    GoRouter.of(context).pushNamed(widget.layerKey, pathParameters: {
                      'currentPage': gl.dico
                          .getLayerBase(widget.layerKey)
                          .mPdfPage
                          .toString()
                    });
                  },
                  icon: Icon(Icons.picture_as_pdf)),
            ),
          if ((gl.dico.getLayerBase(widget.layerKey).mGroupe == "APT_FEE" ||
                  gl.dico.getLayerBase(widget.layerKey).mGroupe == "APT_CS") &&
              gl.dico
                  .getEss(gl.dico.getLayerBase(widget.layerKey).getEssCode())
                  .hasFEEapt())
            ListTile(
              title: Text("Consulter la fiche-essence " +
                  gl.dico
                      .getEss(
                          gl.dico.getLayerBase(widget.layerKey).getEssCode())
                      .getNameAndPrefix()),
              leading: IconButton(
                  onPressed: () {
                    context.go(gl.dico
                        .getEss(
                            gl.dico.getLayerBase(widget.layerKey).getEssCode())
                        .getFicheRoute(complete: true));
                  },
                  icon: Icon(Icons.picture_as_pdf)),
            ),
          Container(
              constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.height * .03),
              child: const Text('Légende')),
          Row(children: [
            Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: List.generate(
                    gl.dico.mLayerBases[widget.layerKey]!
                        .getDicoValForLegend()
                        .length, (i) {
                  var key = gl.dico.mLayerBases[widget.layerKey]!
                      .getDicoValForLegend()
                      .elementAt(i);
                  if (_graduatedMode) {
                    if (i % _magicNumber == 0 ||
                        i ==
                            gl.dico.mLayerBases[widget.layerKey]!.mDicoVal
                                    .length -
                                1) {
                      return Container(
                        constraints: _constraintsLeft,
                        child: Text(gl
                            .dico.mLayerBases[widget.layerKey]!.mDicoVal[key]!),
                      );
                    } else {
                      return Container(
                        constraints: BoxConstraints(
                            minWidth: _constraintsLeft.minWidth,
                            maxWidth: _constraintsLeft.maxWidth,
                            minHeight: 0.0,
                            maxHeight: 0.01),
                      );
                    }
                  } else {
                    return Container(
                      constraints: _constraintsLeft,
                      child: Text(
                          gl.dico.mLayerBases[widget.layerKey]!.mDicoVal[key]!),
                    );
                  }
                })),
            Column(
                children: List.generate(
                    gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length, (i) {
              var key = gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.keys
                  .elementAt(i);
              return Container(
                color: gl.dico.mLayerBases[widget.layerKey]!.mDicoCol[key]!,
                constraints: _constraintsRight,
              );
            })),
          ]),
        ],
      ),
    ));
  }

  void _computeBoxContraintsPerColorTile() {
    if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 300) {
      double heightPerColorTile = MediaQuery.of(context).size.height * .0006;
      double graduatedHeight = MediaQuery.of(context).size.height * .02;
      _magicNumber = 37;
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(
          graduatedHeight,
          graduatedHeight,
          widget.constraintsText.maxWidth * 1.15,
          widget.constraintsText.maxWidth * 1.15);
      _constraintsRight = _initConstraints(
          heightPerColorTile,
          heightPerColorTile,
          widget.constraintsColors.maxWidth * 0.85,
          widget.constraintsColors.maxWidth * 0.85);
    } else if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 50) {
      double heightPerColorTile = MediaQuery.of(context).size.height * .003;
      double graduatedHeight = MediaQuery.of(context).size.height *
          gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length /
          11.0 *
          .003;
      _magicNumber =
          (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(graduatedHeight, graduatedHeight,
          widget.constraintsText.maxWidth, widget.constraintsText.maxWidth);
      _constraintsRight = _initConstraints(
          heightPerColorTile,
          heightPerColorTile,
          widget.constraintsColors.maxWidth,
          widget.constraintsColors.maxWidth);
    } else if (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length > 30) {
      double heightPerColorTile = MediaQuery.of(context).size.height * .01;
      double graduatedHeight = MediaQuery.of(context).size.height *
          gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length /
          13.0 *
          .01;
      _magicNumber =
          (gl.dico.mLayerBases[widget.layerKey]!.mDicoCol.length / 10).toInt();
      _graduatedMode = true;
      _constraintsLeft = _initConstraints(graduatedHeight, graduatedHeight,
          widget.constraintsText.maxWidth, widget.constraintsText.maxWidth);
      _constraintsRight = _initConstraints(
          heightPerColorTile,
          heightPerColorTile,
          widget.constraintsColors.maxWidth,
          widget.constraintsColors.maxWidth);
    } else {
      _graduatedMode = false;
      double heightPerColorTile = MediaQuery.of(context).size.height * .02;
      _constraintsLeft = _initConstraints(
          heightPerColorTile,
          heightPerColorTile,
          widget.constraintsText.maxWidth,
          widget.constraintsText.maxWidth);
      _constraintsRight = _initConstraints(
          heightPerColorTile,
          heightPerColorTile,
          widget.constraintsColors.maxWidth,
          widget.constraintsColors.maxWidth);
    }
  }

  BoxConstraints _initConstraints(x, x_, y, y_) {
    return BoxConstraints(
        minHeight: x, maxHeight: x_, minWidth: y, maxWidth: y_);
  }

  void refreshView() {
    setState(() {});
  }
}
