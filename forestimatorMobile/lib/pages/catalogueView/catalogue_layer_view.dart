import 'package:fforestimator/pages/catalogueView/catalogue_view.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:shared_preferences/shared_preferences.dart';

class CatalogueLayerView extends StatefulWidget {
  const CatalogueLayerView({super.key});
  @override
  State<CatalogueLayerView> createState() => _CatalogueLayerView();
}

class _CatalogueLayerView extends State<CatalogueLayerView> {
  List<LayerTile> selectedLayer = [];
  @override
  Widget build(BuildContext context) {
    return gl.firstTimeUse
        ? PopupNotification(
          title: "Bienvenu",
          accept: "oui",
          onAccept: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            final SharedPreferences prefs =
                await SharedPreferences.getInstance();
            await prefs.setBool('firstTimeUse', gl.firstTimeUse);
            for (var key in gl.downloadableLayerKeys) {
              downloadLayer(key);
            }
          },
          decline: "non",
          onDecline: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            final SharedPreferences prefs =
                await SharedPreferences.getInstance();
            await prefs.setBool('firstTimeUse', gl.firstTimeUse);
          },
          dialog:
              "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Autorisez-vous l'aplication à télécharger un jeu de 7 couches pour une utilisation hors ligne? Ces couches couvrent toutes la Région Wallonne et totalisent +- 100 Mo.",
        )
        : Scaffold(
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
