import 'dart:convert';
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
import 'package:flutter/services.dart';
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

Widget popupNoInternet() {
  return AlertDialog(
    title: Text("Oups"),
    content: Text("Vous n'avez pas accès à internet."),
    actions: <Widget>[
      TextButton(
        child: Text("OK"),
        onPressed: () {
          gl.mainStackPopLast();
          gl.refreshMap(() {});
        },
      ),
    ],
  );
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
    Function callbackOnStartTyping,
  ) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          content: SizedBox(
            width:
                gl.display.orientation == Orientation.portrait
                    ? gl.menuBarLength * gl.display.equipixel
                    : gl.menuBarLength * gl.display.equipixel * 1.75,
            height:
                gl.display.orientation == Orientation.portrait
                    ? gl.menuBarThickness * gl.display.equipixel * 1.75
                    : gl.menuBarThickness * gl.display.equipixel * .9,
            child: SingleChildScrollView(
              child: switchRowColWithOrientation([
                SizedBox(
                  width: gl.menuBarLength * gl.display.equipixel,
                  child: TextFormField(
                    maxLength: 22,
                    maxLengthEnforcement: MaxLengthEnforcement.enforced,
                    onChanged: (String str) {
                      state(str);
                    },
                    onTap: () => callbackOnStartTyping(),
                    onTapOutside: (pointer) {
                      after();
                    },
                    controller: TextEditingController(text: currentName),
                  ),
                ),
                SizedBox(
                  width: gl.menuBarLength * .5 * gl.display.equipixel,
                  child: TextButton(
                    style: ButtonStyle(
                      backgroundColor: WidgetStateProperty.fromMap(
                        <WidgetStatesConstraint, Color>{
                          WidgetState.any: gl.colorAgroBioTech.withAlpha(200),
                        },
                      ),
                      shape: WidgetStateProperty<OutlinedBorder>.fromMap(
                        <WidgetStatesConstraint, OutlinedBorder>{
                          WidgetState.any: RoundedRectangleBorder(
                            borderRadius: BorderRadiusGeometry.circular(12.0),
                            side: BorderSide(
                              color: Color.fromRGBO(205, 225, 138, 1.0),
                              width: 2.0,
                            ),
                          ),
                        },
                      ),
                      fixedSize: WidgetStateProperty.fromMap(<
                        WidgetStatesConstraint,
                        Size
                      >{
                        WidgetState.any: Size(
                          gl.display.equipixel * gl.popupReturnButtonWidth * .8,
                          gl.display.equipixel *
                              gl.popupReturnButtonHeight *
                              .6,
                        ),
                      }),
                    ),
                    child: Text(
                      "Renommer",
                      style: TextStyle(color: Colors.black),
                    ),
                    onPressed: () {
                      after();
                      Navigator.of(context, rootNavigator: true).pop();
                    },
                  ),
                ),
              ]),
            ),
          ),
          actions: [],
        );
      },
    );
  }
}

class PolygonListMenu extends StatefulWidget {
  final Function(LatLng) state;
  final Function after;

  const PolygonListMenu({super.key, required this.state, required this.after});

  @override
  State<StatefulWidget> createState() => _PolygonListMenu();
}

