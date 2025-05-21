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
      appBar:
          gl.offlineMode
              ? AppBar(
                title: const Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      "Forestimator offline/terrain",
                      textScaler: TextScaler.linear(0.75),
                      style: TextStyle(color: Colors.black),
                    ),
                  ],
                ),
                toolbarHeight: 20.0,
                backgroundColor: gl.colorAgroBioTech,
              )
              : AppBar(
                title: const Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      "Forestimator online",
                      textScaler: TextScaler.linear(0.75),
                      style: TextStyle(color: Colors.white),
                    ),
                  ],
                ),
                toolbarHeight: 20.0,
                backgroundColor: gl.colorUliege,
              ),
      body: Column(
        mainAxisAlignment: MainAxisAlignment.end,
        children: [
          Container(
            color: gl.colorBackground,
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              minHeight: MediaQuery.of(context).size.height * .67,
              maxHeight: MediaQuery.of(context).size.height * .67,
            ),
            child: CatalogueView(refreshView: refreshView),
          ),
          Container(
            color: gl.colorBackground,
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              maxHeight: MediaQuery.of(context).size.height * .18,
            ),
            child: SelectedLayerView(refreshView: refreshView),
          ),
        ],
      ),
    );
  }

  void refreshView() {
    setState(() {});
  }
}
