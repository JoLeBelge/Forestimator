import 'dart:convert';

import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:flutter/material.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';
import 'package:http/http.dart' as http;
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
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
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
                  surfaceTintColor: Colors.transparent,
                  shadowColor: Colors.transparent,
                  color:
                      i == gl.selectedPolygonLayer
                          ? gl.polygonLayers[i].colorLine
                          : gl.polygonLayers[i].colorInside,
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      IconButton(
                        onPressed:
                            i == gl.selectedPolygonLayer
                                ? () {
                                  setState(() {
                                    //remove polygon
                                    if (i > 0) {
                                      gl.polygonLayers.removeAt(i);
                                      gl.selectedPolygonLayer--;
                                    } else if (i == 0 &&
                                        gl.polygonLayers.isNotEmpty) {
                                      gl.polygonLayers.removeAt(i);
                                    }
                                  });
                                  gl.saveChangesToPolygoneToPrefs = true;
                                }
                                : () {
                                  setState(() {});
                                },
                        icon: Icon(
                          Icons.delete_forever,
                          color:
                              gl.selectedPolygonLayer == i ? active : inactive,
                          size: gl.selectedPolygonLayer == i ? 30 : 20,
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
                                      //change name
                                      setState(() {
                                        gl.polygonLayers[i].name = nameIt;
                                        gl.saveChangesToPolygoneToPrefs = true;
                                      });
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
                                "${(gl.polygonLayers[i].area / 100).round() / 100} Ha",
                                textScaler: TextScaler.linear(1.2),
                              )
                              : Text(
                                "${(gl.polygonLayers[i].area / 100).round() / 100} Ha",
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
                                onPressed:
                                    gl.selectedPolygonLayer == i
                                        ? () {
                                          PopupColorChooser(
                                            gl.polygonLayers[i].colorInside,
                                            gl.notificationContext!,
                                            //change color
                                            (Color col) {
                                              setState(() {
                                                gl.polygonLayers[i]
                                                    .setColorInside(col);
                                                gl.polygonLayers[i]
                                                    .setColorLine(
                                                      Color.fromRGBO(
                                                        (col.r * 255).round(),
                                                        (col.g * 255).round(),
                                                        (col.b * 255).round(),
                                                        1.0,
                                                      ),
                                                    );
                                              });
                                              gl.saveChangesToPolygoneToPrefs =
                                                  true;
                                            },
                                            () {},
                                          );
                                        }
                                        : () {},
                                icon: Icon(
                                  Icons.color_lens,
                                  color:
                                      gl.selectedPolygonLayer == i
                                          ? active
                                          : inactive,
                                  size: gl.selectedPolygonLayer == i ? 30 : 20,
                                ),
                              ),
                              IconButton(
                                onPressed:
                                    gl.selectedPolygonLayer == i
                                        ? () async {
                                          if (await gl.polygonLayers[i]
                                              .onlineSurfaceAnalysis()) {
                                            PopupResultsMenu(
                                              gl.notificationContext!,
                                              gl
                                                  .polygonLayers[gl
                                                      .selectedPolygonLayer]
                                                  .decodedJson,
                                              () {
                                                setState(() {});
                                                (() {});
                                              },
                                              () {
                                                setState(() {});
                                                (() {
                                                  //_settingsMenu = false;
                                                });
                                              },
                                            );
                                          }
                                        }
                                        : () {},
                                icon: Icon(
                                  Icons.analytics,
                                  color:
                                      gl.selectedPolygonLayer == i
                                          ? active
                                          : inactive,
                                  size: gl.selectedPolygonLayer == i ? 30 : 20,
                                ),
                              ),
                            ],
                          ),
                          Container(
                            width: 48,
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
                  //add polygon
                  setState(() {
                    gl.polygonLayers.add(PolygonLayer(polygonName: "Nouveau"));
                    gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                  });
                  gl.saveChangesToPolygoneToPrefs = true;
                },
              ),
            ],
      ),
    );
  }
}