class _PolygonListMenu extends State<PolygonListMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  final ScrollController _controller = ScrollController();
  bool _keyboard = false;

  void _scrollDown() {
    _controller.animateTo(
      _controller.position.maxScrollExtent,
      duration: Duration(seconds: 1),
      curve: Curves.fastOutSlowIn,
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      resizeToAvoidBottomInset: false,
      backgroundColor: Colors.transparent,
      body: switchRowColWithOrientation([
        Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            SizedBox(
              height: gl.display.equipixel * gl.fontSizeL * 1.1,
              child: Text(
                "Liste des polygones",
                textAlign: TextAlign.justify,
                style: TextStyle(
                  fontSize: gl.display.equipixel * gl.fontSizeL,
                  color: Colors.white,
                ),
              ),
            ),
            Container(
              constraints:
                  gl.display.orientation == Orientation.portrait
                      ? BoxConstraints(
                        maxHeight:
                            gl.display.equipixel *
                            (gl.popupWindowsPortraitHeight -
                                gl.fontSizeL * 1.1 -
                                gl.popupReturnButtonHeight -
                                gl.polyNewPolygonButtonHeight),
                        maxWidth:
                            gl.display.equipixel * gl.popupWindowsPortraitWidth,
                      )
                      : BoxConstraints(
                        maxHeight:
                            gl.display.equipixel *
                            (gl.popupWindowsLandscapeHeight -
                                gl.fontSizeL * 1.1),
                        maxWidth:
                            gl.popupWindowsPortraitWidth * gl.display.equipixel,
                      ),
              child: ReorderableListView(
                scrollController: _controller,
                buildDefaultDragHandles: false,
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
                      final PolygonLayer item = gl.polygonLayers.removeAt(
                        oldIndex,
                      );
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
                                    ? gl.polygonLayers[i].colorInside.withAlpha(
                                      255,
                                    )
                                    : Colors.grey.withAlpha(100),
                              )
                              : getColorTextFromBackground(
                                i == gl.selectedPolygonLayer
                                    ? gl.polygonLayers[i].colorInside.withAlpha(
                                      255,
                                    )
                                    : Colors.grey.withAlpha(100),
                              ).withAlpha(128);
                      return TextButton(
                        style: ButtonStyle(
                          fixedSize:
                              i == gl.selectedPolygonLayer &&
                                      gl.display.orientation ==
                                          Orientation.portrait
                                  ? WidgetStateProperty<Size>.fromMap(
                                    <WidgetStatesConstraint, Size>{
                                      WidgetState.any: Size(
                                        gl.display.equipixel *
                                            gl.polyListSelectedCardWidth,
                                        gl.display.equipixel *
                                            gl.polyListSelectedCardHeight,
                                      ),
                                    },
                                  )
                                  : WidgetStateProperty<Size>.fromMap(
                                    <WidgetStatesConstraint, Size>{
                                      WidgetState.any: Size(
                                        gl.display.equipixel *
                                            gl.polyListCardWidth,
                                        gl.display.equipixel *
                                            gl.polyListCardHeight,
                                      ),
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
                          child: SizedBox(
                            height:
                                gl.polyListSelectedCardHeight *
                                gl.display.equipixel,
                            width:
                                gl.polyListSelectedCardWidth *
                                gl.display.equipixel,
                            child: Card(
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadiusGeometry.circular(
                                  12.0,
                                ),
                                side:
                                    i == gl.selectedPolygonLayer &&
                                            gl.display.orientation ==
                                                Orientation.portrait
                                        ? BorderSide(
                                          color: Colors.transparent,
                                          width: 0.0,
                                        )
                                        : i == gl.selectedPolygonLayer
                                        ? BorderSide(
                                          color: gl.polygonLayers[i].colorInside
                                              .withAlpha(100),
                                          width: 2.0,
                                        )
                                        : BorderSide(
                                          color: gl.polygonLayers[i].colorInside
                                              .withAlpha(150),
                                          width: 4.0,
                                        ),
                              ),
                              surfaceTintColor: Colors.transparent,
                              shadowColor: Colors.transparent,
                              color:
                                  i == gl.selectedPolygonLayer
                                      ? gl.polygonLayers[i].colorInside
                                          .withAlpha(255)
                                      : Colors.grey.withAlpha(150),
                              child:
                                  i != gl.selectedPolygonLayer ||
                                          gl.display.orientation ==
                                              Orientation.landscape
                                      ? Row(
                                        mainAxisAlignment:
                                            MainAxisAlignment.center,
                                        children: [
                                          Container(
                                            alignment: Alignment.center,
                                            constraints: BoxConstraints(
                                              maxWidth:
                                                  gl
                                                              .display
                                                              .orientation
                                                              .index ==
                                                          1
                                                      ? gl.display.equipixel *
                                                          gl.polyListCardWidth *
                                                          .5
                                                      : gl.display.equipixel *
                                                          gl.polyListSelectedCardWidth *
                                                          .5,
                                            ),
                                            child: Text(
                                              gl.polygonLayers[i].name,
                                              style: TextStyle(
                                                color: Colors.black,
                                                fontSize:
                                                    gl.display.equipixel *
                                                    gl.fontSizeM,
                                              ),
                                            ),
                                          ),
                                        ],
                                      )
                                      : Row(
                                        mainAxisAlignment:
                                            MainAxisAlignment.spaceBetween,
                                        children: [
                                          if (i == gl.selectedPolygonLayer)
                                            Container(
                                              alignment: Alignment.center,
                                              width:
                                                  gl.display.equipixel *
                                                  gl.iconSize *
                                                  1.1,
                                              height:
                                                  gl.display.equipixel *
                                                  gl.iconSize *
                                                  1.1,
                                              child: IconButton(
                                                onPressed: () {
                                                  PopupDoYouReally(
                                                    gl.notificationContext!,
                                                    () {
                                                      setState(() {
                                                        //remove polygon
                                                        if (i > 0) {
                                                          gl.polygonLayers
                                                              .removeAt(i);
                                                          gl.selectedPolygonLayer--;
                                                        } else if (i == 0 &&
                                                            gl
                                                                .polygonLayers
                                                                .isNotEmpty) {
                                                          gl.polygonLayers
                                                              .removeAt(i);
                                                        }
                                                      });
                                                      gl.saveChangesToPolygoneToPrefs =
                                                          true;
                                                    },
                                                    "Message",
                                                    "\nVoulez vous vraiment supprimer ${gl.polygonLayers[i].name}?\n",
                                                  );
                                                },
                                                icon: Icon(
                                                  Icons.delete_forever,
                                                  color: activeTextColor,
                                                  size:
                                                      gl.display.equipixel *
                                                      gl.iconSize *
                                                      .75,
                                                ),
                                              ),
                                            ),
                                          SizedBox(
                                            width:
                                                gl.display.equipixel *
                                                gl.polyListSelectedCardWidth *
                                                .5,
                                            height:
                                                gl.display.equipixel *
                                                gl.polyListSelectedCardHeight,

                                            child: Column(
                                              mainAxisAlignment:
                                                  MainAxisAlignment.spaceEvenly,
                                              children: [
                                                TextButton(
                                                  child: Container(
                                                    constraints: BoxConstraints(
                                                      maxHeight:
                                                          gl.display.equipixel *
                                                          gl.polyListSelectedCardHeight *
                                                          .4,
                                                      maxWidth:
                                                          gl
                                                                      .display
                                                                      .orientation
                                                                      .index ==
                                                                  1
                                                              ? gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.polyListSelectedCardWidth *
                                                                  .5
                                                              : gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.polyListCardWidth *
                                                                  .5,
                                                    ),
                                                    child: Text(
                                                      gl.polygonLayers[i].name,
                                                      style: TextStyle(
                                                        color: activeTextColor,
                                                        fontSize:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.fontSizeM,
                                                      ),
                                                    ),
                                                  ),
                                                  onPressed: () {
                                                    PopupNameIntroducer(
                                                      context,
                                                      gl.polygonLayers[i].name,
                                                      (String nameIt) {
                                                        setState(() {
                                                          gl
                                                              .polygonLayers[i]
                                                              .name = nameIt;
                                                          gl.saveChangesToPolygoneToPrefs =
                                                              true;
                                                        });
                                                      },
                                                      () {
                                                        setState(() {
                                                          _keyboard = false;
                                                        });
                                                      },
                                                      () {
                                                        setState(() {
                                                          _keyboard = true;
                                                        });
                                                      },
                                                    );
                                                  },
                                                ),

                                                if (i ==
                                                    gl.selectedPolygonLayer)
                                                  Text(
                                                    "${(gl.polygonLayers[i].area / 100).round() / 100} Ha",
                                                    style: TextStyle(
                                                      color: activeTextColor,
                                                      fontSize:
                                                          gl.display.equipixel *
                                                          gl.fontSizeS *
                                                          1.2,
                                                    ),
                                                  ),
                                                if (i ==
                                                    gl.selectedPolygonLayer)
                                                  Text(
                                                    "${(gl.polygonLayers[i].perimeter).round() / 1000} km",
                                                    style: TextStyle(
                                                      color: activeTextColor,
                                                      fontSize:
                                                          gl.display.equipixel *
                                                          gl.fontSizeS *
                                                          1.2,
                                                    ),
                                                  ),
                                              ],
                                            ),
                                          ),
                                          if (i == gl.selectedPolygonLayer)
                                            Container(
                                              alignment: Alignment.center,
                                              width:
                                                  gl
                                                                  .display
                                                                  .orientation
                                                                  .index ==
                                                              1 &&
                                                          i !=
                                                              gl.selectedPolygonLayer
                                                      ? 0.0
                                                      : gl.display.equipixel *
                                                          gl.iconSize *
                                                          1.2,

                                              child: Column(
                                                mainAxisAlignment:
                                                    MainAxisAlignment
                                                        .spaceEvenly,
                                                children: [
                                                  IconButton(
                                                    onPressed: () {
                                                      PopupColorChooser(
                                                        gl
                                                            .polygonLayers[i]
                                                            .colorInside,
                                                        gl.notificationContext!,
                                                        //change color
                                                        (Color col) {
                                                          setState(() {
                                                            gl.polygonLayers[i]
                                                                .setColorInside(
                                                                  col,
                                                                );
                                                            gl.polygonLayers[i]
                                                                .setColorLine(
                                                                  Color.fromRGBO(
                                                                    (col.r *
                                                                            255)
                                                                        .round(),
                                                                    (col.g *
                                                                            255)
                                                                        .round(),
                                                                    (col.b *
                                                                            255)
                                                                        .round(),
                                                                    1.0,
                                                                  ),
                                                                );
                                                          });
                                                          gl.saveChangesToPolygoneToPrefs =
                                                              true;
                                                        },
                                                        () {},
                                                      );
                                                    },
                                                    icon: Icon(
                                                      Icons.color_lens,
                                                      color: activeTextColor,
                                                      size:
                                                          gl.display.equipixel *
                                                          gl.iconSize *
                                                          .75,
                                                    ),
                                                  ),
                                                  SizedBox(
                                                    width:
                                                        gl
                                                                        .display
                                                                        .orientation
                                                                        .index ==
                                                                    1 &&
                                                                i !=
                                                                    gl.selectedPolygonLayer
                                                            ? 0.0
                                                            : gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.iconSize *
                                                                1.2,

                                                    child: IconButton(
                                                      onPressed: () async {
                                                        if (await gl
                                                            .polygonLayers[i]
                                                            .onlineSurfaceAnalysis()) {
                                                          gl.mainStack.add(
                                                            popupResultsMenu(
                                                              gl.notificationContext!,
                                                              gl
                                                                  .polygonLayers[gl
                                                                      .selectedPolygonLayer]
                                                                  .decodedJson,
                                                              () {
                                                                gl.refreshMap(
                                                                  () {},
                                                                );
                                                              },
                                                              () {
                                                                gl.refreshMap(
                                                                  () {},
                                                                );
                                                              },
                                                            ),
                                                          );
                                                          gl.refreshMap(() {});
                                                        }
                                                      },
                                                      icon: Icon(
                                                        Icons.analytics,
                                                        color: activeTextColor,
                                                        size:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.iconSize *
                                                            .75,
                                                      ),
                                                    ),
                                                  ),
                                                ],
                                              ),
                                            ),
                                        ],
                                      ),
                            ),
                          ),
                        ),
                      );
                    }) +
                    [
                      TextButton(
                        key: Key("Placeholder for Autoscroll"),
                        isSemanticButton: false,
                        onPressed: () {},
                        child: SizedBox(height: gl.display.equipixel * 18),
                      ),
                    ],
              ),
            ),
            if (gl.display.orientation == Orientation.portrait && !_keyboard)
              TextButton(
                style: ButtonStyle(
                  backgroundColor: WidgetStateProperty.fromMap(
                    <WidgetStatesConstraint, Color>{
                      WidgetState.any: Colors.white,
                    },
                  ),
                  shape: WidgetStateProperty<OutlinedBorder>.fromMap(
                    <WidgetStatesConstraint, OutlinedBorder>{
                      WidgetState.any: RoundedRectangleBorder(
                        borderRadius: BorderRadiusGeometry.circular(12.0),
                      ),
                    },
                  ),
                  fixedSize: WidgetStateProperty.fromMap(<
                    WidgetStatesConstraint,
                    Size
                  >{
                    WidgetState.any: Size(
                      gl.display.equipixel * gl.polyListCardWidth * .97,
                      gl.display.equipixel * gl.polyNewPolygonButtonHeight * .9,
                    ),
                  }),
                ),
                key: Key('autsch-5-addPoly'),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(
                      Icons.add,
                      size:
                          (gl.polyNewPolygonButtonHeight - 4) *
                          gl.display.equipixel,
                      color: Colors.black,
                    ),
                  ],
                ),
                onPressed: () {
                  setState(() {
                    gl.polygonLayers.add(PolygonLayer(polygonName: "Nouveau"));
                    PopupNameIntroducer(
                      context,
                      "",
                      (String nameIt) {
                        setState(() {
                          gl.polygonLayers[gl.polygonLayers.length - 1].name =
                              nameIt;
                          gl.saveChangesToPolygoneToPrefs = true;
                        });
                      },
                      () {
                        setState(() {
                          _keyboard = false;
                        });
                      },
                      () {
                        setState(() {
                          _keyboard = true;
                        });
                      },
                    );
                    gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                  });

                  gl.refreshMap(() {
                    gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                    gl.saveChangesToPolygoneToPrefs = true;
                  });
                  _scrollDown();
                },
              ),
            if (gl.display.orientation == Orientation.portrait && !_keyboard)
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [_returnButton(context, widget.after)],
              ),
          ],
        ),
        if (gl.display.orientation == Orientation.landscape && !_keyboard)
          Column(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              if (gl.polygonLayers.isNotEmpty)
                SizedBox(
                  width: gl.polyListSelectedCardWidth * gl.display.equipixel,
                  child: Card(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadiusGeometry.circular(12.0),
                    ),

                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color: gl.polygonLayers[gl.selectedPolygonLayer].colorInside
                        .withAlpha(255),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSize * 1.1,
                          child: IconButton(
                            onPressed: () {
                              PopupDoYouReally(
                                gl.notificationContext!,
                                () {
                                  setState(() {
                                    //remove polygon
                                    if (gl.selectedPolygonLayer > 0) {
                                      gl.polygonLayers.removeAt(
                                        gl.selectedPolygonLayer,
                                      );
                                      gl.selectedPolygonLayer--;
                                    } else if (gl.selectedPolygonLayer == 0 &&
                                        gl.polygonLayers.isNotEmpty) {
                                      gl.polygonLayers.removeAt(
                                        gl.selectedPolygonLayer,
                                      );
                                    }
                                  });
                                  gl.saveChangesToPolygoneToPrefs = true;
                                },
                                "Message",
                                "\nVoulez vous vraiment supprimer ${gl.polygonLayers[gl.selectedPolygonLayer].name}?\n",
                              );
                            },
                            icon: Icon(
                              Icons.delete_forever,
                              color: Colors.black,
                              size: gl.display.equipixel * gl.iconSize * .75,
                            ),
                          ),
                        ),
                        SizedBox(
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              TextButton(
                                child: Container(
                                  alignment: Alignment.center,
                                  constraints: BoxConstraints(
                                    maxWidth:
                                        gl.display.equipixel *
                                        gl.polyListSelectedCardWidth *
                                        .5,
                                  ),
                                  child: Text(
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .name,
                                    style: TextStyle(
                                      color: Colors.black,
                                      fontSize:
                                          gl.display.equipixel * gl.fontSizeM,
                                    ),
                                  ),
                                ),
                                onPressed: () {
                                  PopupNameIntroducer(
                                    context,
                                    gl
                                        .polygonLayers[gl.selectedPolygonLayer]
                                        .name,
                                    (String nameIt) {
                                      setState(() {
                                        gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .name = nameIt;
                                        gl.saveChangesToPolygoneToPrefs = true;
                                      });
                                    },
                                    () {
                                      setState(() {
                                        _keyboard = false;
                                      });
                                    },
                                    () {
                                      setState(() {
                                        _keyboard = true;
                                      });
                                    },
                                  );
                                },
                              ),

                              Text(
                                "${(gl.polygonLayers[gl.selectedPolygonLayer].area / 100).round() / 100} Ha",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                              Text(
                                "${(gl.polygonLayers[gl.selectedPolygonLayer].perimeter).round() / 1000} km",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                            ],
                          ),
                        ),
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSize * 1.1,
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.start,
                            children: [
                              Column(
                                mainAxisAlignment:
                                    MainAxisAlignment.spaceBetween,
                                children: [
                                  IconButton(
                                    onPressed: () {
                                      PopupColorChooser(
                                        gl
                                            .polygonLayers[gl
                                                .selectedPolygonLayer]
                                            .colorInside,
                                        gl.notificationContext!,
                                        //change color
                                        (Color col) {
                                          setState(() {
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .setColorInside(col);
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
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
                                    },
                                    icon: Icon(
                                      Icons.color_lens,
                                      color: Colors.black,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSize *
                                          .75,
                                    ),
                                  ),
                                  IconButton(
                                    onPressed: () async {
                                      if (await gl
                                          .polygonLayers[gl
                                              .selectedPolygonLayer]
                                          .onlineSurfaceAnalysis()) {
                                        gl.mainStack.add(
                                          popupResultsMenu(
                                            gl.notificationContext!,
                                            gl
                                                .polygonLayers[gl
                                                    .selectedPolygonLayer]
                                                .decodedJson,
                                            () {
                                              gl.refreshMap(() {});
                                            },
                                            () {
                                              gl.refreshMap(() {});
                                            },
                                          ),
                                        );
                                        gl.refreshMap(() {});
                                      }
                                    },
                                    icon: Icon(
                                      Icons.analytics,
                                      color: Colors.black,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSize *
                                          .75,
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
                ),

              SizedBox(
                width: gl.polyListSelectedCardWidth * gl.display.equipixel,
                child: TextButton(
                  style: ButtonStyle(
                    backgroundColor: WidgetStateProperty.fromMap(
                      <WidgetStatesConstraint, Color>{
                        WidgetState.any: Colors.white,
                      },
                    ),
                    shape: WidgetStateProperty<OutlinedBorder>.fromMap(
                      <WidgetStatesConstraint, OutlinedBorder>{
                        WidgetState.any: RoundedRectangleBorder(
                          borderRadius: BorderRadiusGeometry.circular(12.0),
                        ),
                      },
                    ),
                    fixedSize: WidgetStateProperty.fromMap(
                      <WidgetStatesConstraint, Size>{
                        WidgetState.any: Size(
                          gl.display.equipixel * gl.polyListCardWidth * .97,
                          gl.display.equipixel *
                              gl.polyNewPolygonButtonHeight *
                              .9,
                        ),
                      },
                    ),
                  ),
                  key: Key('autsch-5-addPoly'),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(
                        Icons.add,
                        color: Colors.black,
                        size:
                            gl.display.equipixel *
                            gl.polyNewPolygonButtonHeight *
                            .7,
                      ),
                    ],
                  ),
                  onPressed: () async {
                    setState(() {
                      gl.polygonLayers.add(
                        PolygonLayer(polygonName: "Nouveau"),
                      );
                      PopupNameIntroducer(
                        context,
                        "",
                        (String nameIt) {
                          setState(() {
                            gl.polygonLayers[gl.polygonLayers.length - 1].name =
                                nameIt;
                            gl.saveChangesToPolygoneToPrefs = true;
                          });
                        },
                        () {
                          setState(() {
                            _keyboard = false;
                          });
                        },
                        () {
                          setState(() {
                            _keyboard = true;
                          });
                        },
                      );
                      gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                    });

                    gl.refreshMap(() {
                      gl.selectedPolygonLayer = gl.polygonLayers.length - 1;
                      gl.saveChangesToPolygoneToPrefs = true;
                    });
                    _scrollDown();
                  },
                ),
              ),
              if (!_keyboard)
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    SizedBox(width: gl.display.equipixel * gl.iconSize * .25),
                    _returnButton(context, widget.after),
                  ],
                ),
            ],
          ),
      ]),
    );
  }
}

Widget switchRowColWithOrientation(List<Widget> tree) {
  return gl.display.orientation == Orientation.portrait
      ? Column(mainAxisAlignment: MainAxisAlignment.spaceAround, children: tree)
      : Row(mainAxisAlignment: MainAxisAlignment.spaceAround, children: tree);
}

