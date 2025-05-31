import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:flutter/material.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';
import 'package:latlong2/latlong.dart';

class PopupNotification extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupNotification({
    super.key,
    this.title,
    this.accept,
    this.dialog,
    this.onAccept,
    this.decline,
    this.onDecline,
  });

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(title!),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          onPressed:
              () => {
                Navigator.pop(context, 'OK'),
                if (onAccept != null) onAccept!(),
              },
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed:
                () => {
                  Navigator.pop(context, 'NO'),
                  if (onDecline != null) onDecline!(),
                },
            child: Text(decline!),
          ),
      ],
    );
  }
}

class PopupNoInternet extends StatelessWidget {
  const PopupNoInternet({super.key});

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text("Oups"),
      content: Text("Vous n'avez pas accès à internet."),
      actions: <Widget>[
        TextButton(
          child: Text("OK"),
          onPressed: () {
            Navigator.of(context, rootNavigator: true).pop();
          },
        ),
      ],
    );
  }
}

class PopupPermissions extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupPermissions({
    super.key,
    this.title,
    this.accept,
    this.dialog,
    this.onAccept,
    this.decline,
    this.onDecline,
  });

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(title!),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          onPressed: () => {if (onAccept != null) onAccept!()},
          child: Text(accept!),
        ),
        if (decline != null)
          TextButton(
            onPressed: () => {if (onDecline != null) onDecline!()},
            child: Text(decline!),
          ),
      ],
    );
  }
}

