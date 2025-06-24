import 'dart:convert';

import 'package:downloadsfolder/downloadsfolder.dart';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/pages/catalogueView/legend_view.dart';
import 'package:fforestimator/pages/pdf_screen.dart';
import 'package:fforestimator/tools/color_tools.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:fforestimator/tools/handle_permissions.dart' as permissions;
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:fforestimator/tools/pretty_print_nominatim_results.dart';
import 'package:fforestimator/tools/pretty_print_polygon_results.dart';
import 'package:flutter/material.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';
import 'package:http/http.dart' as http;
import 'package:latlong2/latlong.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:url_launcher/url_launcher.dart';

class PopupDownloadRecomendedLayers extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final Function? onAccept;
  final Function? onDecline;
  const PopupDownloadRecomendedLayers({
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
            List<
              TextButton
            >.generate(gl.polygonLayers.isEmpty ? 0 : gl.polygonLayers.length, (
              int i,
            ) {
              Color activeTextColor =
                  i == gl.selectedPolygonLayer
                      ? getColorTextFromBackground(
                        i == gl.selectedPolygonLayer
                            ? gl.polygonLayers[i].colorInside.withAlpha(255)
                            : Colors.grey.withAlpha(100),
                      )
                      : getColorTextFromBackground(
                        i == gl.selectedPolygonLayer
                            ? gl.polygonLayers[i].colorInside.withAlpha(255)
                            : Colors.grey.withAlpha(100),
                      ).withAlpha(128);
              return TextButton(
                style: ButtonStyle(
                  fixedSize:
                      i == gl.selectedPolygonLayer
                          ? WidgetStateProperty<Size>.fromMap(
                            <WidgetStatesConstraint, Size>{
                              WidgetState.any: Size(200, 160),
                            },
                          )
                          : WidgetStateProperty<Size>.fromMap(
                            <WidgetStatesConstraint, Size>{
                              WidgetState.any: Size(200, 120),
                            },
                          ),
                ),
                key: Key('$i'),
                onPressed:
                    i == gl.selectedPolygonLayer
                        ? () {
                          setState(() {
                            widget.state(gl.polygonLayers[i].center);
                          });
                          gl.refreshMap(() {
                            gl.modeMapShowPolygons = true;
                          });
                        }
                        : () {
                          setState(() {
                            gl.selectedPolygonLayer = i;
                            widget.state(gl.polygonLayers[i].center);
                          });
                          gl.refreshMap(() {
                            gl.modeMapShowPolygons = true;
                          });
                        },
                child: ReorderableDragStartListener(
                  index: i,
                  child: Card(
                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color:
                        i == gl.selectedPolygonLayer
                            ? gl.polygonLayers[i].colorInside.withAlpha(255)
                            : Colors.grey.withAlpha(100),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        IconButton(
                          onPressed:
                              i == gl.selectedPolygonLayer
                                  ? () {
                                    PopupDoYouReally(
                                      gl.notificationContext!,
                                      () {
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
                                      },
                                      "Message",
                                      "\nVoulez vous vraiment supprimer ${gl.polygonLayers[i].name}?\n",
                                    );
                                  }
                                  : () {
                                    setState(() {});
                                  },
                          icon: Icon(
                            Icons.delete_forever,
                            color: activeTextColor,
                            size: gl.selectedPolygonLayer == i ? 30 : 20,
                          ),
                        ),
                        Column(
                          children: [
                            i == gl.selectedPolygonLayer
                                ? TextButton(
                                  child: Text(
                                    gl.polygonLayers[i].name,
                                    style: TextStyle(color: activeTextColor),
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
                                          gl.saveChangesToPolygoneToPrefs =
                                              true;
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
                                  style: TextStyle(color: activeTextColor),
                                  textScaler: TextScaler.linear(1.0),
                                ),

                            i == gl.selectedPolygonLayer
                                ? Text(
                                  "${(gl.polygonLayers[i].area / 100).round() / 100} Ha",
                                  style: TextStyle(color: activeTextColor),
                                  textScaler: TextScaler.linear(1.2),
                                )
                                : Text(
                                  "${(gl.polygonLayers[i].area / 100).round() / 100} Ha",
                                  style: TextStyle(color: activeTextColor),
                                  textScaler: TextScaler.linear(1.0),
                                ),
                            i == gl.selectedPolygonLayer
                                ? Text(
                                  "${(gl.polygonLayers[i].perimeter).round() / 1000} km",
                                  style: TextStyle(color: activeTextColor),
                                  textScaler: TextScaler.linear(1.2),
                                )
                                : Text(
                                  "${(gl.polygonLayers[i].perimeter).round() / 1000} km",
                                  style: TextStyle(color: activeTextColor),
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
                                    color: activeTextColor,
                                    size:
                                        gl.selectedPolygonLayer == i ? 30 : 20,
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
                                    color: activeTextColor,
                                    size:
                                        gl.selectedPolygonLayer == i ? 30 : 20,
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
            }) +
            [
              TextButton(
                style: ButtonStyle(),
                key: Key('autsch-5-addPoly'),
                child: Card(
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [Icon(Icons.add, size: 30)],
                  ),
                ),
                onPressed: () {
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
      barrierDismissible: true,
      barrierColor: Colors.transparent,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(0),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.7),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text("Recherche d'un lieu", textAlign: TextAlign.justify),
            ],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: Container(
              constraints: BoxConstraints(
                minHeight: MediaQuery.of(context).size.height * .4,
                maxHeight: MediaQuery.of(context).size.height * .8,
              ),
              width: MediaQuery.of(context).size.width * .85,
              child: SearchMenu(state: state),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    ).whenComplete(() {
      after();
    });
  }
}

class SearchMenu extends StatefulWidget {
  final Function(LatLng) state;

  const SearchMenu({super.key, required this.state});

  @override
  State<StatefulWidget> createState() => _SearchMenu();
}

class _SearchMenu extends State<SearchMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);

  static String lastSearchKey = "";
  static Map<String, http.Response> searchCache = {};
  static List<TextButton> searchResults = [];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Column(
        children: [
          Container(
            constraints: BoxConstraints(
              minHeight: MediaQuery.of(context).size.height * .05,
              maxHeight: MediaQuery.of(context).size.height * .1,
              minWidth: MediaQuery.of(context).size.width * .8,
              maxWidth: MediaQuery.of(context).size.width * .8,
            ),
            child: Card(
              child: TextFormField(
                autocorrect: false,
                initialValue: lastSearchKey,
                onChanged: (searchString) async {
                  if (searchString.length < 2) {
                    return;
                  }
                  lastSearchKey = searchString;
                  http.Response response;
                  if (searchCache[searchString] != null) {
                    response = searchCache[searchString]!;
                  } else {
                    var request = Uri.parse(
                      'http://appliprfw.gembloux.ulg.ac.be/search?q=${searchString.replaceAll(' ', '+')}+Wallonie&format=json&addressdetails=1',
                    );
                    response = await http.get(request);
                    searchCache[searchString] = response;
                  }
                  List<Map<String, dynamic>> decodedJson;
                  try {
                    (decodedJson =
                        (jsonDecode(response.body) as List)
                            .cast<Map<String, dynamic>>());
                  } catch (e) {
                    gl.print("Error with response from gecoding service! $e");
                    (decodedJson =
                        (jsonDecode(testNominatimJsonResult) as List)
                            .cast<Map<String, dynamic>>());
                  }
                  gl.poiMarkerList.clear();
                  searchResults.clear();
                  setState(() {
                    gl.selectedSearchMarker = -1;
                    int i = 0;
                    for (var entry in decodedJson) {
                      String? typeDeResultat =
                          prettyPrintNominatimResults[entry['addresstype']];
                      if (typeDeResultat == null) {
                        typeDeResultat = entry['addresstype'];
                        gl.print(
                          "Error: not a translated addresstype: ${entry['addresstype']}",
                        );
                      }
                      String? descriptionDeResultat = entry['display_name'];
                      if (descriptionDeResultat == null) {
                        descriptionDeResultat = "Erreur du serveur";
                        gl.print(
                          "Erreur du serveur geocoding : ${entry['display_name']}",
                        );
                      } else {
                        descriptionDeResultat = descriptionDeResultat
                            .replaceAll(", België /", "");
                        descriptionDeResultat = descriptionDeResultat
                            .replaceAll("/ Belgien", "");
                        descriptionDeResultat = descriptionDeResultat
                            .replaceAll("Wallonie, ", "");
                        descriptionDeResultat = descriptionDeResultat
                            .replaceAll("Belgique", "");
                      }
                      Color boxColor = getColorFromName(typeDeResultat!);
                      Color textColor = getColorTextFromBackground(boxColor);
                      gl.poiMarkerList.add(
                        gl.PoiMarker(
                          index: i++,
                          position: LatLng(
                            double.parse(entry['lat']),
                            double.parse(entry['lon']),
                          ),
                          name: typeDeResultat,
                          address: descriptionDeResultat,
                          city:
                              entry['address']['city'] ??
                              entry['address']['county'] ??
                              entry['address']['state'] ??
                              "",
                          postcode: entry['address']['postcode'] ?? "",
                        ),
                      );
                      searchResults.add(
                        TextButton(
                          onPressed: () {
                            gl.refreshMap(() {
                              gl.modeMapShowSearchMarker = true;
                            });
                            widget.state(
                              LatLng(
                                double.parse(entry['lat']),
                                double.parse(entry['lon']),
                              ),
                            );
                          },
                          child: Card(
                            margin: EdgeInsets.all(4),
                            color: boxColor,
                            child: Row(
                              children: [
                                Column(
                                  children: [
                                    Row(
                                      mainAxisAlignment:
                                          MainAxisAlignment.center,
                                      children: [
                                        Container(
                                          alignment: Alignment.center,
                                          constraints: BoxConstraints(
                                            maxWidth:
                                                MediaQuery.of(
                                                  gl.notificationContext!,
                                                ).size.width *
                                                .7,
                                          ),
                                          child: Text(
                                            typeDeResultat,
                                            style: TextStyle(
                                              color: textColor,
                                              fontSize: 18,
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                    Row(
                                      mainAxisAlignment:
                                          MainAxisAlignment.start,
                                      children: [
                                        Container(
                                          padding: EdgeInsets.all(10),
                                          constraints: BoxConstraints(
                                            maxWidth:
                                                MediaQuery.of(
                                                  gl.notificationContext!,
                                                ).size.width *
                                                .75,
                                          ),
                                          child: Text(
                                            descriptionDeResultat,
                                            style: TextStyle(color: textColor),
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
          ),
          Container(
            padding: EdgeInsets.all(0),
            constraints: BoxConstraints(
              minHeight: MediaQuery.of(context).size.height * .2,
              maxHeight: MediaQuery.of(context).size.height * .7,
            ),
            child: ListView(
              padding: const EdgeInsets.symmetric(horizontal: 0),
              children: <Widget>[] + searchResults,
            ),
          ),
        ],
      ),
    );
  }
}

int developperModeCounter = 0;
Widget forestimatorSettingsVersion(Function state) {
  return Container(
    padding: EdgeInsets.all(7.5),
    child: Column(
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
                maxWidth:
                    MediaQuery.of(gl.notificationContext!).size.width * .5,
              ),
              child: TextButton(
                onPressed: () async {
                  developperModeCounter++;
                  if (developperModeCounter > 6) {
                    state(() {
                      gl.modeDevelopper = !gl.modeDevelopper;
                    });
                    final SharedPreferences prefs =
                        await SharedPreferences.getInstance();
                    await prefs.setBool('modeDevelopper', gl.modeDevelopper);
                    developperModeCounter = 0;
                  }
                },
                child: Image.asset("assets/images/LogoForestimator.png"),
              ),
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
                maxWidth:
                    MediaQuery.of(gl.notificationContext!).size.width * .7,
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
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              constraints: BoxConstraints(
                maxWidth:
                    MediaQuery.of(gl.notificationContext!).size.width * .9,
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
                maxWidth:
                    MediaQuery.of(gl.notificationContext!).size.width * .7,
              ),
              child: Text(
                "Contact: Philippe Lejeune",
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
            TextButton(
              onPressed: () {
                launchURL('p.lejeune@uliege.be');
              },
              child: Text(
                "p.lejeune@uliege.be",
                overflow: TextOverflow.clip,
                textAlign: TextAlign.left,
                textScaler: TextScaler.linear(1.0),
                style: TextStyle(color: Colors.black),
              ),
            ),
          ],
        ),
      ],
    ),
  );
}

void launchURL(String url) async {
  final Uri uri = Uri.parse(url);
  try {
    await launchUrl(uri);
  } catch (e) {
    gl.print('Error: Could not launch $uri because $e');
  }
}

Widget forestimatorSettingsContacts() {
  return Container(
    padding: EdgeInsets.all(7.5),
    child: Column(
      children: [
        Image.asset("assets/images/GRF_nouveau_logo_uliege-retina.jpg"),
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Text(
              "Sur notre Site:",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.left,
              textScaler: TextScaler.linear(1.0),
            ),
          ],
        ),
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            TextButton(
              onPressed: () {
                launchURL('https://www.grf.uliege.be');
              },
              child: Text(
                "https://www.grf.uliege.be",
                overflow: TextOverflow.clip,
                textAlign: TextAlign.left,
                textScaler: TextScaler.linear(1.0),
                style: TextStyle(color: Colors.blue),
              ),
            ),
          ],
        ),
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Text(
              "ou par mail:",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.left,
              textScaler: TextScaler.linear(1.0),
            ),
          ],
        ),
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            TextButton(
              onPressed: () {
                launchURL('JO.Lisein@uliege.be');
              },
              child: Text(
                "JO.Lisein@uliege.be",
                overflow: TextOverflow.clip,
                textAlign: TextAlign.left,
                textScaler: TextScaler.linear(1.0),
                style: TextStyle(color: Colors.black),
              ),
            ),
          ],
        ),
      ],
    ),
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

Widget forestimatorSettingsPermissions(Function state) {
  return Container(
    padding: EdgeInsets.all(7.5),
    child: Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Text(
              "Gestion des permissions",
              overflow: TextOverflow.clip,
              textAlign: TextAlign.left,
              textScaler: TextScaler.linear(1.5),
              style: TextStyle(color: Colors.black),
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
              style: TextStyle(color: Colors.black),
            ),
            TextButton(
              onPressed: () {
                openPhoneForestimatorSettings();
              },
              child: Row(
                children: [
                  Icon(
                    getLocation()
                        ? Icons.check_circle
                        : Icons.circle_notifications,
                    color: getLocation() ? Colors.green : Colors.red,
                    size: 25,
                  ),
                  Text(
                    getLocation() ? "Accordé." : "Pas accordé.",
                    overflow: TextOverflow.clip,
                    textAlign: TextAlign.left,
                    textScaler: TextScaler.linear(1.0),
                    style: TextStyle(color: Colors.black),
                  ),
                ],
              ),
            ),
          ],
        ),
        if (permissions.sdkInt < 33)
          Row(
            mainAxisAlignment: MainAxisAlignment.start,
            children: [
              Text(
                "Stockage des pdf: ",
                overflow: TextOverflow.clip,
                textAlign: TextAlign.left,
                textScaler: TextScaler.linear(1.0),
                style: TextStyle(color: Colors.black),
              ),
              TextButton(
                onPressed: () {
                  openPhoneForestimatorSettings();
                },
                child: Row(
                  children: [
                    Icon(
                      getStorage()
                          ? Icons.check_circle
                          : Icons.circle_notifications,
                      color: getStorage() ? Colors.green : Colors.red,
                      size: 25,
                    ),
                    Text(
                      getStorage() ? "Accordé." : "Pas accordé.",
                      overflow: TextOverflow.clip,
                      textAlign: TextAlign.left,
                      textScaler: TextScaler.linear(1.0),
                      style: TextStyle(color: Colors.black),
                    ),
                  ],
                ),
              ),
            ],
          ),
      ],
    ),
  );
}

Widget forestimatorConfidentiality() {
  return Column(
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            padding: EdgeInsets.all(5),
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .9,
            ),
            child: Text(
              "Forestimator mobile ne collecte aucune donnée. Notre politique de confidentialité est consultable au:",
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
          TextButton(
            onPressed: () {
              launchURL(
                'https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_',
              );
            },
            child: Container(
              padding: EdgeInsets.all(5),
              constraints: BoxConstraints(
                maxWidth:
                    MediaQuery.of(gl.notificationContext!).size.width * .8,
              ),
              child: Text(
                "https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_",
                overflow: TextOverflow.clip,
                textAlign: TextAlign.left,
                textScaler: TextScaler.linear(1.0),
                style: TextStyle(color: Colors.blue),
              ),
            ),
          ),
        ],
      ),
      Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            padding: EdgeInsets.all(5),
            constraints: BoxConstraints(
              maxWidth: MediaQuery.of(gl.notificationContext!).size.width * .9,
            ),
            child: Text(
              "L'application utilise le gps pour afficher votre position actuelle sur la carte et seulement pendant l'utilisation.",
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

  bool isExpanded = true;

  Item({required this.entry, required this.name});
}

class ItemSettings {
  final Widget entry;
  final String name;

  bool isExpanded = false;

  ItemSettings({required this.entry, required this.name});
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
  final List<ItemSettings> menuItems = [];
  bool _listInitialzed = false;

  @override
  void initState() {
    if (!_listInitialzed) {
      menuItems.addAll([
        ItemSettings(
          name: "Permissions",
          entry: forestimatorSettingsPermissions((Function f) {
            _listInitialzed = false;
            setState(f());
          }),
        ),
        ItemSettings(
          name: "À propos de Forestimator",
          entry: forestimatorSettingsVersion((Function f) {
            setState(() {
              f();
              gl.modeDevelopper
                  ? menuItems.add(
                    ItemSettings(name: "Debug Logs", entry: ForestimatorLog()),
                  )
                  : menuItems.removeWhere(
                    (item) => item.name == "Debug Logs" ? true : false,
                  );
              gl.modeDevelopper
                  ? gl.print("Developper mode activated")
                  : gl.print("Developper mode deactivated");
            });
          }),
        ),
        ItemSettings(name: "Contact", entry: forestimatorSettingsContacts()),
        ItemSettings(
          name: "Confidentialité",
          entry: forestimatorConfidentiality(),
        ),
        if (gl.modeDevelopper)
          ItemSettings(name: "Debug Logs", entry: ForestimatorLog()),
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
              menuItems.map<ExpansionPanel>((ItemSettings item) {
                return ExpansionPanel(
                  canTapOnHeader: true,
                  headerBuilder: (BuildContext context, bool isExpanded) {
                    return Row(
                      mainAxisAlignment: MainAxisAlignment.center,
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
          title: Container(
            alignment: Alignment.center,
            constraints: BoxConstraints(
              minHeight: 20,
              minWidth: MediaQuery.of(context).size.width * .8,
            ),
            child: Text(
              "Paramètres",
              maxLines: 1,
              style: TextStyle(color: Colors.black),
            ),
          ),
          content: SizedBox(
            width: MediaQuery.of(context).size.width * .95,
            child: SettingsMenu(state: state),
          ),
          actions: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  constraints: BoxConstraints(
                    minHeight: 20,
                    minWidth: MediaQuery.of(context).size.width * .8,
                  ),
                  child: TextButton(
                    child: Text(
                      "Retour!",
                      maxLines: 1,
                      style: TextStyle(color: Colors.black),
                    ),
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
    if (key > 0) key--;
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

Widget forestimatorResultsHeaderContinue(
  Map<String, dynamic> json,
  String layerCode,
) {
  return Column(
    children: List<Widget>.generate(json.length, (i) {
      if (!(i == 0 ||
          prettyPrintContinousResults[layerCode] == null ||
          prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)] ==
              null)) {
        gl.print(
          "Error printing reults: ${prettyPrintContinousResults[layerCode]}",
        );
      }
      return i == 0 ||
              prettyPrintContinousResults[layerCode] == null ||
              prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)] ==
                  null
          ? Container()
          : _resultRow(
            json.keys.elementAt(i),
            "${prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)]}: ${json[json.keys.elementAt(i)].toString()}",
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
      for (var result in widget.json['RequestedLayers'] ?? {}) {
        if (result['mean'] != null) {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              entry: forestimatorResultsHeaderContinue(
                result,
                result['layerCode'],
              ),
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
      backgroundColor: Colors.transparent,
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
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Container(
                          constraints: BoxConstraints(
                            maxWidth: MediaQuery.of(context).size.width * .7,
                          ),
                          child: Text(
                            item.name,
                            style: TextStyle(color: Colors.black, fontSize: 20),
                          ),
                        ),
                      ],
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

String testNominatimJsonResult =
    '[{"place_id": 94311734,"licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright","osm_type": "relation",        "osm_id": 1405439,        "lat": "50.4665284",        "lon": "4.8661892",        "class": "boundary",        "type": "administrative",        "place_rank": 14,        "importance": 0.6128825791589154,        "addresstype": "city",        "name": "ERROR",        "display_name": "Namur, Wallonie, België / Belgique / Belgien",        "address": {            "city": "Namur",            "county": "Namur",            "state": "Namur",            "ISO3166-2-lvl6": "BE-WNA",            "region": "Wallonie",            "ISO3166-2-lvl4": "BE-WAL",            "country": "België / Belgique / Belgien",            "country_code": "be"        },        "boundingbox": [            "50.3872825",            "50.5311017",            "4.7230530",            "4.9843576"        ]    },    {        "place_id": 94426182,        "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",        "osm_type": "relation",        "osm_id": 1311816,        "lat": "50.2169296",        "lon": "4.8011620",        "class": "boundary",        "type": "administrative",        "place_rank": 8,        "importance": 0.5819298876612474,        "addresstype": "state",        "name": "Namur",        "display_name": "Namur, Wallonie, België / Belgique / Belgien",        "address": {            "state": "Namur",            "ISO3166-2-lvl6": "BE-WNA",            "region": "Wallonie",            "ISO3166-2-lvl4": "BE-WAL",            "country": "België / Belgique / Belgien",            "country_code": "be"        },        "boundingbox": [            "49.7855452",            "50.6484447",            "4.2857516",            "5.4026708"        ]    },    {        "place_id": 94479969,        "licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright",        "osm_type": "relation",        "osm_id": 1405410,        "lat": "50.4553015","lon": "4.9000918","class": "boundary","type": "administrative","place_rank": 12,"importance": 0.46874225610105,"addresstype": "county","name": "Namur","display_name": "Namur, Wallonie, België / Belgique / Belgien","address": {"county": "Namur","state": "Namur","ISO3166-2-lvl6": "BE-WNA","region": "Wallonie","ISO3166-2-lvl4": "BE-WAL","country": "België / Belgique / Belgien","country_code": "be"},"boundingbox": ["50.2614207","50.6484447","4.5573790","5.2371384"]},{"place_id": 94206695,"licence": "Data © OpenStreetMap contributors, ODbL 1.0. http://osm.org/copyright","osm_type": "relation","osm_id": 1701297,"lat": "50.4572419","lon": "4.8497825","class": "boundary","type": "administrative","place_rank": 16,"importance": 0.18673333104448,"addresstype": "city_district","name": "Namur","display_name": "Namur, Wallonie, 5000, België / Belgique / Belgien","address": {"city_district": "Namur","city": "Namur","county": "Namur","state": "Namur","ISO3166-2-lvl6": "BE-WNA","region": "Wallonie","ISO3166-2-lvl4": "BE-WAL","postcode": "5000","country": "België / Belgique / Belgien","country_code": "be"},"boundingbox": ["50.4353622","50.4788755","4.8307218","4.9018176"]}]';

class OnlineMapStatusTool extends StatefulWidget {
  final LayerTile layerTile;

  const OnlineMapStatusTool({super.key, required this.layerTile});

  @override
  State<StatefulWidget> createState() => _OnlineMapStatusTool();
}

class _OnlineMapStatusTool extends State<OnlineMapStatusTool> {
  @override
  Widget build(BuildContext context) => Column(
    children: [
      widget.layerTile.downloadedControlBar(),
      if (widget.layerTile.downloadable) LayerDownloader(widget.layerTile),
      gl.anaSurfSelectedLayerKeys.contains(widget.layerTile.key)
          ? TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(
                <WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(
                    MediaQuery.of(context).size.width * .99,
                    MediaQuery.of(context).size.height * .075,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                const Icon(Icons.pentagon, size: 28, color: Colors.black),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .05,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .6,
                  ),
                  child: Text(
                    "La couche est selectionnée pour l'analyse surfacique.",
                    style: TextStyle(color: Colors.black),
                  ),
                ),
              ],
            ),
            onPressed: () async {
              setState(() {
                if (gl.anaSurfSelectedLayerKeys.length > 1) {
                  gl.anaSurfSelectedLayerKeys.remove(widget.layerTile.key);
                  widget.layerTile.selected = false;
                }
              });
              final SharedPreferences prefs =
                  await SharedPreferences.getInstance();
              await prefs.setStringList(
                'anaSurfSelectedLayerKeys',
                gl.anaSurfSelectedLayerKeys,
              );
            },
          )
          : TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(
                <WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(
                    MediaQuery.of(context).size.width * .99,
                    MediaQuery.of(context).size.height * .075,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                const Icon(
                  Icons.pentagon_outlined,
                  size: 28,
                  color: Colors.black,
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .05,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .6,
                  ),
                  child: Text(
                    "La couche n'est pas selectionnée pour l'analyse surfacique.",
                    style: TextStyle(color: Colors.black),
                  ),
                ),
              ],
            ),
            onPressed: () async {
              setState(() {
                if (gl.anaSurfSelectedLayerKeys.length > 1) {
                  gl.anaSurfSelectedLayerKeys.insert(0, widget.layerTile.key);
                  widget.layerTile.selected = true;
                }
              });
              final SharedPreferences prefs =
                  await SharedPreferences.getInstance();
              await prefs.setStringList(
                'anaSurfSelectedLayerKeys',
                gl.anaSurfSelectedLayerKeys,
              );
            },
          ),
      gl.anaPtSelectedLayerKeys.contains(widget.layerTile.key)
          ? TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(
                <WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(
                    MediaQuery.of(context).size.width * .99,
                    MediaQuery.of(context).size.height * .075,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                const Icon(Icons.location_on, size: 28, color: Colors.black),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .05,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .6,
                  ),
                  child: Text(
                    "La couche est selectionnée pour l'analyse ponctuelle.",
                    style: TextStyle(color: Colors.black),
                  ),
                ),
              ],
            ),
            onPressed: () async {
              setState(() {
                if (gl.anaPtSelectedLayerKeys.length > 1) {
                  gl.anaPtSelectedLayerKeys.remove(widget.layerTile.key);
                  widget.layerTile.selected = false;
                }
              });
              final SharedPreferences prefs =
                  await SharedPreferences.getInstance();
              await prefs.setStringList(
                'anaPtSelectedLayerKeys',
                gl.anaPtSelectedLayerKeys,
              );
            },
          )
          : TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(
                <WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(
                    MediaQuery.of(context).size.width * .99,
                    MediaQuery.of(context).size.height * .075,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                const Icon(Icons.location_off, size: 28, color: Colors.black),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .05,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * .6,
                  ),
                  child: Text(
                    "La couche n'est pas selectionnée pour l'analyse ponctuelle.",
                    style: TextStyle(color: Colors.black),
                  ),
                ),
              ],
            ),
            onPressed: () async {
              setState(() {
                if (gl.anaPtSelectedLayerKeys.length > 1) {
                  gl.anaPtSelectedLayerKeys.insert(0, widget.layerTile.key);
                  widget.layerTile.selected = true;
                }
              });
              final SharedPreferences prefs =
                  await SharedPreferences.getInstance();
              await prefs.setStringList(
                'anaPtSelectedLayerKeys',
                gl.anaPtSelectedLayerKeys,
              );
            },
          ),
      if (gl.dico.getLayerBase(widget.layerTile.key).hasDoc())
        TextButton(
          style: ButtonStyle(
            minimumSize: WidgetStateProperty<Size>.fromMap(
              <WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  MediaQuery.of(context).size.width * .99,
                  MediaQuery.of(context).size.height * .075,
                ),
              },
            ),
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.start,
            children: [
              const Icon(Icons.picture_as_pdf, size: 28, color: Colors.black),
              Container(
                constraints: BoxConstraints(
                  maxWidth: MediaQuery.of(context).size.width * .05,
                ),
              ),
              Container(
                constraints: BoxConstraints(
                  maxWidth: MediaQuery.of(context).size.width * .6,
                ),
                child: Text(
                  "Consulter la documentation relative à cette couche cartographique",
                  style: TextStyle(color: Colors.black),
                ),
              ),
            ],
          ),
          onPressed: () {
            PopupPdfMenu(gl.notificationContext!, widget.layerTile);
          },
        ),
      if ((gl.dico.getLayerBase(widget.layerTile.key).mGroupe == "APT_FEE" ||
              gl.dico.getLayerBase(widget.layerTile.key).mGroupe == "APT_CS") &&
          gl.dico
              .getEss(gl.dico.getLayerBase(widget.layerTile.key).getEssCode())
              .hasFEEapt())
        TextButton(
          style: ButtonStyle(
            minimumSize: WidgetStateProperty<Size>.fromMap(
              <WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  MediaQuery.of(context).size.width * .99,
                  MediaQuery.of(context).size.height * .075,
                ),
              },
            ),
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.start,
            children: [
              const Icon(
                Icons.picture_as_pdf_outlined,
                size: 28,
                color: Colors.black,
              ),
              Container(
                constraints: BoxConstraints(
                  maxWidth: MediaQuery.of(context).size.width * .05,
                ),
              ),
              Container(
                constraints: BoxConstraints(
                  maxWidth: MediaQuery.of(context).size.width * .6,
                ),
                child: Text(
                  "Consulter la fiche-essence ${gl.dico.getEss(gl.dico.getLayerBase(widget.layerTile.key).getEssCode()).getNameAndPrefix()}",
                  style: TextStyle(color: Colors.black),
                ),
              ),
            ],
          ),
          onPressed: () {
            String path =
                "${gl.pathExternalStorage}/FEE-${gl.dico.getEss(gl.dico.getLayerBase(widget.layerTile.key).getEssCode()).mCode}.pdf";
            PopupPdfMenu(gl.notificationContext!, widget.layerTile, path: path);
          },
        ),
    ],
  );
}

class OnlineMapMenu extends StatefulWidget {
  const OnlineMapMenu({super.key});

  @override
  State<StatefulWidget> createState() => _OnlineMapMenu();
}

class _OnlineMapMenu extends State<OnlineMapMenu> {
  int _selectedCategory = -1;
  int _selectedMap = -1;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 0),
        children: _injectGroupData(
          (int i, GroupeCouche groupe) => TextButton(
            style: ButtonStyle(
              minimumSize:
                  i == _selectedCategory
                      ? WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .15,
                          ),
                        },
                      )
                      : WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .075,
                          ),
                        },
                      ),
            ),
            key: Key('$i'),
            onPressed:
                i == _selectedCategory
                    ? () {
                      setState(() {
                        _selectedCategory = -1;
                        _selectedMap = -1;
                      });
                    }
                    : () {
                      setState(() {
                        _selectedCategory = i;
                        _selectedMap = -1;
                      });
                    },
            child: Card(
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color:
                  i == _selectedCategory
                      ? gl.colorAgroBioTech.withAlpha(25)
                      : gl.colorAgroBioTech.withAlpha(175),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  i != _selectedCategory
                      ? Container(
                        alignment: Alignment.center,
                        padding: EdgeInsets.all(3),
                        constraints: BoxConstraints(
                          maxWidth: MediaQuery.of(context).size.width * .99,
                          minHeight: MediaQuery.of(context).size.width * .2,
                        ),
                        child: Text(
                          groupe.mLabel,
                          textAlign: TextAlign.justify,
                          style: TextStyle(color: Colors.black, fontSize: 22),
                        ),
                      )
                      : Container(
                        alignment: Alignment.center,
                        padding: EdgeInsets.all(0),
                        constraints: BoxConstraints(
                          maxWidth: MediaQuery.of(context).size.width * .99,
                        ),
                        child: ListBody(
                          children:
                              <Widget>[
                                Card(
                                  color: gl.colorAgroBioTech.withAlpha(175),
                                  child: Container(
                                    alignment: Alignment.center,
                                    padding: EdgeInsets.all(3),
                                    constraints: BoxConstraints(
                                      maxWidth:
                                          MediaQuery.of(context).size.width *
                                          .99,
                                      minHeight:
                                          MediaQuery.of(context).size.width *
                                          .2,
                                    ),
                                    child: Text(
                                      groupe.mLabel,
                                      textAlign: TextAlign.justify,
                                      style: TextStyle(
                                        color: Colors.black,
                                        fontSize: 22,
                                      ),
                                    ),
                                  ),
                                ),
                              ] +
                              _injectLayerData(groupe.mCode, (
                                int i,
                                LayerTile layerTile,
                              ) {
                                return TextButton(
                                  style: ButtonStyle(
                                    minimumSize:
                                        i == _selectedMap
                                            ? WidgetStateProperty<Size>.fromMap(
                                              <WidgetStatesConstraint, Size>{
                                                WidgetState.any: Size(
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.width *
                                                      .99,
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.height *
                                                      .1,
                                                ),
                                              },
                                            )
                                            : WidgetStateProperty<Size>.fromMap(
                                              <WidgetStatesConstraint, Size>{
                                                WidgetState.any: Size(
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.width *
                                                      .99,
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.height *
                                                      .075,
                                                ),
                                              },
                                            ),
                                  ),

                                  onPressed: () {
                                    setState(() {
                                      _selectedMap == i
                                          ? _selectedMap = -1
                                          : _selectedMap = i;
                                    });
                                  },
                                  child: Card(
                                    color:
                                        i == _selectedMap
                                            ? Colors.white.withAlpha(255)
                                            : Colors.white.withAlpha(150),
                                    child:
                                        i != _selectedMap
                                            ? Container(
                                              constraints: BoxConstraints(
                                                minHeight:
                                                    MediaQuery.of(
                                                      context,
                                                    ).size.width *
                                                    .15,
                                              ),
                                              child: Column(
                                                mainAxisAlignment:
                                                    MainAxisAlignment.center,
                                                children: [
                                                  Row(
                                                    mainAxisAlignment:
                                                        MainAxisAlignment
                                                            .center,
                                                    children: [
                                                      Container(
                                                        constraints:
                                                            BoxConstraints(
                                                              maxWidth:
                                                                  MediaQuery.of(
                                                                    context,
                                                                  ).size.width *
                                                                  .8,
                                                            ),
                                                        child: Text(
                                                          layerTile.name,
                                                          textAlign:
                                                              TextAlign.justify,
                                                          style: TextStyle(
                                                            color: Colors.black,
                                                            fontSize: 18,
                                                          ),
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                ],
                                              ),
                                            )
                                            : Column(
                                              children: [
                                                Row(
                                                  mainAxisAlignment:
                                                      MainAxisAlignment.center,
                                                  children: [
                                                    Text(layerTile.name),
                                                  ],
                                                ),
                                                OnlineMapStatusTool(
                                                  layerTile: layerTile,
                                                ),
                                                LegendView(
                                                  layerKey: layerTile.key,
                                                  color:
                                                      gl.colorBackgroundSecondary,
                                                  constraintsText:
                                                      BoxConstraints(
                                                        minWidth:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.width *
                                                            .4,
                                                        maxWidth:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.width *
                                                            .4,
                                                        minHeight:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.height *
                                                            .02,
                                                        maxHeight:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.height *
                                                            .02,
                                                      ),
                                                  constraintsColors:
                                                      BoxConstraints(
                                                        minWidth:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.width *
                                                            .4,
                                                        maxWidth:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.width *
                                                            .4,
                                                        minHeight:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.height *
                                                            .02,
                                                        maxHeight:
                                                            MediaQuery.of(
                                                              context,
                                                            ).size.height *
                                                            .02,
                                                      ),
                                                ),
                                                Row(
                                                  mainAxisAlignment:
                                                      MainAxisAlignment.center,
                                                  children: [
                                                    layerTile.proprietaire(),
                                                  ],
                                                ),
                                              ],
                                            ),
                                  ),
                                );
                              }),
                        ),
                      ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  List<Widget> _injectGroupData(Widget Function(int, GroupeCouche) generate) {
    Map<String, Null> groupesNonVides = {};
    for (String key in gl.dico.mLayerBases.keys) {
      if (gl.offlineMode ? gl.dico.getLayerBase(key).mOffline : true) {
        groupesNonVides[gl.dico.getLayerBase(key).mGroupe] = null;
      }
    }
    List<GroupeCouche> groupes = [];
    for (String key in groupesNonVides.keys) {
      for (GroupeCouche couche in gl.dico.mGrCouches) {
        if (couche.mCode == key) {
          groupes.add(couche);
        }
      }
    }

    return List<Widget>.generate(groupes.length, (i) {
      return generate(i, groupes[i]);
    });
  }

  List<Widget> _injectLayerData(
    String category,
    Widget Function(int, LayerTile) generate,
  ) {
    Map<String, LayerBase> mp = gl.dico.mLayerBases;
    List<LayerTile> layer = [];
    for (var key in mp.keys) {
      if (category == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
          mp[key]!.mVisu &&
          mp[key]?.mTypeGeoservice == "" &&
          (gl.offlineMode ? mp[key]!.mOffline : true)) {
        layer.add(
          LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe",
          ),
        );
      }
    }
    return List<Widget>.generate(layer.length, (i) {
      return generate(i, layer[i]);
    });
  }
}

class PopupOnlineMapMenu {
  final Function after;
  PopupOnlineMapMenu(BuildContext context, this.after) {
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
              Text("Liste des cartes en ligne", textAlign: TextAlign.justify),
            ],
          ),

          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .995,
              child: OnlineMapMenu(),
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
    ).whenComplete(() {
      after();
    });
  }
}

class PopupPdfMenu {
  PopupPdfMenu(BuildContext context, LayerTile layerTile, {String path = ""}) {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        if (path == "") {
          path =
              "${gl.pathExternalStorage}/${gl.dico.getLayerBase(layerTile.key).mPdfName}";
        }
        return PDFScreen(
          path: path,
          titre: "documentation", //+ item.mNomCourt,
          currentPage: int.parse(
            gl.dico.getLayerBase(layerTile.key).mPdfPage.toString(),
          ),
        );
      },
    );
  }
}

class PopupLayerSwitcher {
  PopupLayerSwitcher(BuildContext context, Function after) {
    showDialog(
      barrierDismissible: true,
      barrierColor: Colors.transparent,
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.5),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .80,
              height:
                  MediaQuery.of(context).size.height * .05 +
                  MediaQuery.of(context).size.height * .05 +
                  MediaQuery.of(context).size.height * .05 +
                  MediaQuery.of(context).size.height * .18 +
                  MediaQuery.of(context).size.height * .3 +
                  MediaQuery.of(context).size.width * .15,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * .04,
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          "Controlez les couches visibles",
                          textAlign: TextAlign.justify,
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: MediaQuery.of(context).size.height * .025,
                          ),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * .18,
                    child: UpperLayerControl(),
                  ),
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * .04,
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          "Changez les cartes affichées",
                          textAlign: TextAlign.justify,
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: MediaQuery.of(context).size.height * .025,
                          ),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * .3,
                    child: LayerSwitcher(),
                  ),
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * .04,
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          "Catalogues des couches",
                          textAlign: TextAlign.justify,
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: MediaQuery.of(context).size.height * .025,
                          ),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.width * .15,
                    child: ViewControl(),
                  ),
                ],
              ),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    ).whenComplete(() {
      after();
    });
  }
}

