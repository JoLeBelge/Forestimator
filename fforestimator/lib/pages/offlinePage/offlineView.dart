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
    Category(name: "Couches enregistrées.", filter: "offline"),
    Category(name: "Couches à télécharger", filter: "online")
  ];
  final Map<String, List<LayerTile>> _downlodableLayerTiles = {
    "offline": [],
    "online": []
  };

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
                      gl.rebuildNavigatorBar!();
                      gl.refreshCurrentThreeLayer();
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
                    setState(() {
                      while (gl.interfaceSelectedLayerKeys.length > 1) {
                        if (gl.interfaceSelectedLayerKeys.first.offline) {
                          gl.interfaceSelectedLayerKeys.removeLast();
                        } else {
                          gl.interfaceSelectedLayerKeys.removeAt(0);
                        }
                      }

                      if (!gl.interfaceSelectedLayerKeys.first.offline) {
                        gl.interfaceSelectedLayerKeys.clear();
                        gl.interfaceSelectedLayerKeys.insert(
                            0,
                            gl.selectedLayer(
                                mCode: gl.dico.getLayersOffline().first.mCode));
                      }
                      gl.offlineMode = true;
                      gl.rebuildNavigatorBar!();
                      gl.refreshCurrentThreeLayer();
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
                    child: _buildPanel(category),
                  ),
                )))
        : CircularProgressIndicator();
  }

  Widget _buildPanel(Category category) {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) async {
        setState(() {
          _downlodableLayerTiles[category.filter]![index].isExpanded =
              isExpanded;
        });
      },
      children: _downlodableLayerTiles[category.filter]!
          .map<ExpansionPanel>((LayerTile item) {
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
              child: LayerDownloader(
                  lt, rebuildWidgetTreeForLayerDownloader, reloadLayerData)),
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

  void _getLayerData() {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    _downlodableLayerTiles["offline"]!.clear();
    _downlodableLayerTiles["online"]!.clear();

    for (var key in mp.keys) {
      if (mp[key]!.mOffline) {
        _downlodableLayerTiles["offline"]!.add(LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe"));
      } else if (mp[key]!.mIsDownloadableRW) {
        _downlodableLayerTiles["online"]!.add(LayerTile(
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

  Future<void> reloadLayerData() async {
    print(
        "forget itiaqhgoihqgp,;oi;qjogjk^pqog:po;lqpogqg654654g8q1b068q4g2r6ze87");
    _getLayerData();
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
  }
}