Widget popupSearchMenu(
  BuildContext context,
  String currentName,
  Function(LatLng) state,
  Function after,
) {
  _selectedSearchResultCard = -1;
  return MaterialApp(
    home: OrientationBuilder(
      builder: (context, orientation) {
        return AlertDialog(
          alignment: Alignment.center,
          titlePadding: EdgeInsets.all(0),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: gl.backgroundTransparentBlackBox,
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width:
                  gl.display.orientation == Orientation.portrait
                      ? gl.display.equipixel * gl.popupWindowsPortraitWidth
                      : gl.display.equipixel * gl.popupWindowsLandscapeWidth,
              height:
                  gl.display.orientation == Orientation.portrait
                      ? gl.display.equipixel * gl.popupWindowsPortraitHeight + 1
                      : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
              child: SearchMenu(state: state, after: after),
            ),
          ),
          titleTextStyle: TextStyle(color: Colors.white, fontSize: 25),
          actions: [],
        );
      },
    ),
  );
}

class SearchResultCard extends StatefulWidget {
  final Color boxColor;
  final Function(LatLng) state;
  final String typeDeResultat;
  final String descriptionDeResultat;
  final LatLng entry;
  final int index;

  const SearchResultCard({
    super.key,
    required this.boxColor,
    required this.state,
    required this.typeDeResultat,
    required this.entry,
    required this.descriptionDeResultat,
    required this.index,
  });

  @override
  State<StatefulWidget> createState() => _SearchResultCard();
}

int _selectedSearchResultCard = -1;
Function _revertStateOfSelectedSearchResultCard = () {};

class _SearchResultCard extends State<SearchResultCard> {
  @override
  Widget build(BuildContext context) {
    bool selected = _selectedSearchResultCard == widget.index ? true : false;
    return TextButton(
      onPressed: () {
        widget.state(widget.entry);
        gl.refreshMap(() {
          gl.modeMapShowSearchMarker = true;
        });
        selected
            ? setState(() {
              _selectedSearchResultCard = -1;
              _revertStateOfSelectedSearchResultCard = () {};
            })
            : setState(() {
              _selectedSearchResultCard = widget.index;
              _revertStateOfSelectedSearchResultCard();
              _revertStateOfSelectedSearchResultCard = () {
                if (mounted) {
                  setState(() {
                    selected = false;
                  });
                }
              };
            });
      },
      child: Card(
        margin: EdgeInsets.all(0.0),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side:
              selected
                  ? BorderSide(color: widget.boxColor, width: 2.0)
                  : BorderSide(
                    color: widget.boxColor.withAlpha(255),
                    width: 1.0,
                  ),
        ),

        color:
            selected
                ? widget.boxColor.withAlpha(255)
                : widget.boxColor.withAlpha(150),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Column(
              children: [
                Container(
                  constraints: BoxConstraints(
                    maxWidth:
                        gl.display.equipixel *
                        (gl.popupWindowsPortraitWidth - 15),
                  ),
                  child: Text(
                    widget.typeDeResultat,
                    style: TextStyle(
                      color: getColorTextFromBackground(widget.boxColor),
                      fontSize: gl.display.equipixel * gl.fontSizeS,
                    ),
                  ),
                ),

                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Container(
                      padding: EdgeInsets.all(5),
                      alignment: Alignment.center,
                      constraints: BoxConstraints(
                        maxWidth:
                            gl.display.equipixel *
                            (gl.popupWindowsPortraitWidth - 15),
                      ),
                      child: Text(
                        widget.descriptionDeResultat,
                        textAlign: TextAlign.justify,
                        style: TextStyle(
                          color: getColorTextFromBackground(widget.boxColor),
                          fontSize: gl.display.equipixel * gl.fontSizeS,
                        ),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class SearchMenu extends StatefulWidget {
  final Function(LatLng) state;
  final Function after;

  const SearchMenu({super.key, required this.state, required this.after});

  @override
  State<StatefulWidget> createState() => _SearchMenu();
}

class _SearchMenu extends State<SearchMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);

  static String lastSearchKey = "";
  static Map<String, http.Response> searchCache = {};
  static List<SearchResultCard> searchResults = [];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      resizeToAvoidBottomInset: false,
      backgroundColor: Colors.transparent,
      body: switchRowColWithOrientation([
        if (gl.display.orientation == Orientation.landscape)
          Container(
            constraints: BoxConstraints(
              maxHeight:
                  (gl.popupWindowsLandscapeHeight - 5) * gl.display.equipixel,
              maxWidth: gl.popupWindowsPortraitWidth * gl.display.equipixel,
            ),

            child: ListView(children: <Widget>[] + searchResults),
          ),
        Column(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
            Column(
              children: [
                SizedBox(
                  height: gl.display.equipixel * gl.fontSizeL * 1.1,
                  width: gl.display.equipixel * gl.popupWindowsPortraitWidth,
                  child: Text(
                    "Recherche d'un lieu",
                    textAlign: TextAlign.center,
                    style: TextStyle(
                      fontSize: gl.display.equipixel * gl.fontSizeL,
                      color: Colors.white,
                    ),
                  ),
                ),
                SizedBox(
                  height: gl.display.equipixel * gl.searchBarHeight,
                  width: gl.display.equipixel * gl.searchBarWidth,
                  child: Card(
                    child: TextFormField(
                      decoration: InputDecoration(
                        border: InputBorder.none,
                        hintText: "Tappez le nom d'un lieu",
                        prefixIcon: Icon(Icons.search, color: Colors.black),
                      ),
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
                            'http://gxgfservcarto.gxabt.ulg.ac.be:6666/search?layer=address&q=${searchString.replaceAll(' ', '+')}+Wallonie&format=json&addressdetails=1',
                          );
                          try {
                            response = await http.get(request);
                            searchCache[searchString] = response;
                          } catch (e) {
                            gl.print(e);

                            response = http.Response("", 404);
                          }
                        }
                        List<Map<String, dynamic>> decodedJson;
                        try {
                          (decodedJson =
                              (jsonDecode(response.body) as List)
                                  .cast<Map<String, dynamic>>());
                        } catch (e) {
                          gl.print(
                            "Error with response from gecoding service! $e",
                          );
                          (decodedJson =
                              (jsonDecode(testNominatimJsonResult) as List)
                                  .cast<Map<String, dynamic>>());
                        }
                        gl.poiMarkerList.clear();
                        searchResults.clear();
                        _selectedSearchResultCard = -1;
                        mounted
                            ? setState(() {
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
                                String? descriptionDeResultat =
                                    entry['display_name'];
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
                                gl.poiMarkerList.add(
                                  gl.PoiMarker(
                                    index: i++,
                                    position: LatLng(
                                      double.parse(entry['lat']),
                                      double.parse(entry['lon']),
                                    ),
                                    name: typeDeResultat!,
                                    address: descriptionDeResultat,
                                    city:
                                        entry['address']['city'] ??
                                        entry['address']['county'] ??
                                        entry['address']['state'] ??
                                        "",
                                    postcode:
                                        entry['address']['postcode'] ?? "",
                                  ),
                                );
                                searchResults.add(
                                  SearchResultCard(
                                    boxColor: getColorFromName(typeDeResultat),
                                    state: widget.state,
                                    typeDeResultat: typeDeResultat,
                                    entry: LatLng(
                                      double.parse(entry['lat']),
                                      double.parse(entry['lon']),
                                    ),
                                    descriptionDeResultat:
                                        descriptionDeResultat,
                                    index: i,
                                  ),
                                );
                              }
                            })
                            : () {
                              {
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
                                  String? descriptionDeResultat =
                                      entry['display_name'];
                                  if (descriptionDeResultat == null) {
                                    descriptionDeResultat = "Erreur du serveur";
                                    gl.print(
                                      "Erreur du serveur geocoding : ${entry['display_name']}",
                                    );
                                  } else {
                                    descriptionDeResultat =
                                        descriptionDeResultat.replaceAll(
                                          ", België /",
                                          "",
                                        );
                                    descriptionDeResultat =
                                        descriptionDeResultat.replaceAll(
                                          "/ Belgien",
                                          "",
                                        );
                                    descriptionDeResultat =
                                        descriptionDeResultat.replaceAll(
                                          "Wallonie, ",
                                          "",
                                        );
                                    descriptionDeResultat =
                                        descriptionDeResultat.replaceAll(
                                          "Belgique",
                                          "",
                                        );
                                  }
                                  Color boxColor = getColorFromName(
                                    typeDeResultat!,
                                  );
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
                                      postcode:
                                          entry['address']['postcode'] ?? "",
                                    ),
                                  );
                                  searchResults.add(
                                    SearchResultCard(
                                      boxColor: boxColor,
                                      state: widget.state,
                                      typeDeResultat: typeDeResultat,
                                      entry: LatLng(
                                        double.parse(entry['lat']),
                                        double.parse(entry['lon']),
                                      ),
                                      descriptionDeResultat:
                                          descriptionDeResultat,
                                      index: i,
                                    ),
                                  );
                                }
                              }
                            };
                      },
                    ),
                  ),
                ),
              ],
            ),
            if (gl.display.orientation == Orientation.landscape &&
                MediaQuery.of(context).viewInsets.bottom == 0)
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [_returnButton(context, widget.after)],
              ),
          ],
        ),
        if (gl.display.orientation == Orientation.portrait)
          SizedBox(
            height:
                (gl.popupWindowsPortraitHeight -
                    gl.searchBarHeight -
                    gl.fontSizeL * 1.1 -
                    gl.popupReturnButtonHeight) *
                gl.display.equipixel,
            child: ListView(children: <Widget>[] + searchResults),
          ),
        if (gl.display.orientation == Orientation.portrait)
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [_returnButton(context, widget.after)],
          ),
      ]),
    );
  }
}

