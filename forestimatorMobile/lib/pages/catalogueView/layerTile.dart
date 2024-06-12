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
      leading: ConstrainedBox(
        constraints: const BoxConstraints(
          minWidth: 44,
          minHeight: 44,
          maxWidth: 200,
          maxHeight: 64,
        ),
        child: Image.asset(gl.dico.getLayerBase(key).mLogoAttributionFile,
            fit: BoxFit.cover),
      ),
      trailing: const Icon(Icons.copyright),
    );
  }

  Widget downloadedControlBar() {
    return Container(
        constraints:
            const BoxConstraints(maxWidth: 150, minHeight: 32, maxHeight: 32),
        child: gl.dico.getLayerBase(key).mOffline
            ? const Text(
                "Enregistré",
                style: TextStyle(color: gl.colorAgroBioTech),
              )
            : gl.dico.getLayerBase(key).mIsDownloadableRW
                ? const Text(
                    "Téléchargeable",
                    style: TextStyle(color: gl.colorUliege),
                  )
                : const Text(
                    "",
                  ));
  }
}
