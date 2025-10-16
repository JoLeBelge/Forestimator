import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;

class LayerTile {
  LayerTile({
    required this.key,
    required this.name,
    required this.filter,
    this.downloadable = false,
    this.isExpanded = false,
    this.selected = false,
    this.extern = false,
    this.bits = 8,
  });

  String key;
  String name;
  String filter;
  bool isExpanded;
  bool selected;
  bool extern;
  bool downloadable;
  int bits;

  ListTile attribution() {
    return ListTile(
      subtitle: const Text("Propriétaire de la couche cartographique"),
      title: Text(gl.dico.getLayerBase(key).mWMSattribution),
      leading: Container(
        constraints: BoxConstraints(
          maxWidth: gl.display.equipixel * gl.onCatalogueWidth * .75,
          maxHeight: gl.display.equipixel * gl.onCatalogueWidth * .75,
        ),
        child: Image.asset(
          gl.dico.getLayerBase(key).mLogoAttributionFile,
          fit: BoxFit.cover,
        ),
      ),
      trailing: const Icon(Icons.copyright),
    );
  }

  Container proprietaire() {
    return Container(
      alignment: Alignment.topCenter,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            alignment: Alignment.center,
            constraints: BoxConstraints(
              maxWidth: gl.display.equipixel * gl.onCatalogueWidth * .8,
            ),
            child: Text(
              "Propriétaire de la couche cartographique",
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: gl.display.equipixel * gl.fontSizeS),
            ),
          ),
          Container(
            alignment: Alignment.center,
            constraints: BoxConstraints(
              maxWidth: gl.display.equipixel * gl.onCatalogueWidth * .75,
            ),
            child: Image.asset(
              gl.dico.getLayerBase(key).mLogoAttributionFile,
              fit: BoxFit.cover,
            ),
          ),
          Container(
            alignment: Alignment.center,
            constraints: BoxConstraints(
              maxWidth: gl.display.equipixel * gl.onCatalogueWidth * .75,
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(Icons.copyright),
                Text("  "),
                Text(gl.dico.getLayerBase(key).mWMSattribution),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget downloadedControlBar() {
    return Container(
      constraints: const BoxConstraints(
        maxWidth: 150,
        minHeight: 32,
        maxHeight: 32,
      ),
      child:
          gl.dico.getLayerBase(key).mInDownload
              ? const Text(
                "en attente",
                style: TextStyle(color: Color.fromARGB(255, 155, 124, 38)),
              )
              : gl.dico.getLayerBase(key).mOffline
              ? const Text(
                "Enregistré",
                style: TextStyle(color: gl.colorAgroBioTech),
              )
              : gl.dico.getLayerBase(key).mIsDownloadableRW
              ? const Text(
                "Téléchargeable",
                style: TextStyle(color: gl.colorUliege),
              )
              : const Text(""),
    );
  }
}