int developperModeCounter = 0;
Widget forestimatorSettingsVersion(Function state) {
  return OrientationBuilder(
    builder: (context, orientation) {
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
                    maxWidth: gl.display.equipixel * 45,
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
                        await prefs.setBool(
                          'modeDevelopper',
                          gl.modeDevelopper,
                        );
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
                    maxWidth: gl.display.equipixel * 60,
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
                        gl.display.equipixel *
                        gl.popupWindowsPortraitWidth *
                        .7,
                  ),
                  child: Text(
                    "Le développement est financé par l'Accord Cadre de Recherches et Vulgarisation Forestières.\nLe contenu cartographique est en grande partie issu des recherches menées au sein de l'unité de Gestion des Ressources Forestières de Gembloux Agro-Bio Tech (ULiège).\n",
                    textAlign: TextAlign.justify,
                    style: TextStyle(
                      color: Colors.black,
                      overflow: TextOverflow.fade,
                    ),
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
                        gl.display.equipixel *
                        gl.popupWindowsPortraitWidth *
                        .7,
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
    },
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
  return OrientationBuilder(
    builder: (context, orientation) {
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
    },
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
    return OrientationBuilder(
      builder: (context, orientation) {
        return Column(
          children:
              <Widget>[
                Container(
                  constraints: BoxConstraints(
                    minWidth: gl.display.equipixel * 50,
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
                            minWidth: gl.display.equipixel * 5,
                          ),
                          child: Text(
                            "${gl.onboardLog.length - lengthLog + i}) ",
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxWidth:
                                gl.display.equipixel *
                                gl.popupWindowsPortraitWidth *
                                .9,
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
                            minWidth: gl.display.equipixel * 5,
                          ),
                          child: Text("$i"),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxWidth:
                                gl.display.equipixel *
                                gl.popupWindowsPortraitWidth *
                                .9,
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
      },
    );
  }
}

TextStyle styleSettingMenu() {
  return TextStyle(
    color: Colors.black,
    fontSize: gl.display.equipixel * gl.fontSizeM,
  );
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
              style: styleSettingMenu(),
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
              style: styleSettingMenu(),
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
                    size: gl.display.equipixel * gl.iconSize * .6,
                  ),
                  Text(
                    getLocation() ? "Accordé." : "Pas accordé.",
                    overflow: TextOverflow.clip,
                    textAlign: TextAlign.left,
                    style: styleSettingMenu(),
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
                style: styleSettingMenu(),
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
                      size: gl.display.equipixel * gl.iconSize * .6,
                    ),
                    Text(
                      getStorage() ? "Accordé." : "Pas accordé.",
                      overflow: TextOverflow.clip,
                      textAlign: TextAlign.left,
                      style: styleSettingMenu(),
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
  return OrientationBuilder(
    builder: (context, orientation) {
      return Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                padding: EdgeInsets.all(5),
                constraints: BoxConstraints(
                  maxWidth:
                      gl.display.equipixel * gl.popupWindowsPortraitWidth * .9,
                ),
                child: Text(
                  "Forestimator mobile ne collecte aucune donnée. Notre politique de confidentialité est consultable au:",
                  overflow: TextOverflow.clip,
                  textAlign: TextAlign.justify,
                  style: styleSettingMenu(),
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
                        gl.display.equipixel *
                        gl.popupWindowsPortraitWidth *
                        .8,
                  ),
                  child: Text(
                    "https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_",
                    overflow: TextOverflow.clip,
                    textAlign: TextAlign.left,
                    style: TextStyle(
                      color: Colors.blue,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
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
                  maxWidth:
                      gl.display.equipixel * gl.popupWindowsPortraitWidth * .9,
                ),
                child: Text(
                  "L'application utilise le gps pour afficher votre position actuelle sur la carte et seulement pendant l'utilisation.",
                  overflow: TextOverflow.clip,
                  textAlign: TextAlign.justify,
                  style: styleSettingMenu(),
                ),
              ),
            ],
          ),
        ],
      );
    },
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

Widget popupPolygonListMenu(
  BuildContext context,
  String currentName,
  Function(LatLng) state,
  Function after,
) {
  return MaterialApp(
    home: OrientationBuilder(
      builder: (context, orientation) {
        return AlertDialog(
          titlePadding: EdgeInsets.all(5),
          actionsPadding: EdgeInsets.all(0),
          contentPadding: EdgeInsets.all(0),
          insetPadding: EdgeInsets.all(0),
          buttonPadding: EdgeInsets.all(0),
          iconPadding: EdgeInsets.all(0),
          backgroundColor: gl.backgroundTransparentBlackBox,
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          content: Theme(
            data: Theme.of(context).copyWith(
              canvasColor: Colors.transparent,
              shadowColor: Colors.transparent,
            ),
            child: SizedBox(
              width:
                  gl.display.orientation == Orientation.portrait
                      ? gl.display.equipixel * gl.popupWindowsPortraitWidth
                      : gl.display.equipixel * gl.popupWindowsLandscapeWidth,
              height:
                  gl.display.orientation == Orientation.portrait
                      ? gl.display.equipixel * gl.popupWindowsPortraitHeight + 1
                      : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
              child: PolygonListMenu(state: state, after: after),
            ),
          ),

          actions: [],
        );
      },
    ),
  );
}

Widget _returnButton(BuildContext context, Function after) {
  return TextButton(
    style: ButtonStyle(
      backgroundColor: WidgetStateProperty.fromMap(
        <WidgetStatesConstraint, Color>{
          WidgetState.any: gl.colorAgroBioTech.withAlpha(200),
        },
      ),
      shape: WidgetStateProperty<OutlinedBorder>.fromMap(
        <WidgetStatesConstraint, OutlinedBorder>{
          WidgetState.any: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(12.0),
            side: BorderSide(
              color: Color.fromRGBO(205, 225, 138, 1.0),
              width: 2.0,
            ),
          ),
        },
      ),
      fixedSize: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Size>{
        WidgetState.any: Size(
          gl.display.equipixel * gl.popupReturnButtonWidth,
          gl.display.equipixel * gl.popupReturnButtonHeight,
        ),
      }),
    ),

    child: Text(
      "Fermer",
      maxLines: 1,
      textAlign: TextAlign.center,
      style: TextStyle(
        fontSize: gl.display.equipixel * gl.fontSizeM,
        color: Colors.black,
      ),
    ),
    onPressed: () {
      after();
      gl.mainStackPopLast();
    },
  );
}

Widget popupSettingsMenu(
  BuildContext context,
  String currentName,
  Function state,
  Function after,
) {
  return MaterialApp(
    home: OrientationBuilder(
      builder: (context, orientation) {
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
            child: Text(
              "Paramètres",
              maxLines: 1,
              style: TextStyle(color: Colors.black),
            ),
          ),
          content: SizedBox(
            width: gl.display.equipixel * gl.popupWindowsPortraitWidth,
            child: SettingsMenu(state: state),
          ),
          actions: [
            gl.display.orientation == Orientation.portrait
                ? Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [_returnButton(context, after)],
                )
                : Row(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    if (gl.display.orientation == Orientation.landscape)
                      SizedBox(width: gl.display.equipixel * gl.iconSize * .25),
                    _returnButton(context, after),
                  ],
                ),
          ],
        );
      },
    ),
  );
}

Widget _resultRow(String key, String value) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.start,
    children: [
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .8,
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
          minHeight: gl.display.equipixel * 5,
          minWidth: gl.display.equipixel * 5,
        ),
        child: Container(color: col),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .7,
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
          maxWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .2,
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
              maxWidth:
                  gl.display.equipixel * gl.popupWindowsPortraitWidth * .5,
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
                            maxWidth:
                                gl.display.equipixel *
                                gl.popupWindowsPortraitWidth *
                                .7,
                          ),
                          child: Text(
                            item.name,
                            style: TextStyle(
                              color: Colors.black,
                              fontSize: gl.fontSizeM * gl.display.equipixel,
                            ),
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

Widget popupResultsMenu(
  BuildContext context,
  Map<String, dynamic> json,
  Function state,
  Function after,
) {
  return OrientationBuilder(
    builder: (context, orientation) {
      return AlertDialog(
        alignment: Alignment.topCenter,
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
          width: gl.display.equipixel * gl.popupWindowsPortraitWidth,
          child: ResultsMenu(json: json),
        ),
        actions: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [_returnButton(context, after)],
          ),
        ],
      );
    },
  );
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
  Widget build(BuildContext context) {
    return Column(
      children: [
        if (widget.layerTile.downloadable) LayerDownloader(widget.layerTile),
        gl.anaSurfSelectedLayerKeys.contains(widget.layerTile.key)
            ? TextButton(
              style: ButtonStyle(
                minimumSize: WidgetStateProperty<Size>.fromMap(
                  <WidgetStatesConstraint, Size>{
                    WidgetState.any: Size(
                      gl.display.equipixel * gl.onCatalogueWidth * .98,
                      gl.display.equipixel * 15,
                    ),
                  },
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(
                    Icons.pentagon,
                    size: gl.display.equipixel * gl.onCatalogueIconSize,
                    color: Colors.black,
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 5,
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 60,
                    ),
                    child: Text(
                      "La couche est selectionnée pour l'analyse surfacique.",
                      style: TextStyle(color: Colors.black),
                    ),
                  ),
                ],
              ),
              onPressed: () async {
                gl.rebuildStatusSymbols(() {});
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
                      gl.display.equipixel * gl.onCatalogueWidth * .98,
                      gl.display.equipixel * 15,
                    ),
                  },
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(
                    Icons.pentagon_outlined,
                    size: gl.display.equipixel * gl.onCatalogueIconSize,
                    color: Colors.black,
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 5,
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 60,
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
                  gl.rebuildStatusSymbols(() {});
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
                      gl.display.equipixel * gl.onCatalogueWidth * .98,
                      gl.display.equipixel * 15,
                    ),
                  },
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(
                    Icons.location_on,
                    size: gl.display.equipixel * gl.onCatalogueIconSize,
                    color: Colors.black,
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 5,
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 60,
                    ),
                    child: Text(
                      "La couche est selectionnée pour l'analyse ponctuelle.",
                      style: TextStyle(color: Colors.black),
                    ),
                  ),
                ],
              ),
              onPressed: () async {
                gl.rebuildStatusSymbols(() {});
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
                      gl.display.equipixel * gl.onCatalogueWidth * .98,
                      gl.display.equipixel * 15,
                    ),
                  },
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(
                    Icons.location_off,
                    size: gl.display.equipixel * gl.onCatalogueIconSize,
                    color: Colors.black,
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 5,
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.display.equipixel * 60,
                    ),
                    child: Text(
                      "La couche n'est pas selectionnée pour l'analyse ponctuelle.",
                      style: TextStyle(color: Colors.black),
                    ),
                  ),
                ],
              ),
              onPressed: () async {
                gl.rebuildStatusSymbols(() {});
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
                    gl.display.equipixel * gl.onCatalogueWidth * .98,
                    gl.display.equipixel * 15,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Icon(
                  Icons.picture_as_pdf,
                  size: gl.display.equipixel * gl.onCatalogueIconSize,
                  color: Colors.black,
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: gl.display.equipixel * 5,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: gl.display.equipixel * 60,
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
                gl.dico.getLayerBase(widget.layerTile.key).mGroupe ==
                    "APT_CS") &&
            gl.dico
                .getEss(gl.dico.getLayerBase(widget.layerTile.key).getEssCode())
                .hasFEEapt())
          TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(
                <WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(
                    gl.display.equipixel * gl.onCatalogueWidth * .98,
                    gl.display.equipixel * 15,
                  ),
                },
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Icon(
                  Icons.picture_as_pdf_outlined,
                  size: gl.display.equipixel * gl.onCatalogueIconSize,
                  color: Colors.black,
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: gl.display.equipixel * 5,
                  ),
                ),
                Container(
                  constraints: BoxConstraints(
                    maxWidth: gl.display.equipixel * 60,
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
              PopupPdfMenu(
                gl.notificationContext!,
                widget.layerTile,
                path: path,
              );
            },
          ),
      ],
    );
  }
}

class MapLayerSelectionButton extends StatefulWidget {
  final bool offlineMode;
  final int selectionMode;
  final int index;
  final LayerTile layerTile;
  final Function state;
  const MapLayerSelectionButton({
    super.key,
    required this.offlineMode,
    this.selectionMode = -1,
    required this.index,
    required this.layerTile,
    required this.state,
  });

  @override
  State<StatefulWidget> createState() => _MapLayerSelectionButtonState();
}

class _MapLayerSelectionButtonState extends State<MapLayerSelectionButton> {
  static final Map<int, Function> _layerSelectionSetStates = {
    -1: () {},
    0: () {},
    1: () {},
    2: () {},
  };

  void _callSelectedButtonsSetStates() {
    for (Function function in _layerSelectionSetStates.values) {
      function();
    }
    gl.refreshMap(() {});
    gl.rebuildSwitcherBox(() {});
  }