class PopupSearchMenu {
  PopupSearchMenu(
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
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.2),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text("Recherche en ligne", textAlign: TextAlign.justify),
            ],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .95,
              child: SearchMenu(state: state),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  constraints: BoxConstraints(minHeight: 20, minWidth: 200),
                  child: FloatingActionButton(
                    backgroundColor: gl.colorAgroBioTech,
                    child: Text("Retour!", maxLines: 1),
                    onPressed: () {
                      after();
                      Navigator.of(context, rootNavigator: true).pop();
                    },
                  ),
                ),
              ],
            ),
          ],
        );
      },
    );
  }
}

class SearchMenu extends StatefulWidget {
  final Function(LatLng) state;

  const SearchMenu({super.key, required this.state});

  @override
  State<StatefulWidget> createState() => _SearchMenu();
}

List<TextButton> _searchResults = [];

class _SearchMenu extends State<SearchMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);

  int previousLength = 0;

  @override
  Widget build(BuildContext context) {
    gl.refreshSearch = (Function x) {
      mounted
          ? setState(() {
            x();
          })
          : () {
            x();
          };
    };
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 2),
        children:
            [
              TextButton(
                style: ButtonStyle(),
                key: Key('autsch-5'),
                child: Card(
                  child: TextFormField(
                    onChanged: (that) async {
                      if (that.length < previousLength) {
                        previousLength = that.length;
                        return;
                      }
                      previousLength = that.length;
                      if (that.length < 3) {
                        return;
                      }
                      var request = Uri.parse(
                        'http://appliprfw.gembloux.ulg.ac.be/search?q=${that.replaceAll(' ', '+')}+Wallonie&format=json&addressdetails=1',
                      );
                      final response = await http.get(request);
                      List<Map<String, dynamic>> decodedJson;
                      try {
                        (decodedJson =
                            (jsonDecode(response.body) as List)
                                .cast<Map<String, dynamic>>());
                      } catch (e) {
                        gl.print("Error with answer from gecoding service! $e");
                        (decodedJson =
                            (jsonDecode(testNominatimJsonResult) as List)
                                .cast<Map<String, dynamic>>());
                      }
                      gl.poiMarkerList.clear();
                      _searchResults.clear();
                      gl.refreshSearch(() {
                        gl.selectedSearchMarker = -1;
                        gl.print("Selected searchMarker reset");
                        int i = 0;
                        for (var entry in decodedJson) {
                          gl.poiMarkerList.add(
                            gl.PoiMarker(
                              index: i++,
                              position: LatLng(
                                double.parse(entry['lat']),
                                double.parse(entry['lon']),
                              ),
                              name: entry['name'] ?? "Noname??",
                              address:
                                  entry['address']['road'] ??
                                  entry['address']['neighbourhood'] ??
                                  entry['address']['village'] ??
                                  entry['address']['state'] ??
                                  "Lokolobömmele",
                              city:
                                  entry['address']['city'] ??
                                  entry['address']['county'] ??
                                  entry['address']['state'] ??
                                  "Lokolobömmele",
                              postcode: entry['address']['postcode'] ?? "",
                            ),
                          );
                          _searchResults.add(
                            TextButton(
                              onPressed: () {
                                widget.state(
                                  LatLng(
                                    double.parse(entry['lat']),
                                    double.parse(entry['lon']),
                                  ),
                                );
                              },
                              child: Card(
                                margin: EdgeInsets.all(10),
                                color: Color.fromRGBO(200, 255, 150, 0.75),
                                child: Row(
                                  children: [
                                    Column(
                                      children: [
                                        Row(
                                          mainAxisAlignment:
                                              MainAxisAlignment.start,
                                          children: [
                                            Container(
                                              constraints: BoxConstraints(
                                                maxWidth:
                                                    MediaQuery.of(
                                                      gl.notificationContext!,
                                                    ).size.width *
                                                    .7,
                                              ),
                                              child: Text(entry['addresstype']),
                                            ),
                                          ],
                                        ),
                                        Row(
                                          mainAxisAlignment:
                                              MainAxisAlignment.start,
                                          children: [
                                            Container(
                                              constraints: BoxConstraints(
                                                maxWidth:
                                                    MediaQuery.of(
                                                      gl.notificationContext!,
                                                    ).size.width *
                                                    .7,
                                              ),
                                              child: Text(
                                                entry['display_name'],
                                              ),
                                            ),
                                          ],
                                        ),
                                      ],
                                    ),
                                  ],
                                ),
                              ),
                            ),
                          );
                        }
                      });
                    },
                  ),
                ),
                onPressed: () {
                  setState(() {});
                },
              ),
            ] +
            _searchResults,
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
            "version ${gl.forestimatorMobileVersion}",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .5,
            ),
            child: Image.asset("assets/images/LogoForestimator.png"),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "Programmé et soigné par:\nJonathan Lisein et Thierry Thissen\n",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .7,
            ),
            child: Text(
              "Finançements du projet",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.justify,
              textScaler: TextScaler.linear(1.5),
            ),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .7,
            ),
            child: Text(
              "Le développement est financé par l'Accord Cadre de Recherches et Vulgarisation Forestières.\nLe contenu cartographique est en grande partie issu des recherches menées au sein de l'unité de Gestion des Ressources Forestières de Gembloux Agro-Bio Tech (ULiège).\n",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.justify,
              textScaler: TextScaler.linear(1.0),
            ),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .7,
            ),
            child: Text(
              "Contact: Philippe Lejeune\np.lejeune@uliege.be",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.justify,
              textScaler: TextScaler.linear(1.0),
            ),
          ),
        ],
      ),
    ],
  );
}

