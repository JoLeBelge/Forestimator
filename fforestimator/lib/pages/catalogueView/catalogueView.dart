import 'dart:ffi';

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:fforestimator/pages/catalogueView/legendView.dart';

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
    if (finishedInitializingCategories) {
      return SingleChildScrollView(
        physics: that,
        child: Container(
          child: _buildPanel(),
        ),
      );
    } else {
      return const CircularProgressIndicator();
    }
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expandIconColor: Colors.black,
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _categories[index].isExpanded = isExpanded;
        });
      },
      children: _categories.map<ExpansionPanel>((Category item) {
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: gl.colorBackground,
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              iconColor: Colors.red,
              title: Text(item.name),
            );
          },
          body: CategoryView(
            refreshView: widget.refreshView,
            category: item,
          ),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  void _getCategories() {
    for (groupe_couche gr in gl.dico.mGrCouches) {
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
  }
}

class CategoryView extends StatefulWidget {
  final Category category;
  final Function refreshView;
  const CategoryView(
      {super.key, required this.category, required this.refreshView});
  @override
  State<CategoryView> createState() => _CategoryView();
}

class _CategoryView extends State<CategoryView> {
  static Map<Category, List<LayerTile>> _layerTiles = {};
  static Map<Category, bool> _finishedInitializingCategory = {};
  bool _flipForBackground = false;

