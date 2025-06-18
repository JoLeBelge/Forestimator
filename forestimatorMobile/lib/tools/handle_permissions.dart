/* Implementation of the permission_handler 12.0.0+1 Interface  */

import 'dart:io';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';

PermissionStatus location = PermissionStatus.granted;
PermissionStatus storage = PermissionStatus.granted;
PermissionStatus storage2 = PermissionStatus.granted;
PermissionStatus extStorage = PermissionStatus.granted;

bool askOnceForLocation = true;
bool askOnceForStorage = true;

bool getVersion = true;
double release = 33;
int sdkInt = 33;

/* Ask permissions at start of map and if not granted ask to grant them */
bool getLocation() => location.isGranted;
bool getStorage() => storage.isGranted;
bool getExtStorage() => extStorage.isGranted;

Future<bool> openPhoneForestimatorSettings() async => await openAppSettings();

void initPermissions() async {
  refreshPermissionInfos();
  if (getVersion && Platform.isAndroid) {
    DeviceInfoPlugin infos = DeviceInfoPlugin();
    AndroidDeviceInfo androidInfo = await infos.androidInfo;
    release = double.parse(
      androidInfo.version.release[0] + androidInfo.version.release[1],
    );
    sdkInt = androidInfo.version.sdkInt;
    gl.print("Android $release (sdk $sdkInt)");
  } else if (getVersion && Platform.isIOS) {
    DeviceInfoPlugin infos = DeviceInfoPlugin();
    IosDeviceInfo iOSInfo = await infos.iosInfo;
    release = double.parse(iOSInfo.systemVersion);
    gl.print("iOS $release ${iOSInfo.systemName}");
  }
  getVersion = false;
}

void refreshPermissionInfos() async {
  location = await Permission.location.status;
  storage = await Permission.storage.status;
  extStorage = await Permission.manageExternalStorage.status;
}

bool locationGranted() => location.isGranted;
bool storageGranted() => storage.isGranted;
bool extStorageGranted() => extStorage.isGranted;

void makeAllPermissionRequests() {
  Permission.location.request();
}

Widget handlePermissionForLocation({
  required Widget child,
  required Function refreshParentWidgetTree,
}) {
  if (!askOnceForLocation) return child;
  if (location.isPermanentlyDenied) {
    return PopupPermissions(
      title: "GPS desactivées",
      accept: "Ouvrir paramètres",
      onAccept: () async {
        await openAppSettings();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          askOnceForLocation = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          askOnceForLocation = false;
        });
      },
      dialog:
          "L'application utilise le gps pour afficher votre position actuelle sur la carte. Si vous voulez utiliser cette fonctionalité il faut donner la permission dans les parametres.\nSouhaitez vous ouvrir les parametres de votre smartphone?",
    );
  } else if (location.isDenied) {
    return PopupPermissions(
      title: "Permission pour le GPS",
      accept: "oui",
      onAccept: () async {
        await Permission.location.request();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          askOnceForLocation = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          askOnceForLocation = false;
        });
      },
      dialog:
          "L'application utilise le gps pour afficher votre position actuelle sur la carte et seulement pendant l'utilisation. Autorisez-vous cette fonctionalité?",
    );
  }
  return child;
}

Widget handlePermissionForStorage({
  required Widget child,
  required Function refreshParentWidgetTree,
}) {
  if (!askOnceForStorage) return child;
  if (storage.isDenied && sdkInt < 33) {
    return PopupPermissions(
      title: "Permission pour le stockage des données.",
      accept: "oui",
      onAccept: () async {
        await Permission.storage.request();
        await Permission.manageExternalStorage.request();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          askOnceForStorage = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          askOnceForStorage = false;
        });
      },
      dialog:
          "Autorisez-vous l'application à stocker des resultats en forme de PDF dans le dossier 'Downloads' de votre smartphone?",
    );
  } else if (storage.isPermanentlyDenied && sdkInt < 33) {
    return PopupPermissions(
      title: "Permission pour le stockage des données.",
      accept: "Ouvrir paramètres",
      onAccept: () async {
        await openAppSettings();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          askOnceForStorage = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          askOnceForStorage = false;
        });
      },
      dialog:
          "Vous pouvez ouvrir les paramètres et autoriser l'application à stocker des resultats en forme de PDF dans le dossier 'Downloads' de votre smartphone.",
    );
  }
  return child;
}
