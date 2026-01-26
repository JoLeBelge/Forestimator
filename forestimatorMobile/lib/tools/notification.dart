import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:dart_jts/dart_jts.dart' hide Orientation, Geometry, Key;
import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/myicons.dart';
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/pages/catalogueView/legend_view.dart';
import 'package:fforestimator/pages/pdf_screen.dart';
import 'package:fforestimator/tools/layout_tools.dart';
import 'package:fforestimator/tools/customLayer/path_layer.dart';
import 'package:fforestimator/tools/customLayer/polygon_layer.dart';
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
    backgroundColor: WidgetStateProperty.fromMap(
      <WidgetStatesConstraint, Color>{WidgetState.any: color.withAlpha(200)},
    ),
    shape: WidgetStateProperty<OutlinedBorder>.fromMap(
      <WidgetStatesConstraint, OutlinedBorder>{
        WidgetState.any: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: color, width: borderWidth),
        ),
      },
    ),
    fixedSize: WidgetStateProperty.fromMap(<WidgetStatesConstraint, Size>{
      WidgetState.any: Size(
        width == 0
            ? gl.display.equipixel * gl.popupReturnButtonWidth * .7
            : width,
        height == 0
            ? gl.display.equipixel * gl.popupReturnButtonHeight
            : height,
      ),
    }),
  );
}

TextStyle dialogTextButtonStyle() {
  return TextStyle(
    color: Colors.black,
    fontWeight: FontWeight.w600,
    fontSize: gl.display.equipixel * gl.fontSizeM,
  );
}

void popupBarrierWrapper({
  required Widget popup,
  bool dismiss = true,
  VoidCallback? after,
  VoidCallback? barrierHit,
}) {
  gl.mainStack.add(
    Stack(
      children: [
        OrientationBuilder(
          builder: (context, orientation) {
            return TextButton(
              style: ButtonStyle(
                shape: WidgetStateProperty.fromMap(
                  <WidgetStatesConstraint, ContinuousRectangleBorder>{
                    WidgetState.any: ContinuousRectangleBorder(),
                  },
                ),
                shadowColor: WidgetStateProperty.fromMap(
                  <WidgetStatesConstraint, Color>{
                    WidgetState.any: Colors.transparent,
                  },
                ),
                backgroundColor:
                    WidgetStateProperty.fromMap(<WidgetStatesConstraint, Color>{
                      WidgetState.any:
                          dismiss
                              ? Colors.transparent
                              : gl.backgroundTransparentBlackBox,
                    }),
                overlayColor: WidgetStateProperty.fromMap(
                  <WidgetStatesConstraint, Color>{
                    WidgetState.any: Colors.transparent,
                  },
                ),
                animationDuration: Duration.zero,
              ),
              onPressed:
                  dismiss
                      ? () {
                        after == null
                            ? gl.print(
                              "popup wrapper: Voidcall after() was null!",
                            )
                            : after();
                        dismissPopup();
                      }
                      : () {
                        barrierHit == null
                            ? gl.print(
                              "popup wrapper: Voidcall barrierHit() was null!",
                            )
                            : barrierHit();
                      },
              child: SizedBox(
                width: gl.display.equipixel * gl.display.equiwidth,
                height: gl.display.equipixel * gl.display.equiheight,
              ),
            );
          },
        ),
        popup,
      ],
    ),
  );
}

/// Present a popup either on the app's mainStack (preferred) or via
/// `showDialog` as a fallback when no stable notification context is
/// available (tests may set `gl.notificationContext` manually).
void presentPopup({
  required Widget popup,
  bool dismiss = true,
  VoidCallback? after,
  BuildContext? context,
}) {
  // Prefer mainStack overlay when the global notification context has been
  // initialized by the app. If it's not available then prefer the standard
  // `showDialog` fallback when a concrete BuildContext has been provided by
  // the caller (tests often pass a context and expect dialogs to be visible
  // in the widget tree via a standard showDialog call).
  if (gl.notificationContext != null) {
    gl.refreshMainStack(() {
      popupBarrierWrapper(popup: popup, dismiss: dismiss, after: after);
    });
    return;
  }

  if (context != null) {
    showDialog(
      context: context,
      barrierDismissible: dismiss,
      builder: (BuildContext _) => popup,
    ).then((_) {
      if (after != null) after();
    });
    return;
  }

  // Last resort: still push to mainStack so UI can attempt to render it.
  gl.refreshMainStack(() {
    popupBarrierWrapper(popup: popup, dismiss: dismiss, after: after);
  });
}

/// Centralized dismissal for popups that are presented on the
/// `gl.mainStack` overlay.
void dismissPopup({VoidCallback? after}) {
  try {
    gl.mainStackPopLast();
    gl.refreshMainStack(() {});
  } catch (e) {
    gl.print('dismissPopup: failed to pop mainStack: $e');
  }
  if (after != null) after();
}

class PopupMessage {
  PopupMessage(String title, String message) {
    presentPopup(
      dismiss: false,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: Colors.white,
        title: Row(
          children: [
            forestimatorIcon(),
            Text(
              title,
              style: TextStyle(
                color: Colors.black,
                fontWeight: FontWeight.w400,
                fontSize: gl.display.equipixel * gl.fontSizeM,
              ),
            ),
          ],
        ),
        content: Text(
          message,
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w400,
            fontSize: gl.display.equipixel * gl.fontSizeS,
          ),
        ),
        actions: [
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            child: Text("OK", style: dialogTextButtonStyle()),
            onPressed: () {
              dismissPopup();
            },
          ),
        ],
      ),
    );
  }
}

class PopupDownloadRecomendedLayers extends StatelessWidget {
  final String? title;
  final String? dialog;
  final String? accept;
  final String? decline;
  final VoidCallback? onAccept;
  final VoidCallback? onDecline;
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
            style: TextStyle(
              color: Colors.black,
              fontWeight: FontWeight.w400,
              fontSize: gl.display.equipixel * gl.fontSizeM,
            ),
          ),
        ],
      ),
      content: Text(
        dialog!,
        style: TextStyle(
          color: Colors.black,
          fontWeight: FontWeight.w400,
          fontSize: gl.display.equipixel * gl.fontSizeM,
        ),
      ),
      actions: <Widget>[
        TextButton(
          style: dialogButtonStyle(
            height: gl.display.equipixel * 12,
            width: gl.display.equipixel * 20,
          ),
          onPressed: () => {if (onAccept != null) onAccept!()},
          child: Text(accept!, style: dialogTextButtonStyle()),
        ),
        if (decline != null)
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            onPressed: () => {if (onDecline != null) onDecline!()},
            child: Text(decline!, style: dialogTextButtonStyle()),
          ),
      ],
    );
  }
}

Widget popupGeometryAlreadySent() {
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
          "Message",
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w400,
            fontSize: gl.display.equipixel * gl.fontSizeM,
          ),
        ),
      ],
    ),
    content: Text(
      "Vous nous avez déja envoyé cette entité.",
      style: TextStyle(
        color: Colors.black,
        fontWeight: FontWeight.w400,
        fontSize: gl.display.equipixel * gl.fontSizeM,
      ),
    ),
    actions: <Widget>[
      TextButton(
        style: dialogButtonStyle(
          height: gl.display.equipixel * 12,
          width: gl.display.equipixel * 20,
        ),
        child: Text("OK", style: dialogTextButtonStyle()),
        onPressed: () {
          dismissPopup();
        },
      ),
    ],
  );
}

Widget popupNoInternet() {
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
          "Message",
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w400,
            fontSize: gl.display.equipixel * gl.fontSizeM,
          ),
        ),
      ],
    ),
    content: Text(
      "Vous n'avez pas accès à internet.",
      style: TextStyle(
        color: Colors.black,
        fontWeight: FontWeight.w400,
        fontSize: gl.display.equipixel * gl.fontSizeM,
      ),
    ),
    actions: <Widget>[
      TextButton(
        style: dialogButtonStyle(
          height: gl.display.equipixel * 12,
          width: gl.display.equipixel * 20,
        ),
        child: Text("OK", style: dialogTextButtonStyle()),
        onPressed: () {
          dismissPopup();
        },
      ),
    ],
  );
}

Widget forestimatorIcon({double width = 0, double height = 0}) {
  return Container(
    padding: EdgeInsets.only(right: gl.display.equipixel * 2),
    width: width == 0 ? gl.display.equipixel * gl.iconSizeXS : width,
    height: height == 0 ? gl.display.equipixel * gl.iconSizeXS : height,
    child: Image.asset("assets/images/LogoForestimator.png"),
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
            style: TextStyle(
              color: Colors.black,
              fontWeight: FontWeight.w400,
              fontSize: gl.display.equipixel * gl.fontSizeM,
            ),
          ),
        ],
      ),
      content: Text(dialog!),
      actions: <Widget>[
        TextButton(
          style: dialogButtonStyle(
            height: gl.display.equipixel * 12,
            width: gl.display.equipixel * 20,
          ),
          onPressed: () => {if (onAccept != null) onAccept!()},
          child: Text(accept!, style: dialogTextButtonStyle()),
        ),
        if (decline != null)
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            onPressed: () => {if (onDecline != null) onDecline!()},
            child: Text(decline!, style: dialogTextButtonStyle()),
          ),
      ],
    );
  }
}

class PopupDownloadSuccess {
  PopupDownloadSuccess(BuildContext context, String layerName) {
    presentPopup(
      context: context,
      dismiss: false,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: Colors.white,
        title: Row(
          children: [
            forestimatorIcon(),
            Text(
              "Message",
              style: TextStyle(
                color: Colors.black,
                fontWeight: FontWeight.w400,
                fontSize: gl.display.equipixel * gl.fontSizeM,
              ),
            ),
          ],
        ),
        content: Text(
          "$layerName a été téléchargée avec succès.",
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w400,
            fontSize: gl.display.equipixel * gl.fontSizeM,
          ),
        ),
        actions: [
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            child: Text("OK", style: dialogTextButtonStyle()),
            onPressed: () {
              dismissPopup();
            },
          ),
        ],
      ),
    );
  }
}

class PopupDownloadFailed {
  PopupDownloadFailed(BuildContext context, String layerName) {
    presentPopup(
      context: context,
      dismiss: false,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: Colors.white,
        title: Row(
          children: [
            forestimatorIcon(),
            Text(
              "Message",
              style: TextStyle(
                color: Colors.black,
                fontWeight: FontWeight.w400,
                fontSize: gl.display.equipixel * gl.fontSizeM,
              ),
            ),
          ],
        ),
        content: Text(
          "$layerName n'a pas été téléchargé.",
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w400,
            fontSize: gl.display.equipixel * gl.fontSizeM,
          ),
        ),
        actions: [
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            child: Text("OK", style: dialogTextButtonStyle()),
            onPressed: () {
              dismissPopup();
            },
          ),
        ],
      ),
    );
  }
}

class PopupPolygonNotWellDefined {
  PopupPolygonNotWellDefined(BuildContext context) {
    gl.refreshMainStack(() {
      popupBarrierWrapper(
        dismiss: true,
        popup: AlertDialog(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(12.0),
            side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
          ),
          backgroundColor: Colors.white,
          title: Row(
            children: [
              forestimatorIcon(),
              Text(
                "Attention",
                style: TextStyle(
                  color: Colors.black,
                  fontWeight: FontWeight.w400,
                  fontSize: gl.display.equipixel * gl.fontSizeM,
                ),
              ),
            ],
          ),
          content: Text(
            "Avec ce point, le polygone n'est pas bien défini, c'est-à-dire on ne peut pas croiser des segments.",
            style: TextStyle(
              color: Colors.black,
              fontWeight: FontWeight.w400,
              fontSize: gl.display.equipixel * gl.fontSizeM,
            ),
          ),
          actions: [
            TextButton(
              style: dialogButtonStyle(
                height: gl.display.equipixel * 12,
                width: gl.display.equipixel * 20,
              ),
              child: Text("OK", style: dialogTextButtonStyle()),
              onPressed: () {
                dismissPopup();
              },
            ),
          ],
        ),
      );
    });
  }
}

Widget popupPDFSaved(String pdfName, VoidCallback after) {
  return Builder(
    builder:
        (dialogContext) => AlertDialog(
          shadowColor: Colors.black,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadiusGeometry.circular(12.0),
            side: BorderSide(
              color: Color.fromRGBO(205, 225, 138, 1.0),
              width: 2.0,
            ),
          ),
          backgroundColor: Colors.white,
          title: Row(
            children: [
              forestimatorIcon(),
              Text(
                "Export du pdf: $pdfName",
                style: TextStyle(
                  color: Colors.black,
                  fontWeight: FontWeight.w400,
                  fontSize: gl.display.equipixel * gl.fontSizeM,
                ),
              ),
            ],
          ),
          content: Text(
            "Le document a été enregistré!",
            style: TextStyle(
              color: Colors.black,
              fontWeight: FontWeight.w400,
              fontSize: gl.display.equipixel * gl.fontSizeM,
            ),
          ),
          actions: [
            TextButton(
              style: dialogButtonStyle(
                height: gl.display.equipixel * 12,
                width: gl.display.equipixel * 20,
              ),
              child: Text("OK", style: dialogTextButtonStyle()),
              onPressed: () {
                if (gl.notificationContext == null) {
                  try {
                    Navigator.of(dialogContext, rootNavigator: true).pop();
                  } catch (_) {}
                } else {
                  dismissPopup();
                }
                after();
              },
            ),
          ],
        ),
  );
}

class PopupColorChooser {
  Color pickerColor = Color(0xff443a49);

  PopupColorChooser(
    Color currentColor,
    BuildContext context,
    ValueChanged<Color> colorChange,
    VoidCallback after,
    VoidCallback onAccept,
  ) {
    pickerColor = currentColor;
    presentPopup(
      context: context,
      dismiss: true,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: Colors.white,
        title: Text(
          "Choisissez une couleur!",
          textAlign: TextAlign.center,
          style: TextStyle(
            color: Colors.black,
            fontWeight: FontWeight.w500,
            fontSize: gl.display.equipixel * gl.fontSizeM,
          ),
        ),
        content: SingleChildScrollView(
          child: ColorPicker(
            pickerColor: pickerColor,
            onColorChanged: colorChange,
          ),
        ),
        actions: [
          TextButton(
            style: dialogButtonStyle(
              height: gl.display.equipixel * 12,
              width: gl.display.equipixel * 20,
            ),
            child: Text("OK", style: dialogTextButtonStyle()),
            onPressed: () {
              currentColor = pickerColor;
              onAccept();
              dismissPopup();
              after();
            },
          ),
        ],
      ),
    );
  }
}

class PopupNameIntroducer {
  PopupNameIntroducer(
    BuildContext context,
    String currentName,
    ValueChanged<String> state,
    VoidCallback after,
    VoidCallback callbackOnStartTyping,
    VoidCallback onAccept,
  ) {
    presentPopup(
      context: context,
      dismiss: true,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: Colors.white,
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
                    dismissPopup();
                    after();
                  },
                  controller: TextEditingController(text: currentName),
                ),
              ),
              SizedBox(
                width: gl.menuBarLength * .5 * gl.display.equipixel,
                child: TextButton(
                  style: dialogButtonStyle(
                    height: gl.display.equipixel * 12,
                    width: gl.display.equipixel * 10 * "Ok".length,
                  ),
                  child: Text("Ok", style: dialogTextButtonStyle()),
                  onPressed: () {
                    onAccept();
                    dismissPopup();
                    after();
                  },
                ),
              ),
            ]),
          ),
        ),
      ),
    );
  }
}

class PopupNewPolygon {
  PopupNewPolygon(
    BuildContext context,
    String currentName,
    Color currentColor,
    ValueChanged<String> typeChanged,
    ValueChanged<String> nameChanged,
    ValueChanged<Color> colorChanged,
    VoidCallback after,
    VoidCallback callbackOnStartTyping,
    VoidCallback onAccept,
  ) {
    presentPopup(
      context: context,
      dismiss: true,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 2
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: SingleChildScrollView(
            child: switchRowColWithOrientation([
              Column(
                children: [
                  Text(
                    "Choisissez le type",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SelectPolyType(state: typeChanged),
                  stroke(
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    gl.colorAgroBioTech,
                  ),
                  Text(
                    "Introduisez un nom",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
                      maxLengthEnforcement: MaxLengthEnforcement.enforced,
                      onChanged: (String str) {
                        nameChanged(str);
                      },
                      onTap: () => callbackOnStartTyping(),
                      onTapOutside: (pointer) {
                        after();
                      },
                      controller: TextEditingController(text: ""),
                      style: TextStyle(color: Colors.white),
                    ),
                  ),
                  Text(
                    "Choisissez une couleur",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SelectPolyColor(
                    colorChanged: colorChanged,
                    currentColor: currentColor,
                  ),
                ],
              ),
              SizedBox(
                width: gl.menuBarLength * .5 * gl.display.equipixel,
                child: TextButton(
                  style: dialogButtonStyle(
                    height: gl.display.equipixel * 12,
                    width: gl.display.equipixel * 10 * "Ok".length,
                  ),
                  child: Text("Créer", style: dialogTextButtonStyle()),
                  onPressed: () {
                    dismissPopup();
                    onAccept();
                  },
                ),
              ),
            ]),
          ),
        ),
      ),
    );
  }
}

