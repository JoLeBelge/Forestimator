import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/pages/catalogueView/catalogueView.dart';
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:fforestimator/tools/layerDownloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';

class OfflineView extends StatefulWidget {
  const OfflineView({super.key});
  @override
  State<OfflineView> createState() => _OfflineView();
}

bool _finishedInitializingCategory = false;

class _OfflineView extends State<OfflineView> {
  final List<Category> _categories = [
    Category(name: "Couches enregistrées.", filter: "offline"),
    //Category(name: "Couches à télécharger", filter: "online")
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
                  onPressed: () async {
                    setState(() {
                      gl.offlineMode = false;
                      gl.rebuildNavigatorBar!();
                      gl.refreshCurrentThreeLayer();
                      gl.refreshMap(() {});
                    });
                    if (await InternetConnection().hasInternetAccess) {
                      showDialog(
                        context: context,
                        builder: (BuildContext context) {
                          return PopupNoInternet();
                        },
                      );
                    }
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
                    maxHeight: MediaQuery.of(context).size.height * .15),
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

  void addToList(String key) {
    _downlodableLayerTiles["offline"]!.add(LayerTile(
      key: key,
      name: gl.dico.getLayerBase(key).mNom,
      filter: gl.dico.getLayerBase(key).mGroupe,
      extern: gl.dico.getLayerBase(key).mCategorie == "Externe",
      downloadable: gl.dico.getLayerBase(key).mIsDownloadableRW,
    ));
  }

  void removeFromList(String key) {
    for (var it in _downlodableLayerTiles["offline"]!) {
      if (it.key == key) {
        _downlodableLayerTiles["offline"]!.remove(it);
        break;
      }
    }
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
          isExpanded: true, //category.isExpanded,
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
                      maxWidth: MediaQuery.of(context).size.width * .2 > 144
                          ? MediaQuery.of(context).size.width * .2
                          : 144,
                      maxHeight: MediaQuery.of(context).size.width * .2 > 48
                          ? MediaQuery.of(context).size.width * .2
                          : 48,
                      minHeight: 48,
                    ),
                    child: Text(item.name,
                        textScaler: const TextScaler.linear(1.2)),
                  ),
                  _downloadedControlBar(item),
                  _selectLayerButton(item),
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

  Widget _selectLayerButton(LayerTile lt) {
    return gl.isSelectedLayer(lt.key, offline: true)
        ? Container(
            decoration: const BoxDecoration(
                shape: BoxShape.circle, color: gl.colorAgroBioTech),
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
                    }
                  });
                }),
          )
        : Container(
            decoration: BoxDecoration(
              shape: BoxShape.circle,
            ),
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
                      (gl.offlineMode && gl.dico.getLayerBase(lt.key).mOffline))
                    setState(() {
                      if (gl.interfaceSelectedLayerKeys.length <
                          gl.nMaxSelectedLayer) {
                        setState(() {
                          lt.selected = true;
                        });
                        gl.refreshMap(() {
                          gl.addLayerToList(lt.key);
                        });
                      } else {
                        setState(() {
                          lt.selected = true;
                        });
                        gl.refreshMap(() {
                          gl.interfaceSelectedLayerKeys.removeLast();
                          gl.addLayerToList(lt.key);
                        });
                      }
                    });
                  //TODO else popup warning: file is not on disk
                }),
          );
  }

  Widget _expandedLegendView(LayerTile lt) {
    return Column(
      children: <Widget>[
        if (lt.downloadable)
          ColoredBox(color: gl.colorBackground, child: LayerDownloader(lt)),
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
    _getLayerData();
  }

  @override
  void initState() {
    super.initState();
    if (!_finishedInitializingCategory) {
      _getLayerData();
    }
    gl.rebuildOfflineView = rebuildOfflineWidgetTreeForLayerDownloader;
    gl.removeFromOfflineList = removeFromList;
    gl.addToOfflineList = addToList;
  }

  void rebuildOfflineWidgetTreeForLayerDownloader(
      void Function() setter) async {
    setState(setter);
  }
}