  @override
  Widget build(BuildContext context) {
    int interfaceSelectedMapKey = gl.getIndexForLayer(
      widget.layerTile.key,
      widget.offlineMode,
    );
    _layerSelectionSetStates[interfaceSelectedMapKey] = () {
      if (mounted) setState(() {});
    };
    int interfaceSelectedMapSwitcherSlot = widget.selectionMode;
    if (interfaceSelectedMapKey == -1) {
      return TextButton(
        style: ButtonStyle(
          backgroundColor: WidgetStateProperty<Color>.fromMap(
            <WidgetStatesConstraint, Color>{
              WidgetState.any: Colors.transparent,
            },
          ),
          minimumSize:
              WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  gl.display.equipixel * gl.onCatalogueLayerSelectionButton,
                  gl.display.equipixel * gl.onCatalogueLayerSelectionButton,
                ),
              }),
        ),
        onPressed: () {
          if (!widget.offlineMode) {
            if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, false) !=
                -1) {
              setState(() {
                int index = gl.sameOnlineAsOfflineLayer(
                  widget.layerTile.key,
                  false,
                );
                gl.removeLayerFromList(index: index, offline: true);
                gl.replaceLayerFromList(
                  widget.layerTile.key,
                  index: index,
                  offline: false,
                );
              });
            } else {
              setState(() {
                gl.replaceLayerFromList(
                  widget.layerTile.key,
                  index: interfaceSelectedMapSwitcherSlot,
                  offline: false,
                );
              });
            }
          } else {
            if (gl.getCountOfflineLayerSelected() == 0) {
              if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true) !=
                  -1) {
                setState(() {
                  int index = gl.sameOnlineAsOfflineLayer(
                    widget.layerTile.key,
                    true,
                  );
                  gl.removeLayerFromList(index: index, offline: false);
                  gl.replaceLayerFromList(
                    widget.layerTile.key,
                    index: index,
                    offline: true,
                  );
                });
              } else {
                setState(() {
                  gl.replaceLayerFromList(
                    widget.layerTile.key,
                    index: interfaceSelectedMapSwitcherSlot,
                    offline: true,
                  );
                });
              }
            } else if (gl.getCountOfflineLayerSelected() == 1) {
              if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true) !=
                  -1) {
                setState(() {
                  int index = gl.getIndexForNextLayerOffline();
                  gl.replaceLayerFromList(
                    gl.selectedLayerForMap[index].mCode,
                    index: index,
                    offline: false,
                  );
                  index = gl.sameOnlineAsOfflineLayer(
                    widget.layerTile.key,
                    true,
                  );
                  gl.removeLayerFromList(index: index, offline: false);
                  gl.replaceLayerFromList(
                    widget.layerTile.key,
                    index: index,
                    offline: true,
                  );
                });
              } else {
                setState(() {
                  int index = gl.getIndexForNextLayerOffline();
                  gl.removeLayerFromList(index: index, offline: true);
                  gl.replaceLayerFromList(
                    widget.layerTile.key,
                    index: index,
                    offline: true,
                  );
                });
              }
            }
          }
          gl.refreshMap(() {});
          _callSelectedButtonsSetStates();
        },
        child: Icon(
          Icons.layers,
          size: gl.display.equipixel * gl.onCatalogueLayerSelectionButton,
          color: Colors.black,
        ),
      );
    } else {
      return TextButton(
        style: ButtonStyle(
          backgroundColor: WidgetStateProperty<Color>.fromMap(
            <WidgetStatesConstraint, Color>{
              WidgetState.any: gl.colorAgroBioTech,
            },
          ),
          minimumSize:
              WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  gl.display.equipixel * gl.onCatalogueLayerSelectionButton,
                  gl.display.equipixel * gl.onCatalogueLayerSelectionButton,
                ),
              }),
        ),
        onPressed: () {
          setState(() {
            gl.slotContainsLayer(interfaceSelectedMapKey, widget.layerTile.key)
                ? gl.removeLayerFromList(
                  index: interfaceSelectedMapKey,
                  offline: gl.offlineMode,
                )
                : {
                  gl.replaceLayerFromList(
                    gl.selectedLayerForMap[interfaceSelectedMapKey].mCode,
                    index: gl.getIndexForLayer(
                      widget.layerTile.key,
                      widget.offlineMode,
                    ),
                    offline: gl.offlineMode,
                  ),
                  gl.replaceLayerFromList(
                    widget.layerTile.key,
                    index: interfaceSelectedMapKey,
                    offline: gl.offlineMode,
                  ),
                };
          });
          _callSelectedButtonsSetStates();
        },
        child: Container(
          alignment: Alignment.center,
          child: Text(
            (interfaceSelectedMapKey + 1).toString(),
            style: TextStyle(
              fontSize: gl.display.equipixel * gl.fontSizeM,
              color: Colors.black,
            ),
          ),
        ),
      );
    }
  }
}

class OnlineMapMenu extends StatefulWidget {
  final Function? stateOfLayerSwitcher;
  final bool offlineMode;
  final int selectionMode;
  final Function after;
  final String selectedMapCode;
  const OnlineMapMenu({
    super.key,
    required this.offlineMode,
    this.selectionMode = -1,
    this.selectedMapCode = "",
    this.stateOfLayerSwitcher,
    required this.after,
  });

  @override
  State<StatefulWidget> createState() => _OnlineMapMenu();
}

class _OnlineMapMenu extends State<OnlineMapMenu> {
  static bool modified = false;
  static int selectedCategory = -1;
  static int selectedMap = -1;
  static LayerTile? selectedLayerTile;
  final ScrollController _controller = ScrollController();

  bool _showCatalogue = true;
  final List<String> _resultOfMapSearch = [];

  void scrollToBeginning(double more) {
    _controller.animateTo(
      _controller.position.minScrollExtent + more,
      duration: Duration(milliseconds: 500),
      curve: Curves.fastEaseInToSlowEaseOut,
    );
  }

  @override
  void initState() {
    super.initState();
    modified = false;
    if (!"123".contains(widget.selectedMapCode)) {
      selectedCategory = -1;
      selectedMap = -1;
    }
  }

  @override
  Widget build(BuildContext context) {
    Function stateOfLayerSwitcher;
    if (widget.stateOfLayerSwitcher == null) {
      stateOfLayerSwitcher = (Function f) {
        f();
      };
    } else {
      stateOfLayerSwitcher = widget.stateOfLayerSwitcher!;
    }
    gl.rebuildOfflineCatalogue = (Function f) {
      mounted
          ? setState(() {
            f();
          })
          : (Function f) {
            f();
          };
    };
    return gl.firstTimeUse
        ? PopupDownloadRecomendedLayers(
          title: "Bienvenu",
          accept: "oui",
          onAccept: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            final SharedPreferences prefs =
                await SharedPreferences.getInstance();
            await prefs.setBool('firstTimeUse', gl.firstTimeUse);
            for (var key in gl.downloadableLayerKeys) {
              downloadLayer(key);
            }
          },
          decline: "non",
          onDecline: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            final SharedPreferences prefs =
                await SharedPreferences.getInstance();
            await prefs.setBool('firstTimeUse', gl.firstTimeUse);
          },
          dialog:
              "Autorisez-vous l'aplication à télécharger un jeu de 6 couches pour une utilisation hors ligne? Ces couches couvrent toutes la Région Wallonne et totalisent +- 214 Mo.",
        )
        : switchRowColWithOrientation([
          Column(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              SizedBox(
                height: gl.display.equipixel * gl.searchBarHeight,
                width: gl.display.equipixel * gl.searchBarWidth,
                child: Card(
                  child: TextFormField(
                    decoration: InputDecoration(
                      border: InputBorder.none,
                      hintText: "Tappez le nom d'une couche",
                      contentPadding: EdgeInsets.all(10),
                      prefixIcon: Icon(Icons.search, color: Colors.black),
                    ),
                    onTap: () {
                      setState(() {});
                    },
                    textAlign: TextAlign.start,
                    autocorrect: false,
                    enableSuggestions: true,
                    onChanged: (String value) {
                      _resultOfMapSearch.clear();
                      if (value.isNotEmpty) {
                        for (String term in value.split(' ')) {
                          if (term != '') {
                            for (var layer in gl.dico.mLayerBases.values) {
                              if (!layer.mExpert &&
                                  (widget.offlineMode
                                      ? layer.mOffline
                                      : true) &&
                                  (layer.mNom
                                      .toLowerCase()
                                      .replaceAll('è', 'e')
                                      .replaceAll('é', 'e')
                                      .replaceAll('ê', 'e')
                                      .replaceAll('â', 'a')
                                      .replaceAll('à', 'a')
                                      .replaceAll('ç', 'c')
                                      .contains(
                                        term
                                            .toLowerCase()
                                            .replaceAll('è', 'e')
                                            .replaceAll('é', 'e')
                                            .replaceAll('ê', 'e')
                                            .replaceAll('â', 'a')
                                            .replaceAll('à', 'a')
                                            .replaceAll('ç', 'c'),
                                      ))) {
                                _resultOfMapSearch.add(layer.mCode);
                              }
                            }
                          }
                        }
                      }
                      if (_resultOfMapSearch.isEmpty || value.isEmpty) {
                        setState(() {
                          _showCatalogue = true;
                        });
                      } else {
                        setState(() {
                          _showCatalogue = false;
                        });
                      }
                    },
                  ),
                ),
              ),

              SizedBox(
                height:
                    gl.display.orientation == Orientation.portrait
                        ? (gl.popupWindowsPortraitHeight -
                                gl.searchBarHeight -
                                gl.popupReturnButtonHeight) *
                            gl.display.equipixel
                        : (gl.popupWindowsLandscapeHeight -
                                gl.searchBarHeight) *
                            gl.display.equipixel,
                width: gl.popupWindowsPortraitWidth * gl.display.equipixel,
                child:
                    _showCatalogue
                        ? ListView(
                          controller: _controller,
                          padding: const EdgeInsets.symmetric(horizontal: 0),
                          children: _injectGroupData(
                            (int i, GroupeCouche groupe) => TextButton(
                              style: ButtonStyle(
                                minimumSize:
                                    i == selectedCategory
                                        ? WidgetStateProperty<Size>.fromMap(
                                          <WidgetStatesConstraint, Size>{
                                            WidgetState.any: Size(
                                              gl.display.equipixel *
                                                  gl.onCatalogueWidth *
                                                  .7,
                                              gl.display.equipixel *
                                                  gl.onCatalogueMapHeight,
                                            ),
                                          },
                                        )
                                        : WidgetStateProperty<Size>.fromMap(
                                          <WidgetStatesConstraint, Size>{
                                            WidgetState.any: Size(
                                              gl.display.equipixel *
                                                  gl.onCatalogueWidth *
                                                  .7,
                                              gl.display.equipixel *
                                                  gl.onCatalogueCategoryHeight,
                                            ),
                                          },
                                        ),
                              ),
                              key: Key('$i'),
                              onPressed:
                                  i == selectedCategory
                                      ? () {}
                                      : () {
                                        setState(() {
                                          selectedCategory = i;
                                          selectedMap = -1;
                                          selectedLayerTile = null;
                                        });
                                        scrollToBeginning(
                                          i *
                                              gl.onCatalogueCategoryHeight *
                                              5.5,
                                        );
                                      },
                              child: Card(
                                surfaceTintColor: Colors.transparent,
                                shadowColor: Colors.transparent,
                                color:
                                    i == selectedCategory
                                        ? gl.colorAgroBioTech.withAlpha(50)
                                        : gl.colorAgroBioTech.withAlpha(200),
                                child: Column(
                                  mainAxisAlignment: MainAxisAlignment.center,
                                  children: [
                                    i != selectedCategory
                                        ? Container(
                                          alignment: Alignment.center,
                                          padding: EdgeInsets.all(3),
                                          constraints: BoxConstraints(
                                            maxWidth:
                                                gl.display.equipixel *
                                                gl.onCatalogueWidth *
                                                .97,
                                            minHeight:
                                                gl.display.equipixel *
                                                gl.onCatalogueCategoryHeight,
                                          ),
                                          child: Text(
                                            groupe.mLabel,
                                            textAlign: TextAlign.center,
                                            style: TextStyle(
                                              color: Colors.black,
                                              fontSize:
                                                  gl.display.equipixel *
                                                  gl.fontSizeM,
                                            ),
                                          ),
                                        )
                                        : Container(
                                          alignment: Alignment.center,
                                          padding: EdgeInsets.all(0),
                                          constraints: BoxConstraints(
                                            maxWidth:
                                                gl.display.equipixel *
                                                gl.onCatalogueWidth *
                                                .97,
                                          ),
                                          child: ListBody(
                                            children:
                                                <Widget>[
                                                  TextButton(
                                                    onPressed: () {
                                                      setState(() {
                                                        selectedCategory = -1;
                                                        selectedMap = -1;
                                                        selectedLayerTile =
                                                            null;
                                                      });
                                                    },
                                                    child: Card(
                                                      color: gl.colorAgroBioTech
                                                          .withAlpha(200),
                                                      child: Container(
                                                        alignment:
                                                            Alignment.center,
                                                        padding: EdgeInsets.all(
                                                          3,
                                                        ),
                                                        constraints: BoxConstraints(
                                                          maxWidth:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.onCatalogueWidth *
                                                              .97,
                                                          minWidth:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.onCatalogueWidth *
                                                              .97,
                                                          minHeight:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.onCatalogueMapHeight *
                                                              .97,
                                                        ),
                                                        child: Text(
                                                          groupe.mLabel,
                                                          textAlign:
                                                              TextAlign.center,
                                                          style: TextStyle(
                                                            color: Colors.black,
                                                            fontSize:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.fontSizeM,
                                                          ),
                                                        ),
                                                      ),
                                                    ),
                                                  ),
                                                ] +
                                                _injectLayerData(groupe.mCode, (
                                                  int i,
                                                  LayerTile layerTile,
                                                ) {
                                                  return layerTileCard(
                                                    i,
                                                    layerTile,
                                                    widget.offlineMode,
                                                    widget.selectionMode,
                                                    stateOfLayerSwitcher,
                                                    setState,
                                                    scrollToBeginning,
                                                    noLegend:
                                                        gl
                                                            .display
                                                            .orientation
                                                            .index ==
                                                        1,
                                                  );
                                                }),
                                          ),
                                        ),
                                  ],
                                ),
                              ),
                            ),
                          ),
                        )
                        : ListView(
                          children: List<Widget>.generate(
                            _resultOfMapSearch.length,
                            (int i) {
                              LayerTile layerTile = LayerTile(
                                key: _resultOfMapSearch[i],
                                name:
                                    gl.dico
                                        .getLayerBase(_resultOfMapSearch[i])
                                        .mNom,
                                filter:
                                    gl.dico
                                        .getLayerBase(_resultOfMapSearch[i])
                                        .mGroupe,
                                downloadable:
                                    gl.dico
                                        .getLayerBase(_resultOfMapSearch[i])
                                        .mIsDownloadableRW,
                              );
                              return layerTileCard(
                                i,
                                layerTile,
                                widget.offlineMode,
                                widget.selectionMode,
                                stateOfLayerSwitcher,
                                setState,
                                scrollToBeginning,
                                noLegend:
                                    gl.display.orientation ==
                                    Orientation.landscape,
                              );
                            },
                          ),
                        ),
              ),
            ],
          ),
          Column(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              if (gl.display.orientation == Orientation.landscape)
                Container(
                  constraints: BoxConstraints(
                    maxWidth:
                        gl.display.equipixel * gl.popupWindowsPortraitWidth,
                    maxHeight:
                        gl.display.equipixel *
                        (gl.popupWindowsLandscapeHeight -
                            gl.popupReturnButtonHeight),
                  ),
                  child: ListView(
                    children: [
                      layerTileCard(
                        selectedMap,
                        selectedLayerTile,
                        widget.offlineMode,
                        widget.selectionMode,
                        stateOfLayerSwitcher,
                        setState,
                        scrollToBeginning,
                      ),
                    ],
                  ),
                ),
              Row(
                mainAxisAlignment:
                    gl.display.orientation == Orientation.landscape
                        ? MainAxisAlignment.end
                        : MainAxisAlignment.center,
                children: [_returnButton(context, widget.after)],
              ),
            ],
          ),
        ]);
  }

  List<Widget> _injectGroupData(Widget Function(int, GroupeCouche) generate) {
    Map<String, Null> groupesNonVides = {};
    for (String key in gl.dico.mLayerBases.keys) {
      if ((widget.offlineMode ? gl.dico.getLayerBase(key).mOffline : true) &&
          !gl.dico.getLayerBase(key).mExpert) {
        groupesNonVides[gl.dico.getLayerBase(key).mGroupe] = null;
      }
    }
    int i = 0;
    List<GroupeCouche> groupes = [];
    for (String key in groupesNonVides.keys) {
      for (GroupeCouche couche in gl.dico.mGrCouches) {
        if (couche.mCode == key) {
          i++;
          if (couche.mCode ==
                  gl.dico.getLayerBase(widget.selectedMapCode).mGroupe &&
              !modified) {
            selectedCategory = i - 1;
          }
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
    int i = 0;
    for (var key in mp.keys) {
      if (category == mp[key]!.mGroupe &&
          !mp[key]!.mExpert &&
          mp[key]!.mVisu &&
          mp[key]?.mTypeGeoservice == "" &&
          (widget.offlineMode ? mp[key]!.mOffline : true)) {
        i++;
        layer.add(
          LayerTile(
            name: mp[key]!.mNom,
            filter: mp[key]!.mGroupe,
            key: key,
            downloadable: mp[key]!.mIsDownloadableRW,
            extern: mp[key]!.mCategorie == "Externe",
          ),
        );
        if (widget.selectedMapCode == key && !modified) {
          selectedMap = i - 1;
          selectedLayerTile = layer.last;
        }
      }
    }
    WidgetsBinding.instance.addPostFrameCallback(
      (_) => scrollToBeginning(
        (selectedCategory + 1) * gl.onCatalogueCategoryHeight * 5.5 +
            selectedMap * gl.onCatalogueMapHeight * 4.5,
      ),
    );
    return List<Widget>.generate(layer.length, (i) {
      return generate(i, layer[i]);
    });
  }
}

class MapStatusSymbols extends StatefulWidget {
  final bool offlineMode;
  final String layerCode;
  const MapStatusSymbols({
    super.key,
    required this.offlineMode,
    this.layerCode = "",
  });

  @override
  State<StatefulWidget> createState() => _MapStatusSymbols();
}

class _MapStatusSymbols extends State<MapStatusSymbols> {
  static final Map<String, Function> _setStateStatusMaps = {};
  static bool _onlyOnce = true;
  String? mapName;

  @override
  void initState() {
    mapName = widget.layerCode;
    _setStateStatusMaps[mapName!] = (Function f) {
      mounted
          ? setState(() {
            f();
          })
          : f();
    };
    if (_onlyOnce) {
      gl.rebuildStatusSymbols = (Function f) {
        for (Function status in _setStateStatusMaps.values) {
          status(f);
        }
      };
      _onlyOnce = false;
    }
    super.initState();
  }

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    double multi = 0.4;
    List<Widget> statusIcons = [
      if (gl.dico.getLayerBase(mapName!).mIsDownloadableRW &&
          gl.dico.getLayerBase(mapName!).mOffline &&
          !widget.offlineMode)
        Icon(
          color: Colors.blue,
          Icons.save,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).mIsDownloadableRW &&
          !gl.dico.getLayerBase(mapName!).mOffline &&
          !widget.offlineMode)
        Icon(
          color: Colors.lightBlue,
          Icons.file_download,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).mCategorie != "Externe")
        Icon(
          color: Colors.brown,
          Icons.legend_toggle,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).hasDoc())
        Icon(
          color: Colors.brown,
          Icons.picture_as_pdf,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
      if (gl.anaSurfSelectedLayerKeys.contains(mapName!))
        Icon(
          color: Colors.deepOrange,
          Icons.pentagon,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
      if (gl.anaPtSelectedLayerKeys.contains(mapName!))
        Icon(
          color: Colors.deepOrange,
          Icons.location_on,
          size: gl.iconSize * multi * gl.display.equipixel,
        ),
    ];
    return statusIcons.length > 3
        ? Row(
          children: [
            Column(
              mainAxisAlignment: MainAxisAlignment.start,
              children: statusIcons.sublist(0, 3),
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.start,
              children: statusIcons.sublist(3),
            ),
          ],
        )
        : Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [Column(children: statusIcons), SizedBox()],
        );
  }
}

