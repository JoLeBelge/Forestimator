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
      return Scrollbar(
          controller: it,
          child: SingleChildScrollView(
            physics: that,
            child: Container(
              child: _buildPanel(),
            ),
          ));
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
          backgroundColor: Colors.grey[200],
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
  static Map<String, LegendView> _legendViews = {};
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
        if (_legendViews[item.key] == null) {
          _legendViews[item.key] = LegendView(
            layerKey: item.key,
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
          );
        }
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: Colors.grey[200],
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              tileColor: selectLayerBarColor(item),
              title: Text(item.name),
              leading: selectLayerBar(item),
            );
          },
          body: _legendViews[item.key]!,
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  Color selectLayerBarColor(LayerTile lt) {
    if (gl.interfaceSelectedLayerKeys.contains(lt.key) &&
        gl.anaPtSelectedLayerKeys.contains(lt.key)) {
      return Colors.red;
    } else if (gl.interfaceSelectedLayerKeys.contains(lt.key)) {
      return gl.colorAgroBioTech;
    } else if (gl.anaPtSelectedLayerKeys.contains(lt.key)) {
      return gl.colorUliege;
    }
    return Colors.grey[200]!;
  }

  Widget selectLayerBar(LayerTile lt) {
    return Container(
        constraints: BoxConstraints(
            minWidth: MediaQuery.of(context).size.width * .25,
            maxWidth: MediaQuery.of(context).size.width * .25,
            minHeight: MediaQuery.of(context).size.height * .04,
            maxHeight: MediaQuery.of(context).size.height * .04),
        child: Row(children: [
          gl.interfaceSelectedLayerKeys.contains(lt.key)
              ? IconButton(
                  icon: const Icon(Icons.layers_clear),
                  onPressed: () {
                    setState(() {
                      if (gl.interfaceSelectedLayerKeys.length > 1) {
                        lt.selected = false;
                        widget.refreshView();
                        gl.refreshMap(() {
                          gl.interfaceSelectedLayerKeys.remove(lt.key);
                        });
                      }
                    });
                  })
              : IconButton(
                  icon: const Icon(Icons.layers),
                  onPressed: () {
                    setState(() {
                      if (gl.interfaceSelectedLayerKeys.length < 3) {
                        lt.selected = true;
                        widget.refreshView();
                        gl.refreshMap(() {
                          gl.interfaceSelectedLayerKeys.insert(0, lt.key);
                        });
                      } else {
                        lt.selected = true;
                        widget.refreshView();
                        gl.refreshMap(() {
                          gl.interfaceSelectedLayerKeys.removeLast();
                          gl.interfaceSelectedLayerKeys.insert(0, lt.key);
                        });
                      }
                    });
                  }),
          if (widget.category.filter != "APT_CS" &&
              widget.category.filter != "APT_FEE" &&
              !lt.extern)
            gl.anaPtSelectedLayerKeys.contains(lt.key)
                ? IconButton(
                    icon: const Icon(Icons.show_chart),
                    onPressed: () {
                      setState(() {
                        if (gl.anaPtSelectedLayerKeys.length > 1) {
                          gl.anaPtSelectedLayerKeys.remove(lt.key);
                          lt.selected = false;
                          widget.refreshView();
                        }
                      });
                    })
                : IconButton(
                    icon: const Icon(Icons.chair),
                    onPressed: () {
                      setState(() {
                        gl.anaPtSelectedLayerKeys.insert(0, lt.key);
                        lt.selected = true;
                        widget.refreshView();
                      });
                    }),
        ]));
  }

  void _getLayerData() async {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    for (var key in mp.keys) {
      if (widget.category.filter == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
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
      return ListView(
        children: List<Widget>.generate(
          3,
          (i) => gl.interfaceSelectedLayerKeys.length > i
              ? ListTile(
                  leading: Container(
                    color: Colors.amber,
                    constraints: BoxConstraints(
                      maxHeight: MediaQuery.of(context).size.height * .04,
                      maxWidth: MediaQuery.of(context).size.width * .35,
                    ),
                    child: Row(children: <Widget>[
                      IconButton(
                        icon: const Icon(Icons.arrow_upward_rounded),
                        onPressed: () {
                          setState(() {
                            if (i > 0) {
                              gl.refreshMap(() {
                                String tmp = gl.interfaceSelectedLayerKeys[i];
                                gl.interfaceSelectedLayerKeys[i] =
                                    gl.interfaceSelectedLayerKeys[i - 1];
                                gl.interfaceSelectedLayerKeys[i - 1] = tmp;
                              });
                              widget.refreshView();
                            }
                          });
                        },
                      ),
                      IconButton(
                        icon: const Icon(Icons.arrow_downward_rounded),
                        onPressed: () {
                          setState(() {
                            if (gl.interfaceSelectedLayerKeys.length > i + 1) {
                              gl.refreshMap(() {
                                String tmp = gl.interfaceSelectedLayerKeys[i];
                                gl.interfaceSelectedLayerKeys[i] =
                                    gl.interfaceSelectedLayerKeys[i + 1];
                                gl.interfaceSelectedLayerKeys[i + 1] = tmp;
                              });
                              widget.refreshView();
                            }
                          });
                        },
                      ),
                      IconButton(
                        icon: const Icon(Icons.layers_clear_rounded),
                        onPressed: () {
                          setState(() {
                            if (gl.interfaceSelectedLayerKeys.length > 1) {
                              widget.refreshView();
                              gl.refreshMap(() {
                                gl.interfaceSelectedLayerKeys
                                    .remove(gl.interfaceSelectedLayerKeys[i]);
                              });
                            }
                          });
                        },
                      ),
                    ]),
                  ),
                  title: Text(
                    gl.dico.mLayerBases.keys
                            .contains(gl.interfaceSelectedLayerKeys[i])
                        ? gl.dico.mLayerBases[gl.interfaceSelectedLayerKeys[i]]!
                            .mNom!
                        : gl.interfaceSelectedLayerKeys[i],
                  ))
              : ListTile(
                  leading: Container(
                      constraints: BoxConstraints(
                    maxHeight: MediaQuery.of(context).size.height * .04,
                    maxWidth: MediaQuery.of(context).size.width * .35,
                  )),
                  title: const Text('Pas de couche selectionnée.'),
                ),
        ),
      );
    }
  }
}

class SearchBarView extends StatefulWidget {
  SearchController _searchIt = SearchController();
  SearchBarView({super.key});
  @override
  State<SearchBarView> createState() => _SearchBarView();
}

class _SearchBarView extends State<SearchBarView> {
  @override
  Widget build(BuildContext context) {
    return SearchAnchor(builder: (context, _searchIt) {
      return SearchBar();
    }, suggestionsBuilder: (context, _searchIt) {
      return <Widget>[
        Tooltip(
          message: 'Change brightness mode',
          child: IconButton(
            onPressed: () {
              setState(() {});
            },
            icon: const Icon(Icons.wb_sunny_outlined),
            selectedIcon: const Icon(Icons.brightness_2_outlined),
          ),
        )
      ];
    });
    //return SearchBar(controller: widget._searchIt,);
  }
}
