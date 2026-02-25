import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/myicons.dart';
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/pages/catalogueView/legend_view.dart';
import 'package:fforestimator/pages/pdf_screen.dart';
import 'package:fforestimator/tools/geometry_layer.dart';
import 'package:fforestimator/tools/layout_tools.dart' as lt;
import 'package:fforestimator/tools/geometry/geometry.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:fforestimator/tools/handle_permissions.dart' as permissions;
import 'package:fforestimator/tools/layer_downloader.dart';
import 'package:fforestimator/tools/pretty_print_nominatim_results.dart';
import 'package:fforestimator/tools/pretty_print_polygon_results.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:pdf/pdf.dart';
import 'package:pdf/widgets.dart' as pw;
import 'package:flutter_colorpicker/flutter_colorpicker.dart';
import 'package:http/http.dart' as http;
import 'package:latlong2/latlong.dart';
import 'package:path_provider/path_provider.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;
import 'package:url_launcher/url_launcher.dart';
import 'package:intl/intl.dart';

// A helper typedef: accepts a function to run inside a setter (used by global rebuild callbacks)
typedef VoidSetter = void Function(void Function());

//https://github.com/fleaflet/flutter_map/blob/master/example/lib/pages/custom_crs/custom_crs.dart
proj4.Projection epsg4326 = proj4.Projection.get('EPSG:4326')!;
// si epsg31370 est dans la db proj 4, on prend, sinon on définit
proj4.Projection epsg31370 =
    proj4.Projection.get('EPSG:31370') ??
    proj4.Projection.add(
      'EPSG:31370',
      '+proj=lcc +lat_1=51.16666723333333 +lat_2=49.8333339 +lat_0=90 +lon_0=4.367486666666666 +x_0=150000.013 +y_0=5400088.438 +ellps=intl +towgs84=-106.8686,52.2978,-103.7239,0.3366,-0.457,1.8422,-1.2747 +units=m +no_defs +type=crs',
    );
// map extend in BL72.
final epsg31370Bounds = Rect.fromPoints(
  Offset(42250.0, 21170.0), // lower left
  Offset(295170.0, 167700.0), // upper right
);

double tileSize = 256.0;

List<double> getResolutions2(int nbZoom) {
  // résolution numéro 1: une tile pour tout l'extend de la Wallonie
  int maxResolution = 1280;
  return List.generate(nbZoom, (z) => maxResolution / pow(2, z));
}

Proj4Crs epsg31370CRS = Proj4Crs.fromFactory(
  code: 'EPSG:31370',
  proj4Projection: epsg31370,
  bounds: epsg31370Bounds,
  resolutions: getResolutions2(15),
);

ButtonStyle dialogButtonStyle({
  double width = 0,
  double height = 0,
  Color color = gl.colorAgroBioTech,
  double borderWidth = 0,
}) {
  return ButtonStyle(
    backgroundColor: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Color>{
      WidgetState.any: color.withAlpha(200),
    }),
    shape: WidgetStateProperty<OutlinedBorder>.fromMap(<WidgetStatesConstraint, OutlinedBorder>{
      WidgetState.any: RoundedRectangleBorder(
        borderRadius: BorderRadiusGeometry.circular(12.0),
        side: BorderSide(color: color, width: borderWidth),
      ),
    }),
    fixedSize: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Size>{
      WidgetState.any: Size(
        width == 0 ? gl.eqPx * gl.popupReturnButtonWidth * .7 : width,
        height == 0 ? gl.eqPx * gl.popupReturnButtonHeight : height,
      ),
    }),
    padding: WidgetStateProperty.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
      WidgetState.any: EdgeInsetsGeometry.zero,
    }),
  );
}

TextStyle dialogTextButtonStyle() {
  return TextStyle(color: Colors.black, fontWeight: FontWeight.w600, fontSize: gl.eqPx * gl.fontSizeM);
}

Widget popupNoInternet(VoidCallback after) {
  return Card(
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadiusGeometry.circular(12.0),
      side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
    ),
    color: gl.backgroundTransparentBlackBox,
    child: Container(
      alignment: AlignmentGeometry.center,
      height: gl.eqPx * 60,
      width: gl.eqPx * 70,
      child: Column(
        children: [
          Row(
            children: [
              Container(
                alignment: AlignmentGeometry.center,
                height: gl.eqPx * 15,
                width: gl.eqPx * 15,
                child: forestimatorIcon(width: gl.eqPx * 15, height: gl.eqPx * 15),
              ),
              Container(
                alignment: AlignmentGeometry.center,
                height: gl.eqPx * 10,
                width: gl.eqPx * 40,
                child: Text(
                  "Attention",
                  style: TextStyle(color: Colors.white, fontWeight: FontWeight.w400, fontSize: gl.eqPx * gl.fontSizeM),
                ),
              ),
              SizedBox(
                height: gl.eqPx * 15,
                width: gl.eqPx * 15,
                child: lt.forestimatorButton(after, Icons.arrow_drop_up_outlined),
              ),
            ],
          ),
          lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                alignment: AlignmentGeometry.center,
                height: gl.eqPx * 20,
                width: gl.eqPx * 60,
                child: Text(
                  "Vous n'avez pas accès à internet!",
                  style: TextStyle(color: Colors.white, fontWeight: FontWeight.w400, fontSize: gl.eqPx * gl.fontSizeM),
                ),
              ),
            ],
          ),
        ],
      ),
    ),
  );
}

Widget forestimatorIcon({double width = 0, double height = 0}) {
  return Container(
    padding: EdgeInsets.only(right: gl.eqPx * 2),
    width: width == 0 ? gl.eqPx * gl.iconSizeXS : width,
    height: height == 0 ? gl.eqPx * gl.iconSizeXS : height,
    child: Image.asset("assets/images/LogoForestimatorWhiteAlpha.png"),
  );
}

class PopupPermissions extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final VoidCallback? onAccept;
  final VoidCallback? onDecline;
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
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadiusGeometry.circular(12.0),
        side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
      ),
      backgroundColor: Colors.white,
      title: Row(
        children: [
          forestimatorIcon(),
          Text(
            title!,
            style: TextStyle(color: Colors.black, fontWeight: FontWeight.w400, fontSize: gl.eqPx * gl.fontSizeM),
          ),
        ],
      ),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          style: dialogButtonStyle(height: gl.eqPx * 12, width: gl.eqPx * 20),
          onPressed: () => {if (onAccept != null) onAccept!()},
          child: Text(accept!, style: dialogTextButtonStyle()),
        ),
        if (decline != null)
          TextButton(
            style: dialogButtonStyle(height: gl.eqPx * 12, width: gl.eqPx * 20),
            onPressed: () => {if (onDecline != null) onDecline!()},
            child: Text(decline!, style: dialogTextButtonStyle()),
          ),
      ],
    );
  }
}

class PopupDownloadSuccess {
  PopupDownloadSuccess(BuildContext context, String layerName) {
    gl.refreshStack(() {
      popupForestimatorMessage(title: "Couche téléchargée", message: "La couche $layerName à été téléchargée.");
    });
  }
}

class PopupDownloadFailed {
  PopupDownloadFailed(BuildContext context, String layerName) {
    gl.refreshStack(() {
      popupForestimatorMessage(title: "Erreur", message: "La couche $layerName n'a pas été téléchargée.");
    });
  }
}

class PopupPolygonNotWellDefined {
  PopupPolygonNotWellDefined() {
    gl.refreshStack(() {
      popupForestimatorMessage(
        title: "Attention",
        message: "Avec ce point, le polygone n'est pas bien défini, c'est-à-dire on ne peut pas croiser des segments.",
      );
    });
  }
}

class PopupSearchMenu {
  PopupSearchMenu(Function(LatLng) moveToPointOnMap, VoidCallback onDiscard) {
    gl.refreshStack(() {
      popupForestimatorWindow(
        id: "SearchMenu",
        title: "Recherche d'un lieu",
        child: SearchMenu(moveToPoint: moveToPointOnMap),
        onDiscard: onDiscard,
      );
    });
  }
}

class PopupSettingsMenu {
  PopupSettingsMenu() {
    gl.refreshStack(() {
      popupForestimatorWindow(
        id: "SettingsMenu",
        title: "Paramètres",
        child: SettingsMenu(),
        onDiscard: () {
          gl.refreshStack(() {
            gl.stack.pop("SettingsMenu");
            gl.modeSettings = false;
          });
        },
      );
    });
  }
}

class PopupOnlineMapMenu {
  final VoidCallback onDiscard;
  final bool offlineMode;
  final int selectionMode;
  final String mapcode;
  final void Function(void Function()) layerSwitcherState;
  PopupOnlineMapMenu(this.onDiscard, this.offlineMode, this.selectionMode, this.mapcode, this.layerSwitcherState) {
    gl.refreshStack(() {
      popupForestimatorWindow(
        id: "Catalogue",
        title: "Catalogue des couches",
        onDiscard: onDiscard,
        child: OnlineMapMenu(
          after: onDiscard,
          offlineMode: offlineMode,
          selectionMode: selectionMode,
          selectedMapCode: mapcode,
          stateOfLayerSwitcher: layerSwitcherState,
        ),
      );
    });
  }
}

Widget popupPDFSaved(String pdfName, VoidCallback after) {
  return Card(
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadiusGeometry.circular(12.0),
      side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
    ),
    color: gl.backgroundTransparentBlackBox,
    child: Container(
      alignment: AlignmentGeometry.center,
      height: gl.eqPx * 60,
      width: gl.eqPx * 70,
      child: Column(
        children: [
          Row(
            children: [
              Container(
                alignment: AlignmentGeometry.center,
                height: gl.eqPx * 15,
                width: gl.eqPx * 15,
                child: forestimatorIcon(width: gl.eqPx * 15, height: gl.eqPx * 15),
              ),
              Container(
                alignment: AlignmentGeometry.center,
                height: gl.eqPx * 10,
                width: gl.eqPx * 40,
                child: Text(
                  "Export du pdf",
                  style: TextStyle(color: Colors.white, fontWeight: FontWeight.w400, fontSize: gl.eqPx * gl.fontSizeM),
                ),
              ),
              SizedBox(
                height: gl.eqPx * 15,
                width: gl.eqPx * 15,
                child: lt.forestimatorButton(after, Icons.arrow_drop_up_outlined),
              ),
            ],
          ),
          lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
          Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Container(
                    alignment: AlignmentGeometry.center,
                    height: gl.eqPx * 40,
                    width: gl.eqPx * 60,
                    child: Text(
                      "$pdfName a été enregistré!",
                      style: TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.w400,
                        fontSize: gl.eqPx * gl.fontSizeM,
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

int messageCount = 0;

void popupForestimatorMessage({
  String? id,
  String? title,
  Icon? leadingSymbol,
  String? message,
  Widget? child,
  Widget? pdfChild,
  VoidCallback? onDiscard,
  VoidCallback? onAccept,
  String? messageAccept,
  VoidCallback? onDecline,
  String? messageDecline,
  double? height,
  double? width,
  Duration? duration,
}) {
  int count = ++messageCount;
  gl.stack.add(
    id ?? "popMsg$count",
    Card(
      margin: EdgeInsetsGeometry.zero,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadiusGeometry.circular(gl.eqPx * 5),
        side: BorderSide(color: gl.colorAgroBioTech, width: gl.eqPx),
      ),
      color: gl.backgroundTransparentBlackBox,
      child: Container(
        alignment: AlignmentGeometry.center,
        height: height ?? gl.eqPx * 65,
        width: width ?? gl.eqPx * 70,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Column(
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Container(
                      alignment: AlignmentGeometry.center,
                      height: gl.eqPx * 15,
                      width: gl.eqPx * 15,
                      child: leadingSymbol ?? forestimatorIcon(width: gl.eqPx * 12, height: gl.eqPx * 12),
                    ),
                    Container(
                      alignment: AlignmentGeometry.center,
                      height: gl.eqPx * 15,
                      width: gl.eqPx * 40,
                      child: Text(
                        title ?? "Message $count",
                        style: TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.w700,
                          fontSize: gl.eqPx * gl.fontSizeS,
                        ),
                      ),
                    ),
                    SizedBox(
                      height: gl.eqPx * 15,
                      width: gl.eqPx * 15,
                      child: lt.forestimatorButton(
                        onDiscard ??
                            () {
                              gl.stack.pop(id ?? "popMsg$count");
                            },
                        Icons.arrow_drop_up_outlined,
                      ),
                    ),
                  ],
                ),
                lt.stroke(0, gl.eqPx * 1, gl.colorAgroBioTech),
              ],
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    pdfChild ??
                        lt.ForestimatorScrollView(
                          height: (height ?? gl.eqPx * 65) - gl.eqPx * 30,
                          width: (width ?? gl.eqPx * 70) - gl.eqPx * 5,
                          child:
                              child ??
                              Text(
                                message ?? "",
                                style: TextStyle(
                                  color: Colors.white,
                                  fontWeight: FontWeight.w400,
                                  fontSize: gl.eqPx * gl.fontSizeS,
                                ),
                              ),
                        ),
                  ],
                ),
              ],
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    if (onAccept != null && messageAccept != null)
                      Container(
                        alignment: AlignmentGeometry.center,
                        height: gl.eqPx * 10,
                        width: gl.eqPx * 25,
                        child: TextButton(
                          style: dialogButtonStyle(borderWidth: gl.eqPx),
                          onPressed: onAccept,
                          child: Text(
                            messageAccept,
                            style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                          ),
                        ),
                      ),
                    if (onDecline != null && messageDecline != null)
                      Container(
                        alignment: AlignmentGeometry.center,
                        height: gl.eqPx * 10,
                        width: gl.eqPx * 25,
                        child: TextButton(
                          style: dialogButtonStyle(borderWidth: gl.eqPx),
                          onPressed: onDecline,
                          child: Text(
                            messageDecline,
                            style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                          ),
                        ),
                      ),
                  ],
                ),
                Container(height: gl.eqPx * 2),
              ],
            ),
          ],
        ),
      ),
    ),
    duration ?? Duration(milliseconds: 200),
    gl.Anim.onScreenPosCenter,
    gl.Anim.offScreenPosMessages,
  );
}

void popupForestimatorWindow({
  String? id,
  String? title,
  Icon? leadingSymbol,
  String? message,
  Widget? child,
  Widget? pdfChild,
  VoidCallback? onDiscard,
  double? height,
  double? width,
  Duration? duration,
  bool? bigVersion,
}) {
  int count = ++messageCount;
  gl.stack.add(
    id ?? "popWin$count",
    PopupForestimatorWindow(
      count,
      id,
      title,
      leadingSymbol,
      message,
      child,
      pdfChild,
      onDiscard,
      height,
      width,
      duration,
      bigVersion ?? false,
    ),
    duration ?? Duration(milliseconds: 400),
    gl.Anim.onScreenPosCenter,
    gl.Anim.offScreenPosWindows,
  );
}

class PopupForestimatorWindow extends StatelessWidget {
  final String? id;
  final String? title;
  final Icon? leadingSymbol;
  final String? message;
  final Widget? child;
  final Widget? pdfChild;
  final VoidCallback? onDiscard;
  final double? height;
  final double? width;
  final Duration? duration;
  final int count;
  final bool big;

  const PopupForestimatorWindow(
    this.count,
    this.id,
    this.title,
    this.leadingSymbol,
    this.message,
    this.child,
    this.pdfChild,
    this.onDiscard,
    this.height,
    this.width,
    this.duration,
    this.big, {
    super.key,
  });

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (c, o) {
        double cWidth = width ?? (gl.eqPxW - 2) * gl.eqPx;
        double cHeight = height ?? (gl.eqPxH - 10) * gl.eqPx;
        return Card(
          margin: EdgeInsetsGeometry.zero,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(gl.eqPx * 5),
            side: BorderSide(color: gl.colorAgroBioTech, width: gl.eqPx * .5),
          ),
          color: gl.backgroundTransparentBlackBox,
          child: Container(
            alignment: AlignmentGeometry.center,
            height: cHeight,
            width: cWidth,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                if (!big)
                  Column(
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Container(
                            alignment: AlignmentGeometry.center,
                            height: gl.eqPx * 15,
                            width: gl.eqPx * 15,
                            child: leadingSymbol ?? forestimatorIcon(width: gl.eqPx * 12, height: gl.eqPx * 12),
                          ),
                          Container(
                            alignment: AlignmentGeometry.center,
                            height: gl.eqPx * 15,
                            width: gl.eqPx * 65,
                            child: Text(
                              title ?? "Message $count",
                              style: TextStyle(
                                color: Colors.white,
                                fontWeight: FontWeight.w700,
                                fontSize: gl.eqPx * gl.fontSizeM,
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.eqPx * 15,
                            width: gl.eqPx * 15,
                            child: lt.forestimatorButton(
                              onDiscard ??
                                  () {
                                    gl.stack.pop(id ?? "popWin$count");
                                  },
                              Icons.arrow_drop_up_outlined,
                            ),
                          ),
                        ],
                      ),
                      lt.stroke(0, gl.eqPx * .5, gl.colorAgroBioTech),
                    ],
                  ),
                Stack(
                  alignment: AlignmentGeometry.xy(0, 0),
                  children: [
                    pdfChild ??
                        lt.ForestimatorScrollView(
                          height: cHeight - gl.eqPx * 17,
                          width: cWidth - gl.eqPx * 5,
                          child:
                              child ??
                              Text(
                                message ?? "",
                                style: TextStyle(
                                  color: Colors.white,
                                  fontWeight: FontWeight.w400,
                                  fontSize: gl.eqPx * gl.fontSizeM,
                                ),
                              ),
                        ),
                    if (big)
                      SizedBox(
                        height: cHeight,
                        width: cWidth,
                        child: Container(
                          alignment: Alignment.topRight,
                          height: gl.eqPx * 15,
                          width: gl.eqPx * 15,
                          child: lt.forestimatorButton(
                            onDiscard ??
                                () {
                                  gl.stack.pop(id ?? "popWin$count");
                                },
                            Icons.arrow_drop_up_outlined,
                          ),
                        ),
                      ),
                  ],
                ),
              ],
            ),
          ),
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
    ValueChanged<Color> colorChange,
    VoidCallback onDecline,
    VoidCallback onAccept,
  ) {
    pickerColor = currentColor;
    gl.refreshStack(() {
      popupForestimatorMessage(
        height: gl.eqPx * 100,
        id: "colorPicker",
        title: "Choisissez une couleur",
        child: ColorPicker(pickerColor: pickerColor, onColorChanged: colorChange, colorPickerWidth: gl.eqPx * 50),
        messageAccept: "Changer",
        onAccept: () {
          currentColor = pickerColor;
          onAccept();
          gl.stack.pop("colorPicker");
        },
        messageDecline: "Annuler",
        onDecline: () {
          onDecline();
          gl.stack.pop("colorPicker");
        },
        onDiscard: () {
          onDecline();
          gl.stack.pop("colorPicker");
        },
      );
    });
  }
}

