import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:fforestimator/globals.dart' as gl;

class anaPtpage extends StatefulWidget {
  List<layerAnaPt> requestedLayers = [];
  @override
  anaPtpage(this.requestedLayers);
  State<anaPtpage> createState() => _anaPtpageState();
}

class _anaPtpageState extends State<anaPtpage> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.grey[200],
      appBar: AppBar(
        title: Text("analyse pour cette position"),
      ),
      body: widget.requestedLayers.length > 0
          ? ListView.builder(
              itemBuilder: (context, index) {
                return ListTile(
                  title: Text(gl.dico
                      .getLayerBase(widget.requestedLayers[index].mCode)
                      .mNom),
                );
              },
              itemCount: widget.requestedLayers.length,
            )
          : Center(
              child: Text("auncune information disponible pour ce point"),
            ),
      // pour la construction du tableau d'aptitude
      //aptsFEE apts = aptsFEE(requestedLayers);
      //drawer: myDrawer(),
    );
  }
}
