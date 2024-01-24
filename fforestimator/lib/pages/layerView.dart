import 'package:flutter/material.dart';

// stores ExpansionPanel state information
class Item {
  Item({
    required this.expandedValue,
    required this.headerValue,
    this.isExpanded = false,
  });

  String expandedValue;
  String headerValue;
  bool isExpanded;
}

List<Item> getCategories() {
  return <Item>[
    Item(expandedValue: "Couches", headerValue: "Cartes de référence"),
    Item(expandedValue: "Couches", headerValue: "Conditions stationelles"),
    Item(
        expandedValue: "Couches",
        headerValue: "Cartographie des peuplement forestiers"),
    Item(
        expandedValue: "Couches",
        headerValue: "Adéquation des essences aux conditions stationelles"),
    Item(expandedValue: "Couches", headerValue: "Guide des stations"),
    Item(expandedValue: "Couches", headerValue: "Etat sanitaire de la pessière")
  ];
}

class LayerView extends StatefulWidget {
  const LayerView({super.key});

  @override
  State<LayerView> createState() => _LayerView();
}

class _LayerView extends State<LayerView> {
  final List<Item> _data = getCategories();

  @override
  Widget build(BuildContext context) {
    return Scrollbar(
        child: SingleChildScrollView(
      child: Container(
        child: _buildPanel(),
      ),
    ));
  }

  Widget _buildPanel() {
    return ExpansionPanelList(
      expansionCallback: (int index, bool isExpanded) {
        setState(() {
          _data[index].isExpanded = isExpanded;
        });
      },
      children: _data.map<ExpansionPanel>((Item item) {
        return ExpansionPanel(
          headerBuilder: (BuildContext context, bool isExpanded) {
            return ListTile(
              title: Text(item.headerValue),
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
                  children: const <Widget>[
                    Card(
                        child: ListTile(
                            title: Text(
                                'Les couches seront chargés en ListTile'))),
                    Card(
                      child: ListTile(
                        leading: FlutterLogo(),
                        title: Text('One-line with leading widget'),
                      ),
                    ),
                    Card(
                      child: ListTile(
                        title: Text('One-line with trailing widget'),
                        trailing: Icon(Icons.more_vert),
                      ),
                    ),
                    Card(
                      child: ListTile(
                        leading: FlutterLogo(),
                        title: Text('One-line with both widgets'),
                        trailing: Icon(Icons.more_vert),
                      ),
                    ),
                    Card(
                      child: ListTile(
                        title: Text('One-line dense ListTile'),
                        dense: true,
                      ),
                    ),
                    Card(
                      child: ListTile(
                        leading: FlutterLogo(size: 56.0),
                        title: Text('Two-line ListTile'),
                        subtitle: Text('Here is a second line'),
                        trailing: Icon(Icons.more_vert),
                      ),
                    ),
                    Card(
                      child: ListTile(
                        leading: FlutterLogo(size: 72.0),
                        title: Text('Three-line ListTile'),
                        subtitle: Text(
                            'A sufficiently long subtitle warrants three lines.'),
                        trailing: Icon(Icons.more_vert),
                        isThreeLine: true,
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