Card layerTileCard(
  int i,
  LayerTile? layerTile,
  bool offlineMode,
  int selectionMode,
  Function stateOfLayerSwitcher,
  Function setState,
  Function scroll, {
  bool noLegend = false,
}) {
  return layerTile != null
      ? Card(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side:
              i == _OnlineMapMenu.selectedMap
                  ? BorderSide(
                    color: gl.colorAgroBioTech.withAlpha(255),
                    width: 2.0,
                  )
                  : BorderSide(color: Colors.transparent, width: 0.0),
        ),
        color:
            i == _OnlineMapMenu.selectedMap
                ? Colors.white.withAlpha(255)
                : Colors.white.withAlpha(200),
        child:
            i != _OnlineMapMenu.selectedMap || noLegend
                ? Container(
                  constraints: BoxConstraints(
                    minHeight: gl.display.equipixel * gl.onCatalogueMapHeight,
                  ),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          MapStatusSymbols(
                            offlineMode: offlineMode,
                            layerCode: layerTile.key,
                          ),
                          SizedBox(
                            height:
                                gl.display.equipixel *
                                gl.onCatalogueMapHeight *
                                .9,
                            width: gl.display.equipixel * 58,
                            child: TextButton(
                              onPressed: () {
                                setState(() {
                                  _OnlineMapMenu.selectedMap == i
                                      ? {
                                        _OnlineMapMenu.selectedMap = -1,
                                        _OnlineMapMenu.selectedLayerTile = null,
                                        _OnlineMapMenu.modified = true,
                                      }
                                      : {
                                        _OnlineMapMenu.selectedMap = i,
                                        _OnlineMapMenu.selectedLayerTile =
                                            layerTile,
                                        _OnlineMapMenu.modified = true,
                                        scroll(
                                          (_OnlineMapMenu.selectedCategory +
                                                      1) *
                                                  gl.onCatalogueCategoryHeight *
                                                  5.5 +
                                              i * gl.onCatalogueMapHeight * 4.5,
                                        ),
                                      };
                                });
                              },
                              child: Text(
                                layerTile.name,
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeM * .85,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.display.equipixel * gl.iconSize,
                            width: gl.display.equipixel * gl.iconSize * 1.2,
                            child: MapLayerSelectionButton(
                              layerTile: layerTile,
                              offlineMode: offlineMode,
                              index: i,
                              selectionMode: selectionMode,
                              state: stateOfLayerSwitcher,
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                )
                : Column(
                  children: [
                    if (gl.display.orientation == Orientation.portrait)
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          MapStatusSymbols(
                            offlineMode: offlineMode,
                            layerCode: layerTile.key,
                          ),
                          SizedBox(
                            height:
                                gl.display.equipixel * gl.onCatalogueMapHeight,
                            width: gl.display.equipixel * 55,
                            child: TextButton(
                              onPressed: () {
                                setState(() {
                                  _OnlineMapMenu.selectedMap == i
                                      ? {
                                        _OnlineMapMenu.selectedMap = -1,
                                        _OnlineMapMenu.selectedLayerTile = null,
                                      }
                                      : {
                                        _OnlineMapMenu.selectedMap = i,
                                        _OnlineMapMenu.selectedLayerTile =
                                            layerTile,
                                      };
                                });
                              },
                              child: Text(
                                layerTile.name,
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.display.equipixel * gl.iconSize * 1,
                            width: gl.display.equipixel * gl.iconSize * 1.2,
                            child: MapLayerSelectionButton(
                              layerTile: layerTile,
                              offlineMode: offlineMode,
                              index: i,
                              selectionMode: selectionMode,
                              state: stateOfLayerSwitcher,
                            ),
                          ),
                        ],
                      ),
                    OnlineMapStatusTool(layerTile: layerTile),
                    LegendView(
                      layerKey: layerTile.key,
                      color: gl.colorBackgroundSecondary,
                      constraintsText: BoxConstraints(
                        minWidth: gl.display.equipixel * 40,
                        maxWidth: gl.display.equipixel * 40,
                        minHeight: gl.display.equipixel * 2,
                        maxHeight: gl.display.equipixel * 2,
                      ),
                      constraintsColors: BoxConstraints(
                        minWidth: gl.display.equipixel * 40,
                        maxWidth: gl.display.equipixel * 40,
                        minHeight: gl.display.equipixel * 2,
                        maxHeight: gl.display.equipixel * 2,
                      ),
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [layerTile.proprietaire()],
                    ),
                  ],
                ),
      )
      : Card(
        color: Colors.grey,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            SizedBox(
              child: Text(
                "Selectionnez une carte pour voir les détails.",
                style: TextStyle(
                  fontSize: gl.display.equipixel * gl.fontSizeM,
                  color: Colors.black,
                ),
              ),
            ),
          ],
        ),
      );
}

Widget popupOnlineMapMenu(
  BuildContext context,
  Function after,
  bool offlineMode,
  int selectionMode,
  String selectedLayer,
  stateOfLayerSwitcher,
) {
  return OrientationBuilder(
    builder: (context, orientation) {
      return AlertDialog(
        alignment: Alignment.center,
        titlePadding: EdgeInsets.all(0),
        actionsPadding: EdgeInsets.all(0),
        contentPadding: EdgeInsets.all(0),
        insetPadding: EdgeInsets.all(0),
        buttonPadding: EdgeInsets.all(0),
        iconPadding: EdgeInsets.all(0),
        backgroundColor: gl.backgroundTransparentBlackBox,
        surfaceTintColor: Colors.transparent,
        shadowColor: Colors.transparent,
        title: null,
        content: Theme(
          data: Theme.of(context).copyWith(
            canvasColor: Colors.transparent,
            shadowColor: Colors.transparent,
          ),
          child: SizedBox(
            width:
                gl.display.orientation == Orientation.portrait
                    ? gl.display.equipixel * gl.popupWindowsPortraitWidth
                    : gl.display.equipixel * gl.popupWindowsLandscapeWidth,
            height:
                gl.display.orientation == Orientation.portrait
                    ? gl.display.equipixel * gl.popupWindowsPortraitHeight + 1
                    : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
            child: OnlineMapMenu(
              offlineMode: offlineMode,
              selectionMode: selectionMode,
              stateOfLayerSwitcher: stateOfLayerSwitcher,
              after: after,
              selectedMapCode: selectedLayer,
            ),
          ),
        ),
        titleTextStyle: TextStyle(
          color: Colors.white,
          fontSize: gl.display.equipixel * gl.fontSizeM,
        ),
        actions: [],
      );
    },
  );
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

Widget popupLayerSwitcher(
  BuildContext context,
  Function after,
  Function mainMenuBarDummy,
  void Function(LatLng) switchToLocationInSearchMenu,
) {
  return OrientationBuilder(
    builder: (context, orientation) {
      return Stack(
        children: [
          AlertDialog(
            alignment:
                gl.display.orientation == Orientation.portrait
                    ? Alignment.center
                    : Alignment.topCenter,
            titlePadding: EdgeInsets.all(0),
            actionsPadding: EdgeInsets.all(0),
            contentPadding: EdgeInsets.all(0),
            insetPadding: EdgeInsets.all(0),
            buttonPadding: EdgeInsets.all(0),
            iconPadding: EdgeInsets.all(0),
            backgroundColor: gl.backgroundTransparentBlackBox,
            surfaceTintColor: Colors.transparent,
            shadowColor: Colors.transparent,
            content: Theme(
              data: Theme.of(context).copyWith(
                canvasColor: Colors.transparent,
                shadowColor: Colors.transparent,
              ),
              child: LayerSwitcher(switchToLocationInSearchMenu),
            ),
            titleTextStyle: TextStyle(
              color: Colors.white,
              fontSize: gl.display.equipixel * gl.fontSizeM,
            ),
            actions: [],
          ),
          Container(
            alignment: Alignment.center,
            child: Container(
              alignment: Alignment.bottomCenter,
              child: mainMenuBarDummy(() {
                gl.mainStackPopLast();
              }),
            ),
          ),
        ],
      );
    },
  );
}

class LayerSwitcher extends StatefulWidget {
  final void Function(LatLng) switchToLocationInSearchMenu;
  const LayerSwitcher(this.switchToLocationInSearchMenu, {super.key});
  @override
  State<LayerSwitcher> createState() => _LayerSwitcher();
}

class _LayerSwitcher extends State<LayerSwitcher> {
  @override
  Widget build(BuildContext context) {
    gl.rebuildLayerSwitcher = (Function f) {
      mounted
          ? setState(() {
            f();
          })
          : () {
            f();
          };
    };
    return SizedBox(
      width:
          gl.display.orientation == Orientation.portrait
              ? gl.display.equipixel * gl.layerswitcherBoxWidth
              : gl.display.equipixel * gl.layerswitcherBoxWidth * 2.2,
      height:
          gl.display.orientation == Orientation.portrait
              ? gl.offlineMode
                  ? (gl.layerSwitcherBoxHeightPortraitOffline +
                          gl.layerswitcherButtonsBoxHeight +
                          (gl.poiMarkerList.isNotEmpty &&
                                  gl.polygonLayers.isNotEmpty
                              ? gl.layerSwitcherTileHeight +
                                  gl.layerswitcherControlBoxHeight
                              : (gl.poiMarkerList.isNotEmpty ||
                                      gl.polygonLayers.isNotEmpty
                                  ? gl.layerswitcherControlBoxHeight
                                  : 0.0))) *
                      gl.display.equipixel
                  : (gl.layerSwitcherBoxHeightPortrait +
                          gl.layerswitcherButtonsBoxHeight +
                          (gl.poiMarkerList.isNotEmpty &&
                                  gl.polygonLayers.isNotEmpty
                              ? gl.layerSwitcherTileHeight +
                                  gl.layerswitcherControlBoxHeight
                              : (gl.poiMarkerList.isNotEmpty ||
                                      gl.polygonLayers.isNotEmpty
                                  ? gl.layerswitcherControlBoxHeight
                                  : 0.0))) *
                      gl.display.equipixel
              : gl.layerSwitcherBoxHeightLandscape * gl.display.equipixel,
      child: switchRowColWithOrientation([
        if ((gl.polygonLayers.isNotEmpty || gl.poiMarkerList.isNotEmpty) &&
            gl.display.orientation == Orientation.portrait)
          SizedBox(
            width: gl.display.equipixel * gl.layerswitcherBoxWidth,
            height:
                (gl.poiMarkerList.isNotEmpty && gl.polygonLayers.isNotEmpty
                    ? gl.layerSwitcherTileHeight +
                        gl.layerswitcherControlBoxHeight
                    : (gl.poiMarkerList.isNotEmpty ||
                            gl.polygonLayers.isNotEmpty
                        ? gl.layerswitcherControlBoxHeight
                        : 0.0)) *
                gl.display.equipixel,
            child: Column(
              children: [
                SizedBox(
                  width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                  height: gl.display.equipixel * gl.fontSizeL,
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        "Controlez les couches visibles",
                        textAlign: TextAlign.justify,
                        style: TextStyle(
                          color: Colors.white,
                          fontSize: gl.display.equipixel * gl.fontSizeM,
                        ),
                      ),
                    ],
                  ),
                ),
                UpperLayerControl(
                  switchToLocationInSearchMenu:
                      widget.switchToLocationInSearchMenu,
                ),
              ],
            ),
          ),
        SizedBox(
          width: gl.display.equipixel * gl.layerswitcherBoxWidth,
          height:
              gl.display.equipixel *
              (gl.offlineMode
                  ? gl.layerSwitcherBoxHeightPortraitOffline
                  : gl.layerSwitcherBoxHeightPortrait),
          child: Column(
            children: [
              SizedBox(
                width: gl.display.equipixel * gl.layerswitcherBoxWidth,
                height: gl.display.equipixel * gl.fontSizeXL,
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      "Changez les cartes affichées",
                      textAlign: TextAlign.justify,
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: gl.display.equipixel * gl.fontSizeM,
                      ),
                    ),
                  ],
                ),
              ),
              SizedBox(
                width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                height:
                    gl.display.equipixel *
                        (gl.offlineMode
                            ? gl.layerSwitcherBoxHeightPortraitOffline
                            : gl.layerSwitcherBoxHeightPortrait) -
                    gl.display.equipixel * gl.fontSizeXL,
                child: SwitcherBox(),
              ),
            ],
          ),
        ),
        if (gl.display.orientation == Orientation.portrait)
          SizedBox(
            width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
            height: gl.display.equipixel * gl.layerswitcherButtonsBoxHeight,
            child: Column(
              children: [
                SizedBox(
                  width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                  height: gl.display.equipixel * gl.fontSizeXL,
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(
                        "Catalogues des couches",
                        textAlign: TextAlign.justify,
                        style: TextStyle(
                          color: Colors.white,
                          fontSize: gl.display.equipixel * gl.fontSizeM,
                        ),
                      ),
                    ],
                  ),
                ),
                SizedBox(
                  width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                  height:
                      gl.display.equipixel * gl.layerswitcherButtonsBoxHeight -
                      gl.display.equipixel * gl.fontSizeXL,
                  child: ViewCatalogueControl(gl.rebuildSwitcherBox),
                ),
              ],
            ),
          ),
        if (gl.display.orientation == Orientation.landscape)
          Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              if (gl.polygonLayers.isNotEmpty || gl.poiMarkerList.isNotEmpty)
                SizedBox(
                  width: gl.display.equipixel * gl.layerswitcherBoxWidth,
                  height:
                      (gl.poiMarkerList.isNotEmpty &&
                              gl.polygonLayers.isNotEmpty
                          ? gl.layerSwitcherTileHeight +
                              gl.layerswitcherControlBoxHeight
                          : (gl.poiMarkerList.isNotEmpty ||
                                  gl.polygonLayers.isNotEmpty
                              ? gl.layerswitcherControlBoxHeight
                              : 0.0)) *
                      gl.display.equipixel,
                  child: Column(
                    children: [
                      SizedBox(
                        width:
                            gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                        height: gl.display.equipixel * gl.fontSizeL,
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text(
                              "Controlez les couches visibles",
                              textAlign: TextAlign.justify,
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: gl.display.equipixel * gl.fontSizeM,
                              ),
                            ),
                          ],
                        ),
                      ),
                      UpperLayerControl(
                        switchToLocationInSearchMenu:
                            widget.switchToLocationInSearchMenu,
                      ),
                    ],
                  ),
                ),
              SizedBox(
                width: gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                height: gl.display.equipixel * gl.layerswitcherButtonsBoxHeight,
                child: Column(
                  children: [
                    SizedBox(
                      width:
                          gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                      height: gl.display.equipixel * gl.fontSizeXL,
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            "Catalogues des couches",
                            textAlign: TextAlign.justify,
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: gl.display.equipixel * gl.fontSizeM,
                            ),
                          ),
                        ],
                      ),
                    ),
                    SizedBox(
                      width:
                          gl.display.equipixel * gl.layerswitcherBoxWidth - 1,
                      height:
                          gl.display.equipixel *
                              gl.layerswitcherButtonsBoxHeight -
                          gl.display.equipixel * gl.fontSizeXL,
                      child: ViewCatalogueControl(gl.rebuildSwitcherBox),
                    ),
                  ],
                ),
              ),
            ],
          ),
      ]),
    );
  }
}

