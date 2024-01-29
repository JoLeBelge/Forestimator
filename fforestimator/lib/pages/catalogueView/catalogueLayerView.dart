import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';

class CatalogueLayerView extends StatelessWidget {
  List<LayerTile> selectedLayer = [];
  @override
  Widget build(BuildContext context) {
    return Container(
        child: Column(children: [
      const CatalogueView(),
      Container(
        child: SelectedLayerView(selectedLayer: selectedLayer),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .95,
            maxHeight: MediaQuery.of(context).size.height * .5),
      )
    ]));
  }
}
