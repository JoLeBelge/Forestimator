import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';

class CatalogueLayerView extends StatelessWidget {
  List<LayerTile> selectedLayer = [];
  @override
  Widget build(BuildContext context) {
    return Column(children: [
      Container(
        child: SearchBarView(),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .98,
            minHeight: MediaQuery.of(context).size.height * .075,
            maxHeight: MediaQuery.of(context).size.height * .075),
      ),
      Container(
        child: CatalogueView(),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .98,
            minHeight: MediaQuery.of(context).size.height * .6,
            maxHeight: MediaQuery.of(context).size.height * .6),
      ),
      Container(
        child: SelectedLayerView(selectedLayer: selectedLayer),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .98,
            maxHeight: MediaQuery.of(context).size.height * .25),
      )
    ]);
  }
}
