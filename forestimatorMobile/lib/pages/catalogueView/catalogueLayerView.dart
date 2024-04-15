import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;

class CatalogueLayerView extends StatefulWidget {
  const CatalogueLayerView({super.key});
  @override
  State<CatalogueLayerView> createState() => _CatalogueLayerView();
}

class _CatalogueLayerView extends State<CatalogueLayerView> {
  List<LayerTile> selectedLayer = [];
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(mainAxisAlignment: MainAxisAlignment.end, children: [
        Container(
          color: gl.colorBackground,
          child: CatalogueView(refreshView: refreshView),
          constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              minHeight: MediaQuery.of(context).size.height * .75,
              maxHeight: MediaQuery.of(context).size.height * .75),
        ),
        Container(
          color: gl.colorBackground,
          child: SelectedLayerView(refreshView: refreshView),
          constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              maxHeight: MediaQuery.of(context).size.height * .175),
        )
      ]),
    );
  }

  void refreshView() {
    setState(() {});
  }
}
