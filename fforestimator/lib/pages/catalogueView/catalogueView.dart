import 'package:fforestimator/dico/dicoApt.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:fforestimator/pages/catalogueView/legendView.dart';

ScrollController it = ScrollController();
ClampingScrollPhysics that = ClampingScrollPhysics();

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
  List<LayerTile> _layerTiles = [];
  bool _finishedInitializingCategory = false;
  @override
  Widget build(BuildContext context) {
    if (_finishedInitializingCategory) {
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
          _layerTiles[index].isExpanded = isExpanded;
        });
      },
      children: _layerTiles.map<ExpansionPanel>((LayerTile item) {
        return ExpansionPanel(
          canTapOnHeader: true,
          backgroundColor: Colors.grey[200],
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              tileColor: gl.interfaceSelectedLayerKeys.contains(item.key)
                  ? Colors.lightGreen
                  : Colors.grey[200],
              title: Text(item.name),
              leading: gl.interfaceSelectedLayerKeys.contains(item.key)
                  ? IconButton(
                      icon: const Icon(Icons.layers_clear),
                      onPressed: () {
                        setState(() {
                          gl.interfaceSelectedLayerKeys.remove(item.key);
                          item.selected = false;
                          widget.refreshView();
                        });
                      })
                  : IconButton(
                      icon: const Icon(Icons.layers),
                      onPressed: () {
                        setState(() {
                          if (gl.interfaceSelectedLayerKeys.length < 3) {
                            gl.interfaceSelectedLayerKeys.insert(0, item.key);
                            item.selected = true;
                            widget.refreshView();
                          } else {
                            gl.interfaceSelectedLayerKeys.removeLast();
                            gl.interfaceSelectedLayerKeys.insert(0, item.key);
                            item.selected = true;
                            widget.refreshView();
                          }
                        });
                      }),
            );
          },
          body: LegendView(
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
          ),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  void _getLayerData() async {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    for (var key in mp.keys) {
      if (widget.category.filter == mp[key]!.mGroupe && !mp[key]!.mExpert) {
        _layerTiles += [
          LayerTile(name: mp[key]!.mNom, filter: mp[key]!.mGroupe!, key: key)
        ];
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
                              String tmp = gl.interfaceSelectedLayerKeys[i];
                              gl.interfaceSelectedLayerKeys[i] =
                                  gl.interfaceSelectedLayerKeys[i - 1];
                              gl.interfaceSelectedLayerKeys[i - 1] = tmp;
                            }
                            widget.refreshView();
                          });
                        },
                      ),
                      IconButton(
                        icon: const Icon(Icons.arrow_downward_rounded),
                        onPressed: () {
                          setState(() {
                            if (gl.interfaceSelectedLayerKeys.length > i + 1) {
                              String tmp = gl.interfaceSelectedLayerKeys[i];
                              gl.interfaceSelectedLayerKeys[i] =
                                  gl.interfaceSelectedLayerKeys[i + 1];
                              gl.interfaceSelectedLayerKeys[i + 1] = tmp;
                            }
                            widget.refreshView();
                          });
                        },
                      ),
                      IconButton(
                        icon: const Icon(Icons.layers_clear_rounded),
                        onPressed: () {
                          setState(() {
                            gl.interfaceSelectedLayerKeys
                                .remove(gl.interfaceSelectedLayerKeys[i]);
                            widget.refreshView();
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
