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
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _categories[index].isExpanded = isExpanded;
        });
      },
      children: _categories.map<ExpansionPanel>((Category item) {
        return ExpansionPanel(
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
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

  void _getCategories() async {
    List<Map<String, dynamic>> result =
        await gl.dico.db.query('groupe_couche', where: 'expert=0');
    for (var row in result) {
      _categories += [Category(name: row['label'], filter: row['code'])];
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
              maxWidth: MediaQuery.of(context).size.width * .95,
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
              title: Text(item.name),
              leading: IconButton(
                  icon: const Icon(Icons.get_app_rounded),
                  onPressed: () {
                    setState(() {
                      gl.interfaceSelectedLayerKeys.add(item.name);
                      widget.refreshView();
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
    Map<String, LayerBase> mp = gl.dico.mLayerBases;
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
  const SelectedLayerView({super.key});
  @override
  State<SelectedLayerView> createState() => _SelectedLayerView();
}

class _SelectedLayerView extends State<SelectedLayerView> {
  @override
  Widget build(BuildContext context) {
    return ListView(
      children: [
        ListTile(
          shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(10.0), side: BorderSide.none),
          tileColor: Colors.blueAccent,
          title: gl.interfaceSelectedLayerKeys.length > 0
              ? Text(gl.interfaceSelectedLayerKeys[0])
              : Text('Pas de couche selectionnée.'),
        ),
        ListTile(
          title: gl.interfaceSelectedLayerKeys.length > 1
              ? Text(gl.interfaceSelectedLayerKeys[1])
              : Text('Pas de couche selectionnée.'),
        ),
        ListTile(
          title: gl.interfaceSelectedLayerKeys.length > 2
              ? Text(gl.interfaceSelectedLayerKeys[2])
              : Text('Pas de couche selectionnée.'),
        ),
      ],
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