class SelectPolyColor extends StatefulWidget {
  final ValueChanged<Color> colorChanged;
  final Color currentColor;

  const SelectPolyColor({
    super.key,
    required this.colorChanged,
    required this.currentColor,
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
            setState(() {});
          },
          () {},
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
          width: gl.display.equipixel * gl.iconSizeM * 1.65,
          height: gl.display.equipixel * gl.iconSizeM * 1.35,
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
                Text(
                  "Point",
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: gl.display.equipixel * gl.fontSizeXS,
                  ),
                ),
              ],
            ),
          ),
        ),
        Container(
          width: gl.display.equipixel * gl.iconSizeM * 1.65,
          height: gl.display.equipixel * gl.iconSizeM * 1.35,
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
                Text(
                  "Polygon",
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: gl.display.equipixel * gl.fontSizeXS,
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

class PopupUserData {
  PopupUserData(
    BuildContext context,
    VoidCallback callbackOnStartTyping,
    VoidCallback afterCompleting, {
    String oldName = "",
    String oldForename = "",
    String oldMail = "",
  }) {
    presentPopup(
      context: context,
      dismiss: true,
      after: () {
        gl.UserData.forename = oldForename;
        gl.UserData.name = oldName;
        gl.UserData.mail = oldMail;
      },
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 2
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: SingleChildScrollView(
            child: switchRowColWithOrientation([
              Column(
                children: [
                  Text(
                    "Renseignements personnels",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  stroke(
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    gl.colorAgroBioTech,
                  ),
                  Text(
                    "Votre nom",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
                      maxLengthEnforcement: MaxLengthEnforcement.enforced,
                      onChanged: (String str) {
                        gl.UserData.name = str;
                      },
                      onTap: () => callbackOnStartTyping(),
                      onTapOutside: (pointer) {},
                      style: TextStyle(color: Colors.white),
                    ),
                  ),
                  Text(
                    "Votre prénom",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
                      maxLengthEnforcement: MaxLengthEnforcement.enforced,
                      onChanged: (String str) {
                        gl.UserData.forename = str;
                      },
                      onTap: () => callbackOnStartTyping(),
                      onTapOutside: (pointer) {},
                      style: TextStyle(color: Colors.white),
                    ),
                  ),
                  Text(
                    "Mail de contact",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
                      maxLengthEnforcement: MaxLengthEnforcement.enforced,
                      onChanged: (String str) {
                        gl.UserData.mail = str;
                      },
                      onTap: () => callbackOnStartTyping(),
                      onTapOutside: (pointer) {},
                      style: TextStyle(color: Colors.white),
                    ),
                  ),
                ],
              ),
              Row(
                children: [
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Ok".length,
                      ),
                      child: Text("Appliquer", style: dialogTextButtonStyle()),
                      onPressed: () {
                        if (gl.UserData.forename.isNotEmpty &&
                            gl.UserData.name.isNotEmpty &&
                            gl.UserData.mail.isNotEmpty) {
                          dismissPopup();
                          afterCompleting();
                          gl.UserData.serialize();
                        }
                      },
                    ),
                  ),
                  stroke(
                    vertical: true,
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    Colors.transparent,
                  ),
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Retour".length,
                      ),
                      child: Text("Retour", style: dialogTextButtonStyle()),
                      onPressed: () {
                        gl.UserData.forename = oldForename;
                        gl.UserData.name = oldName;
                        gl.UserData.mail = oldMail;
                        dismissPopup();
                      },
                    ),
                  ),
                ],
              ),
            ]),
          ),
        ),
      ),
    );
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
    VoidCallback onTapOutside,
    VoidCallback callbackOnStartTyping,
  ) {
    TextEditingController textEditor = TextEditingController();
    TextEditingController textEditorVal = TextEditingController();
    String type = "string";
    presentPopup(
      context: context,
      dismiss: true,
      after: onTapOutside,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 2
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: SingleChildScrollView(
            child: switchRowColWithOrientation([
              Column(
                children: [
                  Text(
                    "Type de la variable",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SelectAttributeType(
                    typeChanged: (String newType) {
                      type = newType;
                      typeChanged(type);
                      textEditorVal.text = "";
                    },
                  ),
                  stroke(
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    gl.colorAgroBioTech,
                  ),
                  Text(
                    "Introduisez un nom",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
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
                  Text(
                    "Introduisez une valeur",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeM,
                    ),
                  ),
                  SizedBox(
                    width: gl.menuBarLength * gl.display.equipixel,
                    child: TextFormField(
                      maxLength: 22,
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
                                textEditorVal.text = textEditorVal.text
                                    .substring(
                                      0,
                                      textEditorVal.text.length - 1,
                                    );
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
                                textEditorVal.text = textEditorVal.text
                                    .substring(
                                      0,
                                      textEditorVal.text.length - 1,
                                    );
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
              Row(
                children: [
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Ok".length,
                      ),
                      child: Text("Ajouter", style: dialogTextButtonStyle()),
                      onPressed: () {
                        String nom = textEditor.text;
                        if (controlDuplicateAttributeName(textEditor.text)) {
                          PopupMessage("Erreur", "Le nom $nom existe déja!");
                          return;
                        }
                        dismissPopup();
                      },
                    ),
                  ),
                  stroke(
                    vertical: true,
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    Colors.transparent,
                  ),
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Retour".length,
                      ),
                      child: Text("Retour", style: dialogTextButtonStyle()),
                      onPressed: () {
                        onTapOutside();
                        dismissPopup();
                      },
                    ),
                  ),
                ],
              ),
            ]),
          ),
        ),
      ),
    );
  }
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
    presentPopup(
      dismiss: true,
      after: () {
        valueChanged(oldValue);
        onTapOutside;
      },
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 2
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: Column(
            children: [
              Text(
                "Introduisez une nouvelle valeur",
                style: TextStyle(
                  color: Colors.white,
                  fontSize: gl.display.equipixel * gl.fontSizeS,
                ),
              ),
              SizedBox(
                width: gl.menuBarLength * gl.display.equipixel,
                child: TextFormField(
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
                            textEditor.text = textEditor.text.substring(
                              0,
                              textEditor.text.length - 1,
                            );
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
                            textEditor.text = textEditor.text.substring(
                              0,
                              textEditor.text.length - 1,
                            );
                          }
                          if (textEditor.text.isEmpty) {
                            valueChanged(0.0);
                          } else {
                            valueChanged(textEditor.text);
                          }
                        }
                    }
                  },
                  onTapOutside: (pointer) {},
                  controller: textEditor,
                  style: TextStyle(color: Colors.white),
                ),
              ),
              Row(
                children: [
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Ok".length,
                      ),
                      child: Text("Changer", style: dialogTextButtonStyle()),
                      onPressed: () {
                        onAccept();
                        dismissPopup();
                      },
                    ),
                  ),
                  stroke(
                    vertical: true,
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    Colors.transparent,
                  ),
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Retour".length,
                      ),
                      child: Text("Retour", style: dialogTextButtonStyle()),
                      onPressed: () {
                        valueChanged(oldValue);
                        dismissPopup();
                      },
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
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