Widget forestimatorSettingsContacts() {
  return Column(
    children: [
      Image.asset("assets/images/GRF_nouveau_logo_uliege-retina.jpg"),
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Text(
            "\nSur notre Site:\nhttps://www.grf.uliege.be\nPar mail:\nJO.Lisein@uliege.be\n",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            textScaler: TextScaler.linear(1.0),
          ),
        ],
      ),
    ],
  );
}

class ForestimatorLog extends StatefulWidget {
  const ForestimatorLog({super.key});

  @override
  State<StatefulWidget> createState() => _ForestimatorLog();
}

class _ForestimatorLog extends State<ForestimatorLog> {
  int lengthLog = 3;

  @override
  Widget build(BuildContext context) {
    return Column(
      children:
          <Widget>[
            Container(
              constraints: BoxConstraints(
                minWidth: MediaQuery.of(context).size.width * .33,
              ),
              child: FloatingActionButton(
                onPressed: () {
                  setState(() {
                    lengthLog + 5 < gl.onboardLog.length
                        ? lengthLog = lengthLog + 5
                        : lengthLog = gl.onboardLog.length;
                  });
                },
                child:
                    lengthLog != gl.onboardLog.length
                        ? Text("Afficher +")
                        : Text("FIN"),
              ),
            ),
          ] +
          List<Widget>.generate(lengthLog, (i) {
            return gl.onboardLog.length - lengthLog > 0
                ? Row(
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        minWidth: MediaQuery.of(context).size.width * .05,
                      ),
                      child: Text("${gl.onboardLog.length - lengthLog + i}) "),
                    ),
                    Container(
                      constraints: BoxConstraints(
                        maxWidth: MediaQuery.of(context).size.width * .6,
                      ),
                      child: Text(
                        gl.onboardLog[gl.onboardLog.length - lengthLog + i],
                        textScaler: TextScaler.linear(0.75),
                      ),
                    ),
                  ],
                )
                : gl.onboardLog.length - i > 0
                ? Row(
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        minWidth: MediaQuery.of(context).size.width * .05,
                      ),
                      child: Text("$i"),
                    ),
                    Container(
                      constraints: BoxConstraints(
                        minWidth: MediaQuery.of(context).size.width * .6,
                      ),
                      child: Text(
                        gl.onboardLog[i],
                        textScaler: TextScaler.linear(0.75),
                      ),
                    ),
                  ],
                )
                : Row();
          }),
    );
  }
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