class PopupDownloadSuccess {
  PopupDownloadSuccess(BuildContext context, String layerName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Téléchargement de $layerName."),
          content: Text("$layerName a été téléchargée avec succès."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupDownloadFailed {
  PopupDownloadFailed(BuildContext context, String layerName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Téléchargement de $layerName."),
          content: Text("$layerName n'a pas été téléchargé."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupPDFSaved {
  PopupPDFSaved(BuildContext context, String pdfName) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Export du pdf: $pdfName"),
          content: Text("Export effectué avec succès."),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupColorChooser {
  Color pickerColor = Color(0xff443a49);

  PopupColorChooser(
    Color currentColor,
    BuildContext context,
    Function(Color) colorChange,
    Function after,
  ) {
    pickerColor = currentColor;
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Choisisez une couleur!"),
          content: SingleChildScrollView(
            child: ColorPicker(
              pickerColor: pickerColor,
              onColorChanged: colorChange,
            ),
          ),
          actions: [
            TextButton(
              child: Text("OK"),
              onPressed: () {
                currentColor = pickerColor;
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupNameIntroducer {
  PopupNameIntroducer(
    BuildContext context,
    String currentName,
    Function(String) state,
    Function after,
  ) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Changez de nom"),
          content: SingleChildScrollView(
            child: TextFormField(
              onChanged: (String str) {
                state(str);
              },
              controller: TextEditingController(text: currentName),
            ),
          ),
          actions: [
            TextButton(
              child: Text("Rename"),
              onPressed: () {
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class DrawnLayerMenu extends StatefulWidget {
  final Function(LatLng) state;

  const DrawnLayerMenu({super.key, required this.state});

  @override
  State<StatefulWidget> createState() => _DrawnLayerMenu();
}

class _DrawnLayerMenu extends State<DrawnLayerMenu> {
  final Color active = Colors.black;
  final Color inactive = Colors.blueGrey;
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: ReorderableListView(
        buildDefaultDragHandles: true,
        padding: const EdgeInsets.symmetric(horizontal: 2),
        onReorder: (int oldIndex, int newIndex) {
          setState(() {
            if (oldIndex < newIndex) {
              newIndex -= 1;
            }
            if (gl.polygonLayers.length < newIndex + 1 ||
                gl.polygonLayers.length < oldIndex + 1) {
              return;
            }
            gl.refreshMap(() {
              final PolygonLayer item = gl.polygonLayers.removeAt(oldIndex);
              gl.polygonLayers.insert(newIndex, item);
            });
            if (oldIndex == gl.selectedPolygonLayer) {
              gl.selectedPolygonLayer = newIndex;
            } else if (newIndex == gl.selectedPolygonLayer) {
              if (oldIndex > newIndex) {
                gl.selectedPolygonLayer++;
              } else {
                gl.selectedPolygonLayer--;
              }
            } else if (oldIndex < gl.selectedPolygonLayer &&
                gl.selectedPolygonLayer < newIndex) {
              gl.selectedPolygonLayer--;
            } else if (oldIndex > gl.selectedPolygonLayer &&
                gl.selectedPolygonLayer > newIndex) {
              gl.selectedPolygonLayer++;
            }
          });
        },
        children:
            List<TextButton>.generate(
              gl.polygonLayers.isEmpty ? 0 : gl.polygonLayers.length,
              (int i) => TextButton(
                key: Key('$i'),
                onPressed:
                    i == gl.selectedPolygonLayer
                        ? () {
                          setState(() {
                            widget.state(gl.polygonLayers[i].center);
                          });
                        }
                        : () {
                          setState(() {
                            gl.selectedPolygonLayer = i;
                            widget.state(gl.polygonLayers[i].center);
                          });
                        },
                child: Card(
                  color: gl.polygonLayers[i].colorInside,
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      IconButton(
                        onPressed:
                            i == gl.selectedPolygonLayer
                                ? () {
                                  setState(() {
                                    if (i > 0) {
                                      gl.polygonLayers.removeAt(i);
                                      gl.selectedPolygonLayer--;
                                    } else if (i == 0 &&
                                        gl.polygonLayers.isNotEmpty) {
                                      gl.polygonLayers.removeAt(i);
                                    }
                                  });
                                }
                                : () {
                                  setState(() {
                                    gl.polygonLayers.removeAt(i);
                                    if (i < gl.selectedPolygonLayer) {
                                      gl.selectedPolygonLayer--;
                                    }
                                  });
                                },
                        icon: Icon(
                          Icons.delete_forever,
                          color:
                              gl.selectedPolygonLayer == i ? active : inactive,
                        ),
                      ),
                      Column(
                        children: [
                          i == gl.selectedPolygonLayer
                              ? TextButton(
                                child: Text(
                                  gl.polygonLayers[i].name,
                                  style: TextStyle(color: Colors.black),
                                  textScaler: TextScaler.linear(1.1),
                                ),
                                onPressed: () {
                                  PopupNameIntroducer(
                                    context,
                                    gl.polygonLayers[i].name,
                                    (String nameIt) {
                                      if (nameIt.length > 10) {
                                        nameIt = nameIt.substring(0, 10);
                                      }
                                      setState(
                                        () => gl.polygonLayers[i].name = nameIt,
                                      );
                                    },
                                    () {
                                      setState(() {});
                                    },
                                  );
                                },
                              )
                              : Text(
                                gl.polygonLayers[i].name,
                                style: TextStyle(color: Colors.blueGrey),
                                textScaler: TextScaler.linear(1.0),
                              ),

                          i == gl.selectedPolygonLayer
                              ? Text(
                                "${(gl.polygonLayers[i].area / 1000).round() / 100} Ha",
                                textScaler: TextScaler.linear(1.2),
                              )
                              : Text(
                                "${(gl.polygonLayers[i].area / 1000).round() / 100} Ha",
                                textScaler: TextScaler.linear(1.0),
                              ),
                          i == gl.selectedPolygonLayer
                              ? Text(
                                "${(gl.polygonLayers[i].perimeter).round() / 1000} km",
                                textScaler: TextScaler.linear(1.2),
                              )
                              : Text(
                                "${(gl.polygonLayers[i].perimeter).round() / 1000} km",
                                textScaler: TextScaler.linear(1.0),
                              ),
                        ],
                      ),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.start,
                        children: [
                          Column(
                            children: [
                              IconButton(
                                onPressed: () {
                                  PopupColorChooser(
                                    gl.polygonLayers[i].colorInside,
                                    gl.notificationContext!,
                                    (Color col) {
                                      setState(() {
                                        gl.polygonLayers[i].setColorInside(col);
                                        gl.polygonLayers[i].setColorLine(
                                          Color.fromRGBO(
                                            (col.r * 255).round(),
                                            (col.g * 255).round(),
                                            (col.b * 255).round(),
                                            0.8,
                                          ),
                                        );
                                      });
                                    },
                                    () {},
                                  );
                                },
                                icon: Icon(
                                  Icons.color_lens,
                                  color:
                                      gl.selectedPolygonLayer == i
                                          ? active
                                          : inactive,
                                ),
                              ),
                            ],
                          ),
                          Container(
                            width: 32,
                            height: 48,
                            padding: const EdgeInsets.symmetric(),
                            child: ReorderableDragStartListener(
                              index: i,
                              child: Icon(
                                Icons.drag_indicator,
                                size: 28,
                                color:
                                    gl.selectedPolygonLayer == i
                                        ? active
                                        : inactive,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ) +
            [
              TextButton(
                style: ButtonStyle(),
                key: Key('autsch-5'),
                child: Card(
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [Icon(Icons.add)],
                  ),
                ),
                onPressed: () {
                  setState(() {
                    gl.polygonLayers.add(PolygonLayer(polygonName: "Nouveau"));
                    gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                  });
                },
              ),
            ],
      ),
    );
  }
}

Widget forestimatorSettingsVersion() {
  return Column(
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "Forestimator Mobile",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(2.0),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "version 1.0.2-13",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
      Image.asset("assets/images/LogoForestimator.png"),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "Programmé et soigné par:\nJonathan Lisein et Thierry Thissen",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
    ],
  );
}

Widget forestimatorSettingsPermissions() {
  return Column(
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "Gestion des permissions",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.5),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "GPS: ",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
          Icon(
            getLocation() ? Icons.check_circle : Icons.circle_notifications,
            color: getLocation() ? Colors.green : Colors.red,
          ),
          Text(
            getLocation() ? "Accordé." : "Pas accordé.",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
      Row(children: [Text("")]),
    ],
  );
}

class Item {
  final Widget entry;
  final String name;

  bool isExpanded = false;

  Item({required this.entry, required this.name});
}

class SettingsMenu extends StatefulWidget {
  final Function(LatLng) state;

  const SettingsMenu({super.key, required this.state});

  @override
  State<StatefulWidget> createState() => _SettingsMenu();
}

class _SettingsMenu extends State<SettingsMenu> {
  final Color active = Colors.black;
  final Color inactive = Colors.blueGrey;
  final List<Item> menuItems = [];
  bool _listInitialzed = false;

  @override
  void initState() {
    if (!_listInitialzed) {
      menuItems.add(
        Item(name: "About Forestimator", entry: forestimatorSettingsVersion()),
      );
      menuItems.add(
        Item(name: "Permissions", entry: forestimatorSettingsPermissions()),
      );
    } else {
      _listInitialzed = true;
    }
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SingleChildScrollView(
        child: Container(
          child: ExpansionPanelList(
            expansionCallback: (int panelIndex, bool isExpanded) {
              setState(() {
                menuItems[panelIndex].isExpanded = isExpanded;
              });
            },
            children:
                menuItems.map<ExpansionPanel>((Item item) {
                  return ExpansionPanel(
                    canTapOnHeader: true,
                    headerBuilder: (BuildContext context, bool isExpanded) {
                      return Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [Text(item.name)],
                      );
                    },
                    body: item.isExpanded ? item.entry : Container(),
                    isExpanded: item.isExpanded,
                  );
                }).toList(),
          ),
        ),
      ),
    );
  }
}

class PopupDrawnLayerMenu {
  PopupDrawnLayerMenu(
    BuildContext context,
    String currentName,
    Function(LatLng) state,
    Function after,
  ) {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Organisation des polygones"),

          content: SizedBox(
            width: MediaQuery.of(context).size.width * .95,
            child: DrawnLayerMenu(state: state),
          ),

          actions: [
            TextButton(
              child: Text("Terminé!"),
              onPressed: () {
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}

class PopupSettingsMenu {
  PopupSettingsMenu(
    BuildContext context,
    String currentName,
    Function(LatLng) state,
    Function after,
  ) {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text("Paramètres"),
          content: SizedBox(
            width: MediaQuery.of(context).size.width * .95,
            child: SettingsMenu(state: state),
          ),
          actions: [
            TextButton(
              child: Text("Terminé!"),
              onPressed: () {
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
            ),
          ],
        );
      },
    );
  }
}