bool controlDuplicateAttributeName(String attribute) {
  int count = 0;
  for (
    int i = 0;
    i < gl.geometries[gl.selectedGeometry].attributes.length;
    i++
  ) {
    if (attribute == gl.geometries[gl.selectedGeometry].attributes[i].name) {
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
            width: gl.display.equipixel * gl.iconSizeM * 2.1,
            height: gl.display.equipixel * gl.iconSizeM * 1.35,
            color:
                _selectedType == 0 ? gl.colorAgroBioTech : Colors.transparent,
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
                  Text(
                    "Charactères",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeXS,
                    ),
                  ),
                ],
              ),
            ),
          ),
          Container(
            width: gl.display.equipixel * gl.iconSizeM * 1.5,
            height: gl.display.equipixel * gl.iconSizeM * 1.35,
            color:
                _selectedType == 1 ? gl.colorAgroBioTech : Colors.transparent,
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
                  Text(
                    "Entièr",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeXS,
                    ),
                  ),
                ],
              ),
            ),
          ),
          Container(
            width: gl.display.equipixel * gl.iconSizeM * 1.7,
            height: gl.display.equipixel * gl.iconSizeM * 1.35,
            color:
                _selectedType == 2 ? gl.colorAgroBioTech : Colors.transparent,
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
                  Text(
                    "Décimale",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: gl.display.equipixel * gl.fontSizeXS,
                    ),
                  ),
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
  PopupSelectAttributeSet(BuildContext context) {
    String wantedSet = "Observation Composition";
    presentPopup(
      context: context,
      dismiss: true,
      after: () {},
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 4
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: Column(
            children: [
              Text(
                "Choisissez un ensemble",
                style: TextStyle(
                  color: Colors.white,
                  fontSize: gl.display.equipixel * gl.fontSizeM,
                ),
              ),
              SizedBox(
                width: gl.menuBarLength * gl.display.equipixel,
                child: SelectAttributeSet(
                  setChanged: (String it) {
                    wantedSet = it;
                  },
                ),
              ),
              stroke(
                gl.display.equipixel,
                gl.display.equipixel * .5,
                gl.colorAgroBioTech,
              ),
              Row(
                children: [
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Appliquer".length,
                      ),
                      child: Text("Appliquer", style: dialogTextButtonStyle()),
                      onPressed: () {
                        List<String> attributeNames = List<String>.generate(
                          gl.geometries[gl.selectedGeometry].attributes.length,
                          (i) {
                            return gl
                                .geometries[gl.selectedGeometry]
                                .attributes[i]
                                .name;
                          },
                        );

                        switch (wantedSet) {
                          case "Observation Composition":
                            if (!(attributeNames.contains("essence") &&
                                attributeNames.contains("rmq"))) {
                              gl.geometries[gl.selectedGeometry].attributes
                                  .addAll([
                                    Attribute(
                                      name: "essence",
                                      type: "string",
                                      value: "",
                                    ),
                                    Attribute(
                                      name: "rmq",
                                      type: "string",
                                      value: "",
                                    ),
                                  ]);
                            }
                            break;
                          default:
                        }
                        dismissPopup();
                      },
                    ),
                  ),
                  stroke(
                    vertical: true,
                    gl.display.equipixel,
                    gl.display.equipixel * .5,
                    Colors.transparent,
                  ),
                  SizedBox(
                    width: gl.menuBarLength * .5 * gl.display.equipixel,
                    child: TextButton(
                      style: dialogButtonStyle(
                        height: gl.display.equipixel * 12,
                        width: gl.display.equipixel * 10 * "Retour".length,
                      ),
                      child: Text("Retour", style: dialogTextButtonStyle()),
                      onPressed: () {
                        dismissPopup();
                      },
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
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
          width: gl.display.equipixel * gl.iconSizeM * 2.1,
          height: gl.display.equipixel * gl.iconSizeM * 1.5,
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
                  width: gl.display.equipixel * gl.iconSizeM * 1.9,
                  child: Text(
                    "Observation Composition",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: gl.display.equipixel * gl.fontSizeXS,
                    ),
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

/*class PopupDeleteAttribute {
  PopupDeleteAttribute(
    BuildContext context,
    List<String> entries,
    ValueChanged<List<int>> deleteEntries,
    VoidCallback onTapOutside,
  ) {
    TextEditingController textEditor = TextEditingController(text: "");
    presentPopup(
      context: context,
      dismiss: true,
      after: () {
        onTapOutside;
      },
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(color: gl.colorAgroBioTech, width: 2.0),
        ),
        backgroundColor: gl.backgroundTransparentBlackBox,
        content: SizedBox(
          width:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitWidth * gl.display.equipixel
                  : gl.popupWindowsLandscapeWidth * gl.display.equipixel,
          height:
              gl.display.orientation == Orientation.portrait
                  ? gl.popupWindowsPortraitHeight * gl.display.equipixel / 2
                  : gl.popupWindowsLandscapeHeight * gl.display.equipixel,
          child: Column(
            children: [
              Text(
                "Choisissez les lignes à enlever.",
                style: TextStyle(
                  color: Colors.white,
                  fontSize: gl.display.equipixel * gl.fontSizeS,
                ),
              ),
              SizedBox(
                height: 100 * gl.display.equipixel,
                child: DeleteAttribute(
                  deleteEntries: deleteEntries,
                  entries: entries,
                ),
              ),
              SizedBox(
                width: gl.menuBarLength * .5 * gl.display.equipixel,
                child: TextButton(
                  style: dialogButtonStyle(
                    height: gl.display.equipixel * 12,
                    width: gl.display.equipixel * 10 * "Ok".length,
                  ),
                  child: Text("Enlever", style: dialogTextButtonStyle()),
                  onPressed: () {
                    if (controlDuplicateAttributeName(textEditor.text)) {
                      return;
                    }
                    dismissPopup();
                  },
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class DeleteAttribute extends StatefulWidget {
  final List<String> entries;
  final ValueChanged<List<int>> deleteEntries;
  const DeleteAttribute({
    super.key,
    required this.deleteEntries,
    required this.entries,
  });

  @override
  State<StatefulWidget> createState() => _DeleteAttribute();
}

class _DeleteAttribute extends State<DeleteAttribute> {
  List<int> selected = [];

  @override
  Widget build(BuildContext context) {
    return Container();
  }
}*/

class PolygonListMenu extends StatefulWidget {
  final ValueChanged<LatLng> state;
  final VoidCallback after;

  const PolygonListMenu({super.key, required this.state, required this.after});

  @override
  State<StatefulWidget> createState() => _PolygonListMenu();
}

class _PolygonListMenu extends State<PolygonListMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  final ScrollController _controller = ScrollController();
  bool _keyboard = false;
  bool _doingAnaPt = false;

  void _scrollDown() {
    _controller.animateTo(
      _controller.position.maxScrollExtent +
          (gl.display.orientation == Orientation.portrait
              ? gl.display.equipixel * gl.polyListSelectedCardHeight
              : gl.display.equipixel * gl.polyListCardHeight),
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
              height: gl.display.equipixel * gl.fontSizeL * 1.2,
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
                                gl.fontSizeL * 1.2 -
                                gl.popupReturnButtonHeight -
                                gl.polyNewPolygonButtonHeight),
                        maxWidth:
                            gl.display.equipixel * gl.popupWindowsPortraitWidth,
                      )
                      : BoxConstraints(
                        maxHeight:
                            gl.display.equipixel *
                            (gl.popupWindowsLandscapeHeight -
                                gl.fontSizeL * 1.2),
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
                    if (gl.geometries.length < newIndex + 1 ||
                        gl.geometries.length < oldIndex + 1) {
                      return;
                    }
                    gl.refreshMainStack(() {
                      final Geometry item = gl.geometries.removeAt(oldIndex);
                      gl.geometries.insert(newIndex, item);
                    });
                    if (oldIndex == gl.selectedGeometry) {
                      gl.selectedGeometry = newIndex;
                    } else if (newIndex == gl.selectedGeometry) {
                      if (oldIndex > newIndex) {
                        gl.selectedGeometry++;
                      } else {
                        gl.selectedGeometry--;
                      }
                    } else if (oldIndex < gl.selectedGeometry &&
                        gl.selectedGeometry < newIndex) {
                      gl.selectedGeometry--;
                    } else if (oldIndex > gl.selectedGeometry &&
                        gl.selectedGeometry > newIndex) {
                      gl.selectedGeometry++;
                    }
                  });
                },
                children: List<
                  TextButton
                >.generate(gl.geometries.isEmpty ? 0 : gl.geometries.length, (
                  int i,
                ) {
                  return TextButton(
                    style: ButtonStyle(
                      fixedSize:
                          i == gl.selectedGeometry &&
                                  gl.display.orientation == Orientation.portrait
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
                              : WidgetStateProperty<Size>.fromMap(<
                                WidgetStatesConstraint,
                                Size
                              >{
                                WidgetState.any: Size(
                                  gl.display.equipixel * gl.polyListCardWidth,
                                  gl.display.equipixel * gl.polyListCardHeight,
                                ),
                              }),
                    ),
                    key: Key('$i'),
                    onPressed:
                        i == gl.selectedGeometry
                            ? () {
                              setState(() {
                                widget.state(gl.geometries[i].center);
                              });
                              gl.refreshMainStack(() {
                                gl.modeMapShowPolygons = true;
                              });
                            }
                            : () {
                              setState(() {
                                gl.selectedGeometry = i;
                                widget.state(gl.geometries[i].center);
                              });
                              gl.refreshMainStack(() {
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
                            gl.polyListSelectedCardWidth * gl.display.equipixel,
                        child: Card(
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadiusGeometry.circular(12.0),
                            side:
                                i == gl.selectedGeometry
                                    ? BorderSide(
                                      color: gl.geometries[i].colorInside
                                          .withAlpha(255),
                                      width: gl.display.equipixel * .75,
                                    )
                                    : BorderSide(
                                      color: gl.geometries[i].colorInside
                                          .withAlpha(120),
                                      width: gl.display.equipixel * .25,
                                    ),
                          ),
                          surfaceTintColor: Colors.transparent,
                          shadowColor: Colors.transparent,
                          color: Colors.black.withAlpha(100),
                          child:
                              i != gl.selectedGeometry ||
                                      gl.display.orientation ==
                                          Orientation.landscape
                                  ? Row(
                                    mainAxisAlignment:
                                        MainAxisAlignment.spaceBetween,
                                    children: [
                                      IconButton(
                                        onPressed: () {
                                          gl.refreshMainStack(() {
                                            if (!gl.Mode.editPolygon) {
                                              gl.geometries[i].visibleOnMap =
                                                  !gl
                                                      .geometries[i]
                                                      .visibleOnMap;
                                            }
                                          });
                                          setState(() {});
                                        },
                                        icon:
                                            gl.geometries[i].visibleOnMap
                                                ? FaIcon(
                                                  FontAwesomeIcons.eyeSlash,
                                                  size:
                                                      gl.display.equipixel *
                                                      gl.iconSizeS *
                                                      .9,
                                                  color: Colors.white,
                                                )
                                                : FaIcon(
                                                  FontAwesomeIcons.eye,
                                                  size:
                                                      gl.display.equipixel *
                                                      gl.iconSizeS *
                                                      .9,
                                                  color: Colors.white,
                                                ),
                                      ),
                                      SizedBox(
                                        width:
                                            gl.display.equipixel *
                                            gl.chosenPolyBarWidth *
                                            .5,
                                        child: Stack(
                                          children: [
                                            Row(
                                              children: [
                                                Container(
                                                  alignment: Alignment.topLeft,
                                                  child:
                                                      gl.geometries[i].type ==
                                                              "Point"
                                                          ? Text(
                                                            "POINT",
                                                            style: TextStyle(
                                                              color:
                                                                  Colors.yellow,
                                                              fontSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.fontSizeXS *
                                                                  .9,
                                                            ),
                                                          )
                                                          : gl
                                                                  .geometries[i]
                                                                  .type ==
                                                              "Polygon"
                                                          ? Text(
                                                            "POLY",
                                                            style: TextStyle(
                                                              color:
                                                                  Colors.green,
                                                              fontSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.fontSizeXS *
                                                                  .9,
                                                            ),
                                                          )
                                                          : Text(
                                                            "OHA?",
                                                            style: TextStyle(
                                                              color: Colors.red,
                                                              fontSize:
                                                                  gl
                                                                      .display
                                                                      .equipixel *
                                                                  gl.fontSizeXS *
                                                                  .9,
                                                            ),
                                                          ),
                                                ),
                                                SizedBox(
                                                  width:
                                                      gl.display.equipixel * 2,
                                                ),
                                                if (gl
                                                    .geometries[i]
                                                    .sentToServer)
                                                  Container(
                                                    alignment:
                                                        Alignment.topLeft,
                                                    child: Text(
                                                      "SENT",
                                                      style: TextStyle(
                                                        color: Colors.red,
                                                        fontSize:
                                                            gl
                                                                .display
                                                                .equipixel *
                                                            gl.fontSizeXS *
                                                            .9,
                                                      ),
                                                    ),
                                                  ),
                                              ],
                                            ),
                                            Container(
                                              alignment: Alignment.center,
                                              child: SingleChildScrollView(
                                                scrollDirection:
                                                    Axis.horizontal,
                                                child: Text(
                                                  gl.geometries[i].name,
                                                  textAlign: TextAlign.center,
                                                  style: TextStyle(
                                                    color: Colors.white,
                                                    fontSize:
                                                        gl.display.equipixel *
                                                        gl.fontSizeL,
                                                  ),
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                      ),
                                      IconButton(
                                        onPressed: () {
                                          PopupColorChooser(
                                            gl.geometries[i].colorInside,
                                            gl.notificationContext!,
                                            //change color
                                            (Color col) {
                                              setState(() {
                                                gl.geometries[i].setColorInside(
                                                  col,
                                                );
                                                gl.geometries[i].setColorLine(
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
                                              gl.geometries[i].serialize();
                                            },
                                          );
                                        },
                                        icon: Icon(
                                          Icons.color_lens,
                                          color: gl.geometries[i].colorInside,
                                          size:
                                              gl.display.equipixel *
                                              gl.iconSizeM *
                                              .75,
                                        ),
                                      ),
                                    ],
                                  )
                                  : Column(
                                    mainAxisAlignment:
                                        MainAxisAlignment.spaceAround,
                                    children: [
                                      Row(
                                        mainAxisAlignment:
                                            MainAxisAlignment.spaceBetween,
                                        children: [
                                          IconButton(
                                            onPressed: () {
                                              gl.refreshMainStack(() {
                                                if (!gl.Mode.editPolygon) {
                                                  gl
                                                      .geometries[i]
                                                      .visibleOnMap = !gl
                                                          .geometries[i]
                                                          .visibleOnMap;
                                                }
                                              });
                                              setState(() {});
                                            },
                                            icon:
                                                gl.geometries[i].visibleOnMap
                                                    ? FaIcon(
                                                      FontAwesomeIcons.eyeSlash,
                                                      size:
                                                          gl.display.equipixel *
                                                          gl.iconSizeS *
                                                          .9,
                                                      color: Colors.white,
                                                    )
                                                    : FaIcon(
                                                      FontAwesomeIcons.eye,
                                                      size:
                                                          gl.display.equipixel *
                                                          gl.iconSizeS *
                                                          .9,
                                                      color: Colors.white,
                                                    ),
                                          ),
                                          SizedBox(
                                            width:
                                                gl.display.equipixel *
                                                gl.chosenPolyBarWidth *
                                                .5,
                                            child: Stack(
                                              children: [
                                                Row(
                                                  children: [
                                                    Container(
                                                      alignment:
                                                          Alignment.topLeft,
                                                      child:
                                                          gl.geometries[i].type ==
                                                                  "Point"
                                                              ? Text(
                                                                "POINT",
                                                                style: TextStyle(
                                                                  color:
                                                                      Colors
                                                                          .yellow,
                                                                  fontSize:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.fontSizeXS *
                                                                      .9,
                                                                ),
                                                              )
                                                              : gl
                                                                      .geometries[i]
                                                                      .type ==
                                                                  "Polygon"
                                                              ? Text(
                                                                "POLY",
                                                                style: TextStyle(
                                                                  color:
                                                                      Colors
                                                                          .green,
                                                                  fontSize:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.fontSizeXS *
                                                                      .9,
                                                                ),
                                                              )
                                                              : Text(
                                                                "OHA?",
                                                                style: TextStyle(
                                                                  color:
                                                                      Colors
                                                                          .red,
                                                                  fontSize:
                                                                      gl
                                                                          .display
                                                                          .equipixel *
                                                                      gl.fontSizeXS *
                                                                      .9,
                                                                ),
                                                              ),
                                                    ),
                                                    SizedBox(
                                                      width:
                                                          gl.display.equipixel *
                                                          2,
                                                    ),
                                                    if (gl
                                                        .geometries[i]
                                                        .sentToServer)
                                                      Container(
                                                        alignment:
                                                            Alignment
                                                                .bottomLeft,
                                                        child: Text(
                                                          "SENT",
                                                          style: TextStyle(
                                                            color: Colors.red,
                                                            fontSize:
                                                                gl
                                                                    .display
                                                                    .equipixel *
                                                                gl.fontSizeXS *
                                                                .9,
                                                          ),
                                                        ),
                                                      ),
                                                  ],
                                                ),
                                                TextButton(
                                                  style: ButtonStyle(
                                                    animationDuration: Duration(
                                                      seconds: 1,
                                                    ),
                                                    backgroundColor:
                                                        WidgetStateProperty<
                                                          Color
                                                        >.fromMap(<
                                                          WidgetStatesConstraint,
                                                          Color
                                                        >{
                                                          WidgetState.any:
                                                              Colors
                                                                  .transparent,
                                                        }),
                                                    padding: WidgetStateProperty<
                                                      EdgeInsetsGeometry
                                                    >.fromMap(<
                                                      WidgetStatesConstraint,
                                                      EdgeInsetsGeometry
                                                    >{
                                                      WidgetState.any:
                                                          EdgeInsetsGeometry
                                                              .zero,
                                                    }),
                                                  ),
                                                  onPressed: () {
                                                    setState(() {
                                                      PopupValueChange(
                                                        "string",
                                                        gl.geometries[i].name,
                                                        (value) {
                                                          gl
                                                                  .geometries[i]
                                                                  .name =
                                                              value.toString();
                                                        },
                                                        () {},
                                                        () {
                                                          gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .serialize();
                                                        },
                                                      );
                                                    });
                                                  },
                                                  child: Container(
                                                    alignment: Alignment.center,
                                                    child: SingleChildScrollView(
                                                      scrollDirection:
                                                          Axis.horizontal,
                                                      child: Text(
                                                        gl.geometries[i].name,
                                                        textAlign:
                                                            TextAlign.center,
                                                        style: TextStyle(
                                                          color: Colors.white,
                                                          fontSize:
                                                              gl
                                                                  .display
                                                                  .equipixel *
                                                              gl.fontSizeL,
                                                        ),
                                                      ),
                                                    ),
                                                  ),
                                                ),
                                              ],
                                            ),
                                          ),
                                          IconButton(
                                            onPressed: () {
                                              PopupColorChooser(
                                                gl.geometries[i].colorInside,
                                                gl.notificationContext!,
                                                //change color
                                                (Color col) {
                                                  setState(() {
                                                    gl.geometries[i]
                                                        .setColorInside(col);
                                                    gl.geometries[i]
                                                        .setColorLine(
                                                          Color.fromRGBO(
                                                            (col.r * 255)
                                                                .round(),
                                                            (col.g * 255)
                                                                .round(),
                                                            (col.b * 255)
                                                                .round(),
                                                            1.0,
                                                          ),
                                                        );
                                                  });
                                                },
                                                () {},
                                                () {
                                                  gl.geometries[i].serialize();
                                                },
                                              );
                                            },
                                            icon: Icon(
                                              Icons.color_lens,
                                              color:
                                                  gl.geometries[i].colorInside,
                                              size:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM *
                                                  .75,
                                            ),
                                          ),
                                        ],
                                      ),
                                      Row(
                                        mainAxisAlignment:
                                            MainAxisAlignment.spaceBetween,
                                        children: [
                                          Row(
                                            children: [
                                              if (gl.geometries[i]
                                                      .containsAttribute(
                                                        "essence",
                                                      ) &&
                                                  gl.geometries[i]
                                                      .containsAttribute("rmq"))
                                                IconButton(
                                                  onPressed: () {
                                                    if (gl
                                                            .UserData
                                                            .forename
                                                            .isEmpty ||
                                                        gl
                                                            .UserData
                                                            .name
                                                            .isEmpty ||
                                                        gl
                                                            .UserData
                                                            .mail
                                                            .isEmpty) {
                                                      PopupUserData(
                                                        context,
                                                        () {},
                                                        () {
                                                          PopupDoYouReally(
                                                            context,
                                                            () {
                                                              gl.geometries[i]
                                                                  .sendGeometryToServer();
                                                            },
                                                            "Attention !",
                                                            gl.labelSendCompoFeature,
                                                            "Envoyer",
                                                            "Ne pas envoyer",
                                                          );
                                                        },
                                                        oldForename:
                                                            gl
                                                                .UserData
                                                                .forename,
                                                        oldName:
                                                            gl.UserData.name,
                                                        oldMail:
                                                            gl.UserData.mail,
                                                      );
                                                    } else {
                                                      PopupDoYouReally(
                                                        context,
                                                        () {
                                                          gl.geometries[i]
                                                              .sendGeometryToServer();
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
                                                    color: Colors.white,
                                                    size:
                                                        gl.iconSizeS *
                                                        gl.display.equipixel,
                                                  ),
                                                ),
                                              if (gl.geometries[i].type ==
                                                      "Polygon" &&
                                                  gl
                                                          .geometries[i]
                                                          .points
                                                          .length >
                                                      2)
                                                IconButton(
                                                  onPressed: () async {
                                                    if (await gl.geometries[i]
                                                        .onlineSurfaceAnalysis()) {
                                                      gl.mainStack.add(
                                                        popupAnaSurfResultsMenu(
                                                          gl.notificationContext!,
                                                          gl
                                                              .geometries[gl
                                                                  .selectedGeometry]
                                                              .decodedJson,
                                                          () {
                                                            gl.refreshMainStack(
                                                              () {},
                                                            );
                                                          },
                                                          () {
                                                            gl.refreshMainStack(
                                                              () {},
                                                            );
                                                          },
                                                        ),
                                                      );
                                                      gl.refreshMainStack(
                                                        () {},
                                                      );
                                                    }
                                                  },
                                                  icon: Icon(
                                                    Icons.analytics,
                                                    color: Colors.white,
                                                    size:
                                                        gl.display.equipixel *
                                                        gl.iconSizeM *
                                                        .75,
                                                  ),
                                                ),
                                              if (gl.geometries[i].type ==
                                                      "Point" &&
                                                  gl
                                                      .geometries[i]
                                                      .points
                                                      .isNotEmpty)
                                                IconButton(
                                                  onPressed: () async {
                                                    if (!_doingAnaPt) {
                                                      _doingAnaPt = true;
                                                      await gl.geometries[i]
                                                          .runAnaPt();
                                                      gl.refreshMainStack(() {
                                                        PopupAnaResultsMenu(
                                                          gl.notificationContext!,
                                                          gl.requestedLayers,
                                                          () {
                                                            setState(() {});
                                                          },
                                                        );
                                                      });

                                                      _doingAnaPt = false;
                                                    }
                                                  },
                                                  icon: Icon(
                                                    Icons.location_pin,
                                                    color: Colors.white,
                                                    size:
                                                        gl.display.equipixel *
                                                        gl.iconSizeM *
                                                        .75,
                                                  ),
                                                ),
                                            ],
                                          ),
                                          Container(
                                            alignment: Alignment.center,
                                            width:
                                                gl.display.equipixel *
                                                gl.iconSizeM *
                                                1.1,
                                            height:
                                                gl.display.equipixel *
                                                gl.iconSizeM *
                                                1.1,
                                            child: IconButton(
                                              onPressed: () {
                                                PopupDoYouReally(
                                                  gl.notificationContext!,
                                                  () {
                                                    setState(() {
                                                      Geometry.deleteLayerFromShared(
                                                        gl.geometries[i].id,
                                                      );
                                                      if (i > 0) {
                                                        gl.geometries.removeAt(
                                                          i,
                                                        );
                                                        gl.selectedGeometry--;
                                                      } else if (i == 0 &&
                                                          gl
                                                              .geometries
                                                              .isNotEmpty) {
                                                        gl.geometries.removeAt(
                                                          i,
                                                        );
                                                      }
                                                    });
                                                  },
                                                  "Message",
                                                  "\nVoulez vous vraiment supprimer ${gl.geometries[i].name}?\n",
                                                );
                                              },
                                              icon: Icon(
                                                Icons.delete_forever,
                                                color: Colors.white,
                                                size:
                                                    gl.display.equipixel *
                                                    gl.iconSizeM *
                                                    .75,
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
                  );
                }),
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
                    gl.geometries.add(
                      Geometry(
                        polygonName: "Nouveau${gl.geometries.length - 1}",
                      ),
                    );
                    PopupNewPolygon(
                      context,
                      "",
                      gl.geometries[gl.geometries.length - 1].colorInside,
                      (String typeIt) {
                        if (mounted) {
                          setState(() {
                            gl.geometries[gl.geometries.length - 1].type =
                                typeIt;
                            _keyboard = true;
                          });
                        } else {
                          gl.geometries[gl.geometries.length - 1].type = typeIt;
                          _keyboard = true;
                        }
                      },
                      (String nameIt) {
                        if (mounted) {
                          setState(() {
                            gl.geometries[gl.geometries.length - 1].name =
                                nameIt;
                            _keyboard = true;
                          });
                        } else {
                          gl.geometries[gl.geometries.length - 1].name = nameIt;
                          _keyboard = true;
                        }
                      },
                      (Color colorIt) {
                        if (mounted) {
                          setState(() {
                            gl
                                .geometries[gl.geometries.length - 1]
                                .colorInside = colorIt;
                            gl
                                .geometries[gl.geometries.length - 1]
                                .colorLine = colorIt.withAlpha(255);
                            _keyboard = true;
                          });
                        } else {
                          gl.geometries[gl.geometries.length - 1].colorInside =
                              colorIt;
                          gl
                              .geometries[gl.geometries.length - 1]
                              .colorLine = colorIt.withAlpha(255);
                          _keyboard = true;
                        }
                      },
                      () {
                        if (mounted) {
                          setState(() {
                            _keyboard = false;
                          });
                        } else {
                          _keyboard = false;
                        }
                      },
                      () {
                        if (mounted) {
                          setState(() {
                            _keyboard = true;
                          });
                        } else {
                          _keyboard = true;
                        }
                      },
                      () {
                        gl.geometries[gl.geometries.length - 1].serialize();
                      },
                    );
                    gl.selectedGeometry = gl.geometries.length - 1;
                  });

                  gl.refreshMainStack(() {
                    gl.selectedGeometry = gl.geometries.length - 1;
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
        if (gl.display.orientation == Orientation.landscape &&
            !_keyboard &&
            gl.selectedGeometry > -1)
          Column(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              if (gl.geometries.isNotEmpty)
                SizedBox(
                  width: gl.polyListSelectedCardWidth * gl.display.equipixel,
                  child: Card(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadiusGeometry.circular(12.0),
                    ),

                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color: gl.geometries[gl.selectedGeometry].colorInside
                        .withAlpha(255),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSizeM * 1.1,
                          child: IconButton(
                            onPressed: () {
                              PopupDoYouReally(
                                gl.notificationContext!,
                                () {
                                  Geometry.deleteLayerFromShared(
                                    gl.geometries[gl.selectedGeometry].id,
                                  );
                                  setState(() {
                                    //remove polygon
                                    if (gl.selectedGeometry > 0) {
                                      gl.geometries.removeAt(
                                        gl.selectedGeometry,
                                      );
                                      gl.selectedGeometry--;
                                    } else if (gl.selectedGeometry == 0 &&
                                        gl.geometries.isNotEmpty) {
                                      gl.geometries.removeAt(
                                        gl.selectedGeometry,
                                      );
                                    }
                                  });
                                },
                                "Message",
                                "\nVoulez vous vraiment supprimer ${gl.geometries[gl.selectedGeometry].name}?\n",
                              );
                            },
                            icon: Icon(
                              Icons.delete_forever,
                              color: Colors.black,
                              size: gl.display.equipixel * gl.iconSizeM * .75,
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
                                    gl.geometries[gl.selectedGeometry].name,
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
                                    gl.geometries[gl.selectedGeometry].name,
                                    (String nameIt) {
                                      setState(() {
                                        gl
                                            .geometries[gl.selectedGeometry]
                                            .name = nameIt;
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
                                    () {
                                      gl.geometries[gl.selectedGeometry]
                                          .serialize();
                                    },
                                  );
                                },
                              ),

                              Text(
                                "${(gl.geometries[gl.selectedGeometry].area / 100).round() / 100} Ha",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                              Text(
                                "${(gl.geometries[gl.selectedGeometry].perimeter).round() / 1000} km",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                            ],
                          ),
                        ),
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSizeM * 1.1,
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
                                            .geometries[gl.selectedGeometry]
                                            .colorInside,
                                        gl.notificationContext!,
                                        //change color
                                        (Color col) {
                                          setState(() {
                                            gl.geometries[gl.selectedGeometry]
                                                .setColorInside(col);
                                            gl.geometries[gl.selectedGeometry]
                                                .setColorLine(
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
                                          gl.geometries[gl.selectedGeometry]
                                              .serialize();
                                        },
                                      );
                                    },
                                    icon: Icon(
                                      Icons.color_lens,
                                      color: Colors.black,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeM *
                                          .75,
                                    ),
                                  ),
                                  IconButton(
                                    onPressed: () async {
                                      if (await gl
                                          .geometries[gl.selectedGeometry]
                                          .onlineSurfaceAnalysis()) {
                                        gl.mainStack.add(
                                          popupAnaSurfResultsMenu(
                                            gl.notificationContext!,
                                            gl
                                                .geometries[gl.selectedGeometry]
                                                .decodedJson,
                                            () {
                                              gl.refreshMainStack(() {});
                                            },
                                            () {
                                              gl.refreshMainStack(() {});
                                            },
                                          ),
                                        );
                                        gl.refreshMainStack(() {});
                                      }
                                    },
                                    icon: Icon(
                                      Icons.analytics,
                                      color: Colors.black,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeM *
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
                      gl.geometries.add(Geometry(polygonName: "Nouveau"));
                      PopupNewPolygon(
                        context,
                        "",
                        gl.geometries[gl.geometries.length - 1].colorInside,
                        (String typeIt) {
                          if (mounted) {
                            setState(() {
                              gl.geometries[gl.geometries.length - 1].type =
                                  typeIt;
                              _keyboard = true;
                            });
                          } else {
                            gl.geometries[gl.geometries.length - 1].type =
                                typeIt;
                            _keyboard = true;
                          }
                        },
                        (String nameIt) {
                          if (mounted) {
                            setState(() {
                              gl.geometries[gl.geometries.length - 1].name =
                                  nameIt;
                              _keyboard = true;
                            });
                          } else {
                            gl.geometries[gl.geometries.length - 1].name =
                                nameIt;
                            _keyboard = true;
                          }
                        },
                        (Color colorIt) {
                          if (mounted) {
                            setState(() {
                              gl
                                  .geometries[gl.geometries.length - 1]
                                  .colorInside = colorIt;
                              gl
                                  .geometries[gl.geometries.length - 1]
                                  .colorLine = colorIt.withAlpha(255);
                              _keyboard = true;
                            });
                          } else {
                            gl
                                .geometries[gl.geometries.length - 1]
                                .colorInside = colorIt;
                            gl
                                .geometries[gl.geometries.length - 1]
                                .colorLine = colorIt.withAlpha(255);
                            _keyboard = true;
                          }
                        },
                        () {
                          if (mounted) {
                            setState(() {
                              _keyboard = false;
                            });
                          } else {
                            _keyboard = false;
                          }
                        },
                        () {
                          if (mounted) {
                            setState(() {
                              _keyboard = true;
                            });
                          } else {
                            _keyboard = true;
                          }
                        },
                        () {
                          gl.geometries[gl.geometries.length - 1].serialize();
                        },
                      );
                      gl.selectedGeometry = gl.geometries.length - 1;
                    });

                    gl.refreshMainStack(() {
                      gl.selectedGeometry = gl.geometries.length - 1;
                    });
                    _scrollDown();
                  },
                ),
              ),
              if (!_keyboard)
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    SizedBox(width: gl.display.equipixel * gl.iconSizeM * .25),
                    _returnButton(context, widget.after),
                  ],
                ),
            ],
          ),
      ]),
    );
  }
}

class PathListMenu extends StatefulWidget {
  final ValueChanged<LatLng> state;
  final VoidCallback after;

  const PathListMenu({super.key, required this.state, required this.after});

  @override
  State<StatefulWidget> createState() => _PathListMenu();
}

class _PathListMenu extends State<PathListMenu> {
  final Color active = Colors.black;
  final Color inactive = const Color.fromARGB(255, 92, 92, 92);
  final ScrollController _controller = ScrollController();
  bool _keyboard = false;

  void _scrollDown() {
    _controller.animateTo(
      _controller.position.maxScrollExtent +
          (gl.display.orientation == Orientation.portrait
              ? gl.display.equipixel * gl.polyListSelectedCardHeight
              : gl.display.equipixel * gl.polyListCardHeight),
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
              height: gl.display.equipixel * gl.fontSizeL * 1.2,
              child: Text(
                "Liste des chemins",
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
                                gl.fontSizeL * 1.2 -
                                gl.popupReturnButtonHeight -
                                gl.polyNewPolygonButtonHeight),
                        maxWidth:
                            gl.display.equipixel * gl.popupWindowsPortraitWidth,
                      )
                      : BoxConstraints(
                        maxHeight:
                            gl.display.equipixel *
                            (gl.popupWindowsLandscapeHeight -
                                gl.fontSizeL * 1.2),
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
                    if (gl.pathLayers.length < newIndex + 1 ||
                        gl.pathLayers.length < oldIndex + 1) {
                      return;
                    }
                    gl.refreshMainStack(() {
                      final PathLayer item = gl.pathLayers.removeAt(oldIndex);
                      gl.pathLayers.insert(newIndex, item);
                    });
                    if (oldIndex == gl.selectedpathLayer) {
                      gl.selectedpathLayer = newIndex;
                    } else if (newIndex == gl.selectedpathLayer) {
                      if (oldIndex > newIndex) {
                        gl.selectedpathLayer++;
                      } else {
                        gl.selectedpathLayer--;
                      }
                    } else if (oldIndex < gl.selectedpathLayer &&
                        gl.selectedpathLayer < newIndex) {
                      gl.selectedpathLayer--;
                    } else if (oldIndex > gl.selectedpathLayer &&
                        gl.selectedpathLayer > newIndex) {
                      gl.selectedpathLayer++;
                    }
                  });
                },
                children: List<
                  TextButton
                >.generate(gl.pathLayers.isEmpty ? 0 : gl.pathLayers.length, (
                  int i,
                ) {
                  Color activeTextColor =
                      i == gl.selectedpathLayer
                          ? getColorTextFromBackground(
                            i == gl.selectedpathLayer
                                ? gl.pathLayers[i].colorInside.withAlpha(255)
                                : Colors.grey.withAlpha(100),
                          )
                          : getColorTextFromBackground(
                            i == gl.selectedpathLayer
                                ? gl.pathLayers[i].colorInside.withAlpha(255)
                                : Colors.grey.withAlpha(100),
                          ).withAlpha(128);
                  return TextButton(
                    style: ButtonStyle(
                      fixedSize:
                          i == gl.selectedpathLayer &&
                                  gl.display.orientation == Orientation.portrait
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
                              : WidgetStateProperty<Size>.fromMap(<
                                WidgetStatesConstraint,
                                Size
                              >{
                                WidgetState.any: Size(
                                  gl.display.equipixel * gl.polyListCardWidth,
                                  gl.display.equipixel * gl.polyListCardHeight,
                                ),
                              }),
                    ),
                    key: Key('$i'),
                    onPressed:
                        i == gl.selectedpathLayer
                            ? () {
                              setState(() {
                                widget.state(gl.pathLayers[i].center);
                              });
                              gl.refreshMainStack(() {
                                gl.modeMapShowPolygons = true;
                              });
                            }
                            : () {
                              setState(() {
                                gl.selectedpathLayer = i;
                                widget.state(gl.pathLayers[i].center);
                              });
                              gl.refreshMainStack(() {
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
                            gl.polyListSelectedCardWidth * gl.display.equipixel,
                        child: Card(
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadiusGeometry.circular(12.0),
                            side:
                                i == gl.selectedpathLayer &&
                                        gl.display.orientation ==
                                            Orientation.portrait
                                    ? BorderSide(
                                      color: Colors.transparent,
                                      width: 0.0,
                                    )
                                    : i == gl.selectedpathLayer
                                    ? BorderSide(
                                      color: gl.pathLayers[i].colorInside
                                          .withAlpha(100),
                                      width: 2.0,
                                    )
                                    : BorderSide(
                                      color: gl.pathLayers[i].colorInside
                                          .withAlpha(150),
                                      width: 4.0,
                                    ),
                          ),
                          surfaceTintColor: Colors.transparent,
                          shadowColor: Colors.transparent,
                          color:
                              i == gl.selectedpathLayer
                                  ? gl.pathLayers[i].colorInside.withAlpha(255)
                                  : Colors.grey.withAlpha(150),
                          child:
                              i != gl.selectedpathLayer ||
                                      gl.display.orientation ==
                                          Orientation.landscape
                                  ? Row(
                                    mainAxisAlignment: MainAxisAlignment.center,
                                    children: [
                                      Container(
                                        alignment: Alignment.center,
                                        constraints: BoxConstraints(
                                          maxWidth:
                                              gl.display.orientation.index == 1
                                                  ? gl.display.equipixel *
                                                      gl.polyListCardWidth *
                                                      .5
                                                  : gl.display.equipixel *
                                                      gl.polyListSelectedCardWidth *
                                                      .5,
                                        ),
                                        child: Text(
                                          gl.pathLayers[i].name,
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
                                      if (i == gl.selectedpathLayer)
                                        Container(
                                          alignment: Alignment.center,
                                          width:
                                              gl.display.equipixel *
                                              gl.iconSizeM *
                                              1.1,
                                          height:
                                              gl.display.equipixel *
                                              gl.iconSizeM *
                                              1.1,
                                          child: IconButton(
                                            onPressed: () {
                                              PopupDoYouReally(
                                                gl.notificationContext!,
                                                () {
                                                  setState(() {
                                                    //remove polygon
                                                    Geometry.deleteLayerFromShared(
                                                      gl.pathLayers[i].id,
                                                    );
                                                    if (i > 0) {
                                                      gl.pathLayers.removeAt(i);
                                                      gl.selectedpathLayer--;
                                                    } else if (i == 0 &&
                                                        gl
                                                            .pathLayers
                                                            .isNotEmpty) {
                                                      gl.pathLayers.removeAt(i);
                                                    }
                                                  });
                                                },
                                                "Message",
                                                "\nVoulez vous vraiment supprimer ${gl.pathLayers[i].name}?\n",
                                              );
                                            },
                                            icon: Icon(
                                              Icons.delete_forever,
                                              color: activeTextColor,
                                              size:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM *
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
                                                  gl.pathLayers[i].name,
                                                  style: TextStyle(
                                                    color: activeTextColor,
                                                    fontSize:
                                                        gl.display.equipixel *
                                                        gl.fontSizeM,
                                                  ),
                                                ),
                                              ),
                                              onPressed: () {
                                                PopupNameIntroducer(
                                                  context,
                                                  gl.pathLayers[i].name,
                                                  (String nameIt) {
                                                    setState(() {
                                                      gl.pathLayers[i].name =
                                                          nameIt;
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
                                                  () {
                                                    gl.pathLayers[i]
                                                        .serialize();
                                                  },
                                                );
                                              },
                                            ),

                                            if (i == gl.selectedpathLayer)
                                              Text(
                                                "${(gl.pathLayers[i].length).round() / 1000} km",
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
                                      if (i == gl.selectedpathLayer)
                                        Container(
                                          alignment: Alignment.center,
                                          width:
                                              gl.display.orientation.index ==
                                                          1 &&
                                                      i != gl.selectedpathLayer
                                                  ? 0.0
                                                  : gl.display.equipixel *
                                                      gl.iconSizeM *
                                                      1.2,

                                          child: Column(
                                            mainAxisAlignment:
                                                MainAxisAlignment.spaceEvenly,
                                            children: [
                                              IconButton(
                                                onPressed: () {
                                                  PopupColorChooser(
                                                    gl
                                                        .pathLayers[i]
                                                        .colorInside,
                                                    gl.notificationContext!,
                                                    //change color
                                                    (Color col) {
                                                      setState(() {
                                                        gl.pathLayers[i]
                                                            .setColorInside(
                                                              col,
                                                            );
                                                        gl.pathLayers[i]
                                                            .setColorLine(
                                                              Color.fromRGBO(
                                                                (col.r * 255)
                                                                    .round(),
                                                                (col.g * 255)
                                                                    .round(),
                                                                (col.b * 255)
                                                                    .round(),
                                                                1.0,
                                                              ),
                                                            );
                                                      });
                                                    },
                                                    () {},
                                                    () {
                                                      gl.pathLayers[i]
                                                          .serialize();
                                                    },
                                                  );
                                                },
                                                icon: Icon(
                                                  Icons.color_lens,
                                                  color: activeTextColor,
                                                  size:
                                                      gl.display.equipixel *
                                                      gl.iconSizeM *
                                                      .75,
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
                }),
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
                    gl.pathLayers.add(PathLayer(pathName: "Nouveau"));
                    PopupNameIntroducer(
                      context,
                      "",
                      (String nameIt) {
                        if (mounted) {
                          setState(() {
                            _keyboard = true;
                          });
                        } else {
                          gl.pathLayers[gl.pathLayers.length - 1].name = nameIt;
                        }
                      },
                      () {
                        if (mounted) {
                          setState(() {
                            _keyboard = false;
                          });
                        } else {
                          _keyboard = false;
                        }
                      },
                      () {
                        if (mounted) {
                          setState(() {
                            _keyboard = true;
                          });
                        } else {
                          _keyboard = true;
                        }
                      },
                      () {
                        gl.pathLayers[gl.pathLayers.length - 1].serialize();
                      },
                    );
                    gl.selectedpathLayer = gl.pathLayers.length - 1;
                  });

                  gl.refreshMainStack(() {
                    gl.selectedpathLayer = gl.pathLayers.length - 1;
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
              if (gl.pathLayers.isNotEmpty)
                SizedBox(
                  width: gl.polyListSelectedCardWidth * gl.display.equipixel,
                  child: Card(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadiusGeometry.circular(12.0),
                    ),

                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color: gl.pathLayers[gl.selectedpathLayer].colorInside
                        .withAlpha(255),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSizeM * 1.1,
                          child: IconButton(
                            onPressed: () {
                              PopupDoYouReally(
                                gl.notificationContext!,
                                () {
                                  setState(() {
                                    //remove polygon
                                    PathLayer.delete(
                                      gl.pathLayers[gl.selectedpathLayer].id,
                                    );
                                    if (gl.selectedpathLayer > 0) {
                                      gl.pathLayers.removeAt(
                                        gl.selectedpathLayer,
                                      );
                                      gl.selectedpathLayer--;
                                    } else if (gl.selectedpathLayer == 0 &&
                                        gl.pathLayers.isNotEmpty) {
                                      gl.pathLayers.removeAt(
                                        gl.selectedpathLayer,
                                      );
                                    }
                                  });
                                },
                                "Message",
                                "\nVoulez vous vraiment supprimer ${gl.pathLayers[gl.selectedpathLayer].name}?\n",
                              );
                            },
                            icon: Icon(
                              Icons.delete_forever,
                              color: Colors.black,
                              size: gl.display.equipixel * gl.iconSizeM * .75,
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
                                    gl.pathLayers[gl.selectedpathLayer].name,
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
                                    gl.pathLayers[gl.selectedpathLayer].name,
                                    (String nameIt) {
                                      setState(() {
                                        gl
                                            .pathLayers[gl.selectedpathLayer]
                                            .name = nameIt;
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
                                    () {
                                      gl.pathLayers[gl.selectedpathLayer]
                                          .serialize();
                                    },
                                  );
                                },
                              ),

                              Text(
                                "${(gl.pathLayers[gl.selectedpathLayer].length).round() / 1000} km",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeM,
                                ),
                              ),
                            ],
                          ),
                        ),
                        SizedBox(
                          width: gl.display.equipixel * gl.iconSizeM * 1.1,
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
                                            .pathLayers[gl.selectedpathLayer]
                                            .colorInside,
                                        gl.notificationContext!,
                                        //change color
                                        (Color col) {
                                          setState(() {
                                            gl.pathLayers[gl.selectedpathLayer]
                                                .setColorInside(col);
                                            gl.pathLayers[gl.selectedpathLayer]
                                                .setColorLine(
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
                                          gl.pathLayers[gl.selectedpathLayer]
                                              .serialize();
                                        },
                                      );
                                    },
                                    icon: Icon(
                                      Icons.color_lens,
                                      color: Colors.black,
                                      size:
                                          gl.display.equipixel *
                                          gl.iconSizeM *
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
                      gl.pathLayers.add(PathLayer(pathName: "Nouveau"));
                      PopupNameIntroducer(
                        context,
                        "",
                        (String nameIt) {
                          setState(() {
                            gl.pathLayers[gl.pathLayers.length - 1].name =
                                nameIt;
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
                        () {
                          gl.pathLayers[gl.pathLayers.length - 1].serialize();
                        },
                      );
                      gl.selectedpathLayer = gl.pathLayers.length - 1;
                    });

                    gl.refreshMainStack(() {
                      gl.selectedpathLayer = gl.pathLayers.length - 1;
                    });
                    _scrollDown();
                  },
                ),
              ),
              if (!_keyboard)
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    SizedBox(width: gl.display.equipixel * gl.iconSizeM * .25),
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
  ValueChanged<LatLng> state,
  VoidCallback after,
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
        gl.refreshMainStack(() {
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
  final ValueChanged<LatLng> state;
  final VoidCallback after;

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
                        gl.shared!.setBool('modeDevelopper', gl.modeDevelopper);
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

Widget forestimatorSettingsUserData() {
  return OrientationBuilder(
    builder: (context, orientation) {
      return Container(
        padding: EdgeInsets.all(7.5),
        child: Column(
          children: [
            stroke(
              vertical: false,
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
            Row(
              children: [
                SizedBox(
                  width: gl.display.equipixel * 40,
                  child: Text(
                    "Prénom",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: gl.display.equipixel * gl.fontSizeS,
                    ),
                  ),
                ),
                stroke(
                  vertical: true,
                  gl.display.equipixel,
                  gl.display.equipixel * .5,
                  gl.colorAgroBioTech,
                ),
                Container(
                  alignment: Alignment.center,
                  width: gl.display.equipixel * 40,
                  height: gl.display.equipixel * gl.iconSizeXS,
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
                    onLongPress: () {
                      PopupValueChange(
                        "string",
                        gl.UserData.forename,
                        (value) {
                          gl.UserData.forename = value;
                        },
                        () {},
                        () {
                          gl.UserData.serialize();
                        },
                      );
                    },
                    child: Container(
                      alignment: Alignment.centerLeft,
                      width: gl.display.equipixel * 40,
                      child: SingleChildScrollView(
                        scrollDirection: Axis.horizontal,
                        child: Text(
                          gl.UserData.forename,
                          style: TextStyle(
                            color: Colors.black,
                            fontSize: gl.display.equipixel * gl.fontSizeS,
                          ),
                        ),
                      ),
                    ),
                  ),
                ),
              ],
            ),
            stroke(
              vertical: false,
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
            Row(
              children: [
                SizedBox(
                  width: gl.display.equipixel * 40,
                  child: Text(
                    "Nom",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: gl.display.equipixel * gl.fontSizeS,
                    ),
                  ),
                ),
                stroke(
                  vertical: true,
                  gl.display.equipixel,
                  gl.display.equipixel * .5,
                  gl.colorAgroBioTech,
                ),
                Container(
                  alignment: Alignment.center,
                  width: gl.display.equipixel * 40,
                  height: gl.display.equipixel * gl.iconSizeXS,
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
                    onLongPress: () {
                      PopupValueChange(
                        "string",
                        gl.UserData.name,
                        (value) {
                          gl.UserData.name = value;
                        },
                        () {},
                        () {
                          gl.UserData.serialize();
                        },
                      );
                    },
                    child: Container(
                      alignment: Alignment.centerLeft,
                      width: gl.display.equipixel * 40,
                      child: SingleChildScrollView(
                        scrollDirection: Axis.horizontal,
                        child: Text(
                          gl.UserData.name,
                          style: TextStyle(
                            color: Colors.black,
                            fontSize: gl.display.equipixel * gl.fontSizeS,
                          ),
                        ),
                      ),
                    ),
                  ),
                ),
              ],
            ),
            stroke(
              vertical: false,
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
            Row(
              children: [
                SizedBox(
                  width: gl.display.equipixel * 40,
                  child: Text(
                    "Mail",
                    style: TextStyle(
                      color: Colors.black,
                      fontSize: gl.display.equipixel * gl.fontSizeS,
                    ),
                  ),
                ),
                stroke(
                  vertical: true,
                  gl.display.equipixel,
                  gl.display.equipixel * .5,
                  gl.colorAgroBioTech,
                ),
                Container(
                  alignment: Alignment.center,
                  width: gl.display.equipixel * 40,
                  height: gl.display.equipixel * gl.iconSizeXS,
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
                    onLongPress: () {
                      PopupValueChange(
                        "string",
                        gl.UserData.mail,
                        (value) {
                          gl.UserData.mail = value;
                        },
                        () {},
                        () {
                          gl.UserData.serialize();
                        },
                      );
                    },
                    child: Container(
                      alignment: Alignment.centerLeft,
                      width: gl.display.equipixel * 40,
                      child: SingleChildScrollView(
                        scrollDirection: Axis.horizontal,
                        child: Text(
                          gl.UserData.mail,
                          style: TextStyle(
                            color: Colors.black,
                            fontSize: gl.display.equipixel * gl.fontSizeS,
                          ),
                        ),
                      ),
                    ),
                  ),
                ),
              ],
            ),
            stroke(
              vertical: false,
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
          ],
        ),
      );
    },
  );
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
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.display.equipixel * gl.fontSizeS,
                    fontWeight: FontWeight.w400,
                  ),
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
                    style: TextStyle(
                      color: Colors.blue,
                      fontSize: gl.display.equipixel * gl.fontSizeS,
                      fontWeight: FontWeight.w400,
                    ),
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
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.display.equipixel * gl.fontSizeS,
                    fontWeight: FontWeight.w400,
                  ),
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
                      fontSize: gl.display.equipixel * gl.fontSizeS,
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
          gl.refreshMainStack(() {});
        }, false),
        variableBooleanSlider("Scanlines", gl.Mode.debugScanlines, (bool it) {
          setState(() {
            gl.Mode.debugScanlines = it;
          });
          gl.refreshMainStack(() {});
        }, false),
        variableBooleanSlider("Experimental Tools", gl.Mode.expertTools, (
          bool it,
        ) {
          setState(() {
            gl.Mode.expertTools = it;
          });
          gl.refreshMainStack(() {});
        }, true),
        variableBooleanSlider(
          "Deactivate Polygon Well Defined Check",
          gl.Mode.overrideWellDefinedCheck,
          (bool it) {
            setState(() {
              gl.Mode.overrideWellDefinedCheck = it;
            });
            gl.refreshMainStack(() {});
          },
          true,
        ),
        variableBooleanSlider("Tablet Mode", gl.Mode.overrideModeTablet, (
          bool it,
        ) {
          setState(() {
            gl.Mode.overrideModeTablet = it;
          });
          gl.refreshMainStack(() {});
        }, true),
        variableBooleanSlider("Square Mode", gl.Mode.overrideModeSquare, (
          bool it,
        ) {
          setState(() {
            gl.Mode.overrideModeSquare = it;
          });
          gl.refreshMainStack(() {});
        }, true),
      ],
    );
  }
}

Widget variableBooleanSlider(
  String description,
  bool boolean,
  ValueChanged<bool> changed,
  bool dangerousToPlayWith,
) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
    children: [
      Container(
        width: gl.display.equipixel * 55,
        alignment: AlignmentGeometry.centerLeft,
        child: Row(
          children: [
            dangerousToPlayWith
                ? Icon(
                  Icons.warning,
                  color: Colors.yellow,
                  size: gl.display.equipixel * 6,
                )
                : SizedBox(width: gl.display.equipixel * 6),
            SizedBox(width: gl.display.equipixel * 4),
            SizedBox(
              width: gl.display.equipixel * 45,
              child: Text(description),
            ),
          ],
        ),
      ),
      Container(
        alignment: AlignmentGeometry.center,
        width: gl.display.equipixel * 10,
      ),
      Container(
        alignment: AlignmentGeometry.center,
        width: gl.display.equipixel * 10,
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
                    ? Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              constraints: BoxConstraints(
                                minWidth:
                                    gl.display.equipixel *
                                    "${gl.onboardLog.length - lengthLog + i})"
                                        .length *
                                    2,
                              ),
                              child: Text(
                                "${gl.onboardLog.length - lengthLog + i})",
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeXS,
                                ),
                              ),
                            ),
                            Container(
                              constraints: BoxConstraints(
                                maxWidth:
                                    gl.display.equipixel *
                                    gl.popupWindowsPortraitWidth *
                                    .8,
                              ),
                              child: Text(
                                gl.onboardLog[gl.onboardLog.length -
                                    lengthLog +
                                    i],
                                style: TextStyle(
                                  fontWeight: FontWeight.w500,
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeXS,
                                ),
                              ),
                            ),
                          ],
                        ),
                        if (lengthLog != i + 1)
                          stroke(
                            gl.display.equipixel,
                            gl.display.equipixel * .5,
                            gl.colorAgroBioTech,
                          ),
                      ],
                    )
                    : gl.onboardLog.length - i > 0
                    ? Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              constraints: BoxConstraints(
                                minWidth:
                                    gl.display.equipixel * "$i)".length * 2,
                              ),
                              child: Text(
                                "$i)",
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeXS,
                                ),
                              ),
                            ),
                            Container(
                              constraints: BoxConstraints(
                                maxWidth:
                                    gl.display.equipixel *
                                    gl.popupWindowsPortraitWidth *
                                    .7,
                              ),
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
  return TextStyle(
    color: Colors.black,
    fontSize: gl.display.equipixel * gl.fontSizeM,
  );
}

Widget forestimatorSettingsPermissions(VoidSetter state) {
  return Container(
    padding: EdgeInsets.all(7.5),
    constraints: BoxConstraints(
      maxWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * 0.95,
    ),
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
        stroke(
          gl.display.equipixel,
          gl.display.equipixel * .5,
          gl.colorAgroBioTech,
        ),
        Row(
          mainAxisSize: MainAxisSize.min,
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
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    getLocation()
                        ? Icons.check_circle
                        : Icons.circle_notifications,
                    color: getLocation() ? Colors.green : Colors.red,
                    size: gl.display.equipixel * gl.iconSizeM * .6,
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
            constraints: BoxConstraints(
              maxWidth:
                  gl.display.equipixel * gl.popupWindowsPortraitWidth * 0.95,
            ),
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
                        getStorage()
                            ? Icons.check_circle
                            : Icons.circle_notifications,
                        color: getStorage() ? Colors.green : Colors.red,
                        size: gl.display.equipixel * gl.iconSizeM * .6,
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
                      fontSize: gl.display.equipixel * gl.fontSizeS,
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
  final VoidCallback after;
  final VoidCallback state;

  const SettingsMenu({super.key, required this.after, required this.state});

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
        return switchRowColWithOrientation([
          Column(
            children: [
              Container(
                alignment: Alignment.center,
                padding: EdgeInsets.all(3),
                child: Text(
                  "Paramètres",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.w400,
                    fontSize: gl.display.equipixel * gl.fontSizeL,
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
                child: ListView(
                  padding: const EdgeInsets.symmetric(horizontal: 0),
                  children: _injectLayerResults((int i, ItemSettings item) {
                    return Container(
                      alignment: Alignment.center,
                      child:
                          openedItem[i]
                              ? Card(
                                shape: RoundedRectangleBorder(
                                  borderRadius: BorderRadiusGeometry.circular(
                                    12.0,
                                  ),
                                ),
                                surfaceTintColor: Colors.transparent,
                                shadowColor: Colors.transparent,
                                color: gl.colorAgroBioTech.withAlpha(75),
                                child: Card(
                                  shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadiusGeometry.circular(
                                      12.0,
                                    ),
                                    side: BorderSide(
                                      color: gl.colorAgroBioTech,
                                      width: 2.0,
                                    ),
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
                                                minWidth:
                                                    gl.display.equipixel *
                                                    gl.onCatalogueWidth,
                                                minHeight:
                                                    gl.display.equipixel *
                                                    gl.onCatalogueMapHeight,
                                              ),
                                              child: Text(
                                                item.name,
                                                textAlign: TextAlign.center,
                                                style: TextStyle(
                                                  color: Colors.black,
                                                  fontWeight: FontWeight.w400,
                                                  fontSize:
                                                      gl.display.equipixel *
                                                      gl.fontSizeM,
                                                ),
                                              ),
                                            ),
                                          ),
                                          stroke(
                                            gl.display.equipixel,
                                            gl.display.equipixel * .5,
                                            gl.colorAgroBioTech,
                                          ),
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
                                      minWidth:
                                          gl.display.equipixel *
                                          gl.onCatalogueWidth *
                                          .95,
                                      minHeight:
                                          gl.display.equipixel *
                                          gl.onCatalogueMapHeight *
                                          .95,
                                    ),
                                    child: Text(
                                      item.name,
                                      textAlign: TextAlign.center,
                                      style: TextStyle(
                                        color: Colors.black,
                                        fontWeight: FontWeight.w400,
                                        fontSize:
                                            gl.display.equipixel * gl.fontSizeM,
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
          Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _returnButton(context, () {
                    gl.modeSettings = false;
                    widget.after();
                  }),
                ],
              ),
            ],
          ),
        ]);
      },
    );
  }

  List<Widget> _injectLayerResults(
    Widget Function(int, ItemSettings) generate,
  ) {
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
        entry: forestimatorSettingsUserData(),
      ),
      ItemSettings(
        name: "À propos de Forestimator",
        entry: forestimatorSettingsVersion((void Function() f) {
          setState(() {
            f();
            if (gl.modeDevelopper) {
              menuItems.add(
                ItemSettings(name: "Debug Logs", entry: ForestimatorLog()),
              );
              menuItems.add(
                ItemSettings(name: "Variables", entry: ForestimatorVariables()),
              );
            } else {
              menuItems.removeWhere(
                (item) => item.name == "Debug Logs" ? true : false,
              );
              menuItems.removeWhere(
                (item) => item.name == "Variables" ? true : false,
              );
            }
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
      if (gl.modeDevelopper)
        ItemSettings(name: "Variables", entry: ForestimatorVariables()),
    ]);
    return List<Widget>.generate(menuItems.length, (i) {
      return generate(i, menuItems[i]);
    });
  }
}

Widget popupPolygonListMenu(
  BuildContext context,
  String currentName,
  ValueChanged<LatLng> state,
  VoidCallback after,
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
                      ? gl.display.equipixel *
                          (gl.popupWindowsPortraitHeight + 1)
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

Widget popupSettingsMenu(
  BuildContext context,
  String currentName,
  VoidCallback state,
  VoidCallback after,
) {
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
              child: SettingsMenu(state: state, after: after),
            ),
          ),
          actions: [],
        );
      },
    ),
  );
}

Widget popupPathListMenu(
  BuildContext context,
  String currentName,
  ValueChanged<LatLng> state,
  VoidCallback after,
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
                      ? gl.display.equipixel *
                          (gl.popupWindowsPortraitHeight + 1)
                      : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
              child: PathListMenu(state: state, after: after),
            ),
          ),
          actions: [],
        );
      },
    ),
  );
}

Widget _returnButton(
  BuildContext context,
  VoidCallback after, {
  double length = 0.0,
}) {
  if (length == 0.0) {
    length = gl.display.equipixel * gl.popupReturnButtonWidth * .7;
  }
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
          length,
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
      dismissPopup();
    },
  );
}

class PopupSettingsMenu {
  PopupSettingsMenu(
    BuildContext context,
    String currentName,
    VoidCallback state,
    VoidCallback after,
  ) {
    gl.modeSettings = true;
    popupBarrierWrapper(
      after: () {
        gl.modeSettings = false;
      },
      dismiss: true,
      popup: OrientationBuilder(
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
                        ? gl.display.equipixel * gl.popupWindowsPortraitHeight +
                            1
                        : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
                child: SettingsMenu(state: state, after: after),
              ),
            ),
            actions: [],
          );
        },
      ),
    );
  }
}

Widget _resultRow(String key, String value) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceBetween,
    children: [
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          maxWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .59,
        ),

        child: Text(
          key,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.left,
          style: TextStyle(
            color: Colors.black,
            fontSize: gl.display.equipixel * gl.fontSizeS * .9,
            fontWeight: FontWeight.w300,
          ),
        ),
      ),
      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          minWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .25,
        ),

        child: Text(
          value,
          overflow: TextOverflow.clip,
          textAlign: TextAlign.left,
          style: TextStyle(
            color: Colors.black,
            fontSize: gl.display.equipixel * gl.fontSizeS * .9,
            fontWeight: FontWeight.w500,
          ),
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
    mainAxisAlignment: MainAxisAlignment.spaceBetween,
    children: [
      Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Container(
            color: Colors.black,
            padding: EdgeInsets.all(1),
            constraints: BoxConstraints(
              minHeight: gl.display.equipixel * 5,
              minWidth: gl.display.equipixel * 5,
            ),
            child: Container(color: col),
          ),
          Container(
            padding: EdgeInsets.only(left: 10),
            constraints: BoxConstraints(
              maxWidth:
                  gl.display.equipixel * gl.popupWindowsPortraitWidth * .5,
            ),
            child: Text(
              json['value'].toString(),
              overflow: TextOverflow.clip,
              textAlign: TextAlign.start,
              style: TextStyle(
                color: Colors.black,
                fontSize: gl.display.equipixel * gl.fontSizeS * .9,
                fontWeight: FontWeight.w300,
              ),
            ),
          ),
        ],
      ),

      Container(
        padding: EdgeInsets.all(5),
        constraints: BoxConstraints(
          minWidth: gl.display.equipixel * gl.popupWindowsPortraitWidth * .25,
        ),
        child: Text(
          "${json['prop'].toString()}%",
          overflow: TextOverflow.clip,
          textAlign: TextAlign.justify,
          style: TextStyle(
            color: Colors.black,
            fontSize: gl.display.equipixel * gl.fontSizeS * .9,
            fontWeight: FontWeight.w500,
          ),
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
          "Error printing results: ${prettyPrintContinousResults[layerCode]}",
        );
      }
      return i == 0 ||
              prettyPrintContinousResults[layerCode] == null ||
              prettyPrintContinousResults[layerCode]![json.keys.elementAt(i)] ==
                  null
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
        if (widget.layerTile.downloadable)
          stroke(
            gl.display.equipixel,
            gl.display.equipixel * .5,
            gl.colorAgroBioTech,
          ),
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
                gl.shared!.setStringList(
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
                gl.shared!.setStringList(
                  'anaSurfSelectedLayerKeys',
                  gl.anaSurfSelectedLayerKeys,
                );
              },
            ),
        stroke(
          gl.display.equipixel,
          gl.display.equipixel * .5,
          gl.colorAgroBioTech,
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
                gl.shared!.setStringList(
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
                gl.shared!.setStringList(
                  'anaPtSelectedLayerKeys',
                  gl.anaPtSelectedLayerKeys,
                );
              },
            ),
        if (gl.dico.getLayerBase(widget.layerTile.key).hasDoc())
          stroke(
            gl.display.equipixel,
            gl.display.equipixel * .5,
            gl.colorAgroBioTech,
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
              PopupPdfMenu(widget.layerTile.key);
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
  static final Map<int, VoidCallback> _layerSelectionSetStates = {
    -1: () {},
    0: () {},
    1: () {},
    2: () {},
  };

  void _callSelectedButtonsSetStates() {
    for (final VoidCallback function in _layerSelectionSetStates.values) {
      function();
    }
    gl.refreshMainStack(() {});
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
          gl.refreshMainStack(() {});
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
    return gl.firstTimeUse
        ? PopupDownloadRecomendedLayers(
          title: "Bienvenu",
          accept: "oui",
          onAccept: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            gl.shared!.setBool('firstTimeUse', gl.firstTimeUse);
            for (var key in gl.downloadableLayerKeys) {
              downloadLayer(key);
            }
          },
          decline: "non",
          onDecline: () async {
            setState(() {
              gl.firstTimeUse = false;
            });
            gl.shared!.setBool('firstTimeUse', gl.firstTimeUse);
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
                      hintText: "Entrez le nom d'une couche",
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
                      selectedMap = -1;
                      selectedLayerTile = null;
                      selectedCategory = -1;
                      _resultOfMapSearch.clear();
                      if (value.isNotEmpty) {
                        for (String term in value.split(' ')) {
                          if (term != '') {
                            for (var layer in gl.dico.mLayerBases.values) {
                              if ((!layer.mExpert || gl.Mode.expert) &&
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
                          selectedMap = -1;
                          selectedLayerTile = null;
                          selectedCategory = -1;
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
                                              fontWeight: FontWeight.w400,
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
                                                            fontWeight:
                                                                FontWeight.w400,
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
        ((selectedCategory < 0 ? 0 : selectedCategory) +
                    (selectedMap < 0 ? 0 : 1)) *
                (gl.onCatalogueCategoryHeight + correctionFactorCatalogue) *
                (gl.display.equipixel +
                    (100 /
                        (gl.display.orientation == Orientation.portrait
                            ? gl.display.equiheight
                            : gl.display.equiwidth))) +
            (selectedMap < 0 ? 0 : selectedMap) *
                (gl.onCatalogueMapHeight + correctionFactorMap) *
                (gl.display.equipixel +
                    (100 /
                        (gl.display.orientation == Orientation.portrait
                            ? gl.display.equiheight
                            : gl.display.equiwidth))),
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
  const MapStatusSymbols({
    super.key,
    required this.offlineMode,
    this.layerCode = "",
  });

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
        Icon(
          color: Colors.blue,
          Icons.save,
          size: gl.iconSizeM * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).mIsDownloadableRW &&
          !gl.dico.getLayerBase(mapName!).mOffline &&
          !widget.offlineMode)
        Icon(
          color: Colors.lightBlue,
          Icons.file_download,
          size: gl.iconSizeM * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).mCategorie != "Externe")
        Icon(
          color: Colors.brown,
          Icons.legend_toggle,
          size: gl.iconSizeM * multi * gl.display.equipixel,
        ),
      if (gl.dico.getLayerBase(mapName!).hasDoc())
        Icon(
          color: Colors.brown,
          Icons.picture_as_pdf,
          size: gl.iconSizeM * multi * gl.display.equipixel,
        ),
      if (gl.anaSurfSelectedLayerKeys.contains(mapName!))
        Icon(
          color: Colors.deepOrange,
          Icons.pentagon,
          size: gl.iconSizeM * multi * gl.display.equipixel,
        ),
      if (gl.anaPtSelectedLayerKeys.contains(mapName!))
        Icon(
          color: Colors.deepOrange,
          Icons.location_on,
          size: gl.iconSizeM * multi * gl.display.equipixel,
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
          children: [
            Column(children: statusIcons),
            SizedBox(width: gl.iconSizeM * multi * gl.display.equipixel),
          ],
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
                                      };
                                });
                              },
                              child: Text(
                                layerTile.name,
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeM * .85,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.display.equipixel * gl.iconSizeM,
                            width: gl.display.equipixel * gl.iconSizeM * 1.2,
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
                                  fontWeight: FontWeight.w400,
                                  color: Colors.black,
                                  fontSize: gl.display.equipixel * gl.fontSizeS,
                                ),
                              ),
                            ),
                          ),
                          SizedBox(
                            height: gl.display.equipixel * gl.iconSizeM * 1,
                            width: gl.display.equipixel * gl.iconSizeM * 1.2,
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
                    stroke(
                      gl.display.equipixel,
                      gl.display.equipixel * .5,
                      gl.colorAgroBioTech,
                    ),
                    OnlineMapStatusTool(layerTile: layerTile),
                    stroke(
                      gl.display.equipixel,
                      gl.display.equipixel * .5,
                      gl.colorAgroBioTech,
                    ),
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
                    if (gl.dico.mLayerBases[layerTile.key]!
                        .getDicoValForLegend()
                        .isNotEmpty)
                      stroke(
                        gl.display.equipixel,
                        gl.display.equipixel * .5,
                        gl.colorAgroBioTech,
                      ),
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

class PopupOnlineMapMenu {
  PopupOnlineMapMenu(
    BuildContext context,
    VoidCallback after,
    bool offlineMode,
    int selectionMode,
    String selectedLayer,
    VoidSetter? stateOfLayerSwitcher,
  ) {
    popupBarrierWrapper(
      after: after,
      dismiss: true,
      popup: OrientationBuilder(
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
                        ? gl.display.equipixel * gl.popupWindowsPortraitHeight +
                            1
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
      ),
    );
  }
}

class PopupPdfMenu {
  PopupPdfMenu(
    String layerKey, {
    String path = "",
    String titre = "",
    int currentPage = -1,
  }) {
    gl.refreshMainStack(() {
      if (path == "") {
        path =
            "${gl.pathExternalStorage}/${gl.dico.getLayerBase(layerKey).mPdfName}";
      }
      if (currentPage == -1) {
        currentPage = int.parse(
          gl.dico.getLayerBase(layerKey).mPdfPage.toString(),
        );
      }
      if (titre == "") {
        titre = gl.dico.getLayerBase(layerKey).mNom;
      }
      popupBarrierWrapper(
        dismiss: false,
        popup: OrientationBuilder(
          builder: (context, orientation) {
            return SizedBox(
              width: gl.display.equipixel * gl.display.equiwidth,
              height: gl.display.equipixel * gl.display.equiheight,
              child: PDFScreen(
                path: path,
                titre: titre,
                currentPage: currentPage,
              ),
            );
          },
        ),
      );
    });
  }
}

Widget popupLayerSwitcher(
  BuildContext context,
  VoidCallback after,
  Widget Function(VoidCallback) mainMenuBarDummy,
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
                dismissPopup();
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
    gl.rebuildLayerSwitcher = (void Function() f) {
      if (mounted) {
        setState(() {
          f();
        });
      } else {
        f();
      }
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
                                  gl.geometries.isNotEmpty
                              ? gl.layerSwitcherTileHeight +
                                  gl.layerswitcherControlBoxHeight
                              : (gl.poiMarkerList.isNotEmpty ||
                                      gl.geometries.isNotEmpty
                                  ? gl.layerswitcherControlBoxHeight
                                  : 0.0))) *
                      gl.display.equipixel
                  : (gl.layerSwitcherBoxHeightPortrait +
                          gl.layerswitcherButtonsBoxHeight +
                          (gl.poiMarkerList.isNotEmpty &&
                                  gl.geometries.isNotEmpty
                              ? gl.layerSwitcherTileHeight +
                                  gl.layerswitcherControlBoxHeight
                              : (gl.poiMarkerList.isNotEmpty ||
                                      gl.geometries.isNotEmpty
                                  ? gl.layerswitcherControlBoxHeight
                                  : 0.0))) *
                      gl.display.equipixel
              : gl.layerSwitcherBoxHeightLandscape * gl.display.equipixel,
      child: switchRowColWithOrientation([
        if ((gl.geometries.isNotEmpty || gl.poiMarkerList.isNotEmpty) &&
            gl.display.orientation == Orientation.portrait)
          SizedBox(
            width: gl.display.equipixel * gl.layerswitcherBoxWidth,
            height:
                (gl.poiMarkerList.isNotEmpty && gl.geometries.isNotEmpty
                    ? gl.layerSwitcherTileHeight +
                        gl.layerswitcherControlBoxHeight
                    : (gl.poiMarkerList.isNotEmpty || gl.geometries.isNotEmpty
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
                        "Couches points/polygones",
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
                      "Couches thématiques",
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
              if (gl.geometries.isNotEmpty || gl.poiMarkerList.isNotEmpty)
                SizedBox(
                  width: gl.display.equipixel * gl.layerswitcherBoxWidth,
                  height:
                      (gl.poiMarkerList.isNotEmpty && gl.geometries.isNotEmpty
                          ? gl.layerSwitcherTileHeight +
                              gl.layerswitcherControlBoxHeight
                          : (gl.poiMarkerList.isNotEmpty ||
                                  gl.geometries.isNotEmpty
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
                width: gl.display.equipixel * gl.iconSizeM * 1.2,
                height: gl.display.equipixel * gl.iconSizeM * 1.2,
                child: FloatingActionButton(
                  backgroundColor:
                      modeViewOfflineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!modeViewOnlineMap && !modeViewOnlineMap) {
                      PopupOnlineMapMenu(
                        gl.notificationContext!,
                        () {
                          gl.refreshMainStack(() {
                            modeViewOfflineMap = false;
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
                    gl.refreshMainStack(() {
                      modeViewOfflineMap = true;
                    });
                  },
                  child: Icon(
                    Icons.download_for_offline,
                    size: gl.display.equipixel * gl.iconSizeM,
                    color: Colors.black,
                  ),
                ),
              ),
            if (!gl.offlineMode)
              SizedBox(
                width: gl.display.equipixel * gl.iconSizeM * 1.2,
                height: gl.display.equipixel * gl.iconSizeM * 1.2,
                child: FloatingActionButton(
                  backgroundColor:
                      modeViewOnlineMap ? gl.colorAgroBioTech : Colors.grey,
                  onPressed: () {
                    if (!modeViewOnlineMap && !modeViewOnlineMap) {
                      PopupOnlineMapMenu(
                        gl.notificationContext!,
                        () {
                          gl.refreshMainStack(() {
                            modeViewOnlineMap = false;
                          });
                          if (mounted) {
                            setState(() {
                              modeViewOnlineMap = false;
                            });
                          }
                        },
                        gl.offlineMode,
                        -1,
                        "",
                        null,
                      );
                    }
                    setState(() {
                      modeViewOnlineMap = true;
                    });
                    gl.refreshMainStack(() {
                      modeViewOnlineMap = true;
                    });
                  },
                  child: Icon(
                    Icons.layers_outlined,
                    size: gl.display.equipixel * gl.iconSizeM,
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
                            gl.refreshMainStack(() {});
                          },
                        ),
                      );
                      gl.refreshMainStack(() {});
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
                                gl.refreshMainStack(() {});
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
        if (gl.geometries.isNotEmpty)
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
                          gl.geometries[gl.selectedGeometry].name,
                          widget.switchToLocationInSearchMenu,
                          () {
                            gl.refreshMainStack(() {});
                          },
                        ),
                      );
                      gl.refreshMainStack(() {});
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
                                gl.refreshMainStack(() {});
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
            gl.refreshMainStack(() {});
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
                            PopupOnlineMapMenu(
                              gl.notificationContext!,
                              () {
                                gl.rebuildSwitcherCatalogueButtons(() {
                                  _ViewCatalogueControl.modeViewOfflineMap =
                                      false;
                                  _ViewCatalogueControl.modeViewOnlineMap =
                                      false;
                                });
                                gl.refreshMainStack(() {
                                  _ViewCatalogueControl.modeViewOfflineMap =
                                      false;
                                  _ViewCatalogueControl.modeViewOnlineMap =
                                      false;
                                });
                              },
                              gl.offlineMode,
                              i,
                              gl.selectedLayerForMap[i].mCode,
                              null,
                            );
                            gl.refreshMainStack(() {});
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
                                fontSize: gl.display.equipixel * gl.fontSizeXS,
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
                    if ((i == 0 && !gl.offlineMode) ||
                        (i == 1 &&
                            !gl.offlineMode &&
                            gl
                                .Mode
                                .expertTools)) //Pour la transparance de la première tile
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
                                gl.refreshMainStack(() {});
                              },
                              child: Text(
                                gl.modeMapFirstTileLayerTransparancy
                                    ? "Transparence 50%"
                                    : "Transparence 0%",
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize:
                                      gl.display.equipixel * gl.fontSizeXS,
                                ),
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
                            PopupOnlineMapMenu(
                              gl.notificationContext!,
                              () {
                                gl.rebuildSwitcherCatalogueButtons(() {
                                  _ViewCatalogueControl.modeViewOfflineMap =
                                      false;
                                  _ViewCatalogueControl.modeViewOnlineMap =
                                      false;
                                });
                                gl.refreshMainStack(() {
                                  _ViewCatalogueControl.modeViewOfflineMap =
                                      false;
                                  _ViewCatalogueControl.modeViewOnlineMap =
                                      false;
                                });
                              },
                              gl.offlineMode,
                              i,
                              gl.selectedLayerForMap[i].mCode,
                              null,
                            );
                            gl.refreshMainStack(() {});
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
                                fontSize: gl.display.equipixel * gl.fontSizeXS,
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
  String labelYes;
  String labelNo;
  PopupDoYouReally(
    BuildContext context,
    VoidCallback after,
    String title,
    String message, [
    this.labelYes = "Oui",
    this.labelNo = "Non",
  ]) {
    presentPopup(
      context: context,
      dismiss: true,
      popup: AlertDialog(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadiusGeometry.circular(12.0),
          side: BorderSide(
            color: Color.fromRGBO(205, 225, 138, 1.0),
            width: 2.0,
          ),
        ),
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
          Container(
            padding: EdgeInsets.symmetric(vertical: gl.display.equipixel * 2),
            child: TextButton(
              style: dialogButtonStyle(
                height: gl.dyrButtonsize * 0.6 * gl.display.equipixel,
                width: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                color: gl.colorBack,
              ),
              onPressed: () {
                dismissPopup(after: after);
              },
              child: Text(
                labelYes,
                textAlign: TextAlign.center,
                style: TextStyle(
                  color: Colors.black,
                  fontSize: gl.fontSizeM * gl.display.equipixel,
                ),
              ),
            ),
          ),
          Container(
            padding: EdgeInsets.symmetric(vertical: gl.display.equipixel * 2),
            child: FloatingActionButton(
              onPressed: () {
                dismissPopup();
              },
              backgroundColor: gl.colorAgroBioTech,
              child: Container(
                alignment: Alignment.center,
                constraints: BoxConstraints(
                  maxHeight: gl.dyrButtonsize * .6 * gl.display.equipixel,
                  minHeight: gl.dyrButtonsize * 0.6 * gl.display.equipixel,
                  maxWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                  minWidth: gl.dyrButtonsize * 1.25 * gl.display.equipixel,
                ),
                child: Text(
                  labelNo,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.fontSizeM * gl.display.equipixel,
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class PopupAnaResultsMenu {
  PopupAnaResultsMenu(
    BuildContext context,
    List<LayerAnaPt> requestedLayers,
    VoidCallback after,
  ) {
    popupBarrierWrapper(
      after: after,
      dismiss: false,
      popup: OrientationBuilder(
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
                        ? gl.display.equipixel * gl.popupWindowsPortraitHeight +
                            1
                        : gl.display.equipixel * gl.popupWindowsLandscapeHeight,
                child: AnaResultsMenu(after, requestedLayers),
              ),
            ),
            actions: [],
          );
        },
      ),
    );
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
    return switchRowColWithOrientation([
      SizedBox(
        height:
            gl.display.orientation == Orientation.portrait
                ? (gl.popupWindowsPortraitHeight -
                        gl.searchBarHeight -
                        gl.popupReturnButtonHeight) *
                    gl.display.equipixel
                : (gl.popupWindowsLandscapeHeight - gl.searchBarHeight) *
                    gl.display.equipixel,
        width: gl.popupWindowsPortraitWidth * gl.display.equipixel,
        child: ListView(
          padding: const EdgeInsets.symmetric(horizontal: 0),
          children: [
            Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
              ),
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
                        maxWidth:
                            gl.display.equipixel * gl.onCatalogueWidth * .97,
                        minWidth:
                            gl.display.equipixel * gl.onCatalogueWidth * .97,
                        minHeight:
                            gl.display.equipixel *
                            gl.onCatalogueMapHeight *
                            .97,
                      ),
                      child: Text(
                        "Resultats de l'analyse par couche",
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          color: Colors.black,
                          fontWeight: FontWeight.w400,
                          fontSize: gl.display.equipixel * gl.fontSizeM,
                        ),
                      ),
                    ),
                  ),
                  Card(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadiusGeometry.circular(12.0),
                      side: BorderSide(
                        color: gl.colorAgroBioTech.withAlpha(255),
                        width: 2.0,
                      ),
                    ),
                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color: Colors.white.withAlpha(200),
                    child: ListBody(
                      children: _injectLayerResults(
                        (
                          int i,
                          ResultCard result,
                          String mCode,
                          int mRastValue,
                        ) => TextButton(
                          style: ButtonStyle(
                            minimumSize: WidgetStateProperty<Size>.fromMap(<
                              WidgetStatesConstraint,
                              Size
                            >{
                              WidgetState.any: Size(
                                gl.display.equipixel * gl.onCatalogueWidth * .7,
                                gl.display.equipixel *
                                    gl.onCatalogueCategoryHeight,
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
                                      width:
                                          gl.display.equipixel * gl.iconSizeS,
                                      height:
                                          gl.display.equipixel * gl.iconSizeS,
                                      child: Stack(
                                        children: [
                                          Icon(
                                            result.leading,
                                            color: Colors.black,
                                            size:
                                                gl.display.equipixel *
                                                gl.iconSizeS,
                                          ),
                                          if ((gl.dico
                                                      .getLayerBase(mCode)
                                                      .hasDoc() &&
                                                  mCode != "CS_A") ||
                                              (gl.dico
                                                      .getLayerBase(mCode)
                                                      .hasDoc() &&
                                                  mCode == "CS_A" &&
                                                  mRastValue < 99))
                                            Container(
                                              alignment: Alignment.topRight,
                                              width:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM,
                                              height:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM,
                                              child: Icon(
                                                Icons.picture_as_pdf_sharp,
                                                size:
                                                    gl.display.equipixel *
                                                    gl.iconSizeXS *
                                                    .7,
                                                color: Colors.red,
                                              ),
                                            ),
                                        ],
                                      ),
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel * gl.iconSizeXS,
                                      height:
                                          gl.display.equipixel * gl.iconSizeXS,
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel *
                                          gl.onCatalogueWidth *
                                          .7,
                                      child: Text(
                                        result.layerName,
                                        style: TextStyle(
                                          color: Colors.black,
                                          fontSize:
                                              gl.display.equipixel *
                                              gl.fontSizeS,
                                          fontWeight: FontWeight.w300,
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                                stroke(
                                  gl.display.equipixel,
                                  gl.display.equipixel * .5,
                                  Colors.black.withAlpha(50),
                                ),
                                Row(
                                  children: [
                                    Container(
                                      color: Colors.black12,
                                      padding: EdgeInsets.all(1),
                                      constraints: BoxConstraints(
                                        minHeight:
                                            gl.display.equipixel * gl.iconSizeS,
                                        minWidth:
                                            gl.display.equipixel * gl.iconSizeS,
                                      ),
                                      child: Container(
                                        color:
                                            result.legendColor ==
                                                    Colors.transparent
                                                ? Colors.white
                                                : result.legendColor,
                                      ),
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel * gl.iconSizeXS,
                                      height:
                                          gl.display.equipixel * gl.iconSizeXS,
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel *
                                          gl.onCatalogueWidth *
                                          .7,
                                      child: Text(
                                        result.colorCode,
                                        style: TextStyle(
                                          color: Colors.black,
                                          fontSize:
                                              gl.display.equipixel *
                                              gl.fontSizeS,
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
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                ),
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
                          maxWidth:
                              gl.display.equipixel * gl.onCatalogueWidth * .97,
                          minWidth:
                              gl.display.equipixel * gl.onCatalogueWidth * .97,
                          minHeight:
                              gl.display.equipixel *
                              gl.onCatalogueMapHeight *
                              .97,
                        ),
                        child: Text(
                          "Aptitude du Fichier Ecologique des Essences",
                          textAlign: TextAlign.center,
                          style: TextStyle(
                            color: Colors.black,
                            fontWeight: FontWeight.w400,
                            fontSize: gl.display.equipixel * gl.fontSizeM,
                          ),
                        ),
                      ),
                    ),
                    Card(
                      color: Colors.white.withAlpha(200),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadiusGeometry.circular(12.0),
                        side: BorderSide(
                          color: Color.fromRGBO(205, 225, 138, 1.0),
                          width: 2.0,
                        ),
                      ),
                      child: _tabAptFEE(
                        context,
                        AptsFEE(widget.requestedLayers),
                      ),
                    ),
                  ],
                ),
              ),
            if (PropositionGS(widget.requestedLayers).ready)
              Card(
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                ),
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
                          maxWidth:
                              gl.display.equipixel * gl.onCatalogueWidth * .97,
                          minWidth:
                              gl.display.equipixel * gl.onCatalogueWidth * .97,
                          minHeight:
                              gl.display.equipixel *
                              gl.onCatalogueMapHeight *
                              .97,
                        ),
                        child: Text(
                          "Propositions d'Essences du Guide des Stations",
                          textAlign: TextAlign.center,
                          style: TextStyle(
                            color: Colors.black,
                            fontWeight: FontWeight.w400,
                            fontSize: gl.display.equipixel * gl.fontSizeM,
                          ),
                        ),
                      ),
                    ),
                    Card(
                      color: Colors.white.withAlpha(200),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadiusGeometry.circular(12.0),
                        side: BorderSide(
                          color: Color.fromRGBO(205, 225, 138, 1.0),
                          width: 2.0,
                        ),
                      ),
                      child: _tabPropositionCS(
                        context,
                        PropositionGS(widget.requestedLayers),
                      ),
                    ),
                  ],
                ),
              ),
          ],
        ),
      ),
      Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _returnButton(
                context,
                widget.after,
                length: gl.display.equipixel * gl.popupReturnButtonWidth * .6,
              ),
              TextButton(
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
                  fixedSize: WidgetStateProperty.fromMap(
                    <WidgetStatesConstraint, Size>{
                      WidgetState.any: Size(
                        gl.display.equipixel * gl.popupReturnButtonWidth * .7,
                        gl.display.equipixel * gl.popupReturnButtonHeight,
                      ),
                    },
                  ),
                ),
                child: Text(
                  "Enregistrer",
                  maxLines: 2,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    fontSize: gl.display.equipixel * gl.fontSizeM,
                    color: Colors.black,
                  ),
                ),
                onPressed: () async {
                  popupPdfSaveDialog((String pdf, String locationName) async {
                    dismissPopup();
                    if (pdf.isEmpty) {
                      pdf =
                          "analyseForestimator${DateTime.now().day}.${DateTime.now().month}.${DateTime.now().year}.pdf";
                    }
                    if (pdf.length < 4 ||
                        pdf.substring(pdf.length - 4) != ".pdf") {
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
                    gl.mainStack.add(
                      popupPDFSaved(pdf, () {
                        dismissPopup();
                      }),
                    );
                    gl.refreshMainStack(() {});
                  });
                  gl.refreshMainStack(() {});
                },
              ),
            ],
          ),
        ],
      ),
    ]);
  }

  List<Widget> _injectLayerResults(
    Widget Function(int, ResultCard, String, int) generate,
  ) {
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
                ((gl.dico
                            .getLayerBase(layer.mCode)
                            .getValColor(layer.mRastValue)
                            .toARGB32()) !=
                        4294967295)
                    ? gl.dico
                        .getLayerBase(layer.mCode)
                        .getValColor(layer.mRastValue)
                    : Colors.transparent,
            () {
              if ((gl.dico.getLayerBase(layer.mCode).hasDoc() &&
                  layer.mCode != "CS_A")) {
                PopupPdfMenu(layer.mCode);
              }
              if (gl.dico.getLayerBase(layer.mCode).hasDoc() &&
                  layer.mCode == "CS_A" &&
                  layer.mRastValue < 99) {
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
      return generate(
        i,
        results[i],
        widget.requestedLayers[i].mCode,
        widget.requestedLayers[i].mRastValue,
      );
    });
    //return resultWidgets;
    for (int i = 0; i < results.length - 1; i++) {
      resultWidgets.insert(
        i * 2 + 1,
        stroke(
          gl.display.equipixel,
          gl.display.equipixel * .5,
          gl.colorAgroBioTech,
        ),
      );
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
                indicatorWeight: gl.display.equipixel * 1,
                labelColor: Colors.black,
                unselectedLabelColor: Colors.black45,
                tabs: List<Tab>.generate(3, (index) {
                  List<String> tags = [
                    "Optimum",
                    "Tolérance",
                    "Tolérance élargie",
                  ];
                  return Tab(
                    text: "${tags[index]} ${apts.getListEss(index + 1).length}",
                  );
                }),
              ),
              Container(
                constraints: BoxConstraints(
                  maxHeight:
                      max(
                        max(
                          apts.getListEss(1).length,
                          apts.getListEss(2).length,
                        ),
                        apts.getListEss(3).length,
                      ) *
                      gl.display.equipixel *
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
                indicatorWeight: gl.display.equipixel * 1,
                labelColor: Colors.black,
                dividerColor: Colors.black38,
                unselectedLabelColor: Colors.black45,
                overlayColor: WidgetStateProperty.fromMap(
                  <WidgetStatesConstraint, Color>{
                    WidgetState.selected: gl.colorAgroBioTech.withAlpha(200),
                  },
                ),
                tabs: List<Tab>.generate(
                  4,
                  (index) => Tab(
                    text:
                        "${gl.dico.vulnerabiliteLabel(index + 1)} ${apts.getListEss(index + 1).length}",
                  ),
                ),
              ),

              Container(
                constraints: BoxConstraints(
                  maxHeight:
                      max(
                        max(
                          max(
                            apts.getListEss(1).length,
                            apts.getListEss(3).length,
                          ),
                          apts.getListEss(2).length,
                        ),
                        apts.getListEss(4).length,
                      ) *
                      gl.display.equipixel *
                      gl.iconSizeS *
                      1.64,
                ),
                child: TabBarView(
                  children: List<EssencesListViewGS>.generate(
                    4,
                    (index) =>
                        EssencesListViewGS(apts: apts, codeApt: index + 1),
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  void popupPdfSaveDialog(void Function(String, String) after) {
    popupBarrierWrapper(
      dismiss: false,
      popup:
          getStorage()
              ? AlertDialog(
                title: Text(
                  "Nom du pdf et de la localisation",
                  style: TextStyle(
                    color: Colors.black,
                    fontSize: gl.display.equipixel * gl.fontSizeM,
                    fontWeight: FontWeight.w400,
                  ),
                ),
                shadowColor: Colors.black,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                  side: BorderSide(
                    color: Color.fromRGBO(205, 225, 138, 1.0),
                    width: 2.0,
                  ),
                ),
                content: SizedBox(
                  child: SingleChildScrollView(
                    physics: ScrollPhysics(),
                    child: SizedBox(
                      width:
                          gl.display.equipixel * gl.popupWindowsPortraitWidth,
                      child: Column(
                        children: [
                          TextField(
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
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
                actions: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      _returnButton(
                        context,
                        widget.after,
                        length:
                            gl.display.equipixel *
                            gl.popupReturnButtonWidth *
                            .6,
                      ),
                      TextButton(
                        style: ButtonStyle(
                          backgroundColor: WidgetStateProperty.fromMap(
                            <WidgetStatesConstraint, Color>{
                              WidgetState.any: gl.colorAgroBioTech,
                            },
                          ),
                          shape: WidgetStateProperty<OutlinedBorder>.fromMap(<
                            WidgetStatesConstraint,
                            OutlinedBorder
                          >{
                            WidgetState.any: RoundedRectangleBorder(
                              borderRadius: BorderRadiusGeometry.circular(12.0),
                              side: BorderSide(
                                color: Color.fromRGBO(205, 225, 138, 1.0),
                                width: 2.0,
                              ),
                            ),
                          }),
                          fixedSize: WidgetStateProperty.fromMap(<
                            WidgetStatesConstraint,
                            Size
                          >{
                            WidgetState.any: Size(
                              gl.display.equipixel *
                                  gl.popupReturnButtonWidth *
                                  .6,
                              gl.display.equipixel * gl.popupReturnButtonHeight,
                            ),
                          }),
                        ),
                        onPressed: () {
                          after(
                            controllerPdfName!.text,
                            controllerLocationName!.text,
                          );
                        },
                        child: Text(
                          "Générer",
                          style: TextStyle(
                            color: Colors.black,
                            fontSize: gl.display.equipixel * gl.fontSizeM,
                            fontWeight: FontWeight.w400,
                          ),
                        ),
                      ),
                    ],
                  ),
                ],
              )
              : Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Container(
                    constraints: BoxConstraints(
                      minHeight: gl.display.equipixel * 30,
                      minWidth:
                          gl.display.equipixel * gl.popupWindowsPortraitWidth,
                      maxWidth:
                          gl.display.equipixel * gl.popupWindowsPortraitWidth,
                    ),
                    child: Text(
                      "Vous n'avez pas accordé la permission de stockage des pdf!",
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: gl.display.equipixel * gl.fontSizeM,
                        fontWeight: FontWeight.w400,
                        color: Colors.white,
                      ),
                    ),
                  ),
                  Container(
                    alignment: Alignment.center,
                    child: _returnButton(
                      context,
                      widget.after,
                      length:
                          gl.display.equipixel * gl.popupReturnButtonWidth * .6,
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

  Future makePdf(
    List<LayerAnaPt> layers,
    String fileName,
    String dir,
    String locationName,
  ) async {
    final pdf = pw.Document();
    final imageLogo = pw.MemoryImage(
      (await rootBundle.load(
        'assets/images/GRF_nouveau_logo_uliege-retina.jpg',
      )).buffer.asUint8List(),
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
                        style: pw.TextStyle(
                          fontSize: 18,
                          color: PdfColor.fromHex("255f19"),
                        ),
                      ),
                      pw.SizedBox(height: 30),
                      paddedText(
                        "${gl.offlineMode ? "Réalisé en mode hors-ligne" : "Réalisé avec connexion internet"} le ${DateFormat('yyyy-MM-dd').format(now)}",
                      ),
                    ],
                    crossAxisAlignment: pw.CrossAxisAlignment.start,
                  ),
                  pw.SizedBox(
                    height: 150,
                    width: 150,
                    child: pw.Image(imageLogo),
                  ),
                ],
              ), //first row
              paddedText("Localisation: $locationName", pad: 3),
              paddedText("Coordonnée (EPSG:31370) ", pad: 3),
              paddedText("X: ${gl.pt.x.toInt()}", pad: 3),
              paddedText("Y: ${gl.pt.y.toInt()}", pad: 3),
              pw.SizedBox(height: 10),
              pw.Text(
                "Couches cartographiques analysées",
                style: pw.TextStyle(fontSize: 16),
              ),
              pw.SizedBox(height: 20),
              ...layers.where((i) => i.mRastValue != 0).map<pw.Widget>((
                LayerAnaPt a,
              ) {
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

  ResultCard(
    this.layerName,
    this.colorCode,
    this.leading,
    void Function() doc, {
    this.legendColor = Colors.black,
  }) {
    documentation = doc;
  }
}

class EssencesListViewGS extends StatelessWidget {
  final PropositionGS apts;
  final int codeApt; // maintentant c'est plus un code de vulnérabilités

  const EssencesListViewGS({
    super.key,
    required this.apts,
    required this.codeApt,
  });

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort(
      (a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR),
    );
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
                  if (gl.dico
                      .getEss(code.elementAt(index))
                      .getFicheRoute()
                      .isNotEmpty)
                    Container(
                      alignment: Alignment.topRight,
                      width: gl.display.equipixel * gl.iconSizeS,
                      height: gl.display.equipixel * gl.iconSizeS,
                      child: Icon(
                        Icons.picture_as_pdf_sharp,
                        size: gl.display.equipixel * gl.iconSizeXS * .5,
                        color: Colors.red,
                      ),
                    ),
                  gl.dico.getEss(code.elementAt(index)).mFR == 1
                      ? SizedBox(
                        width: gl.display.equipixel * gl.iconSizeS,
                        height: gl.display.equipixel * gl.iconSizeS,
                        child: Icon(
                          CustomIcons.tree,
                          color: Colors.black87,
                          size: gl.display.equipixel * gl.iconSizeS,
                        ),
                      )
                      : SizedBox(
                        width: gl.display.equipixel * gl.iconSizeS,
                        height: gl.display.equipixel * gl.iconSizeS,
                        child: Icon(
                          Icons.forest_outlined,
                          color: Colors.black87,
                          size: gl.display.equipixel * gl.iconSizeXS,
                        ),
                      ),
                ],
              ),
              title: SizedBox(
                width: gl.display.equipixel * gl.popupWindowsPortraitWidth * .6,
                child: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
              ),
              subtitle:
                  codeApt != mEss[code.elementAt(index)]
                      ? SizedBox(
                        child: Text(
                          gl.dico.aptLabel(mEss[code.elementAt(index)]!),
                        ),
                      )
                      : null,
              trailing: SizedBox(width: gl.display.equipixel * gl.iconSizeXS),
              onTap: () {
                String path =
                    "/${gl.pathExternalStorage}/FEE-${gl.dico.getEss(code.elementAt(index)).mCode}.pdf";
                PopupPdfMenu("", path: path, currentPage: 0);
              },
            ),
            stroke(
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
          ],
        );
      },
    );
  }
}

pw.Widget paddedText(
  final String text, {
  final pw.TextAlign align = pw.TextAlign.left,
  final double pad = 5.0,
}) => pw.Padding(
  padding: pw.EdgeInsets.all(pad),
  child: pw.Text(text, textAlign: align),
);

class EssencesListView extends StatelessWidget {
  final AptsFEE apts;
  final int codeApt;

  const EssencesListView({
    super.key,
    required this.apts,
    required this.codeApt,
  });

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort(
      (a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR),
    );
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
                  if (gl.dico
                      .getEss(code.elementAt(index))
                      .getFicheRoute()
                      .isNotEmpty)
                    Container(
                      alignment: Alignment.topRight,
                      width: gl.display.equipixel * gl.iconSizeS,
                      height: gl.display.equipixel * gl.iconSizeS,
                      child: Icon(
                        Icons.picture_as_pdf,
                        size: gl.display.equipixel * gl.iconSizeXS * .5,
                        color: Colors.red,
                      ),
                    ),

                  gl.dico.getEss(code.elementAt(index)).mFR == 1
                      ? SizedBox(
                        width: gl.display.equipixel * gl.iconSizeS,
                        height: gl.display.equipixel * gl.iconSizeS,
                        child: Icon(
                          CustomIcons.tree,
                          color: Colors.black87,
                          size: gl.display.equipixel * gl.iconSizeS,
                        ),
                      )
                      : SizedBox(
                        width: gl.display.equipixel * gl.iconSizeS,
                        height: gl.display.equipixel * gl.iconSizeS,
                        child: Icon(
                          Icons.forest_outlined,
                          color: Colors.black87,
                          size: gl.display.equipixel * gl.iconSizeXS,
                        ),
                      ),
                ],
              ),
              title: SizedBox(
                width: gl.display.equipixel * gl.popupWindowsPortraitWidth * .6,
                child: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
              ),
              subtitle:
                  codeApt != mEss[code.elementAt(index)]
                      ? SizedBox(
                        child: Text(
                          gl.dico.aptLabel(mEss[code.elementAt(index)]!),
                        ),
                      )
                      : null,
              trailing:
                  apts.mCompensations[code.elementAt(index)]!
                      ? SizedBox(
                        width: gl.display.equipixel * gl.iconSizeXS,
                        child: IconButton(
                          icon: Icon(
                            Icons.info_outline,
                            color: gl.colorUliege,
                            size: gl.display.equipixel * gl.iconSizeXS,
                          ),
                          onPressed: () {},
                          tooltip:
                              "La situation topographique provoque un effet de compensation (positif ou négatif) sur l'aptitude de cette essence",
                        ),
                      )
                      : SizedBox(width: gl.display.equipixel * gl.iconSizeXS),
              onTap: () {
                String path =
                    "/${gl.pathExternalStorage}/FEE-${gl.dico.getEss(code.elementAt(index)).mCode}.pdf";
                PopupPdfMenu("", path: path, currentPage: 0);
              },
            ),
            stroke(
              gl.display.equipixel,
              gl.display.equipixel * .5,
              gl.colorAgroBioTech,
            ),
          ],
        );
      },
    );
  }
}

