import 'dart:ffi';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/categoryTile.dart';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter_map/flutter_map.dart';

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
          child: SingleChildScrollView(
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
          body: CategoryView(category: item,),
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
              maxHeight: MediaQuery.of(context).size.height * .5),
          child: Scrollbar(
              child: SingleChildScrollView(
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
            print(item.name);
            return ListTile(
              title: Text(item.name),
            );
          },
          body: Text('nana'), //_buildLayerListPerTopic(),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }

  void _getLayerData() async {
    List<Map<String, dynamic>> result = await gl.dico.db
        .query('fichiersGis', where: 'expert=0 AND groupe IS NOT NULL');
    result +=
        await gl.dico.db.query('layerApt', where: 'expert=0 AND groupe IS NOT NULL');
    for (var row in result) {
      if (widget.category.filter == row['groupe']) {
        _layerTiles += [
          LayerTile(name: row['NomCourt'], filter: row['groupe'])
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
