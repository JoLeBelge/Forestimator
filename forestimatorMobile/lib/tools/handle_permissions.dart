/* Implementation of the permission_handler 12.0.0+1 Interface  */

import 'dart:io';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';

PermissionStatus _location = PermissionStatus.granted;
PermissionStatus _storage = PermissionStatus.granted;
PermissionStatus _extStorage = PermissionStatus.granted;

bool _askOnceForLocation = true;
bool _askOnceForStorage = true;

bool _getVersion = true;
double _release = 33;
int sdkNmb = 33;
String _system = "";

/* Ask permissions at start of map and if not granted ask to grant them */
bool getLocation() => _location.isGranted;
bool getStorage() => _system == "Android" && _release < 13 ? _storage.isGranted : true;
bool getExtStorage() => _extStorage.isGranted;

Future<bool> openPhoneForestimatorSettings() async => await openAppSettings();

void initPermissions() async {
  refreshPermissionInfos();
  if (_getVersion && Platform.isAndroid) {
    _system = "Android";
    DeviceInfoPlugin infos = DeviceInfoPlugin();
    AndroidDeviceInfo androidInfo = await infos.androidInfo;
    if (androidInfo.version.release.length > 1) {
      _release = double.parse(androidInfo.version.release[0] + androidInfo.version.release[1]);
    } else {
      _release = double.parse(androidInfo.version.release[0]);
    }
    sdkNmb = androidInfo.version.sdkInt;
    gl.print("Android $_release (sdk $sdkNmb)");
  } /* else if (getVersion && Platform.isIOS) {
    system = "iOS";
    DeviceInfoPlugin infos = DeviceInfoPlugin();
    IosDeviceInfo iOSInfo = await infos.iosInfo;
    release = double.parse(iOSInfo.systemVersion);
    gl.print("iOS $release ${iOSInfo.systemName}");
  }*/
  _getVersion = false;
}

void refreshPermissionInfos() async {
  _location = await Permission.location.status;
  _storage = await Permission.storage.status;
  _extStorage = await Permission.manageExternalStorage.status;
}

bool locationGranted() => _location.isGranted;
bool storageGranted() => _storage.isGranted;
bool extStorageGranted() => _extStorage.isGranted;

void makeAllPermissionRequests() {
  Permission.location.request();
}

Widget handlePermissionForLocation({required Widget child, required VoidSetter refreshParentWidgetTree}) {
  if (_location.isPermanentlyDenied || !_askOnceForLocation || gl.firstTimeUse) return child;
  if (_location.isDenied) {
    return PopupPermissions(
      title: "Permission pour le GPS",
      accept: "oui",
      onAccept: () async {
        await Permission.location.request();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          _askOnceForLocation = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          _askOnceForLocation = false;
        });
      },
      dialog:
          "L'application utilise le gps pour afficher votre position actuelle sur la carte et seulement pendant l'utilisation. Autorisez-vous cette fonctionalité?",
    );
  }
  return child;
}

Widget handlePermissionForStorage({required Widget child, required VoidSetter refreshParentWidgetTree}) {
  if (_storage.isPermanentlyDenied || !_askOnceForStorage || gl.firstTimeUse) return child;
  if (_storage.isDenied && _release < 13) {
    return PopupPermissions(
      title: "Permission pour le stockage des données.",
      accept: "oui",
      onAccept: () async {
        await Permission.storage.request();
        await Permission.manageExternalStorage.request();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          _askOnceForStorage = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          _askOnceForStorage = false;
        });
      },
      dialog:
          "Autorisez-vous l'application à stocker des resultats en forme de PDF dans le dossier 'Downloads' de votre smartphone?",
    );
  }
  return child;
}