  @override
  Widget build(BuildContext context) {
    if (_finishedInitializingCategory[widget.category]!) {
      return Container(
          constraints: BoxConstraints(
              maxWidth: MediaQuery.of(context).size.width * 1.0,
              maxHeight: _layerTiles.length *
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
              )));
    } else {
      return const CircularProgressIndicator();
    }
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _layerTiles[widget.category]![index].isExpanded = isExpanded;
        });
      },
      children:
          _layerTiles[widget.category]!.map<ExpansionPanel>((LayerTile item) {
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: _getswitchBackgroundColorForList(),
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ColoredBox(
              color: _getswitchBackgroundColorForList(),
              child: Row(
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
                      child:
                          Text(item.name, textScaler: TextScaler.linear(1.2)),
                    ),
                    selectLayerBar(item),
                  ]),
            );
          },
          body: LegendView(
            layerKey: item.key,
            color: _getBackgroundColorForList(),
            constraintsText: BoxConstraints(
                minWidth: MediaQuery.of(context).size.width * .4,
                maxWidth: MediaQuery.of(context).size.width * .4,
                minHeight: MediaQuery.of(context).size.height * .02,
                maxHeight: MediaQuery.of(context).size.height * .02),
            constraintsColors: BoxConstraints(
                minWidth: MediaQuery.of(context).size.width * .4,
                maxWidth: MediaQuery.of(context).size.width * .4,
                minHeight: MediaQuery.of(context).size.height * .02,
                maxHeight: MediaQuery.of(context).size.height * .02),
          ),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  Widget selectLayerBar(LayerTile lt) {
    double barWidth = 48.0;
    if (widget.category.filter != "APT_CS" &&
        widget.category.filter != "APT_FEE" &&
        !lt.extern) barWidth = 96.0;

    return Container(
        constraints: BoxConstraints(
          maxWidth: barWidth,
          minWidth: barWidth,
          maxHeight: MediaQuery.of(context).size.width * .04 > 48
              ? MediaQuery.of(context).size.width * .04
              : 48,
          minHeight: 48,
        ),
        child: Row(children: [
          if (widget.category.filter != "APT_CS" &&
              widget.category.filter != "APT_FEE" &&
              !lt.extern)
            gl.anaPtSelectedLayerKeys.contains(lt.key)
                ? Container(
                    decoration: const BoxDecoration(
                        shape: BoxShape.circle, color: gl.colorUliege),
                    constraints: const BoxConstraints(
                      maxWidth: 48,
                      minWidth: 48,
                      maxHeight: 48,
                      minHeight: 48,
                    ),
                    padding: EdgeInsets.all(0),
                    child: IconButton(
                        icon: const Icon(Icons.location_on, size: 28),
                        onPressed: () {
                          setState(() {
                            if (gl.anaPtSelectedLayerKeys.length > 1) {
                              gl.anaPtSelectedLayerKeys.remove(lt.key);
                              lt.selected = false;
                              widget.refreshView();
                            }
                          });
                        }))
                : Container(
                    decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: _getBackgroundColorForList()),
                    constraints: const BoxConstraints(
                      maxWidth: 48,
                      minWidth: 48,
                      maxHeight: 48,
                      minHeight: 48,
                    ),
                    padding: EdgeInsets.all(0),
                    child: IconButton(
                        icon: const Icon(Icons.location_off, size: 28),
                        onPressed: () {
                          setState(() {
                            gl.anaPtSelectedLayerKeys.insert(0, lt.key);
                            lt.selected = true;
                            widget.refreshView();
                          });
                        }),
                  ),
          _isSelectedLayer(lt.key)
              ? Container(
                  decoration: const BoxDecoration(
                      shape: BoxShape.circle, color: gl.colorAgroBioTech),
                  constraints: const BoxConstraints(
                    maxWidth: 48,
                    minWidth: 48,
                    maxHeight: 48,
                    minHeight: 48,
                  ),
                  padding: EdgeInsets.all(0),
                  child: IconButton(
                      icon: const Icon(Icons.layers_clear, size: 28),
                      onPressed: () {
                        setState(() {
                          if (gl.interfaceSelectedLayerKeys.length > 1) {
                            lt.selected = false;
                            widget.refreshView();
                            gl.refreshMap(() {
                              _removeLayerFromList(lt.key);
                            });
                          }
                        });
                      }),
                )
              : Container(
                  decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: _getBackgroundColorForList()),
                  constraints: const BoxConstraints(
                    maxWidth: 48,
                    minWidth: 48,
                    maxHeight: 48,
                    minHeight: 48,
                  ),
                  padding: EdgeInsets.all(0),
                  child: IconButton(
                      icon: const Icon(Icons.layers, size: 28),
                      onPressed: () {
                        setState(() {
                          if (gl.interfaceSelectedLayerKeys.length < 3) {
                            lt.selected = true;
                            widget.refreshView();
                            gl.refreshMap(() {
                              gl.interfaceSelectedLayerKeys
                                  .insert(0, gl.selectedLayer(mCode: lt.key));
                            });
                          } else {
                            lt.selected = true;
                            widget.refreshView();
                            gl.refreshMap(() {
                              gl.interfaceSelectedLayerKeys.removeLast();
                              gl.interfaceSelectedLayerKeys
                                  .insert(0, gl.selectedLayer(mCode: lt.key));
                            });
                          }
                        });
                      }),
                ),
        ]));
  }

  void _getLayerData() async {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    for (var key in mp.keys) {
      if (widget.category.filter == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
          mp[key]!.mVisu &&
          mp[key]?.mTypeGeoservice == "") {
        if (_layerTiles[widget.category] == null) {
          _layerTiles[widget.category] = [];
        }
        _layerTiles[widget.category]!.add(LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            extern: mp[key]!.mCategorie == "Externe"));
      }
    }

    setState(() {
      _finishedInitializingCategory[widget.category] = true;
    });
  }

  void _removeLayerFromList(String key) {
    gl.selectedLayer? sL = null;
    for (var layer in gl.interfaceSelectedLayerKeys) {
      if (layer.mCode == key) {
        sL = layer;
      }
    }
    if (sL != null)
      gl.interfaceSelectedLayerKeys.remove(sL);
  }

  bool _isSelectedLayer(String key) {
    for (var layer in gl.interfaceSelectedLayerKeys) {
      if (layer.mCode == key) {
        return true;
      }
    }
    return false;
  }

  Color _getBackgroundColorForList() {
    if (_flipForBackground) {
      return gl.colorBackground;
    }
    return gl.colorBackgroundSecondary;
  }

  Color _getswitchBackgroundColorForList() {
    _flipForBackground = !_flipForBackground;
    return _getBackgroundColorForList();
  }

  Color _initSwitchBackgroundColorForList() {
    _flipForBackground = false;
    return _getBackgroundColorForList();
  }

  @override
  void initState() {
    super.initState();
    if (_finishedInitializingCategory[widget.category] == null) {
      _finishedInitializingCategory[widget.category] = false;
    }
    if (!_finishedInitializingCategory[widget.category]!) {
      _getLayerData();
    }
    gl.refreshCatalogueView = setState;
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
              final gl.selectedLayer item =
                  gl.interfaceSelectedLayerKeys.removeAt(oldIndex);
              gl.interfaceSelectedLayerKeys.insert(newIndex, item);
            });
          });
        },
        children: List<Widget>.generate(
          3,
          (i) => gl.interfaceSelectedLayerKeys.length > i
              ? Card(
                  key: Key('$i'),
                  color: gl.colorBackground,
                  surfaceTintColor: gl.colorBackgroundSecondary,
                  shadowColor: const Color.fromARGB(255, 44, 44, 44),
                  child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: <Widget>[
                        Container(
                          color: gl.colorBackground,
                          constraints: BoxConstraints(
                            maxHeight: 48,
                            maxWidth: MediaQuery.of(context).size.width * .65,
                          ),
                          child: Text(
                            textScaler: TextScaler.linear(1.2),
                            gl.dico.mLayerBases.keys.contains(
                                    gl.interfaceSelectedLayerKeys[i].mCode)
                                ? gl
                                    .dico
                                    .mLayerBases[
                                        gl.interfaceSelectedLayerKeys[i].mCode]!
                                    .mNom
                                : gl.interfaceSelectedLayerKeys[i].mCode,
                          ),
                        ),
                        Container(
                            color: gl.colorBackground,
                            constraints: BoxConstraints(
                              maxHeight:
                                  MediaQuery.of(context).size.width * .04 > 48
                                      ? MediaQuery.of(context).size.width * .04
                                      : 48,
                              minHeight: 48,
                              maxWidth: 100,
                              minWidth: 100,
                            ),
                            child: Row(children: [
                              IconButton(
                                icon: const Icon(Icons.layers_clear_rounded,
                                    size: 28),
                                onPressed: () {
                                  setState(() {
                                    if (gl.interfaceSelectedLayerKeys.length >
                                        1) {
                                      widget.refreshView();
                                      gl.refreshMap(() {
                                        gl.interfaceSelectedLayerKeys.remove(
                                            gl.interfaceSelectedLayerKeys[i]);
                                      });
                                    }
                                  });
                                },
                              ),
                              Container(
                                width: 48,
                                height: 48,
                                padding: const EdgeInsets.symmetric(),
                                child: ReorderableDragStartListener(
                                  index: i,
                                  child: const Icon(Icons.drag_indicator,
                                      size: 28),
                                ),
                              ),
                            ])),
                      ]),
                )
              : ListTile(
                  key: Key('$i'),
                  /*leading: Container(
                      constraints: BoxConstraints(
                    maxHeight: MediaQuery.of(context).size.height * .04,
                    maxWidth: MediaQuery.of(context).size.width * .35,
                  )),*/
                  title: const Text('Pas de couche selectionnée.'),
                ),
        ),
      );
    }
  }
}