Widget forestimatorConfidentiality() {
  return Column(
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            padding: EdgeInsets.all(5),
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .7,
            ),
            child: Text(
              "Forestimator mobile ne collecte aucune donnée. Notre politique de confidentialité est consultable au \nhttps://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_.\nL'application utilise le gps pour afficher votre position actuelle sur la carte et seulement pendant l'utilisation.",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.justify,
              textScaler: TextScaler.linear(1.0),
            ),
          ),
        ],
      ),
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
  final Function state;

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
      menuItems.addAll([
        Item(name: "Permissions", entry: forestimatorSettingsPermissions()),
        Item(name: "About Forestimator", entry: forestimatorSettingsVersion()),
        Item(name: "Contact", entry: forestimatorSettingsContacts()),
        Item(name: "Confidentialité", entry: forestimatorConfidentiality()),
        Item(name: "Debug Logs", entry: ForestimatorLog()),
      ]);
    } else {
      _listInitialzed = true;
    }
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SingleChildScrollView(
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
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.2),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,

          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text("Liste des polygones", textAlign: TextAlign.justify),
            ],
          ),

          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * 95,
              child: DrawnLayerMenu(state: state),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  constraints: BoxConstraints(minHeight: 20, minWidth: 200),
                  child: FloatingActionButton(
                    backgroundColor: gl.colorAgroBioTech,
                    child: Text("Retour!", maxLines: 1),
                    onPressed: () {
                      after();
                      Navigator.of(context, rootNavigator: true).pop();
                    },
                  ),
                ),
              ],
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
    Function state,
    Function after,
  ) {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: gl.colorAgroBioTech,
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
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

Widget _resultRow(String key, String value) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.start,
    children: [
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .8,
        ),
        child: Text(
          key,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          textScaler: TextScaler.linear(1.0),
        ),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .8,
        ),
        child: Text(
          value,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          textScaler: TextScaler.linear(1.0),
        ),
      ),
    ],
  );
}

Widget _resultClassRow(Map<String, dynamic> json, mCode) {
  Color col = Colors.transparent;
  int key;
  try {
    key = gl.dico.mLayerBases[mCode]!.mDicoCol.keys.elementAt(
      json['rastValue'],
    );
  } catch (e) {
    key = -1234567891011;
  }
  if (key != -1234567891011) {
    col = gl.dico.mLayerBases[mCode]!.mDicoCol[key]!;
  }
  return Row(
    mainAxisAlignment: MainAxisAlignment.start,
    children: [
      Container(
        color: col,
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          minHeight: MediaQuery.of(gl.notificationContext!).size.width * .05,
          minWidth: MediaQuery.of(gl.notificationContext!).size.width * .05,
        ),
        child: Container(color: col),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .7,
        ),
        child: Text(
          json['value'].toString(),
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          textScaler: TextScaler.linear(1.0),
        ),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .2,
        ),
        child: Text(
          "${json['prop'].toString()}%",
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          textScaler: TextScaler.linear(1.0),
        ),
      ),
    ],
  );
}

Widget forestimatorResultsHeaderClasse(Map<String, dynamic> json) {
  return Column(
    children:
        <Widget>[
          Container(
            alignment: Alignment.centerLeft,
            padding: EdgeInsets.all(5),
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .5,
            ),
            child: Text(
              "(en % de la surface)",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.justify,
              textScaler: TextScaler.linear(0.75),
            ),
          ),
        ] +
        List<Widget>.generate(json['classes'].length, (i) {
          return _resultClassRow(json['classes'][i], json['layerCode']);
        }),
  );
}

Widget forestimatorResultsHeaderContinue(Map<String, dynamic> json) {
  return Column(
    children: List<Widget>.generate(json.length, (i) {
      return i == 0
          ? Column()
          : _resultRow(
            json.keys.elementAt(i),
            json[json.keys.elementAt(i)].toString(),
          );
    }),
  );
}

class ResultsMenu extends StatefulWidget {
  final Map<String, dynamic> json;

  const ResultsMenu({super.key, required this.json});

  @override
  State<ResultsMenu> createState() => _ResultsMenu();
}

