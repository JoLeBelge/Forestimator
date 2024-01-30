import 'dart:ffi';
import 'dart:io';
import 'dart:ui';

import 'package:fforestimator/dico/dicoApt.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter_map/flutter_map.dart';

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
          backgroundColor: Colors.grey,
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              iconColor: Colors.grey,
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
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              tileColor: item.selected &&
                      gl.interfaceSelectedLayerKeys.contains(item.name)
                  ? Colors.lightGreen
                  : Colors.grey,
              title: Text(item.name),
              leading: item.selected &&
                      gl.interfaceSelectedLayerKeys.contains(item.name)
                  ? IconButton(
                      icon: const Icon(Icons.upload_rounded),
                      onPressed: () {
                        setState(() {
                          gl.interfaceSelectedLayerKeys.remove(item.name);
                          item.selected = false;
                          widget.refreshView();
                        });
                      })
                  : IconButton(
                      icon: const Icon(Icons.download_rounded),
                      onPressed: () {
                        setState(() {
                          if (gl.interfaceSelectedLayerKeys.length < 3) {
                            gl.interfaceSelectedLayerKeys.add(item.name);
                            item.selected = true;
                            widget.refreshView();
                          }
                        });
                      }),
            );
          },
          body: Text('nana'),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  void _getLayerData() async {
    Map<String, layerBase> mp = gl.dico.mLayerBases;
    for (var key in mp.values) {
      if (widget.category.filter == key.mGroupe) {
        _layerTiles += [LayerTile(name: key.mNom!, filter: key.mGroupe!)];
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
    return ListView(
      children: List<Widget>.generate(
        3,
        (i) => gl.interfaceSelectedLayerKeys.length > i
            ? ListTile(
                leading: Container(
                  constraints: BoxConstraints(
                    maxHeight: MediaQuery.of(context).size.height * .04,
                    maxWidth: MediaQuery.of(context).size.width * .35,
                  ),
                  child: Row(children: <Widget>[
                    IconButton(
                      icon: Icon(Icons.keyboard_arrow_up_rounded),
                      onPressed: () {
                        setState(() {
                          if (i > 0){
                            String tmp = gl.interfaceSelectedLayerKeys[i];
                            gl.interfaceSelectedLayerKeys[i] = gl.interfaceSelectedLayerKeys[i - 1];
                            gl.interfaceSelectedLayerKeys[i - 1] = tmp;
                          }
                          widget.refreshView();
                        });
                      },
                    ),
                    IconButton(
                      icon: Icon(Icons.keyboard_arrow_down_rounded),
                      onPressed: () {
                        setState(() {
                          if (gl.interfaceSelectedLayerKeys.length > i + 1){
                            String tmp = gl.interfaceSelectedLayerKeys[i];
                            gl.interfaceSelectedLayerKeys[i] = gl.interfaceSelectedLayerKeys[i + 1];
                            gl.interfaceSelectedLayerKeys[i + 1] = tmp;
                          }
                          widget.refreshView();
                        });
                      },
                    ),
                    IconButton(
                      icon: Icon(Icons.delete_rounded),
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
                title: Text(gl.interfaceSelectedLayerKeys[i]),
              )
            : ListTile(
                leading: Container(
                    constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.height * .04,
                  maxWidth: MediaQuery.of(context).size.width * .35,
                )),
                title: Text('Pas de couche selectionnée.'),
              ),
      ),
    );
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
