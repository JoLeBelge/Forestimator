import 'dart:io';

import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/category.dart';

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
          body: _buildLayerListPerTopic(),
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
    if (!finishedInitializingCategories){
      _getCategories();
      }
  }

  Widget _buildLayerListPerTopic() {
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
          body: Scrollbar(
            child: Row(children: <Widget>[
              Container(
                constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .95,
                    maxHeight: MediaQuery.of(context).size.height * .5),
                child: ListView(
                  clipBehavior: Clip.antiAlias,
                  children: <Widget>[
                    Card(child: ListTile(title: Text('gl.dico.db.query()'))),
                    Card(
                      child: ListTile(
                        leading: FlutterLogo(),
                        title: Text('One-line with leading widget'),
                      ),
                    ),
                  ],
                ),
              ),
              Container(
                constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .05,
                    maxHeight: MediaQuery.of(context).size.height * .5),
              ),
            ]),
          ),
          isExpanded: item.isExpanded,
        );
      }).toList(),
    );
  }
}