class ViewControl extends StatefulWidget {
  const ViewControl({super.key});
  @override
  State<ViewControl> createState() => _ViewControl();
}

class _ViewControl extends State<ViewControl> {
  bool _modeViewOfflineMap = false;
  bool _modeViewOnlineMap = false;
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        SizedBox(
          width: MediaQuery.of(context).size.width * .15,
          height: MediaQuery.of(context).size.width * .15,
          child: FloatingActionButton(
            backgroundColor:
                _modeViewOfflineMap ? gl.colorAgroBioTech : Colors.grey,
            onPressed: () {
              setState(() {
                _modeViewOfflineMap = !_modeViewOfflineMap;
              });
              PopupOfflineMenu(gl.notificationContext!, () {
                setState(() {
                  _modeViewOfflineMap = !_modeViewOfflineMap;
                });
              });
            },
            child: Icon(
              Icons.download_for_offline,
              size: MediaQuery.of(context).size.width * .13,
              color: Colors.black,
            ),
          ),
        ),
        SizedBox(
          width: MediaQuery.of(context).size.width * .15,
          height: MediaQuery.of(context).size.width * .15,
          child: FloatingActionButton(
            backgroundColor:
                _modeViewOnlineMap ? gl.colorAgroBioTech : Colors.grey,
            onPressed: () {
              setState(() {
                _modeViewOnlineMap = !_modeViewOnlineMap;
              });
              PopupOnlineMapMenu(gl.notificationContext!, () {
                setState(() {
                  _modeViewOnlineMap = false;
                });
              });
            },
            child: Icon(
              Icons.layers_outlined,
              size: MediaQuery.of(context).size.width * .13,
              color: Colors.black,
            ),
          ),
        ),
      ],
    );
  }
}