class ViewCatalogueControl extends StatefulWidget {
  final Function stateOfLayerSwitcher;
  const ViewCatalogueControl(this.stateOfLayerSwitcher, {super.key});
  @override
  State<ViewCatalogueControl> createState() => _ViewCatalogueControl();
}

class _ViewCatalogueControl extends State<ViewCatalogueControl> {
  bool _modeViewOfflineMap = false;
  bool _modeViewOnlineMap = false;

  @override
  void initState() {
    gl.rebuildSwitcherCatalogueButtons = (Function f) {
      mounted
          ? setState(() {
            f();
          })
          : () {
            f();
          };
    };
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        bool offline = false;
        for (LayerBase l in gl.dico.mLayerBases.values) {
          if (l.mOffline) {
            offline = true;
            break;
          }
        }
        return Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            if (offline)
              SizedBox(
                width: gl.display.equipixel * gl.iconSize * 1.2,
                height: gl.display.equipixel * gl.iconSize * 1.2,
                child: FloatingActionButton(
                  backgroundColor:
                      _modeViewOfflineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!_modeViewOnlineMap && !_modeViewOnlineMap) {
                      gl.mainStack.add(
                        popupOnlineMapMenu(
                          gl.notificationContext!,
                          () {
                            gl.refreshMap(() {
                              _modeViewOfflineMap = false;
                            });
                            if (mounted) {
                              setState(() {
                                _modeViewOfflineMap = false;
                              });
                            }
                          },
                          true,
                          -1,
                          "",
                          widget.stateOfLayerSwitcher,
                        ),
                      );
                    }
                    setState(() {
                      _modeViewOfflineMap = true;
                    });
                    gl.refreshMap(() {
                      _modeViewOfflineMap = true;
                    });
                  },
                  child: Icon(
                    Icons.download_for_offline,
                    size: gl.display.equipixel * gl.iconSize,
                    color: Colors.black,
                  ),
                ),
              ),
            if (!gl.offlineMode)
              SizedBox(
                width: gl.display.equipixel * gl.iconSize * 1.2,
                height: gl.display.equipixel * gl.iconSize * 1.2,
                child: FloatingActionButton(
                  backgroundColor:
                      _modeViewOnlineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!_modeViewOnlineMap && !_modeViewOnlineMap) {
                      gl.mainStack.add(
                        popupOnlineMapMenu(
                          gl.notificationContext!,
                          () {
                            gl.refreshMap(() {
                              _modeViewOnlineMap = false;
                            });
                            if (mounted) {
                              setState(() {
                                _modeViewOnlineMap = false;
                              });
                            }
                          },
                          gl.offlineMode,
                          -1,
                          "",
                          null,
                        ),
                      );
                    }
                    setState(() {
                      _modeViewOnlineMap = true;
                    });
                    gl.refreshMap(() {
                      _modeViewOnlineMap = true;
                    });
                  },
                  child: Icon(
                    Icons.layers_outlined,
                    size: gl.display.equipixel * gl.iconSize,
                    color: Colors.black,
                  ),
                ),
              ),
          ],
        );
      },
    );
  }
}

class UpperLayerControl extends StatefulWidget {
  final void Function(LatLng) switchToLocationInSearchMenu;

  const UpperLayerControl({
    super.key,
    required this.switchToLocationInSearchMenu,
  });
  @override
  State<UpperLayerControl> createState() => _UpperLayerControl();
}

