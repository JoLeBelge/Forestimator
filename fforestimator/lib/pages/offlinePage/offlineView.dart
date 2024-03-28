import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:fforestimator/tools/layerDownloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:go_router/go_router.dart';

class OfflineView extends StatefulWidget {
  const OfflineView({super.key});
  @override
  State<OfflineView> createState() => _OfflineView();
}

bool _finishedInitializingCategory = false;

class _OfflineView extends State<OfflineView> {
  final List<Category> _categories = [
    Category(name: "Couches téléchargées.", filter: "offline")
  ];
  final List<LayerTile> _downlodableLayerTiles = [];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(mainAxisAlignment: MainAxisAlignment.start, children: [
        gl.offlineMode
            ? Container(
                color: gl.colorBackgroundSecondary,
                constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * 1.0,
                    minHeight: MediaQuery.of(context).size.height * .15,
                    maxHeight: MediaQuery.of(context).size.height * .15),
                child: TextButton.icon(
                  onPressed: () {
                    setState(() {
                      gl.offlineMode = false;
                    });
                  },
                  icon: Icon(
                    Icons.download_for_offline,
                    color: gl.colorUliege,
                  ),
                  label: Text(
                    "Desactivez le mode hors ligne.",
                    style: TextStyle(color: gl.colorUliege),
                  ),
                ),
              )
            : Container(
                color: gl.colorBackgroundSecondary,
                constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * 1.0,
                    minHeight: MediaQuery.of(context).size.height * .15,
                    maxHeight: MediaQuery.of(context).size.height * .15),
                child: TextButton.icon(
                  onPressed: () {
                    gl.rebuildWholeWidgetTree(() {
                      setState(() {
                        gl.offlineMode = true;
                      });
                    });
                  },
                  icon: Icon(
                    Icons.download_for_offline,
                    color: gl.colorAgroBioTech,
                  ),
                  label: Text(
                    "Activez le mode hors ligne.",
                    style: TextStyle(color: gl.colorAgroBioTech),
                  ),
                ),
              ),
        Container(
          color: gl.colorBackground,
          constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              minHeight: MediaQuery.of(context).size.height * .75,
              maxHeight: MediaQuery.of(context).size.height * .75),
          child: SingleChildScrollView(
            child: _buildOfflineCategory(),
          ),
        ),
      ]),
    );
  }

  Widget _buildOfflineCategory() {
    return ExpansionPanelList(
      expandIconColor: Colors.black,
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _categories[index].isExpanded = isExpanded;
        });
      },
      children: _categories.map<ExpansionPanel>((Category category) {
        category.isExpanded = true;
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: gl.colorBackground,
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              iconColor: Colors.red,
              title: Text(category.name),
            );
          },
          body: _buildOfflineList(category),
          isExpanded: category.isExpanded,
        );
      }).toList(),
    );
  }

  Widget _buildOfflineList(Category category) {
    return _finishedInitializingCategory
        ? Container(
            constraints: BoxConstraints(
                maxWidth: MediaQuery.of(context).size.width * 1.0,
                maxHeight: _downlodableLayerTiles.length *
                    100 *
                    MediaQuery.of(context).size.width *
                    0.1),
            child: Scrollbar(
                controller: it,
                child: SingleChildScrollView(
                  controller: it,
                  physics: that,
                  child: Container(
                    child: _buildPanel(),
                  ),
                )))
        : CircularProgressIndicator();
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) async {
        setState(() {
          _downlodableLayerTiles[index].isExpanded = isExpanded;
        });
      },
      children: _downlodableLayerTiles.map<ExpansionPanel>((LayerTile item) {
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: gl.colorBackgroundSecondary,
          headerBuilder: (BuildContext context, bool isExpanded) {
            return Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: MediaQuery.of(context).size.width * .55,
                      maxHeight: MediaQuery.of(context).size.width * .2 > 48
                          ? MediaQuery.of(context).size.width * .2
                          : 48,
                      minHeight: 48,
                    ),
                    child: Text(item.name,
                        textScaler: const TextScaler.linear(1.2)),
                  ),
                  _downloadedControlBar(item),
                ]);
          },
          body: _expandedLegendView(item),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  Widget _downloadedControlBar(LayerTile lt) {
    return Container(
        constraints:
            const BoxConstraints(maxWidth: 128, minHeight: 32, maxHeight: 32),
        child: gl.dico.getLayerBase(lt.key).mOffline
            ? Text(
                "Enregistré",
                style: TextStyle(color: gl.colorAgroBioTech),
              )
            : Text(
                "Téléchargable",
                style: TextStyle(color: gl.colorUliege),
              ));
  }

  Widget _expandedLegendView(LayerTile lt) {
    return Column(
      children: <Widget>[
        if (lt.downloadable)
          ColoredBox(
              color: gl.colorBackground,
              child: LayerDownloader(lt, rebuildWidgetTreeForLayerDownloader)),
        if (gl.dico.getLayerBase(lt.key).hasDoc())
          ListTile(
            title: Text(
                "Consulter la documentation relative à la cette couche cartographique"),
            leading: IconButton(
                onPressed: () {
                  GoRouter.of(context).pushNamed(lt.key, pathParameters: {
                    'currentPage':
                        gl.dico.getLayerBase(lt.key).mPdfPage.toString()
                  });
                },
                icon: Icon(Icons.picture_as_pdf)),
          ),
        if ((gl.dico.getLayerBase(lt.key).mGroupe == "APT_FEE" ||
                gl.dico.getLayerBase(lt.key).mGroupe == "APT_CS") &&
            gl.dico
                .getEss(gl.dico.getLayerBase(lt.key).getEssCode())
                .hasFEEapt())
          ListTile(
            title: Text("Consulter la fiche-essence " +
                gl.dico
                    .getEss(gl.dico.getLayerBase(lt.key).getEssCode())
                    .getNameAndPrefix()),
            leading: IconButton(
                onPressed: () {
                  GoRouter.of(context).push(gl.dico
                      .getEss(gl.dico.getLayerBase(lt.key).getEssCode())
                      .getFicheRoute(complete: true));
                },
                icon: Icon(Icons.picture_as_pdf)),
          ),
      ],
    );
  }

  void _getLayerData() async {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    while(_downlodableLayerTiles.isNotEmpty){
      _downlodableLayerTiles.removeLast();
    }
    for (var key in mp.keys) {
      if (mp[key]!.mOffline) {
        _downlodableLayerTiles.add(LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe"));
      }
    }

    setState(() {
      _finishedInitializingCategory = true;
    });
  }

  @override
  void initState() {
    super.initState();
    if (!_finishedInitializingCategory) {
      _getLayerData();
    }
  }

  void rebuildWidgetTreeForLayerDownloader(var setter) async {
    setState(setter);
    _getLayerData();
    gl.dico.checkLayerBaseOfflineRessource();
  }
}