class UpperLayerControl extends StatefulWidget {
  const UpperLayerControl({super.key});
  @override
  State<UpperLayerControl> createState() => _UpperLayerControl();
}

class _UpperLayerControl extends State<UpperLayerControl> {
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Card(
          margin: EdgeInsets.all(5),
          color: Colors.white,
          shadowColor: const Color.fromARGB(255, 44, 44, 120),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: <Widget>[
              TextButton(
                style: ButtonStyle(
                  minimumSize: WidgetStateProperty<Size>.fromMap(
                    <WidgetStatesConstraint, Size>{
                      WidgetState.any: Size(
                        MediaQuery.of(context).size.width * .45,
                        MediaQuery.of(context).size.height * .05,
                      ),
                    },
                  ),
                ),
                onPressed: () {},
                child: Container(
                  padding: EdgeInsets.symmetric(horizontal: 3.0),
                  constraints: BoxConstraints(
                    maxHeight: MediaQuery.of(context).size.height * .05,
                    minHeight: MediaQuery.of(context).size.height * .05,
                    maxWidth: MediaQuery.of(context).size.width * 0.45,
                    minWidth: MediaQuery.of(context).size.width * 0.45,
                  ),
                  child: Text(
                    "Marqueurs des lieux cherchés",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: MediaQuery.of(context).size.height * .018,
                    ),
                  ),
                ),
              ),
              Container(
                constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.width * 0.1,
                  minHeight: MediaQuery.of(context).size.width * 0.1,
                  maxWidth: MediaQuery.of(context).size.width * 0.17,
                  minWidth: MediaQuery.of(context).size.width * 0.17,
                ),
                child: Row(
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        maxHeight: MediaQuery.of(context).size.width * 0.1,
                        minHeight: MediaQuery.of(context).size.width * 0.1,
                        maxWidth: MediaQuery.of(context).size.width * 0.15,
                        minWidth: MediaQuery.of(context).size.width * 0.15,
                      ),
                      color: Colors.white,
                      padding: const EdgeInsets.symmetric(),
                      child: SizedBox(
                        width: MediaQuery.of(context).size.width * .1,
                        height: MediaQuery.of(context).size.width * .1,
                        child: FloatingActionButton(
                          backgroundColor:
                              gl.modeMapShowSearchMarker
                                  ? gl.colorAgroBioTech
                                  : Colors.grey,
                          onPressed: () {
                            setState(() {
                              gl.modeMapShowSearchMarker =
                                  !gl.modeMapShowSearchMarker;
                            });
                            gl.refreshMap(() {});
                          },
                          child: Icon(
                            Icons.remove_red_eye,
                            size: MediaQuery.of(context).size.width * .1,
                            color: Colors.black,
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
        Card(
          margin: EdgeInsets.all(5),
          color: Colors.white,
          shadowColor: const Color.fromARGB(255, 44, 44, 120),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: <Widget>[
              TextButton(
                style: ButtonStyle(
                  minimumSize: WidgetStateProperty<Size>.fromMap(
                    <WidgetStatesConstraint, Size>{
                      WidgetState.any: Size(
                        MediaQuery.of(context).size.width * .45,
                        MediaQuery.of(context).size.height * .05,
                      ),
                    },
                  ),
                ),
                onPressed: () {},
                child: Container(
                  padding: EdgeInsets.symmetric(horizontal: 3.0),
                  constraints: BoxConstraints(
                    maxHeight: MediaQuery.of(context).size.height * .05,
                    minHeight: MediaQuery.of(context).size.height * .05,
                    maxWidth: MediaQuery.of(context).size.width * 0.45,
                    minWidth: MediaQuery.of(context).size.width * 0.45,
                  ),
                  child: Text(
                    "Couche des polygones",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: MediaQuery.of(context).size.height * .018,
                    ),
                  ),
                ),
              ),
              Container(
                constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.width * 0.1,
                  minHeight: MediaQuery.of(context).size.width * 0.1,
                  maxWidth: MediaQuery.of(context).size.width * 0.17,
                  minWidth: MediaQuery.of(context).size.width * 0.17,
                ),
                child: Row(
                  children: [
                    Container(
                      constraints: BoxConstraints(
                        maxHeight: MediaQuery.of(context).size.width * 0.1,
                        minHeight: MediaQuery.of(context).size.width * 0.1,
                        maxWidth: MediaQuery.of(context).size.width * 0.15,
                        minWidth: MediaQuery.of(context).size.width * 0.15,
                      ),
                      color: Colors.white,
                      padding: const EdgeInsets.symmetric(),
                      child: SizedBox(
                        width: MediaQuery.of(context).size.width * .1,
                        height: MediaQuery.of(context).size.width * .1,
                        child: FloatingActionButton(
                          backgroundColor:
                              gl.modeMapShowPolygons
                                  ? gl.colorAgroBioTech
                                  : Colors.grey,
                          onPressed: () {
                            setState(() {
                              gl.modeMapShowPolygons = !gl.modeMapShowPolygons;
                            });
                            gl.refreshMap(() {});
                          },
                          child: Icon(
                            Icons.remove_red_eye,
                            size: MediaQuery.of(context).size.width * .1,
                            color: Colors.black,
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class LayerSwitcher extends StatefulWidget {
  const LayerSwitcher({super.key});
  @override
  State<LayerSwitcher> createState() => _LayerSwitcher();
}

class _LayerSwitcher extends State<LayerSwitcher> {
  @override
  Widget build(BuildContext context) {
    {
      return ReorderableListView(
        scrollDirection: Axis.vertical,
        buildDefaultDragHandles: true,
        padding: const EdgeInsets.symmetric(horizontal: 0),
        onReorder: (int oldIndex, int newIndex) {
          setState(() {
            if (oldIndex < newIndex) {
              newIndex -= 1;
            }
            if (gl.selectedLayerForMap.length < newIndex + 1 ||
                gl.selectedLayerForMap.length < oldIndex + 1) {
              return;
            }
            gl.refreshMap(() {
              final gl.SelectedLayer item = gl.selectedLayerForMap.removeAt(
                oldIndex,
              );
              gl.selectedLayerForMap.insert(newIndex, item);
            });
          });
        },

        children: List<Widget>.generate(3, (i) {
          if (gl.selectedLayerForMap[i].mCode != '${i + 1}') {
            return Card(
              margin: EdgeInsets.all(5),
              key: Key('$i+listOfThree'),
              color: Colors.white,
              shadowColor: const Color.fromARGB(255, 44, 44, 120),
              child: ReorderableDragStartListener(
                index: i,
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: <Widget>[
                        TextButton(
                          style: ButtonStyle(
                            maximumSize: WidgetStateProperty<Size>.fromMap(
                              <WidgetStatesConstraint, Size>{
                                WidgetState.any: Size(
                                  MediaQuery.of(context).size.width * .45,
                                  MediaQuery.of(context).size.height * .1,
                                ),
                              },
                            ),
                          ),
                          onPressed: () {
                            PopupMapSelectionMenu(
                              gl.notificationContext!,
                              i,
                              setState,
                            );
                          },
                          child: Container(
                            padding: EdgeInsets.symmetric(horizontal: 3.0),
                            constraints: BoxConstraints(
                              maxHeight:
                                  MediaQuery.of(context).size.height * .05,
                              maxWidth:
                                  MediaQuery.of(context).size.width * 0.45,
                              minWidth:
                                  MediaQuery.of(context).size.width * 0.45,
                            ),
                            child: Text(
                              gl.dico
                                  .getLayerBase(gl.selectedLayerForMap[i].mCode)
                                  .mNom,
                              style: TextStyle(
                                color: Colors.black,
                                fontSize:
                                    MediaQuery.of(context).size.height * .018,
                              ),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: MediaQuery.of(context).size.width * 0.1,
                            minHeight: MediaQuery.of(context).size.width * 0.1,
                            maxWidth: MediaQuery.of(context).size.width * 0.22,
                            minWidth: MediaQuery.of(context).size.width * 0.22,
                          ),
                          child: Row(
                            children: [
                              Container(
                                color: Colors.white,
                                constraints: BoxConstraints(
                                  maxHeight:
                                      MediaQuery.of(context).size.width * 0.12,
                                  minHeight:
                                      MediaQuery.of(context).size.width * 0.12,
                                  maxWidth:
                                      MediaQuery.of(context).size.width * 0.1,
                                  minWidth:
                                      MediaQuery.of(context).size.width * 0.1,
                                ),
                                padding: const EdgeInsets.symmetric(),
                                child: Image.asset(
                                  gl.dico
                                      .getLayerBase(
                                        gl.selectedLayerForMap[i].mCode,
                                      )
                                      .mLogoAttributionFile,
                                ),
                              ),
                              gl.selectedLayerForMap[i].offline
                                  ? Container(
                                    constraints: BoxConstraints(
                                      maxHeight:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      minHeight:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      maxWidth:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      minWidth:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(
                                      Icons.save,
                                      size:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                    ),
                                  )
                                  : Container(
                                    constraints: BoxConstraints(
                                      maxHeight:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      minHeight:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      maxWidth:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                      minWidth:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(
                                      Icons.wifi,
                                      size:
                                          MediaQuery.of(context).size.width *
                                          0.1,
                                    ),
                                  ),
                            ],
                          ),
                        ),
                      ],
                    ),
                    if (i == 0 && !gl.offlineMode)
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Container(
                            constraints: BoxConstraints(
                              maxHeight:
                                  MediaQuery.of(context).size.height * 0.05,
                              maxWidth: MediaQuery.of(context).size.width * 0.4,
                            ),
                            child: TextButton(
                              style: ButtonStyle(
                                backgroundColor:
                                    gl.modeMapFirstTileLayerTrancparancy
                                        ? WidgetStateProperty<Color>.fromMap(<
                                          WidgetStatesConstraint,
                                          Color
                                        >{WidgetState.any: gl.colorAgroBioTech})
                                        : WidgetStateProperty<Color>.fromMap(
                                          <WidgetStatesConstraint, Color>{
                                            WidgetState.any: Color.fromARGB(
                                              255,
                                              234,
                                              234,
                                              234,
                                            ),
                                          },
                                        ),
                              ),
                              onPressed: () {
                                setState(() {
                                  gl.modeMapFirstTileLayerTrancparancy =
                                      !gl.modeMapFirstTileLayerTrancparancy;
                                });
                                gl.refreshMap(() {});
                              },
                              child: Text(
                                gl.modeMapFirstTileLayerTrancparancy
                                    ? "Transparence 50%"
                                    : "Transparence 0%",
                                style: TextStyle(color: Colors.black),
                              ),
                            ),
                          ),
                        ],
                      ),
                  ],
                ),
              ),
            );
          } else if (gl.offlineMode) {
            return Card(
              margin: EdgeInsets.all(5),
              key: Key('$i+listOfThreeOffline'),
              color: Colors.transparent,
            );
          } else {
            return Card(
              margin: EdgeInsets.all(5),
              key: Key('$i+listOfThreeEmpty'),
              color: Colors.white,
              shadowColor: const Color.fromARGB(255, 44, 44, 120),
              child: ReorderableDragStartListener(
                index: i,
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: <Widget>[
                        TextButton(
                          style: ButtonStyle(
                            maximumSize: WidgetStateProperty<Size>.fromMap(
                              <WidgetStatesConstraint, Size>{
                                WidgetState.any: Size(
                                  MediaQuery.of(context).size.width * .45,
                                  MediaQuery.of(context).size.height * .1,
                                ),
                              },
                            ),
                          ),
                          onPressed: () {
                            PopupMapSelectionMenu(
                              gl.notificationContext!,
                              i,
                              setState,
                            );
                          },
                          child: Container(
                            padding: EdgeInsets.symmetric(horizontal: 3.0),
                            constraints: BoxConstraints(
                              maxHeight:
                                  MediaQuery.of(context).size.height * .05,
                              maxWidth:
                                  MediaQuery.of(context).size.width * 0.45,
                              minWidth:
                                  MediaQuery.of(context).size.width * 0.45,
                            ),
                            child: Text(
                              "Appuyez ici pour ajouter une couche du catalogue",
                              style: TextStyle(
                                color: Colors.black,
                                fontSize:
                                    MediaQuery.of(context).size.height * .018,
                              ),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: MediaQuery.of(context).size.width * 0.1,
                            minHeight: MediaQuery.of(context).size.width * 0.1,
                            maxWidth: MediaQuery.of(context).size.width * 0.22,
                            minWidth: MediaQuery.of(context).size.width * 0.22,
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            );
          }
        }),
      );
    }
  }
}

class PopupDoYouReally {
  PopupDoYouReally(
    BuildContext context,
    Function after,
    String title,
    String message,
  ) {
    showDialog(
      barrierDismissible: true,
      barrierColor: Colors.black.withAlpha(100),
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.85),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [Text(title, textAlign: TextAlign.justify)],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .80,
              child: Text(
                message,
                textAlign: TextAlign.center,
                style: TextStyle(color: Colors.white, fontSize: 24),
              ),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actionsAlignment: MainAxisAlignment.spaceBetween,
          actions: [
            FloatingActionButton(
              onPressed: () {
                after();
                Navigator.of(context, rootNavigator: true).pop();
              },
              backgroundColor: gl.colorAgroBioTech,
              child: Container(
                alignment: Alignment.center,
                constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.width * 0.1,
                  minHeight: MediaQuery.of(context).size.width * 0.1,
                  maxWidth: MediaQuery.of(context).size.width * 0.3,
                  minWidth: MediaQuery.of(context).size.width * 0.3,
                ),
                child: Text(
                  "Oui",
                  textAlign: TextAlign.center,
                  style: TextStyle(color: Colors.black, fontSize: 20),
                ),
              ),
            ),
            FloatingActionButton(
              onPressed: () {
                Navigator.of(context, rootNavigator: true).pop();
              },
              backgroundColor: Colors.red,
              child: Container(
                alignment: Alignment.center,
                constraints: BoxConstraints(
                  maxHeight: MediaQuery.of(context).size.width * 0.1,
                  minHeight: MediaQuery.of(context).size.width * 0.1,
                  maxWidth: MediaQuery.of(context).size.width * 0.3,
                  minWidth: MediaQuery.of(context).size.width * 0.3,
                ),
                child: Text(
                  "Non",
                  textAlign: TextAlign.center,
                  style: TextStyle(color: Colors.black, fontSize: 20),
                ),
              ),
            ),
          ],
        );
      },
    );
  }
}

class PopupOfflineMenu {
  PopupOfflineMenu(BuildContext context, Function after) {
    showDialog(
      barrierDismissible: true,
      barrierColor: Colors.black.withAlpha(100),
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.0),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                "Changez les cartes affichées",
                textAlign: TextAlign.justify,
              ),
            ],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .80,
              height: MediaQuery.of(context).size.height * 0.8,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * 0.8,
                    child: OfflineMapMenu(),
                  ),
                ],
              ),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    ).whenComplete(() {
      after();
    });
  }
}

class OfflineMapMenu extends StatefulWidget {
  const OfflineMapMenu({super.key});

  @override
  State<StatefulWidget> createState() => _OfflineMapMenu();
}

class _OfflineMapMenu extends State<OfflineMapMenu> {
  int _selectedMap = -1;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 0),
        children: _injectOfflineLayerData((i, layerTile) {
          return TextButton(
            style: ButtonStyle(
              maximumSize:
                  i == _selectedMap
                      ? WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .15,
                          ),
                        },
                      )
                      : WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .1,
                          ),
                        },
                      ),
            ),
            key: Key('$i'),
            onPressed:
                i == _selectedMap
                    ? () {
                      setState(() {
                        _selectedMap = -1;
                      });
                    }
                    : () {
                      setState(() {
                        _selectedMap = i;
                      });
                    },
            child: Card(
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color:
                  i == _selectedMap
                      ? gl.colorAgroBioTech.withAlpha(255)
                      : gl.colorAgroBioTech.withAlpha(120),
              child: Row(
                children: [
                  Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      IconButton(
                        icon: Icon(
                          Icons.delete_forever,
                          size: MediaQuery.of(context).size.width * 0.1,
                          color: Colors.black,
                        ),
                        onPressed: () {
                          PopupDoYouReally(
                            gl.notificationContext!,
                            () {
                              fileDelete(
                                join(
                                  gl.dico.docDir.path,
                                  gl.dico
                                      .getLayerBase(layerTile.key)
                                      .mNomRaster,
                                ),
                              ).whenComplete(() {
                                setState(() {
                                  gl.dico.getLayerBase(layerTile.key).mOffline =
                                      false;
                                  gl.removeFromOfflineList(layerTile.key);
                                });
                                gl.refreshWholeCatalogueView(() {
                                  gl.dico.getLayerBase(layerTile.key).mOffline =
                                      false;
                                });
                                gl.rebuildOfflineView(() {
                                  gl.dico.getLayerBase(layerTile.key).mOffline =
                                      false;
                                });
                              });
                            },

                            "Message",
                            "Voulez vous vraiment supprimer la carte?\nVous ne pourriez plus l'utiliser pour le mode hors ligne!",
                          );
                        },
                      ),
                    ],
                  ),
                  Container(
                    alignment: Alignment.center,
                    constraints: BoxConstraints(
                      maxWidth: MediaQuery.of(context).size.width * .55,
                      minHeight: MediaQuery.of(context).size.width * .15,
                      maxHeight: MediaQuery.of(context).size.width * .15,
                    ),
                    child: Text(
                      layerTile.name,
                      style: TextStyle(color: Colors.black, fontSize: 20),
                      textAlign: TextAlign.center,
                    ),
                  ),
                ],
              ),
            ),
          );
        }),
      ),
    );
  }

  List<Widget> _injectOfflineLayerData(
    Widget Function(int, LayerTile) listBuilder,
  ) {
    List<LayerTile> offlineList = [];

    for (var key in gl.dico.mLayerBases.keys) {
      if (gl.dico.mLayerBases[key]!.mOffline ||
          gl.dico.mLayerBases[key]!.mInDownload) {
        offlineList.add(
          LayerTile(
            name: gl.dico.mLayerBases[key]!.mNom,
            filter: gl.dico.mLayerBases[key]!.mGroupe,
            key: key,
            downloadable: gl.dico.mLayerBases[key]!.mIsDownloadableRW,
            extern: gl.dico.mLayerBases[key]!.mCategorie == "Externe",
          ),
        );
      }
    }
    List<Widget> result = [];
    int i = 0;
    for (LayerTile layerTile in offlineList) {
      result.add(listBuilder(i++, layerTile));
    }
    return result;
  }
}

