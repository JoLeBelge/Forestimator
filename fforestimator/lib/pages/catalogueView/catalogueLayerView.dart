import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';

class CatalogueLayerView extends StatefulWidget {
  const CatalogueLayerView({super.key});
  @override
  State<CatalogueLayerView> createState() => _CatalogueLayerView();
}

class _CatalogueLayerView extends State<CatalogueLayerView> {
  List<LayerTile> selectedLayer = [];
  @override
  Widget build(BuildContext context) {
    return Column(children: [
      Container(
        color: Colors.grey[200],
        child: SearchBarView(),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * 1.0,
            minHeight: MediaQuery.of(context).size.height * .075,
            maxHeight: MediaQuery.of(context).size.height * .075),
      ),
      Container(
        color: Colors.grey[200],
        child: CatalogueView(refreshView: refreshView),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * 1.0,
            minHeight: MediaQuery.of(context).size.height * .6,
            maxHeight: MediaQuery.of(context).size.height * .6),
      ),
      Container(
        color: Colors.grey[200],
        child: SelectedLayerView(refreshView: refreshView),
        constraints: BoxConstraints(
            maxWidth: MediaQuery.of(context).size.width * 1.0,
            maxHeight: MediaQuery.of(context).size.height * .25),
      )
    ]);
  }

  void refreshView(){
    setState(() {
      
    });
  }
}
