import 'package:fforestimator/dico/dicoApt.dart';
import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_map/flutter_map.dart';

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
                if (widget.requestedLayers[index].mFoundRastFile) {
                  layerBase l =
                      gl.dico.getLayerBase(widget.requestedLayers[index].mCode);
                  int val = widget.requestedLayers[index].mRastValue;
                  return ListTile(
                    leading: Container(
                      child: CircleAvatar(
                        radius: 25,
                        backgroundColor: l.getValColor(val),
                      ),
                    ),
                    title: Text(l.mNom),
                    subtitle: Text(l.getValLabel(val)),
                  );
                }
              },
              itemCount: widget.requestedLayers.length,
            )
          : Center(
              child: Text("aucune information disponible pour ce point"),
            ),
      // pour la construction du tableau d'aptitude
      //aptsFEE apts = aptsFEE(requestedLayers);
      //drawer: myDrawer(),
    );
  }
}