class _UpperLayerControl extends State<UpperLayerControl> {
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        if (_SearchMenu.searchResults.isNotEmpty)
          SizedBox(
            height: gl.display.equipixel * gl.layerSwitcherTileHeight,
            child: Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
                side: BorderSide(color: Colors.grey, width: 1.0),
              ),
              margin: EdgeInsets.all(2),
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
                            gl.display.equipixel * 50,
                            gl.display.equipixel * 10,
                          ),
                        },
                      ),
                    ),
                    onPressed: () {
                      gl.mainStack.add(
                        popupSearchMenu(
                          gl.notificationContext!,
                          "",
                          widget.switchToLocationInSearchMenu,
                          () {
                            gl.refreshMap(() {});
                          },
                        ),
                      );
                      gl.refreshMap(() {});
                    },
                    child: Container(
                      padding: EdgeInsets.symmetric(horizontal: 1.0),
                      alignment: Alignment.centerLeft,
                      constraints: BoxConstraints(
                        maxHeight: gl.display.equipixel * 10,
                        minHeight: gl.display.equipixel * 10,
                        maxWidth: gl.display.equipixel * 50,
                        minWidth: gl.display.equipixel * 50,
                      ),
                      child: Text(
                        "Marqueurs des lieux cherchés",
                        textAlign: TextAlign.left,
                        style: TextStyle(
                          color: Colors.black,
                          fontSize: gl.display.equipixel * gl.fontSizeS,
                        ),
                      ),
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxHeight: gl.display.equipixel * 10,
                      minHeight: gl.display.equipixel * 10,
                      maxWidth: gl.display.equipixel * 17,
                      minWidth: gl.display.equipixel * 17,
                    ),
                    child: Row(
                      children: [
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.display.equipixel * 10,
                            minHeight: gl.display.equipixel * 10,
                            maxWidth: gl.display.equipixel * 15,
                            minWidth: gl.display.equipixel * 15,
                          ),
                          color: Colors.white,
                          padding: const EdgeInsets.symmetric(),
                          child: SizedBox(
                            width: gl.display.equipixel * 10,
                            height: gl.display.equipixel * 10,
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
                                size: gl.display.equipixel * 10,
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
          ),
        if (gl.polygonLayers.isNotEmpty)
          SizedBox(
            height: gl.display.equipixel * gl.layerSwitcherTileHeight,
            child: Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
                side: BorderSide(color: Colors.grey, width: 1.0),
              ),
              margin: EdgeInsets.all(2),
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
                            gl.display.equipixel * 50,
                            gl.display.equipixel * gl.layerSwitcherTileHeight,
                          ),
                        },
                      ),
                    ),
                    onPressed: () {
                      gl.mainStack.add(
                        popupPolygonListMenu(
                          gl.notificationContext!,
                          gl.polygonLayers[gl.selectedPolygonLayer].name,
                          widget.switchToLocationInSearchMenu,
                          () {
                            gl.refreshMap(() {});
                          },
                        ),
                      );
                      gl.refreshMap(() {});
                    },
                    child: Container(
                      alignment: Alignment.centerLeft,
                      padding: EdgeInsets.symmetric(horizontal: 1.0),
                      constraints: BoxConstraints(
                        maxHeight: gl.display.equipixel * 10,
                        minHeight: gl.display.equipixel * 10,
                        maxWidth: gl.display.equipixel * 50,
                        minWidth: gl.display.equipixel * 50,
                      ),
                      child: Text(
                        "Couche des polygones",
                        textAlign: TextAlign.left,
                        style: TextStyle(
                          color: Colors.black,
                          fontSize: gl.display.equipixel * gl.fontSizeS,
                        ),
                      ),
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxHeight: gl.display.equipixel * 10,
                      minHeight: gl.display.equipixel * 10,
                      maxWidth: gl.display.equipixel * 17,
                      minWidth: gl.display.equipixel * 17,
                    ),
                    child: Row(
                      children: [
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.display.equipixel * 10,
                            minHeight: gl.display.equipixel * 10,
                            maxWidth: gl.display.equipixel * 15,
                            minWidth: gl.display.equipixel * 15,
                          ),
                          color: Colors.white,
                          padding: const EdgeInsets.symmetric(),
                          child: SizedBox(
                            width: gl.display.equipixel * 10,
                            height: gl.display.equipixel * 10,
                            child: FloatingActionButton(
                              backgroundColor:
                                  gl.modeMapShowPolygons
                                      ? gl.colorAgroBioTech
                                      : Colors.grey,
                              onPressed: () {
                                setState(() {
                                  gl.modeMapShowPolygons =
                                      !gl.modeMapShowPolygons;
                                });
                                gl.refreshMap(() {});
                              },
                              child: Icon(
                                Icons.remove_red_eye,
                                size: gl.display.equipixel * 10,
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
          ),
      ],
    );
  }
}

class SwitcherBox extends StatefulWidget {
  const SwitcherBox({super.key});
  @override
  State<SwitcherBox> createState() => _SwitcherBox();
}

class _SwitcherBox extends State<SwitcherBox> {
  @override
  Widget build(BuildContext context) {
    gl.rebuildSwitcherBox = (f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
    };
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
            String tmpKey = gl.selectedLayerForMap[newIndex].mCode;
            bool tmpOffline = gl.selectedLayerForMap[newIndex].offline;
            gl.replaceLayerFromList(
              gl.selectedLayerForMap[oldIndex].mCode,
              index: newIndex,
              offline: gl.selectedLayerForMap[oldIndex].offline,
            );
            gl.replaceLayerFromList(
              tmpKey,
              index: oldIndex,
              offline: tmpOffline,
            );
            gl.refreshMap(() {});
          });
        },

        children: List<Widget>.generate(3, (i) {
          if ((!gl.offlineMode &&
                  !"123".contains(gl.selectedLayerForMap[i].mCode)) ||
              (i == 0 && !"123".contains(gl.selectedLayerForMap[i].mCode))) {
            return Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
                side: BorderSide(
                  color: Color.fromRGBO(205, 225, 138, 1.0),
                  width: 2.0,
                ),
              ),
              margin: EdgeInsets.all(3),
              key: Key('$i+listOfThree'),
              color: Colors.white,
              shadowColor: const Color.fromARGB(255, 44, 44, 120),
              child: ReorderableDragStartListener(
                index: i,
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: <Widget>[
                        TextButton(
                          style: ButtonStyle(
                            fixedSize: WidgetStateProperty<Size>.fromMap(
                              <WidgetStatesConstraint, Size>{
                                WidgetState.any: Size(
                                  gl.display.equipixel *
                                      gl.layerswitcherBoxWidth *
                                      .68,
                                  gl.display.equipixel *
                                      gl.layerSwitcherTileHeight,
                                ),
                              },
                            ),
                          ),
                          onPressed: () {
                            gl.mainStack.add(
                              popupOnlineMapMenu(
                                gl.notificationContext!,
                                () {
                                  gl.refreshMap(() {});
                                },
                                gl.offlineMode,
                                i,
                                gl.selectedLayerForMap[i].mCode,
                                null,
                              ),
                            );
                            gl.refreshMap(() {});
                          },
                          child: Container(
                            alignment: Alignment.centerLeft,
                            padding: EdgeInsets.symmetric(horizontal: 1.0),
                            child: Text(
                              gl.dico
                                  .getLayerBase(gl.selectedLayerForMap[i].mCode)
                                  .mNom,
                              textAlign: TextAlign.left,
                              style: TextStyle(
                                color: Colors.black,
                                fontSize: gl.display.equipixel * gl.fontSizeS,
                              ),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.display.equipixel * 10,
                            minHeight: gl.display.equipixel * 10,
                            maxWidth: gl.display.equipixel * 22,
                            minWidth: gl.display.equipixel * 22,
                          ),
                          child: Row(
                            children: [
                              Container(
                                color: Colors.white,
                                constraints: BoxConstraints(
                                  maxHeight: gl.display.equipixel * 12,
                                  minHeight: gl.display.equipixel * 12,
                                  maxWidth: gl.display.equipixel * 10,
                                  minWidth: gl.display.equipixel * 10,
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
                                      maxHeight: gl.display.equipixel * 10,
                                      minHeight: gl.display.equipixel * 10,
                                      maxWidth: gl.display.equipixel * 10,
                                      minWidth: gl.display.equipixel * 10,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(
                                      Icons.save,
                                      size: gl.display.equipixel * 10,
                                    ),
                                  )
                                  : Container(
                                    constraints: BoxConstraints(
                                      maxHeight: gl.display.equipixel * 10,
                                      minHeight: gl.display.equipixel * 10,
                                      maxWidth: gl.display.equipixel * 10,
                                      minWidth: gl.display.equipixel * 10,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(
                                      Icons.wifi,
                                      size: gl.display.equipixel * 10,
                                    ),
                                  ),
                            ],
                          ),
                        ),
                      ],
                    ),
                    if (i == 0 &&
                        !gl.offlineMode) //Pour la transparance de la première tile
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Container(
                            constraints: BoxConstraints(
                              maxHeight: gl.display.equipixel * 10,
                              maxWidth: gl.display.equipixel * 40,
                            ),
                            child: TextButton(
                              style: ButtonStyle(
                                shape: WidgetStateProperty.fromMap(<
                                  WidgetStatesConstraint,
                                  OutlinedBorder
                                >{
                                  WidgetState.any: RoundedRectangleBorder(
                                    borderRadius: BorderRadiusGeometry.circular(
                                      12.0,
                                    ),
                                    side: BorderSide(
                                      color: Color.fromRGBO(205, 225, 138, 1.0),
                                      width: 2.0,
                                    ),
                                  ),
                                }),
                                fixedSize: WidgetStateProperty<Size>.fromMap(
                                  <WidgetStatesConstraint, Size>{
                                    WidgetState.any: Size(
                                      gl.display.equipixel *
                                          gl.layerswitcherBoxWidth *
                                          .5,
                                      gl.display.equipixel *
                                          gl.layerSwitcherTileHeight,
                                    ),
                                  },
                                ),
                                backgroundColor:
                                    gl.modeMapFirstTileLayerTransparancy
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
                                  gl.modeMapFirstTileLayerTransparancy =
                                      !gl.modeMapFirstTileLayerTransparancy;
                                });
                                gl.refreshMap(() {});
                              },
                              child: Text(
                                gl.modeMapFirstTileLayerTransparancy
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
                                  gl.display.equipixel * 45,
                                  gl.display.equipixel * 20,
                                ),
                              },
                            ),
                          ),
                          onPressed: () {
                            gl.mainStack.add(
                              popupOnlineMapMenu(
                                gl.notificationContext!,
                                () {
                                  gl.refreshMap(() {});
                                },
                                gl.offlineMode,
                                i,
                                gl.selectedLayerForMap[i].mCode,
                                null,
                              ),
                            );
                            gl.refreshMap(() {});
                          },
                          child: Container(
                            padding: EdgeInsets.symmetric(horizontal: 1.0),
                            constraints: BoxConstraints(
                              maxHeight: gl.display.equipixel * 10,
                              maxWidth: gl.display.equipixel * 45,
                              minWidth: gl.display.equipixel * 45,
                            ),
                            child: Text(
                              "Appuyez ici pour ajouter une couche du catalogue",
                              style: TextStyle(
                                color: Colors.black,
                                fontSize: gl.display.equipixel * gl.fontSizeS,
                              ),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.display.equipixel * 10,
                            minHeight: gl.display.equipixel * 10,
                            maxWidth: gl.display.equipixel * 22,
                            minWidth: gl.display.equipixel * 22,
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
          backgroundColor: gl.backgroundTransparentBlackBox,
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
              width: gl.dyrDialogWidth * gl.display.equipixel,
              child: Text(
                message,
                textAlign: TextAlign.center,
                style: TextStyle(
                  color: Colors.white,
                  fontSize: gl.fontSizeM * gl.display.equipixel,
                ),
              ),
            ),
          ),
          titleTextStyle: TextStyle(
            color: Colors.white,
            fontSize: gl.fontSizeM * gl.display.equipixel,
          ),
          actionsAlignment: MainAxisAlignment.spaceAround,
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
                  maxHeight: gl.dyrButtonsize * 0.6 * gl.display.equipixel,
                  minHeight: gl.dyrButtonsize * 0.6 * gl.display.equipixel,
                  maxWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                  minWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                ),
                child: Text(
                  "Oui",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.fontSizeM * gl.display.equipixel,
                  ),
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
                  maxHeight: gl.dyrButtonsize * .6 * gl.display.equipixel,
                  minHeight: gl.dyrButtonsize * 0.6 * gl.display.equipixel,
                  maxWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                  minWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                ),
                child: Text(
                  "Non",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.fontSizeM * gl.display.equipixel,
                  ),
                ),
              ),
            ),
          ],
        );
      },
    );
  }
}