class _ResultsMenu extends State<ResultsMenu> {
  final Color active = Colors.black;
  final Color inactive = Colors.blueGrey;
  final List<Item> menuItems = [];
  bool _listInitialzed = false;

  @override
  void initState() {
    if (!_listInitialzed) {
      for (var result in widget.json['RequestedLayers']) {
        if (result['mean'] != null) {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              entry: forestimatorResultsHeaderContinue(result),
            ),
          );
        } else {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              entry: forestimatorResultsHeaderClasse(result),
            ),
          );
        }
      }
    } else {
      _listInitialzed = true;
    }
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SingleChildScrollView(
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
    );
  }
}

class PopupResultsMenu {
  PopupResultsMenu(
    BuildContext context,
    Map<String, dynamic> json,
    Function state,
    Function after,
  ) {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Colors.transparent,
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                "Résultats",
                textScaler: TextScaler.linear(2.0),
                style: TextStyle(color: Colors.white),
              ),
            ],
          ),
          content: SizedBox(
            width: MediaQuery.of(context).size.width * .95,
            child: ResultsMenu(json: json),
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

String testNominatimJsonResult =
    '[{"place_id": 94311734,"licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright","osm_type": "relation",        "osm_id": 1405439,        "lat": "50.4665284",        "lon": "4.8661892",        "class": "boundary",        "type": "administrative",        "place_rank": 14,        "importance": 0.6128825791589154,        "addresstype": "city",        "name": "Namur",        "display_name": "Namur, Wallonie, België / Belgique / Belgien",        "address": {            "city": "Namur",            "county": "Namur",            "state": "Namur",            "ISO3166-2-lvl6": "BE-WNA",            "region": "Wallonie",            "ISO3166-2-lvl4": "BE-WAL",            "country": "België / Belgique / Belgien",            "country_code": "be"        },        "boundingbox": [            "50.3872825",            "50.5311017",            "4.7230530",            "4.9843576"        ]    },    {        "place_id": 94426182,        "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",        "osm_type": "relation",        "osm_id": 1311816,        "lat": "50.2169296",        "lon": "4.8011620",        "class": "boundary",        "type": "administrative",        "place_rank": 8,        "importance": 0.5819298876612474,        "addresstype": "state",        "name": "Namur",        "display_name": "Namur, Wallonie, België / Belgique / Belgien",        "address": {            "state": "Namur",            "ISO3166-2-lvl6": "BE-WNA",            "region": "Wallonie",            "ISO3166-2-lvl4": "BE-WAL",            "country": "België / Belgique / Belgien",            "country_code": "be"        },        "boundingbox": [            "49.7855452",            "50.6484447",            "4.2857516",            "5.4026708"        ]    },    {        "place_id": 94479969,        "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",        "osm_type": "relation",        "osm_id": 1405410,        "lat": "50.4553015","lon": "4.9000918","class": "boundary","type": "administrative","place_rank": 12,"importance": 0.46874225610105,"addresstype": "county","name": "Namur","display_name": "Namur, Wallonie, België / Belgique / Belgien","address": {"county": "Namur","state": "Namur","ISO3166-2-lvl6": "BE-WNA","region": "Wallonie","ISO3166-2-lvl4": "BE-WAL","country": "België / Belgique / Belgien","country_code": "be"},"boundingbox": ["50.2614207","50.6484447","4.5573790","5.2371384"]},{"place_id": 94206695,"licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright","osm_type": "relation","osm_id": 1701297,"lat": "50.4572419","lon": "4.8497825","class": "boundary","type": "administrative","place_rank": 16,"importance": 0.18673333104448,"addresstype": "city_district","name": "Namur","display_name": "Namur, Wallonie, 5000, België / Belgique / Belgien","address": {"city_district": "Namur","city": "Namur","county": "Namur","state": "Namur","ISO3166-2-lvl6": "BE-WNA","region": "Wallonie","ISO3166-2-lvl4": "BE-WAL","postcode": "5000","country": "België / Belgique / Belgien","country_code": "be"},"boundingbox": ["50.4353622","50.4788755","4.8307218","4.9018176"]}]';