Widget popupAnaSurfResultsMenu(
  BuildContext context,
  Map<String, dynamic> json,
  VoidCallback state,
  VoidCallback after,
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
            child: AnaSurfResultsMenu(json, after, state),
          ),
        ),
        actions: [],
      );
    },
  );
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
    return switchRowColWithOrientation([
      SizedBox(
        height:
            gl.display.orientation == Orientation.portrait
                ? (gl.popupWindowsPortraitHeight -
                        gl.searchBarHeight -
                        gl.popupReturnButtonHeight) *
                    gl.display.equipixel
                : (gl.popupWindowsLandscapeHeight - gl.searchBarHeight) *
                    gl.display.equipixel,
        width: gl.popupWindowsPortraitWidth * gl.display.equipixel,
        child: ListView(
          padding: const EdgeInsets.symmetric(horizontal: 0),
          children: [
            Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadiusGeometry.circular(12.0),
              ),
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
                        maxWidth:
                            gl.display.equipixel * gl.onCatalogueWidth * .97,
                        minWidth:
                            gl.display.equipixel * gl.onCatalogueWidth * .97,
                        minHeight:
                            gl.display.equipixel *
                            gl.onCatalogueMapHeight *
                            .97,
                      ),
                      child: Text(
                        "Resultats de l'analyse par couche",
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          color: Colors.black,
                          fontWeight: FontWeight.w400,
                          fontSize: gl.display.equipixel * gl.fontSizeM,
                        ),
                      ),
                    ),
                  ),
                  Card(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadiusGeometry.circular(12.0),
                      side: BorderSide(
                        color: gl.colorAgroBioTech.withAlpha(255),
                        width: 2.0,
                      ),
                    ),
                    surfaceTintColor: Colors.transparent,
                    shadowColor: Colors.transparent,
                    color: Colors.white.withAlpha(200),
                    child: ListBody(
                      children: _injectLayerResults(
                        (
                          int i,
                          Item item,
                          String mCode,
                          int mRastValue,
                        ) => TextButton(
                          style: ButtonStyle(
                            minimumSize: WidgetStateProperty<Size>.fromMap(<
                              WidgetStatesConstraint,
                              Size
                            >{
                              WidgetState.any: Size(
                                gl.display.equipixel * gl.onCatalogueWidth * .7,
                                gl.display.equipixel *
                                    gl.onCatalogueCategoryHeight,
                              ),
                            }),
                          ),
                          key: Key('$i'),
                          onPressed: () {
                            if ((gl.dico.getLayerBase(mCode).hasDoc() &&
                                mCode != "CS_A")) {
                              PopupPdfMenu(mCode);
                            }
                            if (gl.dico.getLayerBase(mCode).hasDoc() &&
                                mCode == "CS_A" &&
                                mRastValue < 99) {
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
                                      width:
                                          gl.display.equipixel * gl.iconSizeS,
                                      height:
                                          gl.display.equipixel * gl.iconSizeS,
                                      child: Stack(
                                        children: [
                                          Icon(
                                            switch (gl.dico
                                                .getLayerBase(mCode)
                                                .mGroupe) {
                                              "ST" => CustomIcons.mountain,
                                              "PEUP" => CustomIcons.forest,
                                              "CS" => CustomIcons.mountains,
                                              "REF" => CustomIcons.map,
                                              _ => CustomIcons.soil,
                                            },
                                            color: Colors.black,
                                            size:
                                                gl.display.equipixel *
                                                gl.iconSizeS,
                                          ),
                                          if ((gl.dico
                                                      .getLayerBase(mCode)
                                                      .hasDoc() &&
                                                  mCode != "CS_A") ||
                                              (gl.dico
                                                      .getLayerBase(mCode)
                                                      .hasDoc() &&
                                                  mCode == "CS_A" &&
                                                  mRastValue < 99))
                                            Container(
                                              alignment: Alignment.topRight,
                                              width:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM,
                                              height:
                                                  gl.display.equipixel *
                                                  gl.iconSizeM,
                                              child: Icon(
                                                Icons.picture_as_pdf_sharp,
                                                size:
                                                    gl.display.equipixel *
                                                    gl.iconSizeXS *
                                                    .7,
                                                color: Colors.red,
                                              ),
                                            ),
                                        ],
                                      ),
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel * gl.iconSizeXS,
                                      height:
                                          gl.display.equipixel * gl.iconSizeXS,
                                    ),
                                    SizedBox(
                                      width:
                                          gl.display.equipixel *
                                          gl.onCatalogueWidth *
                                          .7,
                                      child: Text(
                                        item.name,
                                        style: TextStyle(
                                          color: Colors.black,
                                          fontSize:
                                              gl.display.equipixel *
                                              gl.fontSizeS,
                                          fontWeight: FontWeight.w400,
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                                stroke(
                                  gl.display.equipixel,
                                  gl.display.equipixel * .5,
                                  Colors.black.withAlpha(50),
                                ),
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
      Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [_returnButton(context, widget.after)],
          ),
        ],
      ),
    ]);
  }

  List<Widget> _injectLayerResults(
    Widget Function(int, Item, String, int) generate,
  ) {
    final List<Item> menuItems = [];
    if (widget.json['RequestedLayers'] != null &&
        widget.json['RequestedLayers'].isNotEmpty) {
      for (var result in widget.json['RequestedLayers']) {
        if (result['mean'] != null) {
          menuItems.add(
            Item(
              name: gl.dico.getLayerBase(result['layerCode']).mNom,
              mCode: result['layerCode'],
              mRastValue: result['rastValue'] ?? 98,
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
              mCode: result['layerCode'],
              mRastValue: result['rastValue'] ?? 98,
              entry: forestimatorResultsHeaderClasse(result),
            ),
          );
        }
      }
    }

    List<Widget> resultWidgets = List<Widget>.generate(menuItems.length, (i) {
      return generate(
        i,
        menuItems[i],
        menuItems[i].mCode!,
        menuItems[i].mRastValue!,
      );
    });
    //return resultWidgets;
    for (int i = 0; i < menuItems.length - 1; i++) {
      resultWidgets.insert(
        i * 2 + 1,
        stroke(
          gl.display.equipixel,
          gl.display.equipixel * .5,
          gl.colorAgroBioTech,
        ),
      );
    }
    return resultWidgets;
  }
}