class PopupNewGeometricLayer {
  PopupNewGeometricLayer(BuildContext context, void Function(String, String, Color) onAccept) {
    String type = "Point";
    String name = "Nouveau";
    Random rand = Random();
    Color color = Color.fromRGBO(rand.nextInt(256), rand.nextInt(256), rand.nextInt(256), 1.0);
    gl.refreshStack(() {
      popupForestimatorMessage(
        height: 100 * gl.eqPx,
        id: "NewEntity",
        title: "Nouvelle Entité",
        onAccept: () {
          onAccept(type, name, color);
          gl.stack.pop("NewEntity");
        },
        messageAccept: "Créer",
        onDecline: () {
          gl.stack.pop("NewEntity");
        },
        messageDecline: "Annuler",
        child: Column(
          children: [
            Text("Choisissez le type", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SelectPolyType(
              state: (it) {
                type = it;
              },
            ),
            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
            Text("Introduisez un nom", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child: TextFormField(
                maxLength: 22,
                maxLengthEnforcement: MaxLengthEnforcement.enforced,
                onChanged: (String it) {
                  name = it;
                },
                controller: TextEditingController(text: ""),
                style: TextStyle(color: Colors.white),
              ),
            ),
            Text("Choisissez une couleur", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SelectPolyColor(
              onAccept: (it) {
                color = it;
              },
              onDecline: (it) {
                color = it;
              },
              colorChanged: (it) {
                color = it;
              },
              currentColor: color,
            ),
          ],
        ),
      );
    });
  }
}

class SelectLayerSymbol extends StatefulWidget {
  final String type;
  final ValueChanged<int> iconChanged;
  final int current;

  const SelectLayerSymbol({super.key, required this.iconChanged, required this.current, required this.type});

  @override
  State<StatefulWidget> createState() => _SelectLayerSymbol();
}

class _SelectLayerSymbol extends State<SelectLayerSymbol> {
  int? _current;
  bool _mode = false;

  @override
  Widget build(BuildContext context) {
    _current ?? (_current = widget.current);
    return !_mode
        ? IconButton(
          style: lt.trNoPadButtonstyle,
          onPressed: () {
            setState(() {
              _mode = !_mode;
            });
          },
          icon: Icon(
            color: Colors.black,
            size: gl.eqPx * gl.iconSizeM,
            widget.type == "Polygon" ? gl.selectableIconGeo[_current!] : gl.selectableIcons[_current!],
          ),
        )
        : lt.ForestimatorScrollView(
          arrowColor: Colors.red,
          horizontal: true,
          width: gl.eqPx * 75,
          height: gl.eqPx * gl.iconSizeM,
          child: Row(
            children: List<Widget>.generate(
              widget.type == "Polygon" ? gl.selectableIconGeo.length : gl.selectableIcons.length,
              (int k) {
                return Container(
                  alignment: Alignment.center,
                  padding: EdgeInsets.symmetric(horizontal: gl.eqPx),
                  color: gl.selLay.defaultPointIcon == k ? gl.colorAgroBioTech : Colors.transparent,
                  height: gl.eqPx * gl.iconSizeM,
                  child: IconButton(
                    style: lt.trNoPadButtonstyle,
                    onPressed: () {
                      setState(() {
                        _mode = !_mode;
                        widget.iconChanged(k);
                        _current = k;
                      });
                    },
                    icon: Icon(
                      widget.type == "Polygon" ? gl.selectableIconGeo[k] : gl.selectableIcons[k],
                      size: gl.iconSizeM * gl.eqPx,
                      color: Colors.black,
                    ),
                    color: Colors.black,
                    iconSize: gl.eqPx * gl.iconSizeM,
                  ),
                );
              },
            ),
          ),
        );
  }
}

class SelectPolyColor extends StatefulWidget {
  final ValueChanged<Color> colorChanged;
  final ValueChanged<Color> onAccept;
  final ValueChanged<Color> onDecline;
  final Color currentColor;

  const SelectPolyColor({
    super.key,
    required this.colorChanged,
    required this.currentColor,
    required this.onAccept,
    required this.onDecline,
  });

  @override
  State<StatefulWidget> createState() => _SelectPolyColor();
}

class _SelectPolyColor extends State<SelectPolyColor> {
  Color currentColor = Colors.black;
  @override
  Widget build(BuildContext context) {
    if (currentColor == Colors.black) currentColor = widget.currentColor;
    return TextButton(
      style: lt.trNoPadButtonstyle,
      onPressed: () {
        PopupColorChooser(
          currentColor,
          gl.notificationContext!,
          (Color color) {
            setState(() {
              widget.colorChanged(color);
              currentColor = color;
            });
          },
          () {
            setState(() {
              widget.onDecline(widget.currentColor);
            });
          },
          () {
            setState(() {
              widget.onAccept(currentColor);
            });
          },
        );
      },
      child: CircleAvatar(backgroundColor: currentColor),
    );
  }
}

class SelectPolyType extends StatefulWidget {
  final ValueChanged<String> state;

  const SelectPolyType({super.key, required this.state});

  @override
  State<StatefulWidget> createState() => _SelectPolyType();
}

class _SelectPolyType extends State<SelectPolyType> {
  final Color active = Colors.black;
  int _selectedType = 0;
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        Container(
          width: gl.eqPx * gl.iconSizeM * 1.65,
          height: gl.eqPx * gl.iconSizeM * 1.35,
          color: _selectedType == 0 ? gl.colorAgroBioTech : Colors.transparent,
          child: TextButton(
            onPressed: () {
              setState(() {
                _selectedType = 0;
              });
              widget.state("Point");
            },
            child: Column(
              children: [
                FaIcon(FontAwesomeIcons.circleDot, color: Colors.white),
                Text("Point", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
              ],
            ),
          ),
        ),
        Container(
          width: gl.eqPx * gl.iconSizeM * 1.65,
          height: gl.eqPx * gl.iconSizeM * 1.35,
          color: _selectedType == 1 ? gl.colorAgroBioTech : Colors.transparent,
          child: TextButton(
            onPressed: () {
              setState(() {
                _selectedType = 1;
              });
              widget.state("Polygon");
            },
            child: Column(
              children: [
                FaIcon(FontAwesomeIcons.pentagon, color: Colors.white),
                Text("Polygon", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class PopupUserData {
  PopupUserData(
    BuildContext context,
    VoidCallback callbackOnStartTyping,
    VoidCallback afterCompleting, {
    String oldName = "",
    String oldForename = "",
    String oldMail = "",
  }) {
    gl.refreshStack(() {
      popupForestimatorMessage(
        id: "usrData",
        title: "Renseignements personnels",
        messageAccept: "Enregister",
        messageDecline: "Annuler",
        height: gl.eqPx * 100,
        onAccept: () {
          gl.refreshStack(() {
            if (gl.UserData.validUserData()) {
              gl.Mode.userDataFilled = true;
              gl.Mode.serialize();
              gl.stack.pop("usrData");
              afterCompleting();
            } else {
              gl.Mode.userDataFilled = false;
              gl.Mode.serialize();
            }
          });
        },
        onDecline: () {
          gl.refreshStack(() {
            gl.UserData.forename = oldForename;
            gl.UserData.name = oldName;
            gl.UserData.mail = oldMail;
            gl.stack.pop("usrData");
          });
        },
        onDiscard: () {
          gl.refreshStack(() {
            gl.UserData.forename = oldForename;
            gl.UserData.name = oldName;
            gl.UserData.mail = oldMail;
            gl.stack.pop("usrData");
          });
        },
        child: Column(
          children: [
            Text("Votre nom", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child: TextFormField(
                maxLength: 255,
                maxLengthEnforcement: MaxLengthEnforcement.enforced,
                initialValue: oldName,
                onChanged: (String str) {
                  gl.UserData.name = str;
                },
                onTap: () => callbackOnStartTyping(),
                onTapOutside: (pointer) {},
                style: TextStyle(color: Colors.white),
              ),
            ),
            Text("Votre prénom", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child: TextFormField(
                maxLength: 255,
                initialValue: oldForename,
                maxLengthEnforcement: MaxLengthEnforcement.enforced,
                onChanged: (String str) {
                  gl.UserData.forename = str;
                },
                onTap: () => callbackOnStartTyping(),
                onTapOutside: (pointer) {},
                style: TextStyle(color: Colors.white),
              ),
            ),
            Text("Mail de contact", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            lt.ValidTextField(
              width: gl.menuBarLength * gl.eqPx,
              height: gl.menuBarThickness * gl.eqPx,
              initialValue: oldMail,
              validate: gl.UserData.validMail,
              onValid: (String str) {
                gl.UserData.mail = str;
                gl.UserData.serialize();
              },
              onInvalid: (String str) {},
              onTap: () => callbackOnStartTyping(),
              onEditingComplete: () {},
            ),
          ],
        ),
      );
    });
  }
}

class PopupNewAttribute {
  PopupNewAttribute(
    BuildContext context,
    String currentName,
    Color currentColor,
    ValueChanged<String> typeChanged,
    ValueChanged<String> attributeNameChanged,
    ValueChanged<dynamic> valueChanged,
    VoidCallback onDecline,
    VoidCallback onAccept,
    VoidCallback callbackOnStartTyping, {
    bool noValue = false,
  }) {
    TextEditingController textEditor = TextEditingController();
    TextEditingController textEditorVal = TextEditingController();
    String type = "string";
    gl.refreshStack(() {
      popupForestimatorMessage(
        height: gl.eqPx * 80,
        id: "AddAttribute",
        title: "Nouveau Attribut",
        messageAccept: "Ajouter",
        messageDecline: "Annuler",
        onAccept: () {
          String nom = textEditor.text;
          if (controlDuplicateAttributeName(textEditor.text, gl.selLay.defaultAttributes)) {
            gl.refreshStack(() {
              popupForestimatorMessage(id: "MSGduplicateName", title: "Erreur", message: "Le nom $nom existe déja!");
            });
            return;
          }
          onAccept();
          gl.refreshStack(() {
            gl.stack.pop("AddAttribute");
          });
        },
        onDecline: () {
          gl.refreshStack(() {
            onDecline();
            gl.stack.pop("AddAttribute");
          });
        },
        onDiscard: () {
          gl.refreshStack(() {
            onDecline();
            gl.stack.pop("AddAttribute");
          });
        },
        child: Column(
          children: [
            Text("Type de la variable", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SelectAttributeType(
              typeChanged: (String newType) {
                type = newType;
                typeChanged(type);
                textEditorVal.text = "";
              },
            ),
            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
            Text("Introduisez un nom", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child: TextFormField(
                maxLength: 255,
                maxLengthEnforcement: MaxLengthEnforcement.enforced,
                onChanged: (String str) {
                  textEditor.text = cleanAttributeName(str.toString());
                  attributeNameChanged(textEditor.text);
                },
                onTap: () => callbackOnStartTyping(),
                onTapOutside: (pointer) {},
                style: TextStyle(color: Colors.white),
                controller: textEditor,
              ),
            ),
            if (!noValue)
              Text("Introduisez une valeur", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM)),
            if (!noValue)
              SizedBox(
                width: gl.menuBarLength * gl.eqPx,
                child: TextFormField(
                  maxLength: 255,
                  maxLengthEnforcement: MaxLengthEnforcement.enforced,
                  onChanged: (String str) {
                    switch (type) {
                      case "string":
                        valueChanged(str);
                      case "int":
                        try {
                          valueChanged(int.parse(str));
                        } catch (e) {
                          if (textEditorVal.text.isNotEmpty) {
                            textEditorVal.text = textEditorVal.text.substring(0, textEditorVal.text.length - 1);
                          }
                          if (textEditorVal.text.isEmpty) {
                            valueChanged(0);
                          } else {
                            valueChanged(int.parse(textEditorVal.text));
                          }
                        }
                      case "double":
                        try {
                          valueChanged(double.parse(str));
                        } catch (e) {
                          if (textEditorVal.text.isNotEmpty) {
                            textEditorVal.text = textEditorVal.text.substring(0, textEditorVal.text.length - 1);
                          }
                          if (textEditorVal.text.isEmpty) {
                            valueChanged(0.0);
                          } else {
                            valueChanged(double.parse(textEditorVal.text));
                          }
                        }
                    }
                  },
                  onTap: () => callbackOnStartTyping(),
                  onTapOutside: (pointer) {},
                  controller: textEditorVal,
                  style: TextStyle(color: Colors.white),
                ),
              ),
          ],
        ),
      );
    });
  }
}

class PopupNewEssenceObservationPoint {
  VoidCallback? callbackOnStartTyping;
  VoidCallback? onTapOutside;

  PopupNewEssenceObservationPoint(
    BuildContext context,
    LatLng coordinates, {
    this.onTapOutside,
    this.callbackOnStartTyping,
  }) {
    gl.refreshStack(() {
      _AddEssence.reset();
      double height = 170 * gl.eqPx;
      popupForestimatorMessage(
        height: height,
        id: "addESS",
        title: "Observation essence",
        onDiscard: () {
          gl.refreshStack(() {
            gl.stack.pop("addESS");
          });
        },
        child: AddEssence(
          height: height,
          messageAccept: "Placer",
          messageDecline: "Annuler",
          onAccept: (String ess, Color col) {
            gl.refreshStack(() {
              GeometricLayer.getEssenceLayer().addGeometry(
                name: "Observation - ${GeometricLayer.getEssenceLayer().geometries.length + 1}",
              );
              GeometricLayer.getEssenceLayer().geometries.last.addPoint(coordinates);
              GeometricLayer.getEssenceLayer().geometries.last.attributes[0].value = ess;
              GeometricLayer.getEssenceLayer().geometries.last.colorLine = col;
              GeometricLayer.getEssenceLayer().geometries.last.colorInside = col.withAlpha(150);
              GeometricLayer.getEssenceLayer().geometries.last.serialize();
              Geometry.sendEssencePointsInBackground();
              gl.Mode.serialize();
              gl.stack.pop("addESS");
            });
          },
          onDecline: () {
            gl.refreshStack(() {
              gl.stack.pop("addESS");
            });
          },
        ),
      );
    });
  }
}

class AddEssence extends StatefulWidget {
  final VoidCallback? callbackOnStartTyping;
  final Function(String, Color) onAccept;
  final VoidCallback onDecline;
  final String messageAccept;
  final String messageDecline;
  final double height;

  const AddEssence({
    super.key,
    this.callbackOnStartTyping,
    required this.onAccept,
    required this.onDecline,
    required this.messageAccept,
    required this.messageDecline,
    required this.height,
  });

  @override
  State<StatefulWidget> createState() => _AddEssence();
}

class _AddEssence extends State<AddEssence> {
  static int _selected = -1;
  static String _custom = "";
  static Color _color = Colors.transparent;

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        AnimatedContainer(
          duration: Duration(milliseconds: 200),
          height:
              _selected == gl.essenceChoice.length - 1 ? widget.height - 65 * gl.eqPx : widget.height - 50 * gl.eqPx,
          child: lt.ForestimatorScrollView(
            height:
                _selected == gl.essenceChoice.length - 1 ? widget.height - 65 * gl.eqPx : widget.height - 50 * gl.eqPx,
            child: Column(
              children: List<Widget>.generate(gl.essenceChoice.length, (index) {
                return AnimatedContainer(
                  color: _selected == index ? gl.colorAgroBioTech.withAlpha(150) : Colors.transparent,
                  duration: Duration(milliseconds: 500),
                  child: TextButton(
                    style: lt.borderlessStyle,
                    onPressed: () {
                      _selected = index;
                      setState(() {
                        _custom = gl.essenceChoice.keys.toList()[index];
                        _color = _getColorPointForEssence(gl.essenceChoice.keys.toList()[index]);
                      });
                    },
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(
                          gl.essenceChoice.keys.toList()[index],
                          style: TextStyle(color: Colors.white, fontSize: gl.fontSizeM * gl.eqPx),
                        ),
                        CircleAvatar(
                          backgroundColor: _getColorPointForEssence(gl.essenceChoice.keys.toList()[index]),
                          radius: gl.iconSizeXS * gl.eqPx * .75,
                        ),
                      ],
                    ),
                  ),
                );
              }),
            ),
          ),
        ),

        AnimatedContainer(
          duration: Duration(milliseconds: 200),
          height: _selected == gl.essenceChoice.length - 1 ? 65 * gl.eqPx : 50 * gl.eqPx,
          width: gl.menuBarLength * gl.eqPx,
          child: Column(
            children: [
              if (_selected == gl.essenceChoice.length - 1) lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              if (_selected == gl.essenceChoice.length - 1)
                AnimatedOpacity(
                  opacity: _selected == gl.essenceChoice.length - 1 ? 1 : 0,
                  duration: Duration(milliseconds: 200),
                  child: TextFormField(
                    cursorColor: Colors.white,
                    maxLength: 256,
                    maxLengthEnforcement: MaxLengthEnforcement.enforced,
                    onChanged: (value) {
                      setState(() {
                        _custom = value;
                      });
                    },
                    onTap: () => widget.callbackOnStartTyping ?? () {},
                    onTapOutside: (pointer) {},
                    style: TextStyle(color: Colors.white),
                  ),
                ),
              lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  if (_selected > -1 && _selected < gl.essenceChoice.length - 1 && _custom.isNotEmpty ||
                      _selected == gl.essenceChoice.length - 1 && _custom.isNotEmpty)
                    SizedBox(
                      width: gl.menuBarLength * .5 * gl.eqPx,
                      child: TextButton(
                        style: dialogButtonStyle(height: gl.eqPx * 12, width: gl.eqPx * 10 * "Ok".length),
                        onPressed: () {
                          widget.onAccept(_custom, _color);
                        },
                        child: Text(widget.messageAccept, style: dialogTextButtonStyle()),
                      ),
                    ),
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.eqPx,
                    child: TextButton(
                      style: dialogButtonStyle(height: gl.eqPx * 12, width: gl.eqPx * 10 * "Retour".length),
                      onPressed: widget.onDecline,
                      child: Text(widget.messageDecline, style: dialogTextButtonStyle()),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }

  Color _getColorPointForEssence(String ess) {
    for (int index in gl.dico.mLayerBases['COMPOALL']!.mDicoVal.keys) {
      if (gl.dico.mLayerBases['COMPOALL']!.mDicoVal[index] == gl.essenceChoice[ess]) {
        return gl.dico.mLayerBases['COMPOALL']!.mDicoCol[index] ?? Colors.tealAccent;
      }
    }
    return Colors.tealAccent;
  }

  static void reset() => {_custom = "", _selected = -1};
}

class PopupValueChange {
  PopupValueChange(
    String type,
    dynamic oldValue,
    ValueChanged<dynamic> valueChanged,
    VoidCallback onTapOutside,
    VoidCallback onAccept,
  ) {
    TextEditingController textEditor = TextEditingController(text: oldValue);
    gl.refreshStack(() {
      popupForestimatorMessage(
        id: "ValChange",
        title: "Introduisez une nouvelle valeur",
        messageAccept: "Changer",
        messageDecline: "Annuler",
        onAccept: () {
          onAccept();
          gl.stack.pop("ValChange");
        },
        onDecline: () {
          valueChanged(oldValue);
          gl.stack.pop("ValChange");
        },
        onDiscard: () {
          valueChanged(oldValue);
          gl.stack.pop("ValChange");
        },
        child: Column(
          children: [
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child:
                  type != "mail"
                      ? TextFormField(
                        maxLength: 255,
                        maxLengthEnforcement: MaxLengthEnforcement.enforced,
                        onChanged: (String str) {
                          switch (type) {
                            case "prop":
                              valueChanged(textEditor.text);
                            case "string":
                              valueChanged(str.toString());
                            case "int":
                              try {
                                valueChanged(int.parse(str));
                              } catch (e) {
                                if (textEditor.text.isNotEmpty) {
                                  textEditor.text = textEditor.text.substring(0, textEditor.text.length - 1);
                                }
                                if (textEditor.text.isEmpty) {
                                  valueChanged(0);
                                } else {
                                  valueChanged(textEditor.text);
                                }
                              }
                            case "double":
                              try {
                                valueChanged(double.parse(str));
                              } catch (e) {
                                if (textEditor.text.isNotEmpty) {
                                  textEditor.text = textEditor.text.substring(0, textEditor.text.length - 1);
                                }
                                if (textEditor.text.isEmpty) {
                                  valueChanged(0.0);
                                } else {
                                  valueChanged(double.parse(textEditor.text));
                                }
                              }
                          }
                        },
                        onTapOutside: (pointer) {},
                        controller: textEditor,
                        style: TextStyle(color: Colors.white),
                      )
                      : lt.ValidTextField(
                        initialValue: gl.UserData.mail,
                        width: gl.menuBarLength * gl.eqPx,
                        height: gl.menuBarThickness * gl.eqPx,
                        validate: gl.UserData.validMail,
                        onValid: (String str) {
                          valueChanged(str);
                        },
                        onInvalid: (String str) {},
                        onTap: () => () {},
                        onEditingComplete: () {},
                      ),
            ),
          ],
        ),
      );
    });
  }
}

String cleanAttributeName(String attribute) {
  String controlled = "";
  String wantedCharacters = "_";
  for (int i = 0; i < attribute.length; i++) {
    if (attribute[i].contains(wantedCharacters) ||
        attribute[i].contains(RegExp(r'[A-Z]')) ||
        attribute[i].contains(RegExp(r'[a-z]')) ||
        attribute[i].contains(RegExp(r'[0-9]'))) {
      controlled = controlled + attribute[i];
    }
  }
  return controlled;
}

bool controlDuplicateAttributeName(String attribute, List<Attribute> lAttr) {
  int count = 0;
  for (int i = 0; i < lAttr.length; i++) {
    if (attribute == lAttr[i].name) {
      count++;
    }
  }
  return count > 1 ? true : false;
}

class SelectAttributeType extends StatefulWidget {
  final ValueChanged<String> typeChanged;
  const SelectAttributeType({super.key, required this.typeChanged});

  @override
  State<StatefulWidget> createState() => _SelectAttributeType();
}

class _SelectAttributeType extends State<SelectAttributeType> {
  final Color active = Colors.black;
  int _selectedType = 0;
  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          Container(
            width: gl.eqPx * gl.iconSizeM * 2.1,
            height: gl.eqPx * gl.iconSizeM * 1.35,
            color: _selectedType == 0 ? gl.colorAgroBioTech : Colors.transparent,
            child: TextButton(
              onPressed: () {
                setState(() {
                  _selectedType = 0;
                });
                widget.typeChanged("string");
              },
              child: Column(
                children: [
                  Icon(Icons.abc, color: Colors.white),
                  Text("Charactères", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
                ],
              ),
            ),
          ),
          Container(
            width: gl.eqPx * gl.iconSizeM * 1.5,
            height: gl.eqPx * gl.iconSizeM * 1.35,
            color: _selectedType == 1 ? gl.colorAgroBioTech : Colors.transparent,
            child: TextButton(
              onPressed: () {
                setState(() {
                  _selectedType = 1;
                });
                widget.typeChanged("int");
              },
              child: Column(
                children: [
                  Icon(Icons.numbers, color: Colors.white),
                  Text("Entièr", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
                ],
              ),
            ),
          ),
          Container(
            width: gl.eqPx * gl.iconSizeM * 1.7,
            height: gl.eqPx * gl.iconSizeM * 1.35,
            color: _selectedType == 2 ? gl.colorAgroBioTech : Colors.transparent,
            child: TextButton(
              onPressed: () {
                setState(() {
                  _selectedType = 2;
                });
                widget.typeChanged("double");
              },
              child: Column(
                children: [
                  Icon(Icons.numbers, color: Colors.white),
                  Text("Décimale", style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeXS)),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class PopupSelectAttributeSet {
  PopupSelectAttributeSet(
    BuildContext context,
    List<Attribute> lAttr, {
    void Function(List<Attribute> Function())? onPressed,
  }) {
    String wantedSet = "Observation Composition";
    gl.refreshStack(() {
      popupForestimatorMessage(
        id: "chooseSet",
        title: "Choisissez un ensemble",
        messageAccept: "Ajouter",
        messageDecline: "Annuler",
        onAccept: () {
          void Function(List<Attribute> Function()) it =
              onPressed ??
              (function) {
                function();
              };
          it(() {
            List<String> attributeNames = List<String>.generate(lAttr.length, (i) {
              return lAttr[i].name;
            });
            List<Attribute> res = [];
            switch (wantedSet) {
              case "Observation Composition":
                if (!(attributeNames.contains("essence") || attributeNames.contains("rmq"))) {
                  res = [
                    Attribute(name: "essence", type: "string", value: ""),
                    Attribute(name: "rmq", type: "string", value: ""),
                  ];
                }
                break;
              default:
            }
            gl.stack.pop("chooseSet");
            return res;
          });
        },
        onDecline: () {
          gl.stack.pop("chooseSet");
        },
        child: Column(
          children: [
            SizedBox(
              width: gl.menuBarLength * gl.eqPx,
              child: SelectAttributeSet(
                setChanged: (String it) {
                  wantedSet = it;
                },
              ),
            ),
            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
          ],
        ),
      );
    });
  }
}

class SelectAttributeSet extends StatefulWidget {
  final ValueChanged<String> setChanged;
  const SelectAttributeSet({super.key, required this.setChanged});

  @override
  State<StatefulWidget> createState() => _SelectAttributeSet();
}

class _SelectAttributeSet extends State<SelectAttributeSet> {
  final Color active = Colors.black;
  int _selectedSet = 0;
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceAround,
      children: [
        Container(
          width: gl.eqPx * gl.iconSizeM * 2.1,
          height: gl.eqPx * gl.iconSizeM * 1.5,
          color: _selectedSet == 0 ? gl.colorAgroBioTech : Colors.transparent,
          child: TextButton(
            onPressed: () {
              setState(() {
                _selectedSet = 0;
                widget.setChanged("Observation Composition");
              });
            },
            child: Column(
              children: [
                FaIcon(FontAwesomeIcons.chartLine, color: Colors.black),
                SizedBox(
                  width: gl.eqPx * gl.iconSizeM * 1.9,
                  child: Text(
                    "Observation Composition",
                    style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
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

class GeoLayerListMenu extends StatefulWidget {
  final ValueChanged<LatLng> mapmove;
  final VoidCallback after;
  final double windowHeight;

  const GeoLayerListMenu({super.key, required this.mapmove, required this.after, required this.windowHeight});

  @override
  State<StatefulWidget> createState() => _GeoLayerListMenu();
}

class _GeoLayerListMenu extends State<GeoLayerListMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  final ScrollController _controller = ScrollController();
  final PageController _pageController = PageController();

  int? _selectedIndex;
  bool _titleLayer = true;

  void _scrollDown() {
    _controller.animateTo(
      _controller.position.maxScrollExtent +
          (gl.dsp.orientation == Orientation.portrait
              ? gl.eqPx * gl.polyListSelectedCardHeight
              : gl.eqPx * gl.polyListCardHeight),
      duration: Duration(seconds: 1),
      curve: Curves.fastOutSlowIn,
    );
  }

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    double titleHeight = gl.eqPx * gl.fontSizeXL * 1.2;
    double listPartHeight =
        gl.eqPx *
        (widget.windowHeight -
            (gl.fontSizeXL * 1.2) -
            gl.popupReturnButtonHeight -
            (gl.polyNewPolygonButtonHeight * .9) -
            2.5 - //lt.stroke
            10);
    if (!_titleLayer) {
      listPartHeight =
          gl.eqPx *
          (widget.windowHeight -
              (gl.fontSizeXL * 1.2) -
              2.5 - //lt.stroke
              10);
    }
    return Card(
      color: gl.backgroundTransparentBlackBox,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadiusGeometry.circular(12.0),
        side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
      ),
      child: SizedBox(
        height: gl.eqPx * gl.eqPxH * .85,
        width: gl.eqPx * gl.eqPxW * .95,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Container(
              alignment: Alignment.center,
              height: titleHeight * 1.2,
              child: Row(
                mainAxisAlignment: _titleLayer ? MainAxisAlignment.spaceBetween : MainAxisAlignment.spaceBetween,
                children: [
                  !_titleLayer
                      ? IconButton(
                        style: lt.trNoPadButtonstyle,
                        icon: Icon(Icons.arrow_back, color: Colors.white, size: gl.eqPx * gl.iconSizeM),
                        onPressed: () {
                          setState(() {
                            _titleLayer = true;
                            _pageController.animateToPage(
                              0,
                              duration: Duration(milliseconds: 300),
                              curve: Curves.easeInOut,
                            );
                          });
                        },
                        padding: EdgeInsets.zero,
                      )
                      : Container(width: gl.eqPx * gl.iconSizeM),
                  Text(
                    _titleLayer ? "Liste des layer" : "Layer",
                    textAlign: TextAlign.justify,
                    style: TextStyle(fontSize: gl.eqPx * gl.fontSizeL, color: Colors.white),
                  ),
                  lt.forestimatorButton(widget.after, Icons.arrow_drop_up_outlined),
                ],
              ),
            ),
            lt.stroke(vertical: false, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
            SizedBox(
              width: gl.eqPx * gl.popupWindowsPortraitWidth,
              height: listPartHeight,
              child: PageView(
                controller: _pageController,
                physics: NeverScrollableScrollPhysics(),
                children: [
                  ReorderableListView.builder(
                    scrollController: _controller,
                    buildDefaultDragHandles: false,
                    onReorder: (int oldIndex, int newIndex) {
                      setState(() {
                        if (oldIndex < newIndex) {
                          newIndex -= 1;
                        }
                        if (gl.geoLayers.length < newIndex + 1 || gl.geoLayers.length < oldIndex + 1) {
                          return;
                        }
                        gl.refreshStack(() {
                          final GeometricLayer item = gl.geoLayers.removeAt(oldIndex);
                          gl.geoLayers.insert(newIndex, item);
                        });
                        if (oldIndex == gl.selectedGeoLayer) {
                          gl.selectedGeoLayer = newIndex;
                        } else if (newIndex == gl.selectedGeoLayer) {
                          if (oldIndex > newIndex) {
                            gl.selectedGeoLayer++;
                          } else {
                            gl.selectedGeoLayer--;
                          }
                        } else if (oldIndex < gl.selectedGeoLayer && gl.selectedGeoLayer < newIndex) {
                          gl.selectedGeoLayer--;
                        } else if (oldIndex > gl.selectedGeoLayer && gl.selectedGeoLayer > newIndex) {
                          gl.selectedGeoLayer++;
                        }
                      });
                    },
                    itemCount: gl.geoLayers.length,
                    itemBuilder:
                        (context, i) => TextButton(
                          style: ButtonStyle(
                            fixedSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                              WidgetState.any: Size(gl.eqPx * gl.polyListCardWidth, gl.eqPx * gl.polyListCardHeight),
                            }),
                            padding: WidgetStateProperty.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                              WidgetState.any: EdgeInsetsGeometry.symmetric(vertical: gl.eqPx * 2),
                            }),
                          ),
                          key: Key('$i'),
                          onPressed:
                              i == gl.selectedGeoLayer
                                  ? () {
                                    setState(() {
                                      gl.selectedGeoLayer = -1;
                                    });
                                  }
                                  : () {
                                    setState(() {
                                      gl.selectedGeoLayer = i;
                                    });
                                  },
                          child: ReorderableDragStartListener(
                            index: i,
                            child: SizedBox(
                              height: gl.polyListSelectedCardHeight * gl.eqPx,
                              width: gl.polyListSelectedCardWidth * gl.eqPx,
                              child: Card(
                                shape: RoundedRectangleBorder(
                                  borderRadius: BorderRadiusGeometry.circular(12.0),
                                  side:
                                      i == gl.selectedGeoLayer
                                          ? BorderSide(
                                            color: gl.geoLayers[i].defaultColor.withAlpha(255),
                                            width: gl.eqPx * .75,
                                          )
                                          : BorderSide(
                                            color: gl.geoLayers[i].defaultColor.withAlpha(120),
                                            width: gl.eqPx * .25,
                                          ),
                                ),
                                surfaceTintColor: Colors.transparent,
                                shadowColor: Colors.transparent,
                                color:
                                    i == gl.selectedGeoLayer
                                        ? gl.selLay.defaultColor.withAlpha(100)
                                        : Colors.black.withAlpha(100),
                                child: Container(
                                  alignment: Alignment.center,
                                  child: Row(
                                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                    children: [
                                      IconButton(
                                        onPressed: () {
                                          gl.refreshStack(() {
                                            if (!gl.Mode.editPolygon) {
                                              gl.geoLayers[i].visibleOnMap = !gl.geoLayers[i].visibleOnMap;
                                              gl.geoLayers[i].visible(gl.geoLayers[i].visibleOnMap);
                                            }
                                          });
                                          setState(() {});
                                        },
                                        icon:
                                            gl.geoLayers[i].visibleOnMap
                                                ? FaIcon(
                                                  FontAwesomeIcons.eyeSlash,
                                                  size: gl.eqPx * gl.iconSizeS * .9,
                                                  color: Colors.white,
                                                )
                                                : FaIcon(
                                                  FontAwesomeIcons.eye,
                                                  size: gl.eqPx * gl.iconSizeS * .9,
                                                  color: Colors.white,
                                                ),
                                      ),
                                      SizedBox(
                                        width: gl.eqPx * gl.chosenPolyBarWidth * .5,
                                        child: Stack(
                                          children: [
                                            Row(
                                              children: [
                                                Container(
                                                  alignment: Alignment.topLeft,
                                                  child:
                                                      gl.geoLayers[i].type.contains("Point")
                                                          ? Text(
                                                            "POINT",
                                                            style: TextStyle(
                                                              color: Colors.yellow,
                                                              fontSize: gl.eqPx * gl.fontSizeXS * .9,
                                                            ),
                                                          )
                                                          : gl.geoLayers[i].type == "Polygon"
                                                          ? Text(
                                                            "POLY",
                                                            style: TextStyle(
                                                              color: Colors.green,
                                                              fontSize: gl.eqPx * gl.fontSizeXS * .9,
                                                            ),
                                                          )
                                                          : Text(
                                                            "OHA?",
                                                            style: TextStyle(
                                                              color: Colors.red,
                                                              fontSize: gl.eqPx * gl.fontSizeXS * .9,
                                                            ),
                                                          ),
                                                ),
                                                SizedBox(width: gl.eqPx * 2),
                                                if (gl.geoLayers[i].allSent())
                                                  Container(
                                                    alignment: Alignment.topLeft,
                                                    child: Text(
                                                      "ALL SENT",
                                                      style: TextStyle(
                                                        color: Colors.red,
                                                        fontSize: gl.eqPx * gl.fontSizeXS * .9,
                                                      ),
                                                    ),
                                                  ),
                                              ],
                                            ),
                                            lt.ForestimatorScrollView(
                                              width: gl.eqPx * gl.chosenPolyBarWidth * .5,
                                              height: gl.eqPx * gl.iconSizeL * 1.5,
                                              sizeArrows: gl.eqPx * gl.iconSizeXS,
                                              arrowColor: gl.colorAgroBioTech,
                                              horizontal: true,
                                              child: Container(
                                                alignment: Alignment.center,
                                                child: Text(
                                                  gl.geoLayers[i].name,
                                                  textAlign: TextAlign.center,
                                                  style: TextStyle(
                                                    color: Colors.white,
                                                    fontSize: gl.eqPx * gl.fontSizeL,
                                                  ),
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                      ),
                                      Container(
                                        alignment: Alignment.center,
                                        height: gl.eqPx * gl.iconSizeM,
                                        width: gl.eqPx * gl.iconSizeM,
                                        child: IconButton(
                                          style: lt.trNoPadButtonstyle,
                                          onPressed: () {
                                            setState(() {
                                              gl.selectedGeoLayer = i;
                                              _selectedIndex = i;
                                              _titleLayer = false;
                                            });
                                            _pageController.animateToPage(
                                              1,
                                              duration: Duration(milliseconds: 300),
                                              curve: Curves.easeInOut,
                                            );
                                          },
                                          icon: Icon(
                                            Icons.arrow_forward,
                                            color: Colors.white,
                                            size: gl.eqPx * gl.iconSizeM,
                                          ),
                                          padding: EdgeInsets.zero,
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              ),
                            ),
                          ),
                        ),
                  ),
                  if (_selectedIndex != null)
                    LayerPropertiesPage(() {
                      setState(() {
                        _titleLayer = true;
                        _pageController.animateToPage(
                          0,
                          duration: Duration(milliseconds: 300),
                          curve: Curves.easeInOut,
                        );
                      });
                    }, widget.mapmove),
                ],
              ),
            ),
            if (gl.dsp.orientation == Orientation.portrait && _titleLayer)
              TextButton(
                style: ButtonStyle(
                  backgroundColor: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Color>{
                    WidgetState.any: Colors.white,
                  }),
                  shape: WidgetStateProperty<OutlinedBorder>.fromMap(<WidgetStatesConstraint, OutlinedBorder>{
                    WidgetState.any: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                  }),
                  fixedSize: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Size>{
                    WidgetState.any: Size(
                      gl.eqPx * gl.polyListCardWidth * .97,
                      gl.eqPx * gl.polyNewPolygonButtonHeight * .9,
                    ),
                  }),
                  padding: WidgetStateProperty.fromMap(<WidgetStatesConstraint, EdgeInsetsGeometry>{
                    WidgetState.any: EdgeInsetsGeometry.zero,
                  }),
                ),
                key: Key('autsch-5-addPoly'),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [Icon(Icons.add, size: (gl.polyNewPolygonButtonHeight - 4) * gl.eqPx, color: Colors.black)],
                ),
                onPressed: () {
                  PopupNewGeometricLayer(context, (String type, String name, Color color) {
                    setState(() {
                      switch (type) {
                        case "Point":
                          gl.geoLayers.add(GeometricLayer.point());
                          break;
                        case "Polygon":
                          gl.geoLayers.add(GeometricLayer.polygon());
                          break;
                      }
                      gl.geoLayers.last.defaultColor = color;
                      gl.geoLayers.last.name = name;
                    });
                    gl.selectedGeoLayer = gl.geoLayers.length - 1;
                    gl.geoLayers.last.serialize();
                    gl.refreshStack(() {
                      gl.selectedGeoLayer = gl.geoLayers.length - 1;
                    });
                    _scrollDown();
                  });
                },
              ),
          ],
        ),
      ),
    );
  }
}

class LayerPropertiesPage extends StatefulWidget {
  final VoidCallback closePage;
  final void Function(LatLng) mapmove;
  const LayerPropertiesPage(this.closePage, this.mapmove, {super.key});

  @override
  State<StatefulWidget> createState() => _LayerPropertiesPage();
}

class _LayerPropertiesPage extends State<LayerPropertiesPage> {
  bool _doingAnaPt = false;
  TextEditingController? controllerPdfName;
  TextEditingController? controllerLocationName;

  final PageController _pageController = PageController();

  ScrollController propertiesTableScrollController = ScrollController();
  bool propertiesOpen = false;
  bool attributesOpen = false;
  bool listOpen = true;

  int? _selectedIndex;
  bool _titleLayer = true;

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return switchRowColWithOrientation([
      SizedBox(
        height:
            gl.dsp.orientation == Orientation.portrait
                ? (gl.popupWindowsPortraitHeight - gl.searchBarHeight - gl.popupReturnButtonHeight) * gl.eqPx
                : (gl.popupWindowsLandscapeHeight - gl.searchBarHeight) * gl.eqPx,
        width: gl.onCatalogueWidth * gl.eqPx,
        child: ListView(
          padding: const EdgeInsets.symmetric(horizontal: 0),
          children: [
            Card(
              shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color: gl.colorAgroBioTech.withAlpha(75),
              child: Column(children: _getPropertiesTab() + _getGeometriesList()),
            ),
          ],
        ),
      ),
    ]);
  }

  List<Widget> _getGeometriesList() {
    if (!gl.layerReady) return [];
    return [
      Card(
        color: gl.colorAgroBioTech.withAlpha(200),
        child: Container(
          alignment: Alignment.center,
          width: gl.eqPx * gl.onCatalogueWidth * .97,
          height: gl.eqPx * 18,
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              SizedBox(width: gl.eqPx * gl.iconSizeL, height: gl.eqPx * gl.iconSizeL),
              Text(
                "Entités",
                textAlign: TextAlign.center,
                style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeL),
              ),
              SizedBox(
                width: gl.eqPx * gl.iconSizeL,
                height: gl.eqPx * gl.iconSizeL,
                child: IconButton(
                  style: lt.trNoPadButtonstyle,
                  onPressed: () {
                    setState(() {
                      listOpen = !listOpen;
                    });
                  },
                  icon: Icon(
                    listOpen ? Icons.arrow_drop_up_outlined : Icons.arrow_drop_down_outlined,
                    size: gl.eqPx * gl.iconSizeL * .8,
                    color: Colors.black,
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
      if (listOpen)
        Card(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(12.0),
            side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
          ),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          color: Colors.white.withAlpha(200),
          child: Container(
            padding: EdgeInsetsGeometry.zero,
            width: gl.eqPx * gl.onCatalogueWidth,
            height: _titleLayer ? gl.eqPx * gl.selLay.geometries.length * (16 + 2.5) : gl.eqPx * 110,
            child: PageView(
              controller: _pageController,
              children: [
                Column(
                  children: List<Widget>.generate(gl.layerReady ? gl.selLay.geometries.length : 1, (int index) {
                    return Column(
                      children: [
                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Container(
                              width: gl.eqPx * 75,
                              height: gl.eqPx * 14,
                              alignment: Alignment.centerLeft,
                              padding: EdgeInsetsGeometry.zero,
                              child: Column(
                                children: [
                                  Container(
                                    padding: EdgeInsets.zero,
                                    width: gl.eqPx * gl.onCatalogueWidth,
                                    height: gl.eqPx * 4,
                                    child: TextButton(
                                      style: lt.trNoPadButtonstyle,
                                      onPressed: () {
                                        setState(() {
                                          PopupValueChange(
                                            "string",
                                            gl.layerReady
                                                ? gl.selLay.geometries[index].name
                                                : "La liste est encore vide!",
                                            (value) {
                                              setState(() {
                                                gl.selLay.geometries[index].name = value.toString();
                                              });
                                            },
                                            () {},
                                            () {
                                              gl.selLay.geometries[index].serialize();
                                            },
                                          );
                                        });
                                      },
                                      child: Row(
                                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                        children: [
                                          Row(
                                            children: [
                                              Text(
                                                " $index",
                                                style: TextStyle(
                                                  color: Colors.black,
                                                  fontSize: gl.eqPx * gl.fontSizeXS,
                                                ),
                                              ),
                                            ],
                                          ),
                                          lt.ForestimatorScrollView(
                                            height: gl.eqPx * gl.fontSizeXS,
                                            width: gl.eqPx * 30,
                                            horizontal: true,
                                            child: Text(
                                              gl.layerReady ? gl.selLay.geometries[index].name : "",
                                              textAlign: TextAlign.center,
                                              style: TextStyle(
                                                color: Colors.black,
                                                fontSize: gl.eqPx * gl.fontSizeXS * .9,
                                              ),
                                            ),
                                          ),
                                          Container(
                                            alignment: Alignment.center,
                                            height: gl.eqPx * gl.iconSizeXXS,
                                            width: gl.eqPx * gl.iconSizeS,
                                            child: IconButton(
                                              style: lt.trNoPadButtonstyle,
                                              onPressed: () {
                                                PopupColorChooser(
                                                  gl.selLay.geometries[index].colorInside,
                                                  gl.notificationContext!,
                                                  //change color
                                                  (Color col) {
                                                    setState(() {
                                                      gl.selLay.geometries[index].setColorInside(col);
                                                      gl.selLay.geometries[index].setColorLine(
                                                        Color.fromRGBO(
                                                          (col.r * 255).round(),
                                                          (col.g * 255).round(),
                                                          (col.b * 255).round(),
                                                          1.0,
                                                        ),
                                                      );
                                                    });
                                                  },
                                                  () {},
                                                  () {
                                                    gl.selLay.geometries[index].serialize();
                                                  },
                                                );
                                              },
                                              icon: Icon(
                                                gl.selLay.geometries[index].type.contains("Point")
                                                    ? gl.selectableIcons[gl.selLay.geometries[index].selectedPointIcon]
                                                    : gl.selectableIconGeo[gl
                                                        .selLay
                                                        .geometries[index]
                                                        .selectedPointIcon],
                                                size: gl.eqPx * gl.iconSizeXXS,
                                                color: gl.selLay.geometries[index].colorLine,
                                              ),
                                            ),
                                          ),
                                        ],
                                      ),
                                    ),
                                  ),
                                  Row(
                                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                    children: [
                                      Row(
                                        children: [
                                          gl.selLay.geometries[index].sentToServer
                                              ? Container(
                                                alignment: Alignment.center,
                                                width: gl.eqPx * gl.iconSizeS,
                                                height: gl.eqPx * gl.iconSizeS,
                                                child: Text(
                                                  "SENT",
                                                  style: TextStyle(
                                                    color: Colors.red,
                                                    fontSize: gl.eqPx * gl.fontSizeXXS,
                                                  ),
                                                ),
                                              )
                                              : gl.selLay.geometries[index].containsAttribute("essence") &&
                                                  gl.selLay.geometries[index].containsAttribute("rmq")
                                              ? Container(
                                                alignment: Alignment.center,
                                                width: gl.eqPx * gl.iconSizeS,
                                                height: gl.eqPx * gl.iconSizeS,
                                                child: IconButton(
                                                  style: lt.trNoPadButtonstyle,
                                                  onPressed: () {
                                                    if (gl.UserData.forename.isEmpty ||
                                                        gl.UserData.name.isEmpty ||
                                                        gl.UserData.mail.isEmpty) {
                                                      PopupUserData(
                                                        context,
                                                        () {},
                                                        () {
                                                          PopupDoYouReally(
                                                            context,
                                                            () {
                                                              gl.selLay.geometries[index].sendGeometryToServer();
                                                            },
                                                            "Attention !",
                                                            gl.labelSendCompoFeature,
                                                            "Envoyer",
                                                            "Ne pas envoyer",
                                                          );
                                                        },
                                                        oldForename: gl.UserData.forename,
                                                        oldName: gl.UserData.name,
                                                        oldMail: gl.UserData.mail,
                                                      );
                                                    } else {
                                                      PopupDoYouReally(
                                                        context,
                                                        () {
                                                          gl.selLay.geometries[index].sendGeometryToServer();
                                                        },
                                                        "Attention !",
                                                        gl.labelSendCompoFeature,
                                                        "Envoyer",
                                                        "Ne pas envoyer",
                                                      );
                                                    }
                                                  },
                                                  icon: Icon(
                                                    Icons.send_and_archive,
                                                    color: Colors.black,
                                                    size: gl.iconSizeXS * gl.eqPx,
                                                  ),
                                                ),
                                              )
                                              : SizedBox(width: gl.eqPx * gl.iconSizeS, height: gl.eqPx * gl.iconSizeS),
                                          (gl.selLay.geometries[index].type == "Polygon" &&
                                                  gl.selLay.geometries[index].points.length > 2)
                                              ? Container(
                                                alignment: Alignment.center,
                                                width: gl.eqPx * gl.iconSizeS,
                                                height: gl.eqPx * gl.iconSizeS,
                                                child: IconButton(
                                                  style: lt.trNoPadButtonstyle,
                                                  onPressed: () async {
                                                    if (await gl.selLay.geometries[index].onlineSurfaceAnalysis()) {
                                                      gl.refreshStack(() {
                                                        popupForestimatorWindow(
                                                          id: "anaSurfResult",
                                                          title: "Resultats de l'analyse",
                                                          child: AnaSurfResultsMenu(
                                                            gl.selLay.geometries[index].decodedJson,
                                                            () {
                                                              gl.refreshStack(() {
                                                                gl.stack.pop("anaSurfResult");
                                                              });
                                                            },
                                                            () {
                                                              gl.refreshStack(() {
                                                                gl.stack.pop("anaSurfResult");
                                                              });
                                                            },
                                                          ),
                                                        );
                                                      });
                                                    }
                                                  },
                                                  icon: Icon(
                                                    Icons.analytics,
                                                    color: Colors.black,
                                                    size: gl.eqPx * gl.iconSizeXS,
                                                  ),
                                                ),
                                              )
                                              : SizedBox(width: gl.eqPx * gl.iconSizeS, height: gl.eqPx * gl.iconSizeS),
                                          (gl.selLay.geometries[index].type.contains("Point") &&
                                                  gl.selLay.geometries[index].points.isNotEmpty)
                                              ? Container(
                                                alignment: Alignment.center,
                                                width: gl.eqPx * gl.iconSizeS,
                                                height: gl.eqPx * gl.iconSizeS,
                                                child: IconButton(
                                                  style: lt.trNoPadButtonstyle,
                                                  onPressed: () async {
                                                    if (!_doingAnaPt) {
                                                      _doingAnaPt = true;
                                                      await gl.selLay.geometries[index].runAnaPt();
                                                      gl.refreshStack(() {
                                                        popupForestimatorWindow(
                                                          id: "anaPres",
                                                          title: "Resultats de l'analyse",
                                                          child: AnaResultsMenu(() {
                                                            gl.refreshStack(() {});
                                                          }, gl.requestedLayers),
                                                        );
                                                      });
                                                      _doingAnaPt = false;
                                                    }
                                                  },
                                                  icon: Icon(
                                                    Icons.location_pin,
                                                    color: Colors.black,
                                                    size: gl.eqPx * gl.iconSizeXS,
                                                  ),
                                                ),
                                              )
                                              : SizedBox(width: gl.eqPx * gl.iconSizeS, height: gl.eqPx * gl.iconSizeS),
                                        ],
                                      ),
                                      (gl.selLay.geometries[index].center.longitude != 0.0 &&
                                              gl.selLay.geometries[index].center.latitude != 0.0)
                                          ? Container(
                                            alignment: Alignment.center,
                                            width: gl.eqPx * gl.iconSizeS,
                                            height: gl.eqPx * gl.iconSizeS,
                                            child: IconButton(
                                              style: lt.borderlessStyle,
                                              onPressed: () {
                                                gl.selLay.geometries[index].visibleOnMap = true;
                                                gl.selLay.geometries[index].serialize();
                                                setState(() {
                                                  widget.mapmove(gl.selLay.geometries[index].center);
                                                  gl.selLay.selectedGeometry = index;
                                                });
                                                gl.refreshStack(() {
                                                  gl.modeMapShowPolygons = true;
                                                });
                                              },
                                              icon: Icon(
                                                Icons.gps_fixed,
                                                size: gl.eqPx * gl.iconSizeXS,
                                                opticalSize: gl.eqPx * gl.iconSizeS,
                                                color: gl.selLay.selectedGeometry == index ? Colors.red : Colors.black,
                                              ),
                                            ),
                                          )
                                          : SizedBox(width: gl.eqPx * gl.iconSizeS, height: gl.eqPx * gl.iconSizeS),
                                      Container(
                                        alignment: Alignment.center,
                                        width: gl.eqPx * gl.iconSizeS,
                                        height: gl.eqPx * gl.iconSizeS,
                                        child: IconButton(
                                          style: lt.trNoPadButtonstyle,
                                          onPressed: () {
                                            PopupDoYouReally(
                                              gl.notificationContext!,
                                              () {
                                                setState(() {
                                                  widget.closePage();
                                                  gl.selLay.removeGeometry(id: gl.selLay.geometries[index].id);
                                                  if (index > 0) {
                                                    gl.selLay.selectedGeometry--;
                                                  } else if (gl.selLay.geometries.isEmpty) {
                                                    gl.selLay.selectedGeometry = -1;
                                                  }
                                                });
                                              },
                                              "Message",
                                              "\nVoulez vous vraiment supprimer ${gl.selLay.geometries[index].name}?\n",
                                            );
                                          },
                                          icon: Icon(
                                            Icons.delete_forever,
                                            color: Colors.black,
                                            size: gl.eqPx * gl.iconSizeXS,
                                          ),
                                        ),
                                      ),
                                    ],
                                  ),
                                ],
                              ),
                            ),
                            Container(
                              width: gl.eqPx * 10,
                              height: gl.eqPx * 16,
                              alignment: Alignment.centerRight,
                              child: IconButton(
                                style: lt.trNoPadButtonstyle,
                                onPressed: () {
                                  setState(() {
                                    gl.selLay.selectedGeometry = index;
                                    _selectedIndex = index;
                                    _titleLayer = false;
                                  });
                                  _pageController.animateToPage(
                                    1,
                                    duration: Duration(milliseconds: 300),
                                    curve: Curves.easeInOut,
                                  );
                                },
                                icon: Icon(Icons.arrow_forward, color: Colors.black, size: gl.eqPx * gl.iconSizeS),
                                padding: EdgeInsets.zero,
                              ),
                            ),
                          ],
                        ),
                        if (index < gl.selLay.geometries.length - 1)
                          lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      ],
                    );
                  }),
                ),
                if (_selectedIndex != null)
                  Column(
                    children: [
                      Row(
                        children: [
                          IconButton(
                            style: lt.trNoPadButtonstyle,
                            icon: Icon(Icons.arrow_back, color: Colors.black, size: gl.eqPx * gl.iconSizeM),
                            onPressed: () {
                              setState(() {
                                _titleLayer = true;
                              });
                              _pageController.animateToPage(
                                0,
                                duration: Duration(milliseconds: 300),
                                curve: Curves.easeInOut,
                              );
                            },
                          ),
                          Container(
                            alignment: Alignment.centerLeft,
                            width: gl.eqPx * gl.chosenPolyBarWidth * .75,
                            child: Text(
                              "Table des attributs",
                              textAlign: TextAlign.center,
                              style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM),
                            ),
                          ),
                        ],
                      ),
                      entitiesAttributes,
                    ],
                  ),
              ],
            ),
          ),
        ),
    ];
  }

  Widget get entitiesAttributes =>
      gl.geoReady
          ? Column(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Column(
                children: [
                  lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                  Column(
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                        children: [
                          SizedBox(
                            width: gl.eqPx * 10,
                            child: Text(
                              "type",
                              textAlign: TextAlign.center,
                              style: TextStyle(
                                color: Colors.black,
                                fontWeight: FontWeight.bold,
                                fontSize: gl.eqPx * gl.fontSizeM * .75,
                              ),
                            ),
                          ),
                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                          SizedBox(
                            width: gl.eqPx * 7,
                            child: Icon(Icons.remove_red_eye, color: Colors.black, size: gl.eqPx * gl.iconSizeXS),
                          ),
                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                          SizedBox(
                            width: gl.eqPx * 32,
                            child: Text(
                              "Attribut",
                              textAlign: TextAlign.center,
                              style: TextStyle(
                                color: Colors.black,
                                fontWeight: FontWeight.bold,
                                fontSize: gl.eqPx * gl.fontSizeM * .75,
                              ),
                            ),
                          ),
                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                          SizedBox(
                            width: gl.eqPx * 32,
                            child: Text(
                              "Valeur",
                              textAlign: TextAlign.center,
                              style: TextStyle(
                                color: Colors.black,
                                fontWeight: FontWeight.bold,
                                fontSize: gl.eqPx * gl.fontSizeM * .75,
                              ),
                            ),
                          ),
                        ],
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                      Scrollbar(
                        scrollbarOrientation: ScrollbarOrientation.right,
                        thickness: gl.eqPx * 3,
                        controller: propertiesTableScrollController,
                        child: Container(
                          color: gl.backgroundTransparentBlackBox.withAlpha(100),
                          height: gl.eqPx * gl.attributeTableHeight,
                          child: ListView(
                            controller: propertiesTableScrollController,
                            children:
                                <Widget>[
                                  _getFixedAttribute("type", gl.selGeo.type),
                                  _getFixedAttribute("nom", gl.selGeo.name, checked: true),
                                  if (gl.selGeo.type == "Polygon")
                                    _getFixedAttribute("surface", "${(gl.selGeo.area / 100).round() / 100}"),
                                  if (gl.selGeo.type == "Polygon")
                                    _getFixedAttribute("circonference", "${(gl.selGeo.perimeter).round() / 1000}"),

                                  _getFixedAttribute("coordinates", gl.selGeo.getPolyPointsString()),
                                ] +
                                List<Widget>.generate(gl.selGeo.attributes.length, (i) {
                                  return Column(
                                    children: [
                                      Row(
                                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                        children: [
                                          SizedBox(
                                            width: gl.eqPx * 10,
                                            height: gl.eqPx * gl.iconSizeS,
                                            child: TextButton(
                                              style: ButtonStyle(
                                                animationDuration: Duration(seconds: 1),
                                                backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                  <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                ),
                                                padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                  <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                    WidgetState.any: EdgeInsetsGeometry.zero,
                                                  },
                                                ),
                                              ),
                                              onPressed: () {},
                                              onLongPress: () {},
                                              child: Container(
                                                alignment: Alignment.center,
                                                child:
                                                    gl.selGeo.attributes[i].type == "int"
                                                        ? Text(
                                                          "INT",
                                                          style: TextStyle(
                                                            color: Colors.yellow,
                                                            fontSize: gl.fontSizeXXS * gl.eqPx,
                                                          ),
                                                        )
                                                        : gl.selGeo.attributes[i].type == "string"
                                                        ? Text(
                                                          "STRING",
                                                          style: TextStyle(
                                                            color: Colors.lightBlue,
                                                            fontSize: gl.fontSizeXXS * gl.eqPx,
                                                          ),
                                                        )
                                                        : gl.selGeo.attributes[i].type == "double"
                                                        ? Text(
                                                          "DOUBLE",
                                                          style: TextStyle(
                                                            color: Colors.red,
                                                            fontSize: gl.fontSizeXXS * gl.eqPx,
                                                          ),
                                                        )
                                                        : Text(
                                                          "UFO",
                                                          style: TextStyle(
                                                            color: Colors.green,
                                                            fontSize: gl.fontSizeXXS * gl.eqPx,
                                                          ),
                                                        ),
                                              ),
                                            ),
                                          ),
                                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                          SizedBox(
                                            width: gl.eqPx * 7,
                                            height: gl.eqPx * gl.iconSizeM,
                                            child: IconButton(
                                              style: ButtonStyle(
                                                animationDuration: Duration(seconds: 1),
                                                backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                  <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                ),
                                                padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                  <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                    WidgetState.any: EdgeInsetsGeometry.zero,
                                                  },
                                                ),
                                              ),
                                              onPressed: () {},
                                              onLongPress: () async {
                                                setState(() {
                                                  gl
                                                      .selLay
                                                      .geometries[gl.selLay.selectedGeometry]
                                                      .attributes[i]
                                                      .visibleOnMapLabel = !gl
                                                          .selLay
                                                          .geometries[gl.selLay.selectedGeometry]
                                                          .attributes[i]
                                                          .visibleOnMapLabel;
                                                });
                                                gl.selGeo.serialize();
                                              },
                                              icon:
                                                  gl
                                                          .selLay
                                                          .geometries[gl.selLay.selectedGeometry]
                                                          .attributes[i]
                                                          .visibleOnMapLabel
                                                      ? Icon(
                                                        Icons.check_box_outlined,
                                                        color: Colors.black,
                                                        size: gl.eqPx * gl.iconSizeXS,
                                                      )
                                                      : Icon(
                                                        Icons.check_box_outline_blank,
                                                        color: Colors.black,
                                                        size: gl.eqPx * gl.iconSizeXS,
                                                      ),
                                            ),
                                          ),
                                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                          SizedBox(
                                            width: gl.eqPx * 32,
                                            height: gl.eqPx * gl.iconSizeS,
                                            child: TextButton(
                                              style: ButtonStyle(
                                                animationDuration: Duration(seconds: 1),
                                                backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                  <WidgetStatesConstraint, Color>{WidgetState.any: Colors.transparent},
                                                ),
                                                padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                  <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                    WidgetState.any: EdgeInsetsGeometry.zero,
                                                  },
                                                ),
                                              ),
                                              onPressed: () {},
                                              child: Container(
                                                alignment: Alignment.centerLeft,
                                                child: SingleChildScrollView(
                                                  scrollDirection: Axis.horizontal,
                                                  child: Text(
                                                    gl.selGeo.attributes[i].name,
                                                    textAlign: TextAlign.start,
                                                    style: TextStyle(
                                                      color: Colors.black,
                                                      fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                    ),
                                                  ),
                                                ),
                                              ),
                                            ),
                                          ),
                                          lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                          SizedBox(
                                            width: gl.eqPx * 32,
                                            height: gl.eqPx * gl.iconSizeS,
                                            child: TextButton(
                                              style: lt.trNoPadButtonstyle,
                                              onPressed: () {},
                                              onLongPress: () {
                                                PopupValueChange(
                                                  gl.selGeo.attributes[i].type,
                                                  gl.selGeo.attributes[i].value,
                                                  (value) {
                                                    gl.selGeo.attributes[i].value = value;
                                                  },
                                                  () {},
                                                  () {
                                                    gl.selGeo.serialize();
                                                  },
                                                );
                                              },
                                              child: Container(
                                                alignment: Alignment.centerLeft,
                                                child: SingleChildScrollView(
                                                  scrollDirection: Axis.horizontal,
                                                  child:
                                                      gl.selGeo.attributes[i].type == "string"
                                                          ? Text(
                                                            gl
                                                                .selLay
                                                                .geometries[gl.selLay.selectedGeometry]
                                                                .attributes[i]
                                                                .value,
                                                            textAlign: TextAlign.start,
                                                            style: TextStyle(
                                                              color: Colors.black,
                                                              fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                            ),
                                                          )
                                                          : gl
                                                                  .selLay
                                                                  .geometries[gl.selLay.selectedGeometry]
                                                                  .attributes[i]
                                                                  .type ==
                                                              "int"
                                                          ? Text(
                                                            gl.selGeo.attributes[i].value.toString(),
                                                            textAlign: TextAlign.start,
                                                            style: TextStyle(
                                                              color: Colors.white,
                                                              fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                            ),
                                                          )
                                                          : gl
                                                                  .selLay
                                                                  .geometries[gl.selLay.selectedGeometry]
                                                                  .attributes[i]
                                                                  .type ==
                                                              "double"
                                                          ? Text(
                                                            gl.selGeo.attributes[i].value.toString(),
                                                            textAlign: TextAlign.start,
                                                            style: TextStyle(
                                                              color: Colors.white,
                                                              fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                            ),
                                                          )
                                                          : gl
                                                                  .selLay
                                                                  .geometries[gl.selLay.selectedGeometry]
                                                                  .attributes[i]
                                                                  .type ==
                                                              "special"
                                                          ? Text(
                                                            "special value",
                                                            style: TextStyle(
                                                              color: Colors.white,
                                                              fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                            ),
                                                          )
                                                          : Text(
                                                            "ERROR TYPE ${gl.selGeo.attributes[i].type}",
                                                            style: TextStyle(
                                                              color: Colors.white,
                                                              fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                            ),
                                                          ),
                                                ),
                                              ),
                                            ),
                                          ),
                                        ],
                                      ),
                                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                                    ],
                                  );
                                }),
                          ),
                        ),
                      ),
                    ],
                  ),
                  lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                ],
              ),
            ],
          )
          : Column();

  List<Widget> _getDefaultAttributes() {
    return [
      Card(
        shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
        surfaceTintColor: Colors.transparent,
        shadowColor: Colors.transparent,
        color: Colors.transparent,
        child: Column(
          children: [
            Card(
              color: gl.colorAgroBioTech.withAlpha(255),
              child: Container(
                alignment: Alignment.center,
                width: gl.eqPx * gl.onCatalogueWidth * .97,
                height: gl.eqPx * 18,
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    SizedBox(width: gl.eqPx * gl.iconSizeL, height: gl.eqPx * gl.iconSizeL),
                    Text(
                      "Attributs",
                      textAlign: TextAlign.center,
                      style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeL),
                    ),
                    SizedBox(
                      width: gl.eqPx * gl.iconSizeL,
                      height: gl.eqPx * gl.iconSizeL,
                      child: IconButton(
                        style: lt.trNoPadButtonstyle,
                        onPressed: () {
                          setState(() {
                            attributesOpen = !attributesOpen;
                          });
                        },
                        icon: Icon(
                          attributesOpen ? Icons.arrow_drop_up_outlined : Icons.arrow_drop_down_outlined,
                          size: gl.eqPx * gl.iconSizeL * .8,
                          color: Colors.black,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            if (attributesOpen)
              Card(
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                  side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
                ),
                surfaceTintColor: Colors.transparent,
                shadowColor: Colors.transparent,
                color: Colors.white.withAlpha(200),
                child: Column(
                  children: [
                    Column(
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                          children: [
                            SizedBox(
                              width: gl.eqPx * 10,
                              child: Text(
                                "type",
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontWeight: FontWeight.bold,
                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                ),
                              ),
                            ),
                            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                            SizedBox(
                              width: gl.eqPx * 7,
                              child: Icon(Icons.remove_red_eye, color: Colors.black, size: gl.eqPx * gl.iconSizeXS),
                            ),
                            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                            SizedBox(
                              width: gl.eqPx * 64,
                              child: Text(
                                "Attribut",
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontWeight: FontWeight.bold,
                                  fontSize: gl.eqPx * gl.fontSizeM * .75,
                                ),
                              ),
                            ),
                          ],
                        ),
                        lt.stroke(gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                        Scrollbar(
                          scrollbarOrientation: ScrollbarOrientation.right,
                          thickness: gl.eqPx * 3,
                          controller: propertiesTableScrollController,
                          child: Container(
                            color: gl.backgroundTransparentBlackBox.withAlpha(50),
                            height: gl.eqPx * gl.attributeTableHeight,
                            child: ListView(
                              controller: propertiesTableScrollController,
                              children:
                                  <Widget>[
                                    _getFixedAttribute("type", gl.selLay.type, noValues: true),
                                    _getFixedAttribute("nom", gl.selLay.name, noValues: true),
                                  ] +
                                  List<Widget>.generate(gl.layerReady ? gl.selLay.defaultAttributes.length : 0, (i) {
                                    String oldName = gl.selLay.defaultAttributes[i].name;
                                    return Column(
                                      children: [
                                        Row(
                                          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                          children: [
                                            SizedBox(
                                              width: gl.eqPx * 10,
                                              height: gl.eqPx * gl.iconSizeS,
                                              child: TextButton(
                                                style: ButtonStyle(
                                                  animationDuration: Duration(seconds: 1),
                                                  backgroundColor: WidgetStateProperty<Color>.fromMap(
                                                    <WidgetStatesConstraint, Color>{
                                                      WidgetState.any: Colors.transparent,
                                                    },
                                                  ),
                                                  padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                                                    <WidgetStatesConstraint, EdgeInsetsGeometry>{
                                                      WidgetState.any: EdgeInsetsGeometry.zero,
                                                    },
                                                  ),
                                                ),
                                                onPressed: () {},
                                                onLongPress: () {},
                                                child: Container(
                                                  alignment: Alignment.center,
                                                  child:
                                                      gl.selLay.defaultAttributes[i].type == "int"
                                                          ? Text(
                                                            "INT",
                                                            style: TextStyle(
                                                              color: Colors.yellow,
                                                              fontSize: gl.fontSizeXXS * gl.eqPx,
                                                            ),
                                                          )
                                                          : gl.selLay.defaultAttributes[i].type == "string"
                                                          ? Text(
                                                            "STRING",
                                                            style: TextStyle(
                                                              color: Colors.lightBlue,
                                                              fontSize: gl.fontSizeXXS * gl.eqPx,
                                                            ),
                                                          )
                                                          : gl.selLay.defaultAttributes[i].type == "double"
                                                          ? Text(
                                                            "DOUBLE",
                                                            style: TextStyle(
                                                              color: Colors.red,
                                                              fontSize: gl.fontSizeXXS * gl.eqPx,
                                                            ),
                                                          )
                                                          : Text(
                                                            "UFO",
                                                            style: TextStyle(
                                                              color: Colors.green,
                                                              fontSize: gl.fontSizeXXS * gl.eqPx,
                                                            ),
                                                          ),
                                                ),
                                              ),
                                            ),
                                            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                            SizedBox(
                                              width: gl.eqPx * 7,
                                              height: gl.eqPx * gl.iconSizeM,
                                              child: IconButton(
                                                style: lt.trNoPadButtonstyle,
                                                onPressed: () {
                                                  setState(() {
                                                    gl.selLay.defaultAttributes[i].visibleOnMapLabel =
                                                        !gl.selLay.defaultAttributes[i].visibleOnMapLabel;
                                                    gl.selLay.serialize();
                                                  });
                                                  for (Geometry g in gl.selLay.geometries) {
                                                    g.attributes[i].visibleOnMapLabel =
                                                        gl.selLay.defaultAttributes[i].visibleOnMapLabel;
                                                    g.serialize();
                                                  }
                                                },
                                                icon:
                                                    gl.selLay.defaultAttributes[i].visibleOnMapLabel
                                                        ? Icon(
                                                          Icons.check_box_outlined,
                                                          color: Colors.black,
                                                          size: gl.eqPx * gl.iconSizeXS,
                                                        )
                                                        : Icon(
                                                          Icons.check_box_outline_blank,
                                                          color: Colors.black,
                                                          size: gl.eqPx * gl.iconSizeXS,
                                                        ),
                                              ),
                                            ),
                                            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
                                            SizedBox(
                                              width: gl.eqPx * 64,
                                              height: gl.eqPx * gl.iconSizeS,
                                              child: TextButton(
                                                style: lt.trNoPadButtonstyle,
                                                onPressed: () {},
                                                onLongPress: () {
                                                  PopupValueChange(
                                                    "prop",
                                                    gl.selLay.defaultAttributes[i].name,
                                                    (value) {
                                                      setState(() {
                                                        gl.selLay.defaultAttributes[i].name = cleanAttributeName(
                                                          value.toString(),
                                                        );
                                                      });
                                                    },
                                                    () {},
                                                    () {
                                                      String nom = gl.selLay.defaultAttributes[i].name;
                                                      if (controlDuplicateAttributeName(
                                                        gl.selLay.defaultAttributes[i].name,
                                                        gl.selLay.defaultAttributes,
                                                      )) {
                                                        gl.selLay.defaultAttributes[i].name = oldName;
                                                        gl.refreshStack(() {
                                                          popupForestimatorMessage(
                                                            id: "MSGduplicateName",
                                                            title: "Erreur",
                                                            message: "Le nom $nom existe déja!",
                                                          );
                                                        });
                                                        return;
                                                      } else {
                                                        gl.selLay.serialize();
                                                        for (Geometry g in gl.selLay.geometries) {
                                                          g.attributes[i].name = gl.selLay.defaultAttributes[i].name;
                                                        }
                                                      }
                                                    },
                                                  );
                                                },
                                                child: Container(
                                                  alignment: Alignment.centerLeft,
                                                  child: SingleChildScrollView(
                                                    scrollDirection: Axis.horizontal,
                                                    child: Text(
                                                      gl.selLay.defaultAttributes[i].name,
                                                      textAlign: TextAlign.start,
                                                      style: TextStyle(
                                                        color: Colors.black,
                                                        fontSize: gl.eqPx * gl.fontSizeM * .75,
                                                      ),
                                                    ),
                                                  ),
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                        lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                                      ],
                                    );
                                  }),
                            ),
                          ),
                        ),
                      ],
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceAround,
                      children: [
                        TextButton(
                          onPressed: () {
                            gl.selLay.defaultAttributes.add(Attribute(name: "", type: "string", value: ""));
                            PopupNewAttribute(
                              context,
                              "",
                              gl.colorAgroBioTech,
                              (String s) {
                                setState(() {
                                  gl.selLay.defaultAttributes.last.type = s;
                                });
                              },
                              (String s) {
                                setState(() {
                                  gl.selLay.defaultAttributes.last.name = s;
                                });
                              },
                              (dynamic it) {
                                setState(() {
                                  gl.selLay.defaultAttributes.last.value = it;
                                });
                              },
                              () {
                                setState(() {
                                  gl.selLay.defaultAttributes.removeLast();
                                });
                              },
                              () {
                                for (Geometry g in gl.selLay.geometries) {
                                  g.attributes.add(gl.selLay.defaultAttributes.last.clone);
                                }
                              },
                              () {},
                              noValue: true,
                            );
                          },
                          child: Column(
                            children: [
                              Icon(Icons.add_circle, color: Colors.black, size: gl.iconSizeS * gl.eqPx),
                              Text(
                                "Une seule variable",
                                style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                              ),
                            ],
                          ),
                        ),
                        TextButton(
                          onPressed: () {
                            PopupSelectAttributeSet(
                              context,
                              gl.selLay.defaultAttributes,
                              onPressed: (List<Attribute> Function() it) {
                                List<Attribute> res = it();
                                setState(() {
                                  gl.selLay.defaultAttributes.addAll(res);
                                });
                                for (Geometry g in gl.selLay.geometries) {
                                  g.attributes.addAll(res);
                                }
                              },
                            );
                          },
                          child: Column(
                            children: [
                              Icon(
                                Icons.add_circle_outline_outlined,
                                color: Colors.black,
                                size: gl.iconSizeS * gl.eqPx,
                              ),
                              Text(
                                "Un set de variables",
                                style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                              ),
                            ],
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
    ];
  }

  List<Widget> _getPropertiesTab() {
    return [
      Card(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
        ),
        surfaceTintColor: Colors.transparent,
        shadowColor: Colors.transparent,
        color: gl.colorAgroBioTech.withAlpha(255),
        child: Column(
          children: [
            Card(
              surfaceTintColor: Colors.transparent,
              shadowColor: Colors.transparent,
              color: gl.colorAgroBioTech.withAlpha(255),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  SizedBox(width: gl.eqPx * gl.iconSizeL, height: gl.eqPx * gl.iconSizeL),
                  Text(
                    "Proprietés",
                    textAlign: TextAlign.center,
                    style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeL),
                  ),
                  SizedBox(
                    width: gl.eqPx * gl.iconSizeL,
                    height: gl.eqPx * gl.iconSizeL,
                    child: IconButton(
                      style: lt.trNoPadButtonstyle,
                      onPressed: () {
                        setState(() {
                          propertiesOpen = !propertiesOpen;
                        });
                      },
                      icon: Icon(
                        propertiesOpen ? Icons.arrow_drop_up_outlined : Icons.arrow_drop_down_outlined,
                        size: gl.eqPx * gl.iconSizeL * .8,
                        color: Colors.black,
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
      if (propertiesOpen)
        Card(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(12.0),
            side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
          ),
          surfaceTintColor: Colors.transparent,
          shadowColor: Colors.transparent,
          color: Colors.white.withAlpha(200),
          child: ListBody(
            children: [
              Column(
                children:
                    [
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .2,
                        child: Text(
                          "Nom",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .4,
                        child: TextButton(
                          style: lt.trNoPadButtonstyle,
                          onPressed: () {
                            setState(() {
                              PopupValueChange(
                                "string",
                                gl.layerReady ? gl.selLay.name : "",
                                (value) {
                                  setState(() {
                                    gl.selLay.name = value.toString();
                                  });
                                },
                                () {},
                                () {
                                  gl.geoLayers[gl.selectedGeoLayer].serialize();
                                },
                              );
                            });
                          },
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Icon(Icons.edit, size: gl.eqPx * gl.iconSizeXS, color: Colors.black),
                              Container(
                                alignment: Alignment.center,
                                width: gl.eqPx * 65,
                                child: SingleChildScrollView(
                                  scrollDirection: Axis.horizontal,
                                  child: Text(
                                    gl.layerReady ? gl.selLay.name : "",
                                    textAlign: TextAlign.center,
                                    style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM),
                                  ),
                                ),
                              ),
                              SizedBox(width: gl.eqPx * gl.iconSizeXS),
                            ],
                          ),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .2,
                        child: Text(
                          "Type",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .4,
                        child: Text(
                          gl.layerReady ? gl.selLay.type : "oho",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .2,
                        child: Text(
                          gl.layerReady && gl.selLay.labelsVisibleOnMap ? "Labels visible" : "Labels non visible",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Icon(Icons.edit, size: gl.eqPx * gl.iconSizeXS, color: Colors.black),
                          if (gl.layerReady)
                            IconButton(
                              style: lt.trNoPadButtonstyle,
                              onPressed: () {
                                setState(() {
                                  gl.selLay.labelsVisibleOnMap = !gl.selLay.labelsVisibleOnMap;
                                  for (Geometry g in gl.selLay.geometries) {
                                    g.labelsVisibleOnMap = gl.selLay.labelsVisibleOnMap;
                                  }
                                });
                              },
                              icon: Icon(
                                gl.selLay.labelsVisibleOnMap ? Icons.label_off_outlined : Icons.label_outline,
                                color: Colors.black,
                                size: gl.eqPx * gl.iconSizeM,
                              ),
                            ),
                          SizedBox(width: gl.eqPx * gl.iconSizeXS),
                        ],
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .2,
                        child: Text(
                          "Couleur",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Icon(Icons.edit, size: gl.eqPx * gl.iconSizeXS, color: Colors.black),
                          SelectPolyColor(
                            onAccept: (Color c) {
                              setState(() {
                                gl.selLay.defaultColor = c;
                                PopupDoYouReally(
                                  context,
                                  () {
                                    gl.refreshStack(() {
                                      for (Geometry g in gl.selLay.geometries) {
                                        g.colorInside = c;
                                        g.colorLine = c.withAlpha(255);
                                      }
                                    });
                                  },
                                  "Attention",
                                  "Voulez vous changer la couleur pour tous les entités de la layer?",
                                );
                              });
                            },
                            onDecline: (Color c) {
                              setState(() {
                                gl.selLay.defaultColor = c;
                              });
                            },
                            colorChanged: (Color c) {
                              setState(() {
                                gl.selLay.defaultColor = c;
                              });
                            },
                            currentColor: gl.layerReady ? gl.selLay.defaultColor : Colors.transparent,
                          ),
                          SizedBox(width: gl.eqPx * gl.iconSizeXS),
                        ],
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        padding: EdgeInsets.zero,
                        width: gl.eqPx * gl.onCatalogueWidth * .9,
                        height: gl.eqPx * gl.onCatalogueMapHeight * .2,
                        child: Text(
                          "Symbole",
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Icon(Icons.edit, size: gl.eqPx * gl.iconSizeXS, color: Colors.black),
                          if (gl.layerReady)
                            SelectLayerSymbol(
                              type: gl.selLay.type,
                              iconChanged: (int i) {
                                setState(() {
                                  gl.selLay.defaultPointIcon = i;
                                });
                                PopupDoYouReally(
                                  context,
                                  () {
                                    setState(() {
                                      for (Geometry g in gl.selLay.geometries) {
                                        g.selectedPointIcon = i;
                                      }
                                    });
                                  },
                                  "Attention",
                                  "Voulez vous changer le symbole pour tous les entités de la layer?",
                                );
                              },
                              current: gl.selLay.defaultPointIcon,
                            ),
                          SizedBox(width: gl.eqPx * gl.iconSizeXS),
                        ],
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      Container(
                        alignment: Alignment.center,
                        width: gl.eqPx * gl.iconSizeM,
                        height: gl.eqPx * gl.iconSizeM,
                        child: IconButton(
                          style: lt.trNoPadButtonstyle,
                          onPressed: () {
                            PopupDoYouReally(
                              gl.notificationContext!,
                              () {
                                setState(() {
                                  GeometricLayer.deleteLayer(gl.selectedGeoLayer);
                                  if (gl.selectedGeoLayer > -1) {
                                    gl.selectedGeoLayer--;
                                  }
                                });
                                widget.closePage();
                              },
                              "Message",
                              "\nVoulez vous vraiment supprimer ${gl.selLay.name}?\n",
                            );
                          },
                          icon: Icon(Icons.delete_forever, color: Colors.black, size: gl.eqPx * gl.iconSizeM),
                        ),
                      ),
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                    ] +
                    _getDefaultAttributes(),
              ),
            ],
          ),
        ),
    ];
  }

  Column _getFixedAttribute(String name, String values, {bool checked = false, bool noValues = false}) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            Container(
              alignment: Alignment.center,
              width: gl.eqPx * 10,
              child: Text("FIXED", style: TextStyle(color: Colors.black, fontSize: gl.fontSizeXXS * gl.eqPx)),
            ),
            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            SizedBox(
              width: gl.eqPx * 7,
              child:
                  checked
                      ? Icon(Icons.check_box, color: Colors.black, size: gl.eqPx * gl.iconSizeXS)
                      : Icon(Icons.check_box_outline_blank, color: Colors.black, size: gl.eqPx * gl.iconSizeXS),
            ),
            lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            Container(
              alignment: Alignment.centerLeft,
              width: gl.eqPx * (!noValues ? 32 : 64),
              child: Text(name, style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM * .75)),
            ),
            if (!noValues) lt.stroke(vertical: true, gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
            if (!noValues)
              Container(
                alignment: Alignment.centerLeft,
                width: gl.eqPx * 32,
                child: SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  child: Text(
                    values,
                    textAlign: TextAlign.center,
                    style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM * .75),
                  ),
                ),
              ),
          ],
        ),
        lt.stroke(gl.eqPx, gl.eqPx * 0.5, gl.colorAgroBioTech),
      ],
    );
  }
}

Widget switchRowColWithOrientation(List<Widget> tree, {MainAxisAlignment alignment = MainAxisAlignment.spaceAround}) {
  return gl.dsp.orientation == Orientation.portrait
      ? Column(mainAxisAlignment: alignment, children: tree)
      : Row(mainAxisAlignment: alignment, children: tree);
}

class SearchResultCard extends StatefulWidget {
  final Color boxColor;
  final ValueChanged<LatLng> state;
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
VoidCallback _revertStateOfSelectedSearchResultCard = () {};

class _SearchResultCard extends State<SearchResultCard> {
  @override
  Widget build(BuildContext context) {
    bool selected = _selectedSearchResultCard == widget.index ? true : false;
    return TextButton(
      onPressed: () {
        widget.state(widget.entry);
        gl.refreshStack(() {
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
                  : BorderSide(color: widget.boxColor.withAlpha(255), width: 1.0),
        ),
        color: selected ? widget.boxColor.withAlpha(255) : widget.boxColor.withAlpha(150),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Column(
              children: [
                Container(
                  constraints: BoxConstraints(maxWidth: gl.eqPx * (gl.popupWindowsPortraitWidth - 15)),
                  child: Text(
                    widget.typeDeResultat,
                    style: TextStyle(
                      color: lt.getColorTextFromBackground(widget.boxColor),
                      fontSize: gl.eqPx * gl.fontSizeS,
                    ),
                  ),
                ),

                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Container(
                      padding: EdgeInsets.all(5),
                      alignment: Alignment.center,
                      constraints: BoxConstraints(maxWidth: gl.eqPx * (gl.popupWindowsPortraitWidth - 15)),
                      child: Text(
                        widget.descriptionDeResultat,
                        textAlign: TextAlign.justify,
                        style: TextStyle(
                          color: lt.getColorTextFromBackground(widget.boxColor),
                          fontSize: gl.eqPx * gl.fontSizeS,
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
  final ValueChanged<LatLng> moveToPoint;

  const SearchMenu({super.key, required this.moveToPoint});

  @override
  State<StatefulWidget> createState() => _SearchMenu();
}

class _SearchMenu extends State<SearchMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  final Duration animationDuration = Duration(seconds: 1);

  static String lastSearchKey = "";
  static Map<String, http.Response> searchCache = {};
  static List<SearchResultCard> searchResults = [];

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (c, o) {
        return AnimatedContainer(
          duration: Duration(milliseconds: 100),
          width:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitWidth
                  : gl.eqPx * gl.popupWindowsLandscapeWidth,
          height:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitHeight + 1
                  : gl.eqPx * gl.popupWindowsLandscapeHeight,
          child: switchRowColWithOrientation([
            if (gl.dsp.orientation == Orientation.landscape)
              Container(
                constraints: BoxConstraints(
                  maxHeight: (gl.popupWindowsLandscapeHeight - 5) * gl.eqPx,
                  maxWidth: gl.popupWindowsPortraitWidth * gl.eqPx,
                ),
                child: ListView(children: <Widget>[] + searchResults),
              ),
            Column(
              mainAxisAlignment: MainAxisAlignment.spaceAround,
              children: [
                Column(
                  children: [
                    SizedBox(
                      height: gl.eqPx * gl.searchBarHeight,
                      width: gl.eqPx * gl.searchBarWidth,
                      child: Card(
                        child: TextFormField(
                          decoration: InputDecoration(
                            border: InputBorder.none,
                            hintText: "Entrez le nom d'un lieu",
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
                              (decodedJson = (jsonDecode(response.body) as List).cast<Map<String, dynamic>>());
                            } catch (e) {
                              gl.print("Error with response from gecoding service! $e");
                              (decodedJson =
                                  (jsonDecode(testNominatimJsonResult) as List).cast<Map<String, dynamic>>());
                            }
                            gl.poiMarkerList.clear();
                            searchResults.clear();
                            _selectedSearchResultCard = -1;
                            mounted
                                ? setState(() {
                                  gl.selectedSearchMarker = -1;
                                  int i = 0;
                                  for (var entry in decodedJson) {
                                    String? typeDeResultat = prettyPrintNominatimResults[entry['addresstype']];
                                    if (typeDeResultat == null) {
                                      typeDeResultat = entry['addresstype'];
                                      gl.print("Error: not a translated addresstype: ${entry['addresstype']}");
                                    }
                                    String? descriptionDeResultat = entry['display_name'];
                                    if (descriptionDeResultat == null) {
                                      descriptionDeResultat = "Erreur du serveur";
                                      gl.print("Erreur du serveur geocoding : ${entry['display_name']}");
                                    } else {
                                      descriptionDeResultat = descriptionDeResultat.replaceAll(", België /", "");
                                      descriptionDeResultat = descriptionDeResultat.replaceAll("/ Belgien", "");
                                      descriptionDeResultat = descriptionDeResultat.replaceAll("Wallonie, ", "");
                                      descriptionDeResultat = descriptionDeResultat.replaceAll("Belgique", "");
                                    }
                                    gl.poiMarkerList.add(
                                      gl.PoiMarker(
                                        index: i++,
                                        position: LatLng(double.parse(entry['lat']), double.parse(entry['lon'])),
                                        name: typeDeResultat!,
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
                                      SearchResultCard(
                                        boxColor: lt.getColorFromName(typeDeResultat),
                                        state: widget.moveToPoint,
                                        typeDeResultat: typeDeResultat,
                                        entry: LatLng(double.parse(entry['lat']), double.parse(entry['lon'])),
                                        descriptionDeResultat: descriptionDeResultat,
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
                                      String? typeDeResultat = prettyPrintNominatimResults[entry['addresstype']];
                                      if (typeDeResultat == null) {
                                        typeDeResultat = entry['addresstype'];
                                        gl.print("Error: not a translated addresstype: ${entry['addresstype']}");
                                      }
                                      String? descriptionDeResultat = entry['display_name'];
                                      if (descriptionDeResultat == null) {
                                        descriptionDeResultat = "Erreur du serveur";
                                        gl.print("Erreur du serveur geocoding : ${entry['display_name']}");
                                      } else {
                                        descriptionDeResultat = descriptionDeResultat.replaceAll(", België /", "");
                                        descriptionDeResultat = descriptionDeResultat.replaceAll("/ Belgien", "");
                                        descriptionDeResultat = descriptionDeResultat.replaceAll("Wallonie, ", "");
                                        descriptionDeResultat = descriptionDeResultat.replaceAll("Belgique", "");
                                      }
                                      Color boxColor = lt.getColorFromName(typeDeResultat!);
                                      gl.poiMarkerList.add(
                                        gl.PoiMarker(
                                          index: i++,
                                          position: LatLng(double.parse(entry['lat']), double.parse(entry['lon'])),
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
                                        SearchResultCard(
                                          boxColor: boxColor,
                                          state: widget.moveToPoint,
                                          typeDeResultat: typeDeResultat,
                                          entry: LatLng(double.parse(entry['lat']), double.parse(entry['lon'])),
                                          descriptionDeResultat: descriptionDeResultat,
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
              ],
            ),
            if (gl.dsp.orientation == Orientation.portrait)
              SizedBox(
                height:
                    (gl.popupWindowsPortraitHeight -
                        gl.searchBarHeight -
                        gl.fontSizeL * 1.1 -
                        gl.popupReturnButtonHeight) *
                    gl.eqPx,
                child: ListView(children: <Widget>[] + searchResults),
              ),
          ]),
        );
      },
    );
  }
}

int developperModeCounter = 0;
Widget forestimatorSettingsVersion(VoidSetter state) {
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
                  constraints: BoxConstraints(maxWidth: gl.eqPx * 45),
                  child: TextButton(
                    onPressed: () async {
                      developperModeCounter++;
                      if (developperModeCounter > 6) {
                        state(() {
                          gl.modeDevelopper = !gl.modeDevelopper;
                        });
                        gl.shared!.setBool('modeDevelopper', gl.modeDevelopper);
                        developperModeCounter = 0;
                      }
                    },
                    child: Image.asset("assets/images/LogoForestimatorWhiteAlpha.png"),
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
                  constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
                  constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .7),
                  child: Text(
                    "Le développement est financé par l'Accord Cadre de Recherches et Vulgarisation Forestières.\nLe contenu cartographique est en grande partie issu des recherches menées au sein de l'unité de Gestion des Ressources Forestières de Gembloux Agro-Bio Tech (ULiège).\n",
                    textAlign: TextAlign.justify,
                    style: TextStyle(color: Colors.black, overflow: TextOverflow.fade),
                  ),
                ),
              ],
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Container(
                  constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .7),
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

class ForestimatorSettingsUserData extends StatefulWidget {
  final VoidCallback onChanged;
  const ForestimatorSettingsUserData({super.key, required this.onChanged});

  @override
  State<StatefulWidget> createState() => _ForestimatorSettingsUserData();
}

class _ForestimatorSettingsUserData extends State<ForestimatorSettingsUserData> {
  _ForestimatorSettingsUserData();
  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        return Container(
          padding: EdgeInsets.all(7.5),
          child: Column(
            children: [
              lt.stroke(vertical: false, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              Row(
                children: [
                  SizedBox(
                    width: gl.eqPx * 40,
                    child: Text("Prénom", style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS)),
                  ),
                  lt.stroke(vertical: true, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                  Container(
                    alignment: Alignment.center,
                    width: gl.eqPx * 40,
                    height: gl.eqPx * gl.iconSizeXS,
                    child: TextButton(
                      style: ButtonStyle(
                        animationDuration: Duration(seconds: 1),
                        backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                          WidgetState.any: Colors.transparent,
                        }),
                        padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                          <WidgetStatesConstraint, EdgeInsetsGeometry>{WidgetState.any: EdgeInsetsGeometry.zero},
                        ),
                      ),
                      onPressed: () {},
                      onLongPress: () {
                        PopupValueChange(
                          "string",
                          gl.UserData.forename,
                          (value) {
                            setState(() {
                              gl.UserData.forename = value;
                              gl.Mode.userDataFilled = gl.UserData.validUserData();
                              gl.Mode.serialize();
                            });
                            widget.onChanged();
                          },
                          () {},
                          () {
                            gl.UserData.serialize();
                          },
                        );
                      },
                      child: Container(
                        alignment: Alignment.centerLeft,
                        width: gl.eqPx * 40,
                        child: SingleChildScrollView(
                          scrollDirection: Axis.horizontal,
                          child: Text(
                            gl.UserData.forename,
                            style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
              lt.stroke(vertical: false, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              Row(
                children: [
                  SizedBox(
                    width: gl.eqPx * 40,
                    child: Text("Nom", style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS)),
                  ),
                  lt.stroke(vertical: true, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                  Container(
                    alignment: Alignment.center,
                    width: gl.eqPx * 40,
                    height: gl.eqPx * gl.iconSizeXS,
                    child: TextButton(
                      style: ButtonStyle(
                        animationDuration: Duration(seconds: 1),
                        backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                          WidgetState.any: Colors.transparent,
                        }),
                        padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                          <WidgetStatesConstraint, EdgeInsetsGeometry>{WidgetState.any: EdgeInsetsGeometry.zero},
                        ),
                      ),
                      onPressed: () {},
                      onLongPress: () {
                        PopupValueChange(
                          "string",
                          gl.UserData.name,
                          (value) {
                            setState(() {
                              gl.UserData.name = value;
                              gl.Mode.userDataFilled = gl.UserData.validUserData();
                              gl.Mode.serialize();
                            });
                            widget.onChanged();
                          },
                          () {},
                          () {
                            gl.UserData.serialize();
                          },
                        );
                      },
                      child: Container(
                        alignment: Alignment.centerLeft,
                        width: gl.eqPx * 40,
                        child: SingleChildScrollView(
                          scrollDirection: Axis.horizontal,
                          child: Text(
                            gl.UserData.name,
                            style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
              lt.stroke(vertical: false, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              Row(
                children: [
                  SizedBox(
                    width: gl.eqPx * 40,
                    child: Text("Mail", style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS)),
                  ),
                  lt.stroke(vertical: true, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                  Container(
                    alignment: Alignment.center,
                    width: gl.eqPx * 40,
                    height: gl.eqPx * gl.iconSizeXS,
                    child: TextButton(
                      style: ButtonStyle(
                        animationDuration: Duration(seconds: 1),
                        backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                          WidgetState.any: Colors.transparent,
                        }),
                        padding: WidgetStateProperty<EdgeInsetsGeometry>.fromMap(
                          <WidgetStatesConstraint, EdgeInsetsGeometry>{WidgetState.any: EdgeInsetsGeometry.zero},
                        ),
                      ),
                      onPressed: () {},
                      onLongPress: () {
                        PopupValueChange(
                          "mail",
                          gl.UserData.mail,
                          (value) {
                            setState(() {
                              gl.UserData.mail = value;
                              gl.Mode.userDataFilled = gl.UserData.validUserData();
                              gl.Mode.serialize();
                            });
                            widget.onChanged();
                          },
                          () {},
                          () {
                            gl.UserData.serialize();
                          },
                        );
                      },
                      child: Container(
                        alignment: Alignment.centerLeft,
                        width: gl.eqPx * 40,
                        child: SingleChildScrollView(
                          scrollDirection: Axis.horizontal,
                          child: Text(
                            gl.UserData.mail,
                            style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
              lt.stroke(vertical: false, gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
              variableBooleanSlider("Mode Essence", gl.Mode.essence, (bool it) {
                if (gl.Mode.userDataFilled) {
                  setState(() {
                    gl.Mode.essence = it;
                  });
                } else {
                  PopupUserData(
                    context,
                    () {},
                    () {
                      setState(() {
                        gl.Mode.essence = it;
                      });
                    },
                    oldForename: gl.UserData.forename,
                    oldName: gl.UserData.name,
                    oldMail: gl.UserData.mail,
                  );
                }
                gl.refreshStack(() {});
                gl.Mode.serialize();
              }, false),
            ],
          ),
        );
      },
    );
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
                  style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS, fontWeight: FontWeight.w400),
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
                    style: TextStyle(color: Colors.blue, fontSize: gl.eqPx * gl.fontSizeS, fontWeight: FontWeight.w400),
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
                  style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS, fontWeight: FontWeight.w400),
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
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: gl.eqPx * gl.fontSizeS,
                      fontWeight: FontWeight.w400,
                    ),
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

class ForestimatorVariables extends StatefulWidget {
  const ForestimatorVariables({super.key});

  @override
  State<StatefulWidget> createState() => _ForestimatorVariables();
}

class _ForestimatorVariables extends State<ForestimatorVariables> {
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        variableBooleanSlider("Expert Mode", gl.Mode.expert, (bool it) {
          setState(() {
            gl.Mode.expert = it;
          });
          gl.refreshStack(() {});
          gl.Mode.serialize();
        }, false),
        variableBooleanSlider("Gridlines", gl.Mode.debugScanlines, (bool it) {
          setState(() {
            gl.Mode.debugScanlines = it;
          });
          gl.refreshStack(() {});
        }, false),
        variableBooleanSlider("Experimental Tools", gl.Mode.expertTools, (bool it) {
          setState(() {
            gl.Mode.expertTools = it;
          });
          gl.refreshStack(() {});
        }, true),
        variableBooleanSlider("Deactivate Polygon Well Defined Check", gl.Mode.overrideWellDefinedCheck, (bool it) {
          setState(() {
            gl.Mode.overrideWellDefinedCheck = it;
          });
          gl.refreshStack(() {});
          gl.Mode.serialize();
        }, true),
        variableBooleanSlider("Tablet Mode", gl.Mode.overrideModeTablet, (bool it) {
          setState(() {
            gl.Mode.overrideModeTablet = it;
          });
          gl.refreshStack(() {});
        }, true),
        variableBooleanSlider("Square Mode", gl.Mode.overrideModeSquare, (bool it) {
          setState(() {
            gl.Mode.overrideModeSquare = it;
          });
          gl.refreshStack(() {});
        }, true),
        variableBooleanSlider("Petit Label", gl.Mode.smallLabel, (bool it) {
          setState(() {
            gl.Mode.smallLabel = it;
            gl.Mode.labelCross = !it;
          });
          gl.refreshStack(() {});
          gl.Mode.serialize();
        }, false),
      ],
    );
  }
}

Widget variableBooleanSlider(String description, bool boolean, ValueChanged<bool> changed, bool dangerousToPlayWith) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
    children: [
      Container(
        width: gl.eqPx * 55,
        alignment: AlignmentGeometry.centerLeft,
        child: Row(
          children: [
            dangerousToPlayWith
                ? Icon(Icons.warning, color: Colors.yellow, size: gl.eqPx * 6)
                : SizedBox(width: gl.eqPx * 6),
            SizedBox(width: gl.eqPx * 4),
            SizedBox(width: gl.eqPx * 45, child: Text(description)),
          ],
        ),
      ),
      Container(alignment: AlignmentGeometry.center, width: gl.eqPx * 10),
      Container(
        alignment: AlignmentGeometry.center,
        width: gl.eqPx * 10,
        child: Switch(
          value: boolean,
          onChanged: (bool value) {
            changed(value);
          },
        ),
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
  int lengthLog = 10;

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        return Column(
          children:
              <Widget>[
                Container(
                  constraints: BoxConstraints(minWidth: gl.eqPx * 50),
                  child: FloatingActionButton(
                    onPressed: () {
                      setState(() {
                        lengthLog + 5 < gl.onboardLog.length
                            ? lengthLog = lengthLog + 5
                            : lengthLog = gl.onboardLog.length;
                      });
                    },
                    child: lengthLog != gl.onboardLog.length ? Text("Afficher +") : Text("FIN"),
                  ),
                ),
              ] +
              List<Widget>.generate(lengthLog, (i) {
                return gl.onboardLog.length - lengthLog > 0
                    ? Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              constraints: BoxConstraints(
                                minWidth: gl.eqPx * "${gl.onboardLog.length - lengthLog + i})".length * 2,
                              ),
                              child: Text(
                                "${gl.onboardLog.length - lengthLog + i})",
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize: gl.eqPx * gl.fontSizeXS,
                                ),
                              ),
                            ),
                            Container(
                              constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .8),
                              child: Text(
                                gl.onboardLog[gl.onboardLog.length - lengthLog + i],
                                style: TextStyle(
                                  fontWeight: FontWeight.w500,
                                  color: Colors.black,
                                  fontSize: gl.eqPx * gl.fontSizeXS,
                                ),
                              ),
                            ),
                          ],
                        ),
                        if (lengthLog != i + 1) lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                      ],
                    )
                    : gl.onboardLog.length - i > 0
                    ? Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              constraints: BoxConstraints(minWidth: gl.eqPx * "$i)".length * 2),
                              child: Text(
                                "$i)",
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize: gl.eqPx * gl.fontSizeXS,
                                ),
                              ),
                            ),
                            Container(
                              constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .7),
                              child: Text(gl.onboardLog[i]),
                            ),
                          ],
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
  return TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeM);
}

Widget forestimatorSettingsPermissions(VoidSetter state) {
  return Container(
    padding: EdgeInsets.all(7.5),
    constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * 0.95),
    child: Column(
      children: [
        Container(
          alignment: Alignment.center,
          child: Text(
            "Gestion des permissions",
            overflow: TextOverflow.clip,
            textAlign: TextAlign.left,
            style: styleSettingMenu(),
          ),
        ),
        lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
        Row(
          mainAxisSize: MainAxisSize.min,
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Text("GPS: ", overflow: TextOverflow.clip, textAlign: TextAlign.left, style: styleSettingMenu()),
            TextButton(
              onPressed: () {
                openPhoneForestimatorSettings();
              },
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    getLocation() ? Icons.check_circle : Icons.circle_notifications,
                    color: getLocation() ? Colors.green : Colors.red,
                    size: gl.eqPx * gl.iconSizeM * .6,
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
          Container(
            constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * 0.95),
            child: Row(
              mainAxisSize: MainAxisSize.min,
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
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        getStorage() ? Icons.check_circle : Icons.circle_notifications,
                        color: getStorage() ? Colors.green : Colors.red,
                        size: gl.eqPx * gl.iconSizeM * .6,
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
                padding: EdgeInsets.all(10),
                constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .9),
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
                  launchURL('https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_');
                },
                child: Container(
                  padding: EdgeInsets.all(5),
                  constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .8),
                  child: Text(
                    "https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_",
                    overflow: TextOverflow.clip,
                    textAlign: TextAlign.left,
                    style: TextStyle(color: Colors.blue, fontSize: gl.eqPx * gl.fontSizeS),
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
                constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .9),
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
  String? mCode;
  int? mRastValue;

  bool isExpanded = true;

  Item({required this.entry, required this.name, this.mCode, this.mRastValue});
}

class ItemSettings {
  final Widget entry;
  final String name;

  bool isExpanded = false;

  ItemSettings({required this.entry, required this.name});
}

class SettingsMenu extends StatefulWidget {
  const SettingsMenu({super.key});

  @override
  State<StatefulWidget> createState() => _SettingsMenu();
}

class _SettingsMenu extends State<SettingsMenu> {
  final Color active = Colors.black;
  final Color inactive = Colors.blueGrey;
  final List<ItemSettings> menuItems = [];
  static final List<bool> openedItem = [];

  @override
  void initState() {
    if (openedItem.length < 20) {
      for (int i = 20; i > openedItem.length; openedItem.add(true)) {}
    }
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        return AnimatedContainer(
          duration: Duration(milliseconds: 100),
          width:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitWidth
                  : gl.eqPx * gl.popupWindowsLandscapeWidth,
          height:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitHeight + 1
                  : gl.eqPx * gl.popupWindowsLandscapeHeight,
          child: switchRowColWithOrientation([
            Column(
              children: [
                SizedBox(
                  height:
                      gl.dsp.orientation == Orientation.portrait
                          ? (gl.popupWindowsPortraitHeight - gl.searchBarHeight - gl.popupReturnButtonHeight) * gl.eqPx
                          : (gl.popupWindowsLandscapeHeight - gl.searchBarHeight) * gl.eqPx,
                  width: gl.popupWindowsPortraitWidth * gl.eqPx,
                  child: ListView(
                    padding: const EdgeInsets.symmetric(horizontal: 0),
                    children: _injectMenuEntries((int i, ItemSettings item) {
                      return Container(
                        alignment: Alignment.center,
                        child:
                            openedItem[i]
                                ? Card(
                                  shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                                  surfaceTintColor: Colors.transparent,
                                  shadowColor: Colors.transparent,
                                  color: gl.colorAgroBioTech.withAlpha(75),
                                  child: Card(
                                    shape: RoundedRectangleBorder(
                                      borderRadius: BorderRadiusGeometry.circular(12.0),
                                      side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
                                    ),
                                    surfaceTintColor: Colors.transparent,
                                    shadowColor: Colors.transparent,
                                    color: Colors.white.withAlpha(200),
                                    child: Column(
                                      children: [
                                        Column(
                                          children: [
                                            TextButton(
                                              key: Key('$i'),
                                              onPressed: () {
                                                setState(() {
                                                  openedItem[i] = !openedItem[i];
                                                });
                                              },
                                              child: Container(
                                                alignment: Alignment.center,
                                                constraints: BoxConstraints(
                                                  minWidth: gl.eqPx * gl.onCatalogueWidth,
                                                  minHeight: gl.eqPx * gl.onCatalogueMapHeight,
                                                ),
                                                child: Text(
                                                  item.name,
                                                  textAlign: TextAlign.center,
                                                  style: TextStyle(
                                                    color: Colors.black,
                                                    fontWeight: FontWeight.w400,
                                                    fontSize: gl.eqPx * gl.fontSizeM,
                                                  ),
                                                ),
                                              ),
                                            ),
                                            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                                            item.entry,
                                          ],
                                        ),
                                      ],
                                    ),
                                  ),
                                )
                                : TextButton(
                                  key: Key('$i'),
                                  onPressed: () {
                                    setState(() {
                                      openedItem[i] = !openedItem[i];
                                    });
                                  },
                                  child: Card(
                                    color: gl.colorAgroBioTech.withAlpha(200),
                                    child: Container(
                                      alignment: Alignment.center,
                                      constraints: BoxConstraints(
                                        minWidth: gl.eqPx * gl.onCatalogueWidth * .95,
                                        minHeight: gl.eqPx * gl.onCatalogueMapHeight * .95,
                                      ),
                                      child: Text(
                                        item.name,
                                        textAlign: TextAlign.center,
                                        style: TextStyle(
                                          color: Colors.black,
                                          fontWeight: FontWeight.w400,
                                          fontSize: gl.eqPx * gl.fontSizeM,
                                        ),
                                      ),
                                    ),
                                  ),
                                ),
                      );
                    }),
                  ),
                ),
              ],
            ),
          ]),
        );
      },
    );
  }

  List<Widget> _injectMenuEntries(Widget Function(int, ItemSettings) generate) {
    final List<ItemSettings> menuItems = [];
    menuItems.addAll([
      ItemSettings(
        name: "Permissions",
        entry: forestimatorSettingsPermissions((void Function() f) {
          setState(() {
            f();
          });
        }),
      ),
      ItemSettings(
        name: "Données Utilisateur",
        entry: ForestimatorSettingsUserData(
          onChanged: () {
            setState(() {});
          },
        ),
      ),
      ItemSettings(
        name: "À propos de Forestimator",
        entry: forestimatorSettingsVersion((void Function() f) {
          setState(() {
            f();
            if (gl.modeDevelopper) {
              menuItems.add(ItemSettings(name: "Debug Logs", entry: ForestimatorLog()));
              menuItems.add(ItemSettings(name: "Variables", entry: ForestimatorVariables()));
            } else {
              menuItems.removeWhere((item) => item.name == "Debug Logs" ? true : false);
              menuItems.removeWhere((item) => item.name == "Variables" ? true : false);
            }
            gl.modeDevelopper ? gl.print("Developper mode activated") : gl.print("Developper mode deactivated");
          });
        }),
      ),
      ItemSettings(name: "Contact", entry: forestimatorSettingsContacts()),
      ItemSettings(name: "Confidentialité", entry: forestimatorConfidentiality()),
      if (gl.modeDevelopper) ItemSettings(name: "Debug Logs", entry: ForestimatorLog()),
      if (gl.modeDevelopper) ItemSettings(name: "Variables", entry: ForestimatorVariables()),
    ]);
    return List<Widget>.generate(menuItems.length, (i) {
      return generate(i, menuItems[i]);
    });
  }
}

Widget popupLayerListMenu(BuildContext context, String currentName, ValueChanged<LatLng> mapmove, VoidCallback after) {
  return GeoLayerListMenu(
    mapmove: mapmove,
    after: after,
    windowHeight: gl.dsp.orientation == Orientation.portrait ? (gl.eqPxH - 25) : gl.popupWindowsLandscapeHeight,
  );
}

Widget _resultRow(String key, String value) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceBetween,
    children: [
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .59),

        child: Text(
          key,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.left,
          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS * .9, fontWeight: FontWeight.w300),
        ),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(minWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .25),

        child: Text(
          value,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.left,
          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS * .9, fontWeight: FontWeight.w500),
        ),
      ),
    ],
  );
}

Widget _resultClassRow(Map<String, dynamic> json, mCode) {
  Color col = Colors.transparent;
  int key;
  try {
    key = gl.dico.mLayerBases[mCode]!.mDicoCol.keys.elementAt(json['rastValue']);
  } catch (e) {
    key = -1234567891011;
  }
  if (key != -1234567891011) {
    if (key > 0) key--;
    col = gl.dico.mLayerBases[mCode]!.mDicoCol[key]!;
  }
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceBetween,
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            color: Colors.black,
            padding: EdgeInsets.all(1),
            constraints: BoxConstraints(minHeight: gl.eqPx * 5, minWidth: gl.eqPx * 5),
            child: Container(color: col),
          ),
          Container(
            padding: EdgeInsets.only(left: 10),
            constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .5),
            child: Text(
              json['value'].toString(),
              overflow: TextOverflow.clip,
              textAlign: TextAlign.start,
              style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS * .9, fontWeight: FontWeight.w300),
            ),
          ),
        ],
      ),

      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(minWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .25),
        child: Text(
          "${json['prop'].toString()}%",
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS * .9, fontWeight: FontWeight.w500),
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
            constraints: BoxConstraints(maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth * .5),
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

Widget forestimatorResultsHeaderContinue(Map<String, dynamic> json, String layerCode) {
  return Column(
    children: List<Widget>.generate(json.length, (i) {
      if (!(i == 0 ||
          prettyPrintContinousResults[layerCode] == null ||
          prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)] == null)) {
        gl.print("Error printing results: ${prettyPrintContinousResults[layerCode]}");
      }
      return i == 0 ||
              prettyPrintContinousResults[layerCode] == null ||
              prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)] == null
          ? Container()
          : _resultRow(
            "${prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)]}:",
            json[json.keys.elementAt(i)].toString(),
          );
    }),
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
        if (widget.layerTile.downloadable) lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
        gl.anaSurfSelectedLayerKeys.contains(widget.layerTile.key)
            ? TextButton(
              style: ButtonStyle(
                minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
                }),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(Icons.pentagon, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                  Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                  Container(
                    constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
                gl.shared!.setStringList('anaSurfSelectedLayerKeys', gl.anaSurfSelectedLayerKeys);
              },
            )
            : TextButton(
              style: ButtonStyle(
                minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
                }),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(Icons.pentagon_outlined, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                  Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                  Container(
                    constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
                gl.shared!.setStringList('anaSurfSelectedLayerKeys', gl.anaSurfSelectedLayerKeys);
              },
            ),
        lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
        gl.anaPtSelectedLayerKeys.contains(widget.layerTile.key)
            ? TextButton(
              style: ButtonStyle(
                minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
                }),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(Icons.location_on, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                  Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                  Container(
                    constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
                gl.shared!.setStringList('anaPtSelectedLayerKeys', gl.anaPtSelectedLayerKeys);
              },
            )
            : TextButton(
              style: ButtonStyle(
                minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                  WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
                }),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.start,
                children: [
                  Icon(Icons.location_off, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                  Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                  Container(
                    constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
                gl.shared!.setStringList('anaPtSelectedLayerKeys', gl.anaPtSelectedLayerKeys);
              },
            ),
        if (gl.dico.getLayerBase(widget.layerTile.key).hasDoc()) lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
        if (gl.dico.getLayerBase(widget.layerTile.key).hasDoc())
          TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
              }),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Icon(Icons.picture_as_pdf, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                Container(
                  constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
                  child: Text(
                    "Consulter la documentation relative à cette couche cartographique",
                    style: TextStyle(color: Colors.black),
                  ),
                ),
              ],
            ),
            onPressed: () {
              PopupPdfMenu(widget.layerTile.key);
            },
          ),
        if ((gl.dico.getLayerBase(widget.layerTile.key).mGroupe == "APT_FEE" ||
                gl.dico.getLayerBase(widget.layerTile.key).mGroupe == "APT_CS") &&
            gl.dico.getEss(gl.dico.getLayerBase(widget.layerTile.key).getEssCode()).hasFEEapt())
          TextButton(
            style: ButtonStyle(
              minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(gl.eqPx * gl.onCatalogueWidth * .98, gl.eqPx * 15),
              }),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Icon(Icons.picture_as_pdf_outlined, size: gl.eqPx * gl.onCatalogueIconSize, color: Colors.black),
                Container(constraints: BoxConstraints(maxWidth: gl.eqPx * 5)),
                Container(
                  constraints: BoxConstraints(maxWidth: gl.eqPx * 60),
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
              PopupPdfMenu(widget.layerTile.key, path: path);
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
  final VoidSetter state;
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
  static final Map<int, VoidCallback> _layerSelectionSetStates = {-1: () {}, 0: () {}, 1: () {}, 2: () {}};

  void _callSelectedButtonsSetStates() {
    for (final VoidCallback function in _layerSelectionSetStates.values) {
      function();
    }
    gl.refreshStack(() {});
    gl.rebuildSwitcherBox(() {});
  }

  @override
  Widget build(BuildContext context) {
    int interfaceSelectedMapKey = gl.getIndexForLayer(widget.layerTile.key, widget.offlineMode);
    _layerSelectionSetStates[interfaceSelectedMapKey] = () {
      if (mounted) setState(() {});
    };
    int interfaceSelectedMapSwitcherSlot = widget.selectionMode;
    if (interfaceSelectedMapKey == -1) {
      return TextButton(
        style: ButtonStyle(
          backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
            WidgetState.any: Colors.transparent,
          }),
          minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
            WidgetState.any: Size(
              gl.eqPx * gl.onCatalogueLayerSelectionButton,
              gl.eqPx * gl.onCatalogueLayerSelectionButton,
            ),
          }),
        ),
        onPressed: () {
          if (!widget.offlineMode) {
            if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, false) != -1) {
              setState(() {
                int index = gl.sameOnlineAsOfflineLayer(widget.layerTile.key, false);
                gl.removeLayerFromList(index: index, offline: true);
                gl.replaceLayerFromList(widget.layerTile.key, index: index, offline: false);
              });
            } else {
              setState(() {
                gl.replaceLayerFromList(widget.layerTile.key, index: interfaceSelectedMapSwitcherSlot, offline: false);
              });
            }
          } else {
            if (gl.getCountOfflineLayerSelected() == 0) {
              if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true) != -1) {
                setState(() {
                  int index = gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true);
                  gl.removeLayerFromList(index: index, offline: false);
                  gl.replaceLayerFromList(widget.layerTile.key, index: index, offline: true);
                });
              } else {
                setState(() {
                  gl.replaceLayerFromList(widget.layerTile.key, index: interfaceSelectedMapSwitcherSlot, offline: true);
                });
              }
            } else if (gl.getCountOfflineLayerSelected() == 1) {
              if (gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true) != -1) {
                setState(() {
                  int index = gl.getIndexForNextLayerOffline();
                  gl.replaceLayerFromList(gl.selectedLayerForMap[index].mCode, index: index, offline: false);
                  index = gl.sameOnlineAsOfflineLayer(widget.layerTile.key, true);
                  gl.removeLayerFromList(index: index, offline: false);
                  gl.replaceLayerFromList(widget.layerTile.key, index: index, offline: true);
                });
              } else {
                setState(() {
                  int index = gl.getIndexForNextLayerOffline();
                  gl.removeLayerFromList(index: index, offline: true);
                  gl.replaceLayerFromList(widget.layerTile.key, index: index, offline: true);
                });
              }
            }
          }
          gl.refreshStack(() {});
          _callSelectedButtonsSetStates();
        },
        child: Icon(Icons.layers, size: gl.eqPx * gl.onCatalogueLayerSelectionButton, color: Colors.black),
      );
    } else {
      return TextButton(
        style: ButtonStyle(
          backgroundColor: WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
            WidgetState.any: gl.colorAgroBioTech,
          }),
          minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
            WidgetState.any: Size(
              gl.eqPx * gl.onCatalogueLayerSelectionButton,
              gl.eqPx * gl.onCatalogueLayerSelectionButton,
            ),
          }),
        ),
        onPressed: () {
          setState(() {
            gl.slotContainsLayer(interfaceSelectedMapKey, widget.layerTile.key)
                ? gl.removeLayerFromList(index: interfaceSelectedMapKey, offline: gl.offlineMode)
                : {
                  gl.replaceLayerFromList(
                    gl.selectedLayerForMap[interfaceSelectedMapKey].mCode,
                    index: gl.getIndexForLayer(widget.layerTile.key, widget.offlineMode),
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
            style: TextStyle(fontSize: gl.eqPx * gl.fontSizeM, color: Colors.black),
          ),
        ),
      );
    }
  }
}

class OnlineMapMenu extends StatefulWidget {
  final VoidSetter? stateOfLayerSwitcher;
  final bool offlineMode;
  final int selectionMode;
  final VoidCallback after;
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

  void scrollToPoint(double more) {
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
    gl.resetSelected = resetSelected;
    if (gl.firstTimeUse) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        popupForestimatorMessage(
          id: "DLrecomended",
          title: "Bienvenu",
          message:
              "Autorisez-vous l'aplication à télécharger un jeu de 6 couches pour une utilisation hors ligne? Ces couches couvrent toutes la Région Wallonne et totalisent +- 214 Mo.",
          messageAccept: "Oui",
          onAccept: () async {
            gl.refreshStack(() {
              gl.firstTimeUse = false;
            });
            gl.shared!.setBool('firstTimeUse', gl.firstTimeUse);
            for (var key in gl.downloadableLayerKeys) {
              downloadLayer(key);
            }
            gl.refreshStack(() {
              gl.stack.pop("DLrecomended");
            });
          },
          messageDecline: "Non",
          onDecline: () async {
            gl.refreshStack(() {
              gl.firstTimeUse = false;
            });
            gl.shared!.setBool('firstTimeUse', gl.firstTimeUse);
            gl.refreshStack(() {
              gl.stack.pop("DLrecomended");
            });
          },
        );
      });
    }
  }

  static void resetSelected(bool category, bool layer) {
    if (category) selectedCategory = -1;
    if (layer) selectedMap = -1;
  }

  @override
  Widget build(BuildContext context) {
    VoidSetter stateOfLayerSwitcher;
    if (widget.stateOfLayerSwitcher == null) {
      stateOfLayerSwitcher = (void Function() f) {
        f();
      };
    } else {
      stateOfLayerSwitcher = widget.stateOfLayerSwitcher!;
    }
    gl.rebuildOfflineCatalogue = (void Function() f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
    };
    return OrientationBuilder(
      builder: (c, o) {
        return SizedBox(
          width:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitWidth
                  : gl.eqPx * gl.popupWindowsLandscapeWidth,
          height:
              gl.dsp.orientation == Orientation.portrait
                  ? gl.eqPx * gl.popupWindowsPortraitHeight
                  : gl.eqPx * gl.popupWindowsLandscapeHeight,
          child: switchRowColWithOrientation([
            Column(
              mainAxisAlignment: MainAxisAlignment.spaceAround,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    SizedBox(
                      height: gl.eqPx * gl.searchBarHeight,
                      width: gl.eqPx * gl.searchBarWidth,
                      child: Card(
                        child: TextFormField(
                          decoration: InputDecoration(
                            border: InputBorder.none,
                            hintText: "Entrez le nom d'une couche",
                            contentPadding: EdgeInsets.zero,
                            prefixIcon: Icon(Icons.search, color: Colors.black),
                          ),
                          onTap: () {
                            setState(() {});
                          },
                          textAlign: TextAlign.start,
                          autocorrect: false,
                          enableSuggestions: true,
                          onChanged: (String value) {
                            selectedMap = -1;
                            selectedLayerTile = null;
                            selectedCategory = -1;
                            _resultOfMapSearch.clear();
                            if (value.isNotEmpty) {
                              for (String term in value.split(' ')) {
                                if (term != '') {
                                  for (var layer in gl.dico.mLayerBases.values) {
                                    if ((!layer.mExpert || gl.Mode.expert) &&
                                        (widget.offlineMode ? layer.mOffline : true) &&
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
                                selectedMap = -1;
                                selectedLayerTile = null;
                                selectedCategory = -1;
                              });
                            }
                          },
                        ),
                      ),
                    ),
                  ],
                ),

                SizedBox(
                  height:
                      gl.dsp.orientation == Orientation.portrait
                          ? (gl.popupWindowsPortraitHeight - gl.searchBarHeight - gl.popupReturnButtonHeight) * gl.eqPx
                          : (gl.popupWindowsLandscapeHeight - gl.searchBarHeight) * gl.eqPx,
                  width: gl.popupWindowsPortraitWidth * gl.eqPx,
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
                                          ? WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                                            WidgetState.any: Size(
                                              gl.eqPx * gl.onCatalogueWidth * .7,
                                              gl.eqPx * gl.onCatalogueMapHeight,
                                            ),
                                          })
                                          : WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                                            WidgetState.any: Size(
                                              gl.eqPx * gl.onCatalogueWidth * .7,
                                              gl.eqPx * gl.onCatalogueCategoryHeight,
                                            ),
                                          }),
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
                                        },
                                child: Card(
                                  surfaceTintColor: Colors.transparent,
                                  shadowColor: Colors.transparent,
                                  color:
                                      i == selectedCategory
                                          ? gl.colorAgroBioTech.withAlpha(75)
                                          : gl.colorAgroBioTech.withAlpha(200),
                                  child: Column(
                                    mainAxisAlignment: MainAxisAlignment.center,
                                    children: [
                                      i != selectedCategory
                                          ? Container(
                                            alignment: Alignment.center,
                                            padding: EdgeInsets.all(3),
                                            constraints: BoxConstraints(
                                              maxWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                                              minHeight: gl.eqPx * gl.onCatalogueCategoryHeight,
                                            ),
                                            child: Text(
                                              groupe.mLabel,
                                              textAlign: TextAlign.center,
                                              style: TextStyle(
                                                color: Colors.black,
                                                fontWeight: FontWeight.w400,
                                                fontSize: gl.eqPx * gl.fontSizeM,
                                              ),
                                            ),
                                          )
                                          : Container(
                                            alignment: Alignment.center,
                                            padding: EdgeInsets.zero,
                                            constraints: BoxConstraints(maxWidth: gl.eqPx * gl.onCatalogueWidth * .97),
                                            child: ListBody(
                                              children:
                                                  <Widget>[
                                                    TextButton(
                                                      onPressed: () {
                                                        setState(() {
                                                          selectedCategory = -1;
                                                          selectedMap = -1;
                                                          selectedLayerTile = null;
                                                        });
                                                      },
                                                      child: Card(
                                                        color: gl.colorAgroBioTech.withAlpha(200),
                                                        child: Container(
                                                          alignment: Alignment.center,
                                                          padding: EdgeInsets.all(3),
                                                          constraints: BoxConstraints(
                                                            maxWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                                                            minWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                                                            minHeight: gl.eqPx * gl.onCatalogueMapHeight * .97,
                                                          ),
                                                          child: Text(
                                                            groupe.mLabel,
                                                            textAlign: TextAlign.center,
                                                            style: TextStyle(
                                                              color: Colors.black,
                                                              fontWeight: FontWeight.w400,
                                                              fontSize: gl.eqPx * gl.fontSizeM,
                                                            ),
                                                          ),
                                                        ),
                                                      ),
                                                    ),
                                                  ] +
                                                  _injectLayerData(groupe.mCode, (int i, LayerTile layerTile) {
                                                    return layerTileCard(
                                                      i,
                                                      layerTile,
                                                      widget.offlineMode,
                                                      widget.selectionMode,
                                                      stateOfLayerSwitcher,
                                                      setState,
                                                      noLegend: gl.dsp.orientation.index == 1,
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
                            children: List<Widget>.generate(_resultOfMapSearch.length, (int i) {
                              LayerTile layerTile = LayerTile(
                                key: _resultOfMapSearch[i],
                                name: gl.dico.getLayerBase(_resultOfMapSearch[i]).mNom,
                                filter: gl.dico.getLayerBase(_resultOfMapSearch[i]).mGroupe,
                                downloadable: gl.dico.getLayerBase(_resultOfMapSearch[i]).mIsDownloadableRW,
                              );
                              return layerTileCard(
                                i,
                                layerTile,
                                widget.offlineMode,
                                widget.selectionMode,
                                stateOfLayerSwitcher,
                                setState,
                                noLegend: gl.dsp.orientation == Orientation.landscape,
                              );
                            }),
                          ),
                ),
              ],
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                if (gl.dsp.orientation == Orientation.landscape)
                  Container(
                    constraints: BoxConstraints(
                      maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth,
                      maxHeight: gl.eqPx * (gl.popupWindowsLandscapeHeight - gl.popupReturnButtonHeight),
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
                        ),
                      ],
                    ),
                  ),
              ],
            ),
          ]),
        );
      },
    );
  }

  List<Widget> _injectGroupData(Widget Function(int, GroupeCouche) generate) {
    Map<String, Null> groupesNonVides = {};
    for (String key in gl.dico.mLayerBases.keys) {
      if ((widget.offlineMode ? gl.dico.getLayerBase(key).mOffline : true) &&
          (!gl.dico.getLayerBase(key).mExpert || gl.Mode.expert)) {
        groupesNonVides[gl.dico.getLayerBase(key).mGroupe] = null;
      }
    }
    int i = 0;
    List<GroupeCouche> groupes = [];
    for (String key in groupesNonVides.keys) {
      for (GroupeCouche couche in gl.dico.mGrCouches) {
        if (couche.mCode == key) {
          i++;
          if (couche.mCode == gl.dico.getLayerBase(widget.selectedMapCode).mGroupe && !modified) {
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

  List<Widget> _injectLayerData(String category, Widget Function(int, LayerTile) generate) {
    Map<String, LayerBase> mp = gl.dico.mLayerBases;
    List<LayerTile> layer = [];
    int i = 0;
    for (var key in mp.keys) {
      if (category == mp[key]!.mGroupe &&
          (!mp[key]!.mExpert || gl.Mode.expert) &&
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
        if (key != "" && widget.selectedMapCode == key && !modified) {
          selectedMap = i - 1;
          selectedLayerTile = layer.last;
          modified = true;
        }
      }
    }
    WidgetsBinding.instance.addPostFrameCallback((_) {
      double correctionFactorCatalogue = 4;
      double correctionFactorMap = -0.5;
      scrollToPoint(
        ((selectedCategory < 0 ? 0 : selectedCategory) + (selectedMap < 0 ? 0 : 1)) *
                (gl.onCatalogueCategoryHeight + correctionFactorCatalogue) *
                (gl.eqPx + (100 / (gl.dsp.orientation == Orientation.portrait ? gl.eqPxH : gl.eqPxW))) +
            (selectedMap < 0 ? 0 : selectedMap) *
                (gl.onCatalogueMapHeight + correctionFactorMap) *
                (gl.eqPx + (100 / (gl.dsp.orientation == Orientation.portrait ? gl.eqPxH : gl.eqPxW))),
      );
    });
    return List<Widget>.generate(layer.length, (i) {
      return generate(i, layer[i]);
    });
  }
}

class MapStatusSymbols extends StatefulWidget {
  final bool offlineMode;
  final String layerCode;
  const MapStatusSymbols({super.key, required this.offlineMode, this.layerCode = ""});

  @override
  State<StatefulWidget> createState() => _MapStatusSymbols();
}

class _MapStatusSymbols extends State<MapStatusSymbols> {
  static final Map<String, VoidSetter> _setStateStatusMaps = {};
  static bool _onlyOnce = true;
  String? mapName;

  @override
  void initState() {
    mapName = widget.layerCode;
    _setStateStatusMaps[mapName!] = (void Function() f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
    };
    if (_onlyOnce) {
      gl.rebuildStatusSymbols = (void Function() f) {
        for (final VoidSetter status in _setStateStatusMaps.values) {
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
        Icon(color: Colors.blue, Icons.save, size: gl.iconSizeM * multi * gl.eqPx),
      if (gl.dico.getLayerBase(mapName!).mIsDownloadableRW &&
          !gl.dico.getLayerBase(mapName!).mOffline &&
          !widget.offlineMode)
        Icon(color: Colors.lightBlue, Icons.file_download, size: gl.iconSizeM * multi * gl.eqPx),
      if (gl.dico.getLayerBase(mapName!).mCategorie != "Externe")
        Icon(color: Colors.brown, Icons.legend_toggle, size: gl.iconSizeM * multi * gl.eqPx),
      if (gl.dico.getLayerBase(mapName!).hasDoc())
        Icon(color: Colors.brown, Icons.picture_as_pdf, size: gl.iconSizeM * multi * gl.eqPx),
      if (gl.anaSurfSelectedLayerKeys.contains(mapName!))
        Icon(color: Colors.deepOrange, Icons.pentagon, size: gl.iconSizeM * multi * gl.eqPx),
      if (gl.anaPtSelectedLayerKeys.contains(mapName!))
        Icon(color: Colors.deepOrange, Icons.location_on, size: gl.iconSizeM * multi * gl.eqPx),
    ];
    return statusIcons.length > 3
        ? Row(
          children: [
            Column(mainAxisAlignment: MainAxisAlignment.start, children: statusIcons.sublist(0, 3)),
            Column(mainAxisAlignment: MainAxisAlignment.start, children: statusIcons.sublist(3)),
          ],
        )
        : Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [Column(children: statusIcons), SizedBox(width: gl.iconSizeM * multi * gl.eqPx)],
        );
  }
}

Card layerTileCard(
  int i,
  LayerTile? layerTile,
  bool offlineMode,
  int selectionMode,
  VoidSetter stateOfLayerSwitcher,
  VoidSetter setState, {
  bool noLegend = false,
}) {
  return layerTile != null
      ? Card(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side:
              i == _OnlineMapMenu.selectedMap
                  ? BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0)
                  : BorderSide(color: Colors.transparent, width: 0.0),
        ),
        color: i == _OnlineMapMenu.selectedMap ? Colors.white.withAlpha(255) : Colors.white.withAlpha(200),
        child:
            i != _OnlineMapMenu.selectedMap || noLegend
                ? Container(
                  constraints: BoxConstraints(minHeight: gl.eqPx * gl.onCatalogueMapHeight),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          MapStatusSymbols(offlineMode: offlineMode, layerCode: layerTile.key),
                          SizedBox(
                            height: gl.eqPx * gl.onCatalogueMapHeight * .9,
                            width: gl.eqPx * 58,
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
                                        _OnlineMapMenu.selectedLayerTile = layerTile,
                                        _OnlineMapMenu.modified = true,
                                      };
                                });
                              },
                              child: Text(
                                layerTile.name,
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize: gl.eqPx * gl.fontSizeM * .85,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.eqPx * gl.iconSizeM,
                            width: gl.eqPx * gl.iconSizeM * 1.2,
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
                    if (gl.dsp.orientation == Orientation.portrait)
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          MapStatusSymbols(offlineMode: offlineMode, layerCode: layerTile.key),
                          SizedBox(
                            height: gl.eqPx * gl.onCatalogueMapHeight,
                            width: gl.eqPx * 55,
                            child: TextButton(
                              onPressed: () {
                                setState(() {
                                  _OnlineMapMenu.selectedMap == i
                                      ? {_OnlineMapMenu.selectedMap = -1, _OnlineMapMenu.selectedLayerTile = null}
                                      : {_OnlineMapMenu.selectedMap = i, _OnlineMapMenu.selectedLayerTile = layerTile};
                                });
                              },
                              child: Text(
                                layerTile.name,
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize: gl.eqPx * gl.fontSizeS,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.eqPx * gl.iconSizeM * 1,
                            width: gl.eqPx * gl.iconSizeM * 1.2,
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
                    lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                    OnlineMapStatusTool(layerTile: layerTile),
                    lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                    LegendView(
                      layerKey: layerTile.key,
                      color: gl.colorBackgroundSecondary,
                      constraintsText: BoxConstraints(
                        minWidth: gl.eqPx * 40,
                        maxWidth: gl.eqPx * 40,
                        minHeight: gl.eqPx * 2,
                        maxHeight: gl.eqPx * 2,
                      ),
                      constraintsColors: BoxConstraints(
                        minWidth: gl.eqPx * 40,
                        maxWidth: gl.eqPx * 40,
                        minHeight: gl.eqPx * 2,
                        maxHeight: gl.eqPx * 2,
                      ),
                    ),
                    if (gl.dico.mLayerBases[layerTile.key]!.getDicoValForLegend().isNotEmpty)
                      lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
                    layerTile.proprietaire(),
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
                style: TextStyle(fontSize: gl.eqPx * gl.fontSizeM, color: Colors.black),
              ),
            ),
          ],
        ),
      );
}

class PopupPdfMenu {
  PopupPdfMenu(String layerKey, {String path = "", String titre = "", int currentPage = -1}) {
    gl.refreshStack(() {
      if (path == "") {
        path = "${gl.pathExternalStorage}/${gl.dico.getLayerBase(layerKey).mPdfName}";
      }
      if (currentPage == -1) {
        currentPage = int.parse(gl.dico.getLayerBase(layerKey).mPdfPage.toString());
      }
      if (titre == "") {
        titre = gl.dico.getLayerBase(layerKey).mNom;
      }
      popupForestimatorWindow(
        id: "aPDF",
        title: "Documentation",
        bigVersion: true,
        pdfChild: PDFScreen(path: path, titre: titre, currentPage: currentPage),
      );
    });
  }
}

class LayerSwitcher extends StatefulWidget {
  final void Function(LatLng) switchToLocationInSearchMenu;
  final VoidCallback after;
  const LayerSwitcher(this.switchToLocationInSearchMenu, this.after, {super.key});
  @override
  State<LayerSwitcher> createState() => _LayerSwitcher();
}

class _LayerSwitcher extends State<LayerSwitcher> {
  @override
  Widget build(BuildContext context) {
    gl.rebuildLayerSwitcher = (void Function() f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
    };
    return OrientationBuilder(
      builder: (c, o) {
        return Card(
          color: gl.backgroundTransparentBlackBox,
          child: AnimatedContainer(
            duration: Duration(milliseconds: 100),
            width:
                gl.dsp.orientation == Orientation.portrait
                    ? gl.eqPx * gl.layerswitcherBoxWidth
                    : gl.eqPx * gl.layerswitcherBoxWidth * 2.2,
            height:
                gl.dsp.orientation == Orientation.portrait
                    ? gl.offlineMode
                        ? (gl.layerSwitcherBoxHeightPortraitOffline +
                                gl.layerswitcherButtonsBoxHeight +
                                (gl.poiMarkerList.isNotEmpty && gl.selLay.geometries.isNotEmpty
                                    ? gl.layerSwitcherTileHeight + gl.layerswitcherControlBoxHeight
                                    : (gl.poiMarkerList.isNotEmpty || gl.selLay.geometries.isNotEmpty
                                        ? gl.layerswitcherControlBoxHeight
                                        : 0.0))) *
                            gl.eqPx
                        : (gl.layerSwitcherBoxHeightPortrait +
                                gl.layerswitcherButtonsBoxHeight +
                                (gl.poiMarkerList.isNotEmpty && gl.layerReady && gl.selLay.geometries.isNotEmpty
                                    ? gl.layerSwitcherTileHeight + gl.layerswitcherControlBoxHeight
                                    : (gl.poiMarkerList.isNotEmpty || (gl.layerReady && gl.selLay.geometries.isNotEmpty)
                                        ? gl.layerswitcherControlBoxHeight
                                        : 0.0))) *
                            gl.eqPx
                    : gl.layerSwitcherBoxHeightLandscape * gl.eqPx,
            child: switchRowColWithOrientation([
              if (((gl.layerReady && gl.selLay.geometries.isNotEmpty) || gl.poiMarkerList.isNotEmpty) &&
                  gl.dsp.orientation == Orientation.portrait)
                SizedBox(
                  width: gl.eqPx * gl.layerswitcherBoxWidth,
                  height:
                      (gl.poiMarkerList.isNotEmpty && gl.layerReady && gl.selLay.geometries.isNotEmpty
                          ? gl.layerSwitcherTileHeight + gl.layerswitcherControlBoxHeight
                          : (gl.poiMarkerList.isNotEmpty || gl.selLay.geometries.isNotEmpty
                              ? gl.layerswitcherControlBoxHeight
                              : 0.0)) *
                      gl.eqPx,
                  child: Column(
                    children: [
                      SizedBox(
                        width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                        height: gl.eqPx * gl.fontSizeL,
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text(
                              "Couches points/polygones",
                              textAlign: TextAlign.justify,
                              style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM),
                            ),
                          ],
                        ),
                      ),
                      UpperLayerControl(switchToLocationInSearchMenu: widget.switchToLocationInSearchMenu),
                    ],
                  ),
                ),
              SizedBox(
                width: gl.eqPx * gl.layerswitcherBoxWidth,
                height:
                    gl.eqPx *
                    (gl.offlineMode ? gl.layerSwitcherBoxHeightPortraitOffline : gl.layerSwitcherBoxHeightPortrait),
                child: Column(
                  children: [
                    SizedBox(
                      width: gl.eqPx * gl.layerswitcherBoxWidth,
                      height: gl.eqPx * gl.fontSizeXL,
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            "Couches thématiques",
                            textAlign: TextAlign.justify,
                            style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM),
                          ),
                        ],
                      ),
                    ),
                    SizedBox(
                      width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                      height:
                          gl.eqPx *
                              (gl.offlineMode
                                  ? gl.layerSwitcherBoxHeightPortraitOffline
                                  : gl.layerSwitcherBoxHeightPortrait) -
                          gl.eqPx * gl.fontSizeXL,
                      child: SwitcherBox(),
                    ),
                  ],
                ),
              ),
              if (gl.dsp.orientation == Orientation.portrait)
                SizedBox(
                  width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                  height: gl.eqPx * gl.layerswitcherButtonsBoxHeight,
                  child: Column(
                    children: [
                      SizedBox(
                        width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                        height: gl.eqPx * gl.fontSizeXL,
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text(
                              "Catalogues des couches",
                              textAlign: TextAlign.justify,
                              style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM),
                            ),
                          ],
                        ),
                      ),
                      SizedBox(
                        width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                        height: gl.eqPx * gl.layerswitcherButtonsBoxHeight - gl.eqPx * gl.fontSizeXL,
                        child: ViewCatalogueControl(gl.rebuildSwitcherBox),
                      ),
                    ],
                  ),
                ),
              if (gl.dsp.orientation == Orientation.landscape)
                Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    if (gl.geoReady || gl.poiMarkerList.isNotEmpty)
                      SizedBox(
                        width: gl.eqPx * gl.layerswitcherBoxWidth,
                        height:
                            (gl.poiMarkerList.isNotEmpty && gl.selLay.geometries.isNotEmpty
                                ? gl.layerSwitcherTileHeight + gl.layerswitcherControlBoxHeight
                                : (gl.poiMarkerList.isNotEmpty || gl.selLay.geometries.isNotEmpty
                                    ? gl.layerswitcherControlBoxHeight
                                    : 0.0)) *
                            gl.eqPx,
                        child: Column(
                          children: [
                            SizedBox(
                              width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                              height: gl.eqPx * gl.fontSizeL,
                              child: Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Text(
                                    "Controlez les couches visibles",
                                    textAlign: TextAlign.justify,
                                    style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM),
                                  ),
                                ],
                              ),
                            ),
                            UpperLayerControl(switchToLocationInSearchMenu: widget.switchToLocationInSearchMenu),
                          ],
                        ),
                      ),
                    SizedBox(
                      width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                      height: gl.eqPx * gl.layerswitcherButtonsBoxHeight,
                      child: Column(
                        children: [
                          SizedBox(
                            width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                            height: gl.eqPx * gl.fontSizeXL,
                            child: Row(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Text(
                                  "Catalogues des couches",
                                  textAlign: TextAlign.justify,
                                  style: TextStyle(color: Colors.white, fontSize: gl.eqPx * gl.fontSizeM),
                                ),
                              ],
                            ),
                          ),
                          SizedBox(
                            width: gl.eqPx * gl.layerswitcherBoxWidth - 1,
                            height: gl.eqPx * gl.layerswitcherButtonsBoxHeight - gl.eqPx * gl.fontSizeXL,
                            child: ViewCatalogueControl(gl.rebuildSwitcherBox),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
            ]),
          ),
        );
      },
    );
  }
}

class ViewCatalogueControl extends StatefulWidget {
  final VoidSetter stateOfLayerSwitcher;
  const ViewCatalogueControl(this.stateOfLayerSwitcher, {super.key});
  @override
  State<ViewCatalogueControl> createState() => _ViewCatalogueControl();
}

class _ViewCatalogueControl extends State<ViewCatalogueControl> {
  static bool modeViewOfflineMap = false;
  static bool modeViewOnlineMap = false;

  @override
  void initState() {
    gl.rebuildSwitcherCatalogueButtons = (void Function() f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
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
                width: gl.eqPx * gl.iconSizeM * 1.2,
                height: gl.eqPx * gl.iconSizeM * 1.2,
                child: FloatingActionButton(
                  backgroundColor: modeViewOfflineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!modeViewOnlineMap && !modeViewOnlineMap) {
                      PopupOnlineMapMenu(
                        () {
                          gl.refreshStack(() {
                            modeViewOfflineMap = false;
                            gl.stack.pop("Catalogue");
                          });
                          if (mounted) {
                            setState(() {
                              modeViewOfflineMap = false;
                            });
                          }
                        },
                        true,
                        -1,
                        "",
                        widget.stateOfLayerSwitcher,
                      );
                    }
                    setState(() {
                      modeViewOfflineMap = true;
                    });
                    gl.refreshStack(() {
                      modeViewOfflineMap = true;
                    });
                  },
                  child: Icon(Icons.download_for_offline, size: gl.eqPx * gl.iconSizeM, color: Colors.black),
                ),
              ),
            if (!gl.offlineMode)
              SizedBox(
                width: gl.eqPx * gl.iconSizeM * 1.2,
                height: gl.eqPx * gl.iconSizeM * 1.2,
                child: FloatingActionButton(
                  backgroundColor: modeViewOnlineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!modeViewOnlineMap && !modeViewOnlineMap) {
                      PopupOnlineMapMenu(
                        () {
                          gl.refreshStack(() {
                            modeViewOnlineMap = false;
                            modeViewOfflineMap = false;
                            gl.stack.pop("Catalogue");
                          });
                          if (mounted) {
                            setState(() {
                              modeViewOnlineMap = false;
                              modeViewOfflineMap = false;
                            });
                          }
                        },
                        gl.offlineMode,
                        -1,
                        "",
                        (void Function() x) {
                          x();
                        },
                      );
                    }
                    setState(() {
                      modeViewOnlineMap = true;
                    });
                    gl.refreshStack(() {
                      modeViewOnlineMap = true;
                    });
                  },
                  child: Icon(Icons.layers_outlined, size: gl.eqPx * gl.iconSizeM, color: Colors.black),
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

  const UpperLayerControl({super.key, required this.switchToLocationInSearchMenu});
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
            height: gl.eqPx * gl.layerSwitcherTileHeight,
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
                      minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                        WidgetState.any: Size(gl.eqPx * 50, gl.eqPx * 10),
                      }),
                    ),
                    onPressed: () {
                      PopupSearchMenu(widget.switchToLocationInSearchMenu, () {
                        gl.refreshStack(() {
                          gl.stack.pop("SearchMenu");
                        });
                      });
                    },
                    child: Container(
                      padding: EdgeInsets.symmetric(horizontal: 0.0),
                      alignment: Alignment.centerLeft,
                      constraints: BoxConstraints(
                        maxHeight: gl.eqPx * 10,
                        minHeight: gl.eqPx * 10,
                        maxWidth: gl.eqPx * 50,
                        minWidth: gl.eqPx * 50,
                      ),
                      child: Text(
                        "Marqueurs des lieux",
                        textAlign: TextAlign.left,
                        style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                      ),
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxHeight: gl.eqPx * 10,
                      minHeight: gl.eqPx * 10,
                      maxWidth: gl.eqPx * 17,
                      minWidth: gl.eqPx * 17,
                    ),
                    child: Row(
                      children: [
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.eqPx * 10,
                            minHeight: gl.eqPx * 10,
                            maxWidth: gl.eqPx * 15,
                            minWidth: gl.eqPx * 15,
                          ),
                          color: Colors.white,
                          padding: const EdgeInsets.symmetric(),
                          child: SizedBox(
                            width: gl.eqPx * 10,
                            height: gl.eqPx * 10,
                            child: FloatingActionButton(
                              backgroundColor: gl.modeMapShowSearchMarker ? gl.colorAgroBioTech : Colors.grey,
                              onPressed: () {
                                setState(() {
                                  gl.modeMapShowSearchMarker = !gl.modeMapShowSearchMarker;
                                });
                                gl.refreshStack(() {});
                              },
                              child: Icon(Icons.remove_red_eye, size: gl.eqPx * 10, color: Colors.black),
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
        if (gl.layerReady && gl.selLay.geometries.isNotEmpty)
          SizedBox(
            height: gl.eqPx * gl.layerSwitcherTileHeight,
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
                      minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                        WidgetState.any: Size(gl.eqPx * 50, gl.eqPx * gl.layerSwitcherTileHeight),
                      }),
                    ),
                    onPressed: () {
                      gl.stack.add(
                        "LayerList",
                        popupLayerListMenu(gl.notificationContext!, "as", widget.switchToLocationInSearchMenu, () {
                          gl.refreshStack(() {
                            gl.stack.pop("LayerList");
                          });
                        }),
                        Duration(milliseconds: 500),
                        gl.Anim.onScreenPosCenter,
                        Offset(0, -2000),
                      );
                      gl.refreshStack(() {});
                    },
                    child: Container(
                      alignment: Alignment.centerLeft,
                      padding: EdgeInsets.symmetric(horizontal: 1.0),
                      constraints: BoxConstraints(
                        maxHeight: gl.eqPx * 10,
                        minHeight: gl.eqPx * 10,
                        maxWidth: gl.eqPx * 50,
                        minWidth: gl.eqPx * 50,
                      ),
                      child: Text(
                        "Couche des polygones",
                        textAlign: TextAlign.left,
                        style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeS),
                      ),
                    ),
                  ),
                  Container(
                    constraints: BoxConstraints(
                      maxHeight: gl.eqPx * 10,
                      minHeight: gl.eqPx * 10,
                      maxWidth: gl.eqPx * 17,
                      minWidth: gl.eqPx * 17,
                    ),
                    child: Row(
                      children: [
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.eqPx * 10,
                            minHeight: gl.eqPx * 10,
                            maxWidth: gl.eqPx * 15,
                            minWidth: gl.eqPx * 15,
                          ),
                          color: Colors.white,
                          padding: const EdgeInsets.symmetric(),
                          child: SizedBox(
                            width: gl.eqPx * 10,
                            height: gl.eqPx * 10,
                            child: FloatingActionButton(
                              backgroundColor: gl.modeMapShowPolygons ? gl.colorAgroBioTech : Colors.grey,
                              onPressed: () {
                                setState(() {
                                  gl.modeMapShowPolygons = !gl.modeMapShowPolygons;
                                });
                                gl.refreshStack(() {});
                              },
                              child: Icon(Icons.remove_red_eye, size: gl.eqPx * 10, color: Colors.black),
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
            if (gl.selectedLayerForMap.length < newIndex + 1 || gl.selectedLayerForMap.length < oldIndex + 1) {
              return;
            }
            String tmpKey = gl.selectedLayerForMap[newIndex].mCode;
            bool tmpOffline = gl.selectedLayerForMap[newIndex].offline;
            gl.replaceLayerFromList(
              gl.selectedLayerForMap[oldIndex].mCode,
              index: newIndex,
              offline: gl.selectedLayerForMap[oldIndex].offline,
            );
            gl.replaceLayerFromList(tmpKey, index: oldIndex, offline: tmpOffline);
            gl.refreshStack(() {});
          });
        },

        children: List<Widget>.generate(3, (i) {
          if ((!gl.offlineMode && !"123".contains(gl.selectedLayerForMap[i].mCode)) ||
              (i == 0 && !"123".contains(gl.selectedLayerForMap[i].mCode))) {
            return Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
                side: BorderSide(color: Color.fromRGBO(205, 225, 138, 1.0), width: 2.0),
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
                            fixedSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                              WidgetState.any: Size(
                                gl.eqPx * gl.layerswitcherBoxWidth * .68,
                                gl.eqPx * gl.layerSwitcherTileHeight,
                              ),
                            }),
                          ),
                          onPressed: () {
                            PopupOnlineMapMenu(
                              () {
                                gl.rebuildSwitcherCatalogueButtons(() {
                                  _ViewCatalogueControl.modeViewOfflineMap = false;
                                  _ViewCatalogueControl.modeViewOnlineMap = false;
                                });
                                gl.refreshStack(() {
                                  gl.stack.pop("Catalogue");
                                  _ViewCatalogueControl.modeViewOfflineMap = false;
                                  _ViewCatalogueControl.modeViewOnlineMap = false;
                                });
                              },
                              gl.offlineMode,
                              i,
                              gl.selectedLayerForMap[i].mCode,
                              (void Function() x) {
                                x();
                              },
                            );
                            gl.refreshStack(() {});
                          },
                          child: Container(
                            alignment: Alignment.centerLeft,
                            padding: EdgeInsets.symmetric(horizontal: 1.0),
                            child: Text(
                              gl.dico.getLayerBase(gl.selectedLayerForMap[i].mCode).mNom,
                              textAlign: TextAlign.left,
                              style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.eqPx * 10,
                            minHeight: gl.eqPx * 10,
                            maxWidth: gl.eqPx * 22,
                            minWidth: gl.eqPx * 22,
                          ),
                          child: Row(
                            children: [
                              Container(
                                color: Colors.white,
                                constraints: BoxConstraints(
                                  maxHeight: gl.eqPx * 12,
                                  minHeight: gl.eqPx * 12,
                                  maxWidth: gl.eqPx * 10,
                                  minWidth: gl.eqPx * 10,
                                ),
                                padding: const EdgeInsets.symmetric(),
                                child: Image.asset(
                                  gl.dico.getLayerBase(gl.selectedLayerForMap[i].mCode).mLogoAttributionFile,
                                ),
                              ),
                              gl.selectedLayerForMap[i].offline
                                  ? Container(
                                    constraints: BoxConstraints(
                                      maxHeight: gl.eqPx * 10,
                                      minHeight: gl.eqPx * 10,
                                      maxWidth: gl.eqPx * 10,
                                      minWidth: gl.eqPx * 10,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(Icons.save, size: gl.eqPx * 10),
                                  )
                                  : Container(
                                    constraints: BoxConstraints(
                                      maxHeight: gl.eqPx * 10,
                                      minHeight: gl.eqPx * 10,
                                      maxWidth: gl.eqPx * 10,
                                      minWidth: gl.eqPx * 10,
                                    ),
                                    padding: const EdgeInsets.symmetric(),
                                    child: Icon(Icons.wifi, size: gl.eqPx * 10),
                                  ),
                            ],
                          ),
                        ),
                      ],
                    ),
                    if ((i == 0 && !gl.offlineMode) ||
                        (i == 1 && !gl.offlineMode && gl.Mode.expertTools)) //Pour la transparance de la première tile
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Container(
                            constraints: BoxConstraints(maxHeight: gl.eqPx * 10, maxWidth: gl.eqPx * 40),
                            child: TextButton(
                              style: ButtonStyle(
                                shape: WidgetStateProperty.fromMap(<WidgetStatesConstraint, OutlinedBorder>{
                                  WidgetState.any: RoundedRectangleBorder(
                                    borderRadius: BorderRadiusGeometry.circular(12.0),
                                    side: BorderSide(color: Color.fromRGBO(205, 225, 138, 1.0), width: 2.0),
                                  ),
                                }),
                                fixedSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                                  WidgetState.any: Size(
                                    gl.eqPx * gl.layerswitcherBoxWidth * .5,
                                    gl.eqPx * gl.layerSwitcherTileHeight,
                                  ),
                                }),
                                backgroundColor:
                                    gl.modeMapFirstTileLayerTransparancy
                                        ? WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                          WidgetState.any: gl.colorAgroBioTech,
                                        })
                                        : WidgetStateProperty<Color>.fromMap(<WidgetStatesConstraint, Color>{
                                          WidgetState.any: Color.fromARGB(255, 234, 234, 234),
                                        }),
                              ),
                              onPressed: () {
                                setState(() {
                                  gl.modeMapFirstTileLayerTransparancy = !gl.modeMapFirstTileLayerTransparancy;
                                });
                                gl.refreshStack(() {});
                              },
                              child: Text(
                                gl.modeMapFirstTileLayerTransparancy ? "Transparence 50%" : "Transparence 0%",
                                style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
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
            return Card(margin: EdgeInsets.all(5), key: Key('$i+listOfThreeOffline'), color: Colors.transparent);
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
                            maximumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                              WidgetState.any: Size(gl.eqPx * 45, gl.eqPx * 20),
                            }),
                          ),
                          onPressed: () {
                            PopupOnlineMapMenu(
                              () {
                                gl.rebuildSwitcherCatalogueButtons(() {
                                  _ViewCatalogueControl.modeViewOfflineMap = false;
                                  _ViewCatalogueControl.modeViewOnlineMap = false;
                                });
                                gl.refreshStack(() {
                                  gl.stack.pop("Catalogue");
                                  _ViewCatalogueControl.modeViewOfflineMap = false;
                                  _ViewCatalogueControl.modeViewOnlineMap = false;
                                });
                              },
                              gl.offlineMode,
                              i,
                              gl.selectedLayerForMap[i].mCode,
                              (void Function() x) {
                                x();
                              },
                            );
                            gl.refreshStack(() {});
                          },
                          child: Container(
                            padding: EdgeInsets.symmetric(horizontal: 1.0),
                            constraints: BoxConstraints(
                              maxHeight: gl.eqPx * 10,
                              maxWidth: gl.eqPx * 45,
                              minWidth: gl.eqPx * 45,
                            ),
                            child: Text(
                              "Appuyez ici pour ajouter une couche du catalogue",
                              style: TextStyle(color: Colors.black, fontSize: gl.eqPx * gl.fontSizeXS),
                            ),
                          ),
                        ),
                        Container(
                          constraints: BoxConstraints(
                            maxHeight: gl.eqPx * 10,
                            minHeight: gl.eqPx * 10,
                            maxWidth: gl.eqPx * 22,
                            minWidth: gl.eqPx * 22,
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
  String labelYes;
  String labelNo;
  PopupDoYouReally(
    BuildContext context,
    VoidCallback onAccept,
    String title,
    String message, [
    this.labelYes = "Oui",
    this.labelNo = "Non",
  ]) {
    gl.refreshStack(() {
      popupForestimatorMessage(
        id: "DOYOU",
        title: title,
        message: message,
        onAccept: () {
          onAccept();
          gl.stack.pop("DOYOU");
        },
        messageAccept: labelYes,
        messageDecline: labelNo,
        onDecline: () {
          gl.stack.pop("DOYOU");
        },
      );
    });
  }
}

class AnaResultsMenu extends StatefulWidget {
  final List<LayerAnaPt> requestedLayers;
  final VoidCallback after;
  const AnaResultsMenu(this.after, this.requestedLayers, {super.key});

  @override
  State<StatefulWidget> createState() => _AnaResultsMenu();
}

class _AnaResultsMenu extends State<AnaResultsMenu> {
  TextEditingController? controllerPdfName;
  TextEditingController? controllerLocationName;

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        return AnimatedContainer(
          duration: Duration(milliseconds: 100),
          height: (gl.eqPxH - 30) * gl.eqPx,
          width: gl.eqPx * 96,
          child: ListView(
            padding: const EdgeInsets.symmetric(horizontal: 0),
            children: [
              Card(
                shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                surfaceTintColor: Colors.transparent,
                shadowColor: Colors.transparent,
                color: gl.colorAgroBioTech.withAlpha(75),
                child: Column(
                  children: [
                    Card(
                      color: gl.colorAgroBioTech.withAlpha(200),
                      child: Container(
                        alignment: Alignment.center,
                        padding: EdgeInsets.all(3),
                        constraints: BoxConstraints(
                          maxWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                          minWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                          minHeight: gl.eqPx * gl.onCatalogueMapHeight * .97,
                        ),
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Stack(
                              alignment: AlignmentGeometry.center,
                              children: [
                                lt.forestimatorButton(() async {
                                  popupPdfSaveDialog((String pdf, String locationName) async {
                                    if (pdf.isEmpty) {
                                      pdf =
                                          "analyseForestimator${DateTime.now().day}.${DateTime.now().month}.${DateTime.now().year}.pdf";
                                    }
                                    if (pdf.length < 4 || pdf.substring(pdf.length - 4) != ".pdf") {
                                      pdf = "$pdf.pdf";
                                    }
                                    if (locationName.isEmpty) {
                                      locationName = "une position";
                                    }
                                    String dir = "/storage/emulated/0/Download";
                                    if (Platform.isIOS) {
                                      dir = (await getApplicationDocumentsDirectory()).path;
                                    }
                                    makePdf(widget.requestedLayers, pdf, dir, locationName);
                                    // confirmation que le pdf a été créé
                                    gl.stack.add(
                                      "popPDF",
                                      popupPDFSaved(pdf, () {
                                        gl.stack.pop("popPDF");
                                      }),
                                      Duration(milliseconds: 400),
                                      gl.Anim.onScreenPosCenter,
                                      Offset(0, -250),
                                    );
                                    gl.refreshStack(() {});
                                  });
                                  gl.refreshStack(() {});
                                }, Icons.save_alt),
                                Container(
                                  alignment: Alignment.topRight,
                                  width: gl.eqPx * gl.iconSizeM,
                                  height: gl.eqPx * gl.iconSizeM,
                                  child: Icon(
                                    Icons.picture_as_pdf_sharp,
                                    size: gl.eqPx * gl.iconSizeXS * .7,
                                    color: Colors.red,
                                  ),
                                ),
                              ],
                            ),

                            SizedBox(
                              width: gl.eqPx * 65,
                              child: Text(
                                "Par couche selectionnée",
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontWeight: FontWeight.w400,
                                  fontSize: gl.eqPx * gl.fontSizeM,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                    Card(
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadiusGeometry.circular(12.0),
                        side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
                      ),
                      surfaceTintColor: Colors.transparent,
                      shadowColor: Colors.transparent,
                      color: Colors.white.withAlpha(200),
                      child: ListBody(
                        children: _injectLayerResults(
                          (int i, ResultCard result, String mCode, int mRastValue) => TextButton(
                            style: ButtonStyle(
                              minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                                WidgetState.any: Size(
                                  gl.eqPx * gl.onCatalogueWidth * .7,
                                  gl.eqPx * gl.onCatalogueCategoryHeight,
                                ),
                              }),
                            ),
                            key: Key('$i'),
                            onPressed: result.documentation,
                            child: Container(
                              alignment: Alignment.center,
                              child: Column(
                                children: [
                                  Row(
                                    children: [
                                      SizedBox(
                                        width: gl.eqPx * gl.iconSizeS,
                                        height: gl.eqPx * gl.iconSizeS,
                                        child: Stack(
                                          children: [
                                            Icon(result.leading, color: Colors.black, size: gl.eqPx * gl.iconSizeS),
                                            if ((gl.dico.getLayerBase(mCode).hasDoc() && mCode != "CS_A") ||
                                                (gl.dico.getLayerBase(mCode).hasDoc() &&
                                                    mCode == "CS_A" &&
                                                    mRastValue < 99))
                                              Container(
                                                alignment: Alignment.topRight,
                                                width: gl.eqPx * gl.iconSizeM,
                                                height: gl.eqPx * gl.iconSizeM,
                                                child: Icon(
                                                  Icons.picture_as_pdf_sharp,
                                                  size: gl.eqPx * gl.iconSizeXS * .7,
                                                  color: Colors.red,
                                                ),
                                              ),
                                          ],
                                        ),
                                      ),
                                      SizedBox(width: gl.eqPx * gl.iconSizeXS, height: gl.eqPx * gl.iconSizeXS),
                                      SizedBox(
                                        width: gl.eqPx * gl.onCatalogueWidth * .65,
                                        child: Text(
                                          result.layerName,
                                          style: TextStyle(
                                            color: Colors.black,
                                            fontSize: gl.eqPx * gl.fontSizeS,
                                            fontWeight: FontWeight.w300,
                                          ),
                                        ),
                                      ),
                                    ],
                                  ),
                                  lt.stroke(gl.eqPx, gl.eqPx * .5, Colors.black.withAlpha(50)),
                                  Row(
                                    children: [
                                      Container(
                                        color: Colors.black12,
                                        padding: EdgeInsets.all(1),
                                        constraints: BoxConstraints(
                                          minHeight: gl.eqPx * gl.iconSizeS,
                                          minWidth: gl.eqPx * gl.iconSizeS,
                                        ),
                                        child: Container(
                                          color:
                                              result.legendColor == Colors.transparent
                                                  ? Colors.white
                                                  : result.legendColor,
                                        ),
                                      ),
                                      SizedBox(width: gl.eqPx * gl.iconSizeXS, height: gl.eqPx * gl.iconSizeXS),
                                      SizedBox(
                                        width: gl.eqPx * gl.onCatalogueWidth * .65,
                                        child: Text(
                                          result.colorCode,
                                          style: TextStyle(
                                            color: Colors.black,
                                            fontSize: gl.eqPx * gl.fontSizeS,
                                            fontWeight: FontWeight.w400,
                                          ),
                                        ),
                                      ),
                                    ],
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              if (AptsFEE(widget.requestedLayers).ready)
                Card(
                  shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                  surfaceTintColor: Colors.transparent,
                  shadowColor: Colors.transparent,
                  color: gl.colorAgroBioTech.withAlpha(75),
                  child: Column(
                    children: [
                      Card(
                        color: gl.colorAgroBioTech.withAlpha(200),
                        child: Container(
                          alignment: Alignment.center,
                          padding: EdgeInsets.all(3),
                          constraints: BoxConstraints(
                            maxWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                            minWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                            minHeight: gl.eqPx * gl.onCatalogueMapHeight * .97,
                          ),
                          child: Text(
                            "Aptitude du Fichier Ecologique des Essences",
                            textAlign: TextAlign.center,
                            style: TextStyle(
                              color: Colors.black,
                              fontWeight: FontWeight.w400,
                              fontSize: gl.eqPx * gl.fontSizeM,
                            ),
                          ),
                        ),
                      ),
                      Card(
                        color: Colors.white.withAlpha(200),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadiusGeometry.circular(12.0),
                          side: BorderSide(color: Color.fromRGBO(205, 225, 138, 1.0), width: 2.0),
                        ),
                        child: _tabAptFEE(context, AptsFEE(widget.requestedLayers)),
                      ),
                    ],
                  ),
                ),
              if (PropositionGS(widget.requestedLayers).ready)
                Card(
                  shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                  surfaceTintColor: Colors.transparent,
                  shadowColor: Colors.transparent,
                  color: gl.colorAgroBioTech.withAlpha(75),
                  child: Column(
                    children: [
                      Card(
                        color: gl.colorAgroBioTech.withAlpha(200),
                        child: Container(
                          alignment: Alignment.center,
                          padding: EdgeInsets.all(3),
                          constraints: BoxConstraints(
                            maxWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                            minWidth: gl.eqPx * gl.onCatalogueWidth * .97,
                            minHeight: gl.eqPx * gl.onCatalogueMapHeight * .97,
                          ),
                          child: Text(
                            "Propositions d'Essences du Guide des Stations",
                            textAlign: TextAlign.center,
                            style: TextStyle(
                              color: Colors.black,
                              fontWeight: FontWeight.w400,
                              fontSize: gl.eqPx * gl.fontSizeM,
                            ),
                          ),
                        ),
                      ),
                      Card(
                        color: Colors.white.withAlpha(200),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadiusGeometry.circular(12.0),
                          side: BorderSide(color: Color.fromRGBO(205, 225, 138, 1.0), width: 2.0),
                        ),
                        child: _tabPropositionCS(context, PropositionGS(widget.requestedLayers)),
                      ),
                    ],
                  ),
                ),
            ],
          ),
        );
      },
    );
  }

  List<Widget> _injectLayerResults(Widget Function(int, ResultCard, String, int) generate) {
    List<ResultCard> results = [];

    if (widget.requestedLayers.isNotEmpty) {
      for (LayerAnaPt layer in widget.requestedLayers) {
        IconData leading = switch (gl.dico.getLayerBase(layer.mCode).mGroupe) {
          "ST" => CustomIcons.mountain,
          "PEUP" => CustomIcons.forest,
          "CS" => CustomIcons.mountains,
          "REF" => CustomIcons.map,
          _ => CustomIcons.soil,
        };
        if (gl.dico.getLayerBase(layer.mCode).mNom.contains("FEE")) {
          leading = CustomIcons.tree;
        }
        results.add(
          ResultCard(
            gl.dico.getLayerBase(layer.mCode).mNom,
            gl.dico.getLayerBase(layer.mCode).getValLabel(layer.mRastValue),
            leading,
            legendColor:
                ((gl.dico.getLayerBase(layer.mCode).getValColor(layer.mRastValue).toARGB32()) != 4294967295)
                    ? gl.dico.getLayerBase(layer.mCode).getValColor(layer.mRastValue)
                    : Colors.transparent,
            () {
              if ((gl.dico.getLayerBase(layer.mCode).hasDoc() && layer.mCode != "CS_A")) {
                PopupPdfMenu(layer.mCode);
              }
              if (gl.dico.getLayerBase(layer.mCode).hasDoc() && layer.mCode == "CS_A" && layer.mRastValue < 99) {
                PopupPdfMenu(
                  "",
                  path:
                      '${gl.pathExternalStorage}/${gl.dico.getLayerBase(layer.mCode).getFicheRoute(us: layer.mRastValue)}',
                );
              }
            },
          ),
        );
      }
    }
    List<Widget> resultWidgets = List<Widget>.generate(results.length, (i) {
      return generate(i, results[i], widget.requestedLayers[i].mCode, widget.requestedLayers[i].mRastValue);
    });
    //return resultWidgets;
    for (int i = 0; i < results.length - 1; i++) {
      resultWidgets.insert(i * 2 + 1, lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech));
    }
    return resultWidgets;
  }

  Widget _tabAptFEE(BuildContext context, AptsFEE apts) {
    return Column(
      children: [
        DefaultTabController(
          length: 3,
          child: Column(
            children: <Widget>[
              TabBar(
                isScrollable: true,
                indicatorSize: TabBarIndicatorSize.tab,
                indicatorColor: gl.colorAgroBioTech,
                indicatorWeight: gl.eqPx * 1,
                labelColor: Colors.black,
                unselectedLabelColor: Colors.black45,
                tabs: List<Tab>.generate(3, (index) {
                  List<String> tags = ["Optimum", "Tolérance", "Tolérance élargie"];
                  return Tab(text: "${tags[index]} ${apts.getListEss(index + 1).length}");
                }),
              ),
              Container(
                constraints: BoxConstraints(
                  maxHeight:
                      max(max(apts.getListEss(1).length, apts.getListEss(2).length), apts.getListEss(3).length) *
                      gl.eqPx *
                      gl.iconSizeS *
                      1.7,
                ),
                child: TabBarView(
                  children: List<EssencesListView>.generate(
                    3,
                    (index) => EssencesListView(apts: apts, codeApt: index + 1),
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _tabPropositionCS(BuildContext context, PropositionGS apts) {
    return Column(
      children: [
        DefaultTabController(
          length: 4,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              TabBar(
                isScrollable: true,
                indicatorSize: TabBarIndicatorSize.tab,
                indicatorColor: gl.colorAgroBioTech,
                indicatorWeight: gl.eqPx * 1,
                labelColor: Colors.black,
                dividerColor: Colors.black38,
                unselectedLabelColor: Colors.black45,
                overlayColor: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Color>{
                  WidgetState.selected: gl.colorAgroBioTech.withAlpha(200),
                }),
                tabs: List<Tab>.generate(
                  4,
                  (index) => Tab(text: "${gl.dico.vulnerabiliteLabel(index + 1)} ${apts.getListEss(index + 1).length}"),
                ),
              ),

              Container(
                constraints: BoxConstraints(
                  maxHeight:
                      max(
                        max(max(apts.getListEss(1).length, apts.getListEss(3).length), apts.getListEss(2).length),
                        apts.getListEss(4).length,
                      ) *
                      gl.eqPx *
                      gl.iconSizeS *
                      1.64,
                ),
                child: TabBarView(
                  children: List<EssencesListViewGS>.generate(
                    4,
                    (index) => EssencesListViewGS(apts: apts, codeApt: index + 1),
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  void popupPdfSaveDialog(void Function(String, String) onAccept) {
    popupForestimatorMessage(
      id: "davepdf",
      title: "Saufgardez un pdf",
      messageAccept: "Enregistrer",
      messageDecline: "Annuler",
      onAccept: () {
        onAccept(controllerPdfName!.text, controllerLocationName!.text);
        gl.stack.pop("davepdf");
      },
      onDecline: () {
        gl.stack.pop("davepdf");
      },
      child:
          getStorage()
              ? SizedBox(
                width: gl.eqPx * gl.popupWindowsPortraitWidth,
                child: Column(
                  children: [
                    TextField(
                      style: TextStyle(color: Colors.white),
                      controller: controllerPdfName,
                      autofocus: true,
                      decoration: InputDecoration(
                        hintText:
                            "forestimatorAnalyse${DateTime.now().day}.${DateTime.now().month}.${DateTime.now().year}.pdf",
                      ),
                    ),
                    TextField(
                      controller: controllerLocationName,
                      decoration: InputDecoration(hintText: "Point 5"),
                      style: TextStyle(color: Colors.white),
                    ),
                  ],
                ),
              )
              : Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Container(
                    constraints: BoxConstraints(
                      minHeight: gl.eqPx * 30,
                      minWidth: gl.eqPx * gl.popupWindowsPortraitWidth,
                      maxWidth: gl.eqPx * gl.popupWindowsPortraitWidth,
                    ),
                    child: Text(
                      "Vous n'avez pas accordé la permission de stockage des pdf!",
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: gl.eqPx * gl.fontSizeM,
                        fontWeight: FontWeight.w400,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ],
              ),
    );
  }

  @override
  void initState() {
    super.initState();
    controllerPdfName = TextEditingController();
    controllerLocationName = TextEditingController();
  }

  Future makePdf(List<LayerAnaPt> layers, String fileName, String dir, String locationName) async {
    final pdf = pw.Document();
    final imageLogo = pw.MemoryImage(
      (await rootBundle.load('assets/images/GRF_nouveau_logo_uliege-retina.jpg')).buffer.asUint8List(),
    );
    final now = DateTime.now();

    pdf.addPage(
      pw.Page(
        build: (context) {
          return pw.Column(
            crossAxisAlignment: pw.CrossAxisAlignment.start,
            children: [
              pw.Row(
                mainAxisAlignment: pw.MainAxisAlignment.spaceBetween,
                children: [
                  pw.Column(
                    children: [
                      pw.Text(
                        "Analyse ponctuelle Forestimator",
                        style: pw.TextStyle(fontSize: 18, color: PdfColor.fromHex("255f19")),
                      ),
                      pw.SizedBox(height: 30),
                      paddedText(
                        "${gl.offlineMode ? "Réalisé en mode hors-ligne" : "Réalisé avec connexion internet"} le ${DateFormat('yyyy-MM-dd').format(now)}",
                      ),
                    ],
                    crossAxisAlignment: pw.CrossAxisAlignment.start,
                  ),
                  pw.SizedBox(height: 150, width: 150, child: pw.Image(imageLogo)),
                ],
              ), //first row
              paddedText("Localisation: $locationName", pad: 3),
              paddedText("Coordonnée (EPSG:31370) ", pad: 3),
              paddedText("X: ${gl.pt.x.toInt()}", pad: 3),
              paddedText("Y: ${gl.pt.y.toInt()}", pad: 3),
              pw.SizedBox(height: 10),
              pw.Text("Couches cartographiques analysées", style: pw.TextStyle(fontSize: 16)),
              pw.SizedBox(height: 20),
              ...layers.where((i) => i.mRastValue != 0).map<pw.Widget>((LayerAnaPt a) {
                LayerBase l = gl.dico.getLayerBase(a.mCode);
                return paddedText("${l.mNom} : ${l.getValLabel(a.mRastValue)}");
              }),
            ],
          );
        },
      ),
    );

    File out = File("$dir/$fileName");
    if (await out.exists()) {
      // on renomme le pdf
      int nb = 2;
      do {
        out = File("$dir/${fileName.substring(0, fileName.length - 4)}$nb.pdf");
        nb++;
        //print(out.path);
      } while (await out.exists());
    }
    out.writeAsBytes(await pdf.save(), flush: true);
    // FlutterLogs.logError("anaPt", "pdf", "pdf exported to. ${out.path}");
  }
}

class ResultCard {
  final String layerName;
  final Color? legendColor;
  final String colorCode;
  final IconData leading;
  void Function() documentation = () {};

  ResultCard(this.layerName, this.colorCode, this.leading, void Function() doc, {this.legendColor = Colors.black}) {
    documentation = doc;
  }
}

class EssencesListViewGS extends StatelessWidget {
  final PropositionGS apts;
  final int codeApt; // maintentant c'est plus un code de vulnérabilités

  const EssencesListViewGS({super.key, required this.apts, required this.codeApt});

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort((a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR));
    return ListView.builder(
      itemCount: mEss.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index) {
        return Column(
          children: [
            ListTile(
              leading: Stack(
                children: [
                  if (gl.dico.getEss(code.elementAt(index)).getFicheRoute().isNotEmpty)
                    Container(
                      alignment: Alignment.topRight,
                      width: gl.eqPx * gl.iconSizeS,
                      height: gl.eqPx * gl.iconSizeS,
                      child: Icon(Icons.picture_as_pdf_sharp, size: gl.eqPx * gl.iconSizeXS * .5, color: Colors.red),
                    ),
                  gl.dico.getEss(code.elementAt(index)).mFR == 1
                      ? SizedBox(
                        width: gl.eqPx * gl.iconSizeS,
                        height: gl.eqPx * gl.iconSizeS,
                        child: Icon(CustomIcons.tree, color: Colors.black87, size: gl.eqPx * gl.iconSizeS),
                      )
                      : SizedBox(
                        width: gl.eqPx * gl.iconSizeS,
                        height: gl.eqPx * gl.iconSizeS,
                        child: Icon(Icons.forest_outlined, color: Colors.black87, size: gl.eqPx * gl.iconSizeXS),
                      ),
                ],
              ),
              title: SizedBox(
                width: gl.eqPx * gl.popupWindowsPortraitWidth * .6,
                child: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
              ),
              subtitle:
                  codeApt != mEss[code.elementAt(index)]
                      ? SizedBox(child: Text(gl.dico.aptLabel(mEss[code.elementAt(index)]!)))
                      : null,
              trailing: SizedBox(width: gl.eqPx * gl.iconSizeXS),
              onTap: () {
                String path = "/${gl.pathExternalStorage}/FEE-${gl.dico.getEss(code.elementAt(index)).mCode}.pdf";
                PopupPdfMenu("", path: path, currentPage: 0);
              },
            ),
            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
          ],
        );
      },
    );
  }
}

pw.Widget paddedText(final String text, {final pw.TextAlign align = pw.TextAlign.left, final double pad = 5.0}) =>
    pw.Padding(padding: pw.EdgeInsets.all(pad), child: pw.Text(text, textAlign: align));

class EssencesListView extends StatelessWidget {
  final AptsFEE apts;
  final int codeApt;

  const EssencesListView({super.key, required this.apts, required this.codeApt});

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort((a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR));
    return ListView.builder(
      itemCount: mEss.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index) {
        return Column(
          children: [
            ListTile(
              leading: Stack(
                children: [
                  if (gl.dico.getEss(code.elementAt(index)).getFicheRoute().isNotEmpty)
                    Container(
                      alignment: Alignment.topRight,
                      width: gl.eqPx * gl.iconSizeS,
                      height: gl.eqPx * gl.iconSizeS,
                      child: Icon(Icons.picture_as_pdf, size: gl.eqPx * gl.iconSizeXS * .5, color: Colors.red),
                    ),

                  gl.dico.getEss(code.elementAt(index)).mFR == 1
                      ? SizedBox(
                        width: gl.eqPx * gl.iconSizeS,
                        height: gl.eqPx * gl.iconSizeS,
                        child: Icon(CustomIcons.tree, color: Colors.black87, size: gl.eqPx * gl.iconSizeS),
                      )
                      : SizedBox(
                        width: gl.eqPx * gl.iconSizeS,
                        height: gl.eqPx * gl.iconSizeS,
                        child: Icon(Icons.forest_outlined, color: Colors.black87, size: gl.eqPx * gl.iconSizeXS),
                      ),
                ],
              ),
              title: SizedBox(
                width: gl.eqPx * gl.popupWindowsPortraitWidth * .6,
                child: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
              ),
              subtitle:
                  codeApt != mEss[code.elementAt(index)]
                      ? SizedBox(child: Text(gl.dico.aptLabel(mEss[code.elementAt(index)]!)))
                      : null,
              trailing:
                  apts.mCompensations[code.elementAt(index)]!
                      ? SizedBox(
                        width: gl.eqPx * gl.iconSizeXS,
                        child: IconButton(
                          icon: Icon(Icons.info_outline, color: gl.colorUliege, size: gl.eqPx * gl.iconSizeXS),
                          onPressed: () {},
                          tooltip:
                              "La situation topographique provoque un effet de compensation (positif ou négatif) sur l'aptitude de cette essence",
                        ),
                      )
                      : SizedBox(width: gl.eqPx * gl.iconSizeXS),
              onTap: () {
                String path = "/${gl.pathExternalStorage}/FEE-${gl.dico.getEss(code.elementAt(index)).mCode}.pdf";
                PopupPdfMenu("", path: path, currentPage: 0);
              },
            ),
            lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech),
          ],
        );
      },
    );
  }
}

class AnaSurfResultsMenu extends StatefulWidget {
  final Map<String, dynamic> json;
  final VoidCallback state;
  final VoidCallback after;
  const AnaSurfResultsMenu(this.json, this.after, this.state, {super.key});

  @override
  State<StatefulWidget> createState() => _AnaSurfResultsMenu();
}

class _AnaSurfResultsMenu extends State<AnaSurfResultsMenu> {
  TextEditingController? controllerPdfName;
  TextEditingController? controllerLocationName;

  @override
  Widget build(BuildContext context) {
    return OrientationBuilder(
      builder: (context, orientation) {
        return switchRowColWithOrientation([
          SizedBox(
            height: (gl.eqPxH - 30) * gl.eqPx,
            width: gl.eqPx * 96,
            child: ListView(
              padding: const EdgeInsets.symmetric(horizontal: 0),
              children: [
                Card(
                  shape: RoundedRectangleBorder(borderRadius: BorderRadiusGeometry.circular(12.0)),
                  surfaceTintColor: Colors.transparent,
                  shadowColor: Colors.transparent,
                  color: gl.backgroundTransparentBlackBox,
                  child: Column(
                    children: [
                      Card(
                        color: gl.colorAgroBioTech.withAlpha(200),
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Container(
                              alignment: Alignment.center,
                              height: gl.eqPx * gl.fontSizeXL * 2,
                              width: gl.eqPx * gl.popupWindowsPortraitWidth * .8,
                              child: Text(
                                "Par couche",
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  fontSize: gl.eqPx * gl.fontSizeL,
                                  color: Colors.black,
                                  fontWeight: FontWeight.w400,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ),
                      Card(
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadiusGeometry.circular(12.0),
                          side: BorderSide(color: gl.colorAgroBioTech.withAlpha(255), width: 2.0),
                        ),
                        surfaceTintColor: Colors.transparent,
                        shadowColor: Colors.transparent,
                        color: Colors.white.withAlpha(200),
                        child: ListBody(
                          children: _injectLayerResults(
                            (int i, Item item, String mCode, int mRastValue) => TextButton(
                              style: ButtonStyle(
                                minimumSize: WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                                  WidgetState.any: Size(
                                    gl.eqPx * gl.onCatalogueWidth * .7,
                                    gl.eqPx * gl.onCatalogueCategoryHeight,
                                  ),
                                }),
                              ),
                              key: Key('$i'),
                              onPressed: () {
                                if ((gl.dico.getLayerBase(mCode).hasDoc() && mCode != "CS_A")) {
                                  PopupPdfMenu(mCode);
                                }
                                if (gl.dico.getLayerBase(mCode).hasDoc() && mCode == "CS_A" && mRastValue < 99) {
                                  PopupPdfMenu(
                                    "",
                                    path:
                                        '${gl.pathExternalStorage}/${gl.dico.getLayerBase(mCode).getFicheRoute(us: mRastValue)}',
                                  );
                                }
                              },
                              child: Container(
                                alignment: Alignment.center,
                                child: Column(
                                  children: [
                                    Row(
                                      children: [
                                        SizedBox(
                                          width: gl.eqPx * gl.iconSizeS,
                                          height: gl.eqPx * gl.iconSizeS,
                                          child: Stack(
                                            children: [
                                              Icon(
                                                switch (gl.dico.getLayerBase(mCode).mGroupe) {
                                                  "ST" => CustomIcons.mountain,
                                                  "PEUP" => CustomIcons.forest,
                                                  "CS" => CustomIcons.mountains,
                                                  "REF" => CustomIcons.map,
                                                  _ => CustomIcons.soil,
                                                },
                                                color: Colors.black,
                                                size: gl.eqPx * gl.iconSizeS,
                                              ),
                                              if ((gl.dico.getLayerBase(mCode).hasDoc() && mCode != "CS_A") ||
                                                  (gl.dico.getLayerBase(mCode).hasDoc() &&
                                                      mCode == "CS_A" &&
                                                      mRastValue < 99))
                                                Container(
                                                  alignment: Alignment.topRight,
                                                  width: gl.eqPx * gl.iconSizeM,
                                                  height: gl.eqPx * gl.iconSizeM,
                                                  child: Icon(
                                                    Icons.picture_as_pdf_sharp,
                                                    size: gl.eqPx * gl.iconSizeXS * .7,
                                                    color: Colors.red,
                                                  ),
                                                ),
                                            ],
                                          ),
                                        ),
                                        SizedBox(width: gl.eqPx * gl.iconSizeXS, height: gl.eqPx * gl.iconSizeXS),
                                        SizedBox(
                                          width: gl.eqPx * gl.onCatalogueWidth * .65,
                                          child: Text(
                                            item.name,
                                            style: TextStyle(
                                              color: Colors.black,
                                              fontSize: gl.eqPx * gl.fontSizeS,
                                              fontWeight: FontWeight.w400,
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                    lt.stroke(gl.eqPx, gl.eqPx * .5, Colors.black.withAlpha(50)),
                                    item.entry,
                                  ],
                                ),
                              ),
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
        ]);
      },
    );
  }

  List<Widget> _injectLayerResults(Widget Function(int, Item, String, int) generate) {
    final List<Item> menuItems = [];
    if (widget.json['RequestedLayers'] != null && widget.json['RequestedLayers'].isNotEmpty) {
      for (var result in widget.json['RequestedLayers']) {
        if (result['mean'] != null) {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              mCode: result['layerCode'],
              mRastValue: result['rastValue'] ?? 98,
              entry: forestimatorResultsHeaderContinue(result, result['layerCode']),
            ),
          );
        } else {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              mCode: result['layerCode'],
              mRastValue: result['rastValue'] ?? 98,
              entry: forestimatorResultsHeaderClasse(result),
            ),
          );
        }
      }
    }

    List<Widget> resultWidgets = List<Widget>.generate(menuItems.length, (i) {
      return generate(i, menuItems[i], menuItems[i].mCode!, menuItems[i].mRastValue!);
    });
    //return resultWidgets;
    for (int i = 0; i < menuItems.length - 1; i++) {
      resultWidgets.insert(i * 2 + 1, lt.stroke(gl.eqPx, gl.eqPx * .5, gl.colorAgroBioTech));
    }
    return resultWidgets;
  }
}
