import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/pages/catalogueView/catalogue_view.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';

class OfflineView extends StatefulWidget {
  const OfflineView({super.key});

  @override
  State<OfflineView> createState() => _OfflineView();
}

bool _finishedInitializingLayerData = false;

class _OfflineView extends State<OfflineView> {
  final List<LayerTile> _downloadedLayers = [];

  @override
  Widget build(BuildContext context) {
    gl.notificationContext = context;
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
      body: Scrollbar(
        controller: it,
        child: SingleChildScrollView(
          controller: it,
          physics: that,
          child: Column(
            mainAxisAlignment: MainAxisAlignment.start,
            children: [
              gl.offlineMode
                  ? Container(
                    color: gl.colorBackgroundSecondary,
                    constraints: BoxConstraints(
                      maxWidth: MediaQuery.of(context).size.width * 1.0,
                      minHeight: MediaQuery.of(context).size.height * .15,
                      maxHeight: MediaQuery.of(context).size.height * .15,
                    ),
                    child: TextButton.icon(
                      onPressed: () async {
                        setState(() {
                          gl.offlineMode = false;
                          gl.rebuildNavigatorBar!();
                          // pas nécéssaire vu que de offline vers online on peut garder la même sélection de couche (les cartes offlines peuvent être affichées en mode online, c'est l'inverse qui ne va pas)
                          //gl.refreshCurrentThreeLayer();
                          //gl.refreshMap(() {});
                        });
                        final SharedPreferences prefs =
                            await SharedPreferences.getInstance();
                        await prefs.setBool('offlineMode', gl.offlineMode);
                      },
                      icon: Icon(
                        Icons.download_for_offline,
                        color: gl.colorUliege,
                      ),
                      label: Text(
                        "Désactivez le mode hors ligne.",
                        style: TextStyle(color: gl.colorUliege),
                      ),
                    ),
                  )
                  : Container(
                    color: gl.colorBackgroundSecondary,
                    constraints: BoxConstraints(
                      maxWidth: MediaQuery.of(context).size.width * 1.0,
                      minHeight: MediaQuery.of(context).size.height * .15,
                      maxHeight: MediaQuery.of(context).size.height * .15,
                    ),
                    child: TextButton.icon(
                      onPressed: () async {
                        setState(() {
                          gl.changeSelectedLayerModeOffline();
                          gl.offlineMode = true;
                          gl.rebuildNavigatorBar!();
                          gl.refreshCurrentThreeLayer();
                          gl.refreshMap(() {});
                        });
                        final SharedPreferences prefs =
                            await SharedPreferences.getInstance();
                        await prefs.setStringList(
                          'interfaceSelectedLCode',
                          gl.getInterfaceSelectedLCode(),
                        );
                        await prefs.setBool('offlineMode', gl.offlineMode);
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
              _buildPanel(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) async {
        setState(() {
          _downloadedLayers[index].isExpanded = isExpanded;
        });
      },
      children:
          _downloadedLayers.map<ExpansionPanel>((LayerTile item) {
            return ExpansionPanel(
              canTapOnHeader: true,
              backgroundColor: gl.colorBackgroundSecondary,
              headerBuilder: (BuildContext context, bool isExpanded) {
                return Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        maxWidth:
                            MediaQuery.of(context).size.width * .2 > 144
                                ? MediaQuery.of(context).size.width * .2
                                : 144,
                        maxHeight:
                            MediaQuery.of(context).size.width * .2 > 48
                                ? MediaQuery.of(context).size.width * .2
                                : 48,
                        minHeight: 48,
                      ),
                      padding: const EdgeInsets.only(left: 8),
                      child: Text(
                        item.name,
                        textScaler: const TextScaler.linear(1.2),
                      ),
                    ),
                    item.downloadedControlBar(),
                    if (gl.dico.getLayerBase(item.key).mBits == 8)
                      _selectLayerButton(item),
                  ],
                );
              },
              body: item.isExpanded ? _expandedLegendView(item) : Container(),
              isExpanded: item.isExpanded,
            );
          }).toList(),
    );
  }

  Widget _selectLayerButton(LayerTile lt) {
    int nLayer = 3;
    return _isSelectedLayer(lt.key)
        ? Container(
          decoration: const BoxDecoration(
            shape: BoxShape.circle,
            color: gl.colorAgroBioTech,
          ),
          constraints: const BoxConstraints(
            maxWidth: 48,
            minWidth: 48,
            maxHeight: 48,
            minHeight: 48,
          ),
          padding: const EdgeInsets.all(0),
          child: IconButton(
            icon: const Icon(Icons.layers_clear, size: 28),
            onPressed: () {
              setState(() {
                if (gl.interfaceSelectedLayerKeys.length > 1) {
                  setState(() {
                    lt.selected = false;
                  });
                  gl.refreshMap(() {
                    gl.removeLayerFromList(lt.key, offline: true);
                  });
                  gl.refreshCurrentThreeLayer();
                  gl.refreshWholeCatalogueView(() {});
                }
              });
            },
          ),
        )
        : Container(
          decoration: BoxDecoration(shape: BoxShape.circle),
          constraints: const BoxConstraints(
            maxWidth: 48,
            minWidth: 48,
            maxHeight: 48,
            minHeight: 48,
          ),
          padding: const EdgeInsets.all(0),
          child: IconButton(
            icon: const Icon(Icons.layers, size: 28),
            onPressed: () {
              if (!gl.offlineMode ||
                  (gl.offlineMode && gl.dico.getLayerBase(lt.key).mOffline)) {
                setState(() {
                  if (gl.interfaceSelectedLayerKeys.length < nLayer) {
                    setState(() {
                      lt.selected = true;
                    });
                    gl.refreshMap(() {
                      gl.addLayerToList(lt.key, offline: true);
                    });
                  } else {
                    setState(() {
                      lt.selected = true;
                    });
                    gl.refreshMap(() {
                      gl.interfaceSelectedLayerKeys.removeLast();
                      gl.addLayerToList(lt.key, offline: true);
                    });
                  }
                  gl.refreshCurrentThreeLayer();
                  gl.refreshWholeCatalogueView(() {});
                });
              }
            },
          ),
        );
  }

  bool _isSelectedLayer(String key) {
    for (var layer in gl.interfaceSelectedLayerKeys) {
      if (layer.mCode == key && layer.offline == true) {
        return true;
      }
    }
    return false;
  }

  Widget _expandedLegendView(LayerTile lt) {
    return Column(
      children: <Widget>[
        if (lt.downloadable)
          ColoredBox(color: gl.colorBackground, child: LayerDownloader(lt)),
        if (gl.dico.getLayerBase(lt.key).hasDoc())
          ListTile(
            title: Text(
              "Consulter la documentation relative à cette couche cartographique",
            ),
            leading: IconButton(
              onPressed: () {
                GoRouter.of(context).pushNamed(
                  lt.key,
                  pathParameters: {
                    'currentPage':
                        gl.dico.getLayerBase(lt.key).mPdfPage.toString(),
                  },
                );
              },
              icon: Icon(Icons.picture_as_pdf),
            ),
          ),
        lt.attribution(),
        if ((gl.dico.getLayerBase(lt.key).mGroupe == "APT_FEE" ||
                gl.dico.getLayerBase(lt.key).mGroupe == "APT_CS") &&
            gl.dico
                .getEss(gl.dico.getLayerBase(lt.key).getEssCode())
                .hasFEEapt())
          ListTile(
            title: Text(
              "Consulter la fiche-essence ${gl.dico.getEss(gl.dico.getLayerBase(lt.key).getEssCode()).getNameAndPrefix()}",
            ),
            leading: IconButton(
              onPressed: () {
                GoRouter.of(context).push(
                  gl.dico
                      .getEss(gl.dico.getLayerBase(lt.key).getEssCode())
                      .getFicheRoute(complete: true),
                );
              },
              icon: Icon(Icons.picture_as_pdf),
            ),
          ),
        if (gl.dico.getLayerBase(lt.key).mBits == 16)
          ListTile(
            title: Text(
              "Cette couche est utilisée pour les analyses ponctuelles mais vous ne pouvez pas la visualiser",
            ),
            leading: Icon(Icons.warning),
          ),
      ],
    );
  }

  void _getLayerData() {
    Map<String, LayerBase> mp = gl.dico.mLayerBases;
    _downloadedLayers.clear();

    for (var key in mp.keys) {
      if (mp[key]!.mOffline || mp[key]!.mInDownload) {
        _downloadedLayers.add(
          LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe",
          ),
        );
      }
    }

    setState(() {
      _finishedInitializingLayerData = true;
    });
  }

  @override
  void initState() {
    super.initState();
    if (!_finishedInitializingLayerData) {
      _getLayerData();
    }
    gl.rebuildOfflineView = rebuildOfflineWidgetTreeForLayerDownloader;
  }

  void rebuildOfflineWidgetTreeForLayerDownloader(
    void Function() setter,
  ) async {
    await gl.dico.checkLayerBaseOfflineRessource();
    _getLayerData();
    setState(setter);
  }
}
