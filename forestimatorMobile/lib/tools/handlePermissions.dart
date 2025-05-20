/* Implementation of the permission_handler 12.0.0+1 Interface  */

import 'dart:io';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';

PermissionStatus location = PermissionStatus.granted;
PermissionStatus storage = PermissionStatus.granted;
PermissionStatus storage2 = PermissionStatus.granted;
PermissionStatus extStorage = PermissionStatus.granted;
PermissionStatus notification = PermissionStatus.granted;

bool askOnceForLocation = true;
bool askOnceForStorage = true;
bool askOnceForNotification = true;

bool getVersion = true;
int release = 20;
int sdkInt = 20;

/* Ask permissions at start of map and if not granted ask to grant them */

void initPermissions() async {
  refreshPermissionInfos();
  if (getVersion && Platform.isAndroid){
    DeviceInfoPlugin infos = DeviceInfoPlugin();
    AndroidDeviceInfo androidInfo = await infos.androidInfo;
    release = int.parse(androidInfo.version.release[0] + androidInfo.version.release[1]);
    sdkInt = androidInfo.version.sdkInt;
    print("Android $release (sdk $sdkInt)");
  }
  getVersion = false;
}

void refreshPermissionInfos() async{
  location = await Permission.location.status;
  storage = await Permission.storage.status;
  extStorage = await Permission.manageExternalStorage.status;
  notification = await Permission.notification.status; // Not used for now!
}

bool locationGranted() => location.isGranted;
bool storageGranted() => storage.isGranted;
bool extStorageGranted() => extStorage.isGranted;
bool notificationsGranted() => notification.isGranted;

void makeAllPermissionRequests() {
  Permission.location.request();
}

Widget handlePermissionForLocation({
  required Widget child,
  required Function refreshParentWidgetTree,
}) {
  if (!askOnceForLocation) return child;
  if (location.isDenied) {
    return PopupNotification2(
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
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Autorisez-vous l'aplication à utiliser la position?",
    );
  } else if (location.isPermanentlyDenied) {
    return PopupNotification2(
      title: "Permission pour le GPS deactivé",
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
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Vous pouvez ouvrir les paramètres et autoriser l'aplication à utiliser la position.",
    );
  }
  return child;
}

Widget handlePermissionForStorage({
  required Widget child,
  required Function refreshParentWidgetTree,
}) {
  if (!askOnceForStorage) return child;
  if (release < 13 && storage.isDenied) {
    return PopupNotification2(
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
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Autorisez-vous l'application à stocker des données de cartographie sur votre smartphone?",
    );
  } else if (release < 13 && storage.isPermanentlyDenied) {
    return PopupNotification2(
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
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Vous pouvez ouvrir les paramètres et autoriser l'application à stocker des données de cartographie sur votre smartphone.",
    );
  }
  return child;
}

Widget handlePermissionForNotifications({
  required Widget child,
  required Function refreshParentWidgetTree,
}) {
    if (!askOnceForNotification) return child;
  if (release > 8) {
    return PopupNotification2(
      title: "Permission pour l'envoi de notifications.",
      accept: "oui",
      onAccept: () async {
        await Permission.notification.request();
        refreshPermissionInfos();
        refreshParentWidgetTree(() {
          askOnceForNotification = false;
        });
      },
      decline: "non",
      onDecline: () async {
        refreshParentWidgetTree(() {
          askOnceForStorage = false;
        });
      },
      dialog:
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Autorisez-vous l'application à stocker des données de cartographie sur votre smartphone?",
    );
  } else if (release > 13) {
    return PopupNotification2(
      title: "Permission pour l'envoi de notifications.",
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
          "Forestimator mobile ne collecte aucune information personnelle. Notre politique de confidentialité est consultable au https://forestimator.gembloux.ulg.ac.be/documentation/confidentialit_. Vous pouvez ouvrir les paramètres et autoriser l'application à stocker des données de cartographie sur votre smartphone.",
    );
  }
  return child;
}