import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/category_tile.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/pages/catalogueView/legend_view.dart';
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';

ScrollController it = ScrollController();
ClampingScrollPhysics that = const ClampingScrollPhysics();

class CatalogueView extends StatefulWidget {
  final Function refreshView;
  const CatalogueView({required this.refreshView, super.key});
  @override
  State<CatalogueView> createState() => _CatalogueView();
}

class _CatalogueView extends State<CatalogueView> {
  static List<Category> _categories = [];
  static bool finishedInitializingCategories = false;

  @override
  Widget build(BuildContext context) {
    gl.notificationContext = context;
    if (finishedInitializingCategories) {
      return SingleChildScrollView(physics: that, child: _buildCategoryPanel());
    } else {
      return const CircularProgressIndicator();
    }
  }

  Widget _buildCategoryPanel() {
    return ExpansionPanelList(
      expandIconColor: Colors.black,
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _categories[index].isExpanded = isExpanded;
        });
      },
      //dividerColor: Colors.amber,
      children:
          _categories.map<ExpansionPanel>((Category item) {
            return ExpansionPanel(
              canTapOnHeader: true,
              backgroundColor: Colors.blueAccent, //.colorBackgroundSecondary,
              headerBuilder: (BuildContext context, bool isExpanded) {
                return Container(
                  color: gl.colorBackgroundSecondary,
                  child: ListTile(title: Text(item.name)),
                );
              },
              body:
                  item.isExpanded
                      ? CategoryView(
                        refreshView: widget.refreshView,
                        category: item,
                      )
                      : Container(),
              isExpanded: item.isExpanded,
            );
          }).toList(),
    );
  }

  void _getCategories() {
    for (GroupeCouche gr in gl.dico.mGrCouches) {
      if (!gr.mExpert) {
        _categories += [Category(name: gr.mLabel, filter: gr.mCode)];
      }
    }
    setState(() {
      finishedInitializingCategories = true;
    });
  }

  @override
  void initState() {
    super.initState();
    if (!finishedInitializingCategories) {
      _getCategories();
    }
    gl.refreshWholeCatalogueView = rebuildWidgetTreeForLayerDownloader;
  }

  void rebuildWidgetTreeForLayerDownloader(void Function() setter) {
    setState(setter);
  }
}

class CategoryView extends StatefulWidget {
  final Category category;
  final Function refreshView;
  const CategoryView({
    super.key,
    required this.category,
    required this.refreshView,
  });
  @override
  State<CategoryView> createState() => _CategoryView();
}

class _CategoryView extends State<CategoryView> {
  static final Map<String, List<LayerTile>> _layerTiles = {};
  static final Map<String, bool> _finishedInitializingCategory = {};

  @override
  Widget build(BuildContext context) {
    if (_finishedInitializingCategory[widget.category.filter]!) {
      return Row(
        children: [
          Container(constraints: const BoxConstraints(minWidth: 5)),
          Container(
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0 - 5,
              maxHeight:
                  _layerTiles.length *
                  100 *
                  MediaQuery.of(context).size.width *
                  0.1,
            ),
            child: Scrollbar(
              controller: it,
              child: SingleChildScrollView(
                controller: it,
                physics: that,
                child: Container(child: _buildPanel()),
              ),
            ),
          ),
        ],
      );
    } else {
      return const CircularProgressIndicator();
    }
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) async {
        setState(() {
          _layerTiles[widget.category.filter]![index].isExpanded = isExpanded;
        });
      },
      dividerColor: Colors.cyan,
      children:
          _layerTiles[widget.category.filter]!.map<ExpansionPanel>((
            LayerTile item,
          ) {
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
                      padding: const EdgeInsets.all(8.0),
                      child: Text(
                        item.name,
                        textScaler: const TextScaler.linear(1.2),
                      ),
                    ),
                    selectLayerBar(item),
                  ],
                );
              },
              body:
                  (item.isExpanded ||
                          gl.dico.getLayerBase(item.key).mInDownload)
                      ? _expandedLegendView(item)
                      : Container(),
              isExpanded: item.isExpanded,
            );
          }).toList(),
    );
  }

  Widget _expandedLegendView(LayerTile lt) {
    return Column(
      children: <Widget>[
        if (lt.downloadable)
          ColoredBox(
            color: gl.colorBackgroundSecondary,
            child: LayerDownloader(lt),
          ),
        if (gl.dico.getLayerBase(lt.key).mUsedForAnalysis)
          ColoredBox(
            color: gl.colorBackgroundSecondary,
            child:
                gl.anaPtSelectedLayerKeys.contains(lt.key)
                    ? Row(
                      children: [
                        Container(
                          constraints: const BoxConstraints(
                            maxWidth: 48,
                            minWidth: 48,
                            maxHeight: 48,
                            minHeight: 48,
                          ),
                          padding: const EdgeInsets.only(left: 8),
                          child: IconButton(
                            icon: const Icon(Icons.location_on, size: 28),
                            onPressed: () async {
                              setState(() {
                                if (gl.anaPtSelectedLayerKeys.length > 1) {
                                  gl.anaPtSelectedLayerKeys.remove(lt.key);
                                  lt.selected = false;
                                  widget.refreshView();
                                }
                              });
                              final SharedPreferences prefs =
                                  await SharedPreferences.getInstance();
                              await prefs.setStringList(
                                'anaPtSelectedLayerKeys',
                                gl.anaPtSelectedLayerKeys,
                              );
                            },
                          ),
                        ),
                        Container(
                          constraints: const BoxConstraints(
                            maxWidth: 256,
                            minWidth: 48,
                            maxHeight: 48,
                            minHeight: 48,
                          ),
                          child: const Text(
                            "La couche est selectionnée pour l'analyse ponctuelle.",
                          ),
                        ),
                      ],
                    )
                    : Row(
                      children: [
                        Container(
                          constraints: const BoxConstraints(
                            maxWidth: 48,
                            minWidth: 48,
                            maxHeight: 48,
                            minHeight: 48,
                          ),
                          padding: const EdgeInsets.all(0),
                          child: IconButton(
                            icon: const Icon(Icons.location_off, size: 28),
                            onPressed: () async {
                              setState(() {
                                gl.anaPtSelectedLayerKeys.insert(0, lt.key);
                                lt.selected = true;
                                widget.refreshView();
                              });
                              final SharedPreferences prefs =
                                  await SharedPreferences.getInstance();
                              await prefs.setStringList(
                                'anaPtSelectedLayerKeys',
                                gl.anaPtSelectedLayerKeys,
                              );
                            },
                          ),
                        ),
                        Container(
                          constraints: const BoxConstraints(
                            maxWidth: 256,
                            minWidth: 48,
                            maxHeight: 48,
                            minHeight: 48,
                          ),
                          child: const Text(
                            "La couche n'est pas selectionnée pour l'analyse ponctuelle.",
                          ),
                        ),
                      ],
                    ),
          ),
        if (gl.dico.getLayerBase(lt.key).hasDoc())
          ListTile(
            title: const Text(
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
              icon: const Icon(Icons.picture_as_pdf),
            ),
          ),
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
              icon: const Icon(Icons.picture_as_pdf),
            ),
          ),
        lt.attribution(),
        LegendView(
          layerKey: lt.key,
          color: gl.colorBackgroundSecondary,
          constraintsText: BoxConstraints(
            minWidth: MediaQuery.of(context).size.width * .4,
            maxWidth: MediaQuery.of(context).size.width * .4,
            minHeight: MediaQuery.of(context).size.height * .02,
            maxHeight: MediaQuery.of(context).size.height * .02,
          ),
          constraintsColors: BoxConstraints(
            minWidth: MediaQuery.of(context).size.width * .4,
            maxWidth: MediaQuery.of(context).size.width * .4,
            minHeight: MediaQuery.of(context).size.height * .02,
            maxHeight: MediaQuery.of(context).size.height * .02,
          ),
        ),
      ],
    );
  }

  Widget selectLayerBar(LayerTile lt) {
    double barWidth = 128.0 + 48;
    return Container(
      constraints: BoxConstraints(
        maxWidth: barWidth,
        minWidth: barWidth,
        maxHeight:
            MediaQuery.of(context).size.width * .04 > 48
                ? MediaQuery.of(context).size.width * .04
                : 48,
        minHeight: 48,
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          lt.downloadedControlBar(),
          gl.isSelectedLayer(lt.key, offline: false)
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
                        lt.selected = false;
                        widget.refreshView();
                        gl.refreshMap(() {
                          gl.removeLayerFromList(lt.key, offline: false);
                        });
                        gl.rebuildOfflineView(() {});
                      }
                    });
                  },
                ),
              )
              : Container(
                decoration: const BoxDecoration(shape: BoxShape.circle),
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
                    setState(() {
                      lt.selected = true;
                      widget.refreshView();
                      gl.refreshMap(() {
                        gl.addLayerToList(lt.key, offline: false);
                      });
                      gl.rebuildOfflineView(() {});
                    });
                  },
                ),
              ),
        ],
      ),
    );
  }

  void _getLayerData() async {
    Map<String, LayerBase> mp = gl.dico.mLayerBases;

    for (var key in mp.keys) {
      if (widget.category.filter == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
          mp[key]!.mVisu &&
          mp[key]?.mTypeGeoservice ==
              "" // "arcgisRest c'est pour le vectoriel, pas actif sous flutter"
              ) {
        if (_layerTiles[widget.category.filter] == null) {
          _layerTiles[widget.category.filter] = [];
        }
        _layerTiles[widget.category.filter]!.add(
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
      _finishedInitializingCategory[widget.category.filter] = true;
    });
  }

  @override
  void initState() {
    super.initState();
    if (_finishedInitializingCategory[widget.category.filter] == null) {
      _finishedInitializingCategory[widget.category.filter] = false;
    }
    if (!_finishedInitializingCategory[widget.category.filter]!) {
      _getLayerData();
    }
  }
}

class SelectedLayerView extends StatefulWidget {
  final Function refreshView;
  const SelectedLayerView({required this.refreshView, super.key});
  @override
  State<SelectedLayerView> createState() => _SelectedLayerView();
}

class _SelectedLayerView extends State<SelectedLayerView> {
  @override
  void initState() {
    gl.refreshCurrentThreeLayer = () => setState(() {});
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    {
      return ReorderableListView(
        buildDefaultDragHandles: false,
        padding: const EdgeInsets.symmetric(horizontal: 0),
        onReorder: (int oldIndex, int newIndex) {
          setState(() {
            if (oldIndex < newIndex) {
              newIndex -= 1;
            }
            if (gl.interfaceSelectedLayerKeys.length < newIndex + 1 ||
                gl.interfaceSelectedLayerKeys.length < oldIndex + 1) {
              return;
            }
            gl.refreshMap(() {
              final gl.SelectedLayer item = gl.interfaceSelectedLayerKeys
                  .removeAt(oldIndex);
              gl.interfaceSelectedLayerKeys.insert(newIndex, item);
            });
          });
        },
        children: List<Widget>.generate(
          gl.nMaxSelectedLayer,
          (i) =>
              gl.interfaceSelectedLayerKeys.length > i
                  ? Card(
                    key: Key('$i'),
                    color: gl.colorBackgroundSecondary,
                    surfaceTintColor: gl.colorBackgroundSecondary,
                    shadowColor: const Color.fromARGB(255, 44, 44, 44),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: <Widget>[
                        Container(
                          color: gl.colorBackgroundSecondary,
                          constraints: BoxConstraints(
                            maxHeight: 36,
                            maxWidth: MediaQuery.of(context).size.width - 150,
                          ),
                          child: Text(
                            textScaler:
                                gl.dico
                                            .getLayerBase(
                                              gl
                                                  .interfaceSelectedLayerKeys[i]
                                                  .mCode,
                                            )
                                            .mNom
                                            .length <
                                        25
                                    ? TextScaler.linear(1.2)
                                    : TextScaler.linear(0.9),
                            gl.dico
                                .getLayerBase(
                                  gl.interfaceSelectedLayerKeys[i].mCode,
                                )
                                .mNom,
                          ),
                        ),
                        Container(
                          color: gl.colorBackgroundSecondary,
                          constraints: BoxConstraints(
                            maxHeight:
                                MediaQuery.of(context).size.width * .04 > 48
                                    ? MediaQuery.of(context).size.width * .04
                                    : 48,
                            minHeight: 48,
                            maxWidth: 150,
                            minWidth: 150,
                          ),
                          child: Row(
                            children: [
                              IconButton(
                                icon: const Icon(
                                  Icons.layers_clear_rounded,
                                  size: 28,
                                ),
                                onPressed: () {
                                  setState(() {
                                    if (gl.interfaceSelectedLayerKeys.length >
                                        1) {
                                      widget.refreshView();
                                      gl.refreshMap(() {});
                                      gl.removeLayerFromList(
                                        gl.interfaceSelectedLayerKeys[i].mCode,
                                        offline:
                                            gl
                                                .interfaceSelectedLayerKeys[i]
                                                .offline,
                                      );
                                      gl.rebuildOfflineView(() {});
                                    }
                                  });
                                },
                              ),
                              gl.interfaceSelectedLayerKeys[i].offline
                                  ? Container(
                                    width: 48,
                                    height: 48,
                                    padding: const EdgeInsets.symmetric(),
                                    child: const Icon(Icons.save, size: 28),
                                  )
                                  : Container(
                                    width: 48,
                                    height: 48,
                                    padding: const EdgeInsets.symmetric(),
                                    child: const Icon(Icons.wifi, size: 28),
                                  ),
                              Container(
                                width: 48,
                                height: 48,
                                padding: const EdgeInsets.symmetric(),
                                child: ReorderableDragStartListener(
                                  index: i,
                                  child: const Icon(
                                    Icons.drag_indicator,
                                    size: 28,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  )
                  : ListTile(
                    key: Key('$i'),
                    title: const Text(
                      '',
                    ), // le texte "pas de couches sélectionnée avait perturbé un des testeurs, on met rien
                  ),
        ),
      );
    }
  }
}