class PopupMapSelectionMenu {
  PopupMapSelectionMenu(
    BuildContext context,
    int interfaceSelectedPosition,
    Function after,
  ) {
    showDialog(
      barrierDismissible: true,
      barrierColor: Colors.black.withAlpha(100),
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.0),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                "Changez les cartes affichées",
                textAlign: TextAlign.justify,
              ),
            ],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .80,
              height: MediaQuery.of(context).size.height * 0.8,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * 0.8,
                    child: MapSelectionMenu(after, interfaceSelectedPosition),
                  ),
                ],
              ),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    );
  }
}

class MapSelectionMenu extends StatefulWidget {
  final Function after;
  final int interfaceSelectedMapKey;

  const MapSelectionMenu(this.after, this.interfaceSelectedMapKey, {super.key});

  @override
  State<StatefulWidget> createState() => _MapSelectionMenu();
}

class _MapSelectionMenu extends State<MapSelectionMenu> {
  int _selectedCategory = -1;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 0),
        children: _injectGroupData(
          (int i, GroupeCouche groupe) => TextButton(
            style: ButtonStyle(
              minimumSize:
                  i == _selectedCategory
                      ? WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .15,
                          ),
                        },
                      )
                      : WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .075,
                          ),
                        },
                      ),
            ),
            key: Key('$i'),
            onPressed:
                i == _selectedCategory
                    ? () {
                      setState(() {
                        _selectedCategory = -1;
                      });
                    }
                    : () {
                      setState(() {
                        _selectedCategory = i;
                      });
                    },
            child: Card(
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color:
                  i == _selectedCategory
                      ? gl.colorAgroBioTech.withAlpha(25)
                      : gl.colorAgroBioTech.withAlpha(175),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  i != _selectedCategory
                      ? Container(
                        alignment: Alignment.center,
                        padding: EdgeInsets.all(3),
                        constraints: BoxConstraints(
                          maxWidth: MediaQuery.of(context).size.width * .99,
                          minHeight: MediaQuery.of(context).size.width * .2,
                        ),
                        child: Text(
                          groupe.mLabel,
                          textAlign: TextAlign.justify,
                          style: TextStyle(color: Colors.black, fontSize: 22),
                        ),
                      )
                      : Container(
                        alignment: Alignment.center,
                        padding: EdgeInsets.all(0),
                        constraints: BoxConstraints(
                          maxWidth: MediaQuery.of(context).size.width * .99,
                        ),
                        child: ListBody(
                          children:
                              <Widget>[
                                Card(
                                  color: gl.colorAgroBioTech.withAlpha(175),
                                  child: Container(
                                    alignment: Alignment.center,
                                    padding: EdgeInsets.all(3),
                                    constraints: BoxConstraints(
                                      maxWidth:
                                          MediaQuery.of(context).size.width *
                                          .99,
                                      minHeight:
                                          MediaQuery.of(context).size.width *
                                          .2,
                                    ),
                                    child: Text(
                                      groupe.mLabel,
                                      textAlign: TextAlign.justify,
                                      style: TextStyle(
                                        color: Colors.black,
                                        fontSize: 22,
                                      ),
                                    ),
                                  ),
                                ),
                              ] +
                              _injectLayerData(groupe.mCode, (
                                int i,
                                LayerTile layerTile,
                              ) {
                                return TextButton(
                                  style: ButtonStyle(
                                    minimumSize:
                                        gl.getInterfaceSelectedLCode().contains(
                                              layerTile.key,
                                            )
                                            ? WidgetStateProperty<Size>.fromMap(
                                              <WidgetStatesConstraint, Size>{
                                                WidgetState.any: Size(
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.width *
                                                      .6,
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.height *
                                                      .1,
                                                ),
                                              },
                                            )
                                            : WidgetStateProperty<Size>.fromMap(
                                              <WidgetStatesConstraint, Size>{
                                                WidgetState.any: Size(
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.width *
                                                      .6,
                                                  MediaQuery.of(
                                                        context,
                                                      ).size.height *
                                                      .075,
                                                ),
                                              },
                                            ),
                                  ),

                                  onPressed: () {
                                    setState(() {
                                      gl.isSelectedLayer(
                                            layerTile.key,
                                            offline: gl.offlineMode,
                                          )
                                          ? gl.removeLayerFromList(
                                            key: layerTile.key,
                                            offline: gl.offlineMode,
                                          )
                                          : gl.replaceLayerFromList(
                                            layerTile.key,
                                            index:
                                                widget.interfaceSelectedMapKey,
                                            offline: gl.offlineMode,
                                          );
                                    });
                                    widget.after(() {});
                                    gl.refreshMap(() {});
                                  },
                                  child: Card(
                                    color:
                                        gl.getInterfaceSelectedLCode().contains(
                                              layerTile.key,
                                            )
                                            ? Colors.white.withAlpha(255)
                                            : Colors.white.withAlpha(150),
                                    child:
                                        !gl
                                                .getInterfaceSelectedLCode()
                                                .contains(layerTile.key)
                                            ? Container(
                                              constraints: BoxConstraints(
                                                minHeight:
                                                    MediaQuery.of(
                                                      context,
                                                    ).size.width *
                                                    .15,
                                              ),
                                              child: Column(
                                                mainAxisAlignment:
                                                    MainAxisAlignment.center,
                                                children: [
                                                  Row(
                                                    mainAxisAlignment:
                                                        MainAxisAlignment
                                                            .center,
                                                    children: [
                                                      Container(
                                                        constraints:
                                                            BoxConstraints(
                                                              maxWidth:
                                                                  MediaQuery.of(
                                                                    context,
                                                                  ).size.width *
                                                                  .6,
                                                            ),
                                                        child: Text(
                                                          layerTile.name,
                                                          textAlign:
                                                              TextAlign.justify,
                                                          style: TextStyle(
                                                            color: Colors.black,
                                                            fontSize: 18,
                                                          ),
                                                        ),
                                                      ),
                                                    ],
                                                  ),
                                                ],
                                              ),
                                            )
                                            : Column(
                                              children: [
                                                Row(
                                                  mainAxisAlignment:
                                                      MainAxisAlignment.center,
                                                  children: [
                                                    Text(layerTile.name),
                                                  ],
                                                ),
                                              ],
                                            ),
                                  ),
                                );
                              }),
                        ),
                      ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  List<Widget> _injectGroupData(Widget Function(int, GroupeCouche) generate) {
    Map<String, Null> groupesNonVides = {};
    for (String key in gl.dico.mLayerBases.keys) {
      if (gl.offlineMode ? gl.dico.getLayerBase(key).mOffline : true) {
        groupesNonVides[gl.dico.getLayerBase(key).mGroupe] = null;
      }
    }
    List<GroupeCouche> groupes = [];
    for (String key in groupesNonVides.keys) {
      for (GroupeCouche couche in gl.dico.mGrCouches) {
        if (couche.mCode == key) {
          groupes.add(couche);
        }
      }
    }

    return List<Widget>.generate(groupes.length, (i) {
      return generate(i, groupes[i]);
    });
  }

  List<Widget> _injectLayerData(
    String category,
    Widget Function(int, LayerTile) generate,
  ) {
    Map<String, LayerBase> mp = gl.dico.mLayerBases;
    List<LayerTile> layer = [];
    for (var key in mp.keys) {
      if (category == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
          mp[key]!.mVisu &&
          mp[key]?.mTypeGeoservice == "" &&
          (gl.offlineMode ? mp[key]!.mOffline : true)) {
        layer.add(
          LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe",
          ),
        );
      }
    }
    return List<Widget>.generate(layer.length, (i) {
      return generate(i, layer[i]);
    });
  }
}
/*
class PopupAnalysisSelection {
  PopupAnalysisSelection(BuildContext context, Function after) {
    showDialog(
      barrierDismissible: true,
      barrierColor: Colors.black.withAlpha(100),
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: Color.fromRGBO(0, 0, 0, 0.0),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          title: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                "Changez les cartes affichées",
                textAlign: TextAlign.justify,
              ),
            ],
          ),
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width: MediaQuery.of(context).size.width * .80,
              height: MediaQuery.of(context).size.height * 0.8,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: MediaQuery.of(context).size.width * .80,
                    height: MediaQuery.of(context).size.height * 0.8,
                    child: MapSelectionMenu(after),
                  ),
                ],
              ),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    );
  }
}

class AnalysisSelection extends StatefulWidget {
  const AnalysisSelection({super.key});

  @override
  State<StatefulWidget> createState() => _AnalysisSelection();
}

class _AnalysisSelection extends State<AnalysisSelection> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 0),
        children: _injectOfflineLayerData((i, layerTile) {
          return TextButton(
            style: ButtonStyle(
              maximumSize:
                  gl.isSelectedLayer(layerTile.key)
                      ? WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .15,
                          ),
                        },
                      )
                      : WidgetStateProperty<Size>.fromMap(
                        <WidgetStatesConstraint, Size>{
                          WidgetState.any: Size(
                            MediaQuery.of(context).size.width * .99,
                            MediaQuery.of(context).size.height * .1,
                          ),
                        },
                      ),
            ),
            key: Key('$i'),
            onPressed:
                gl.isSelectedLayer(layerTile.key)
                    ? () {
                      setState(() {});
                    }
                    : () {
                      setState(() {});
                    },
            child: Card(
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color:
                  gl.isSelectedLayer(layerTile.key)
                      ? gl.colorAgroBioTech.withAlpha(255)
                      : gl.colorAgroBioTech.withAlpha(120),
              child: Row(
                children: [
                  Container(
                    alignment: Alignment.center,
                    constraints: BoxConstraints(
                      maxWidth: MediaQuery.of(context).size.width * .55,
                      minHeight: MediaQuery.of(context).size.width * .15,
                      maxHeight: MediaQuery.of(context).size.width * .15,
                    ),
                    child: Text(
                      layerTile.name,
                      style: TextStyle(color: Colors.black, fontSize: 20),
                      textAlign: TextAlign.center,
                    ),
                  ),
                ],
              ),
            ),
          );
        }),
      ),
    );
  }

  List<Widget> _injectOfflineLayerData(
    Widget Function(int, LayerTile) listBuilder,
  ) {
    List<LayerTile> offlineList = [];

    for (var key in gl.dico.mLayerBases.keys) {
      if (gl.dico.mLayerBases[key]!.mOffline ||
          gl.dico.mLayerBases[key]!.mInDownload) {
        offlineList.add(
          LayerTile(
            name: gl.dico.mLayerBases[key]!.mNom,
            filter: gl.dico.mLayerBases[key]!.mGroupe,
            key: key,
            downloadable: gl.dico.mLayerBases[key]!.mIsDownloadableRW,
            extern: gl.dico.mLayerBases[key]!.mCategorie == "Externe",
          ),
        );
      }
    }
    List<Widget> result = [];
    int i = 0;
    for (LayerTile layerTile in offlineList) {
      result.add(listBuilder(i++, layerTile));
    }
    return result;
  }
}
*/