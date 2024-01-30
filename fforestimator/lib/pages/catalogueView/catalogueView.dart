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
  const CatalogueView({super.key});
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
  const CategoryView({super.key, required this.category});
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
                  icon: Icon(Icons.get_app_rounded),
                  onPressed: () {
                    setState(() {//TODO:Put Layer in selected list});
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
    List<layerBase> ls = await gl.dico.getLayers();
    for (var row in ls) {
      if (widget.category.filter == row.mGroupe) {
        _layerTiles += [LayerTile(name: row.mNomCourt!, filter: row.mGroupe!)];
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
  final List<LayerTile> selectedLayer;
  const SelectedLayerView({super.key, required this.selectedLayer});
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
          title: widget.selectedLayer.length > 0
              ? Text(widget.selectedLayer[0].name)
              : Text('Pas de couche selectionnée.'),
        ),
        ListTile(
          title: widget.selectedLayer.length > 1
              ? Text(widget.selectedLayer[1].name)
              : Text('Pas de couche selectionnée.'),
        ),
        ListTile(
          title: widget.selectedLayer.length > 2
              ? Text(widget.selectedLayer[2].name)
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
