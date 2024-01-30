import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';

class CatalogueLayerView extends StatefulWidget {
  CatalogueLayerView({super.key});
  @override
  State<CatalogueLayerView> createState() => _CatalogueLayerView();
}

class _CatalogueLayerView extends State<CatalogueLayerView> {
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
        child: CatalogueView(refreshView: refreshView),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .98,
            minHeight: MediaQuery.of(context).size.height * .6,
            maxHeight: MediaQuery.of(context).size.height * .6),
      ),
      Container(
        child: SelectedLayerView(refreshView: refreshView),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * .98,
            maxHeight: MediaQuery.of(context).size.height * .25),
      )
    ]);
  }

  void refreshView(){
    setState(() {
      
    });
  }
}
