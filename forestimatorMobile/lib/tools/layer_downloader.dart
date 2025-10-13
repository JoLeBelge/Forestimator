import 'dart:io';
import 'dart:isolate';
import 'dart:ui';
import 'package:downloadsfolder/downloadsfolder.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';

final Map<String, Function> _downloadIdToStateFunction = {};
final Map<String, String> _layerKeyToDownloadId = {};
final Map<String, String> _downloadIdToLayerKey = {};
final Map<String, String> _downloadIdToLayerName = {};
ForestimatorDownloader? fD;

void initDownloader() async {
  WidgetsFlutterBinding.ensureInitialized();
  await FlutterDownloader.initialize(
    debug:
        gl.debug, // optional: set to false to disable printing logs to console (default: true)
    ignoreSsl:
        false, // option: set to false to disable working with http links (default: false)
  );
  fD = ForestimatorDownloader();
}

void downloadLayer(String layerKey) {
  if (fD == null) {
    return;
  }
  fD!.downloadFile(layerKey);
}

class LayerDownloader extends StatefulWidget {
  final LayerTile layer;
  const LayerDownloader(this.layer, {super.key});

  @override
  State<LayerDownloader> createState() => _LayerDownloaderState();
}

class _LayerDownloaderState extends State<LayerDownloader> {
  String? downloadId = "null";

  @override
  Widget build(BuildContext context) {
    if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
      return Container(
        color: gl.colorBackground,
        constraints: BoxConstraints(
          minWidth: gl.onCatalogueWidth * gl.display!.equipixel!,
          maxWidth: gl.onCatalogueWidth * gl.display!.equipixel!,
          minHeight: 10 * gl.display!.equipixel!,
          maxHeight: 10 * gl.display!.equipixel!,
        ),
        child: const Text("Downloads are not supported yet."),
      );
    }

    if (gl.dico.getLayerBase(widget.layer.key).mOffline) {
      return TextButton(
        style: ButtonStyle(
          minimumSize:
              WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  gl.onCatalogueWidth * gl.display!.equipixel!,
                  gl.display!.equipixel! * 10,
                ),
              }),
        ),
        onPressed: () async {
          PopupDoYouReally(
            gl.notificationContext!,
            () {
              fileDelete(
                join(
                  gl.dico.docDir.path,
                  gl.dico.getLayerBase(widget.layer.key).mNomRaster,
                ),
              ).whenComplete(() {
                setState(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                  gl.removeFromOfflineList(widget.layer.key);
                });
                gl.rebuildSwitcherCatalogueButtons(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                });
                gl.rebuildOfflineCatalogue(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                });
              });
            },
            "Message",
            "\nVoulez vous vraiment supprimer ${gl.dico.getLayerBase(widget.layer.key).mNom} de la mémoire?\nVous ne pouvez plus utiliser cette couche hors ligne après.\n",
          );
        },
        child: Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Icon(
              Icons.delete,
              size: gl.onCatalogueIconSize * gl.display!.equipixel!,
              color: Colors.black,
            ),
            Container(
              constraints: BoxConstraints(maxWidth: 5 * gl.display!.equipixel!),
            ),
            Container(
              constraints: BoxConstraints(
                maxWidth: 60 * gl.display!.equipixel!,
              ),
              child: const Text(
                "La couche est enregistrée.",
                style: TextStyle(color: Colors.black),
              ),
            ),
          ],
        ),
      );
    } else if (gl.dico.getLayerBase(widget.layer.key).mInDownload) {
      return TextButton(
        style: ButtonStyle(
          minimumSize:
              WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  gl.onCatalogueWidth * gl.display!.equipixel!,
                  10 * gl.display!.equipixel!,
                ),
              }),
        ),
        onPressed: () async {
          FlutterDownloader.cancel(taskId: downloadId!);
          setState(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          gl.rebuildSwitcherCatalogueButtons(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          gl.rebuildOfflineCatalogue(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          downloadId = await fD?.downloadFile(widget.layer.key);
          setState(() {
            _layerKeyToDownloadId[widget.layer.key] = downloadId!;
            _downloadIdToStateFunction[downloadId!] =
                (Function f) => () {
                  setState(() {
                    f();
                  });
                };
          });
        },
        child: Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Icon(
              Icons.repeat_rounded,
              size: gl.onCatalogueIconSize * gl.display!.equipixel!,
              color: Colors.black,
            ),
            Container(
              constraints: BoxConstraints(
                maxWidth: 10 * gl.display!.equipixel!,
              ),
            ),
            Container(
              constraints: BoxConstraints(
                maxWidth: 60 * gl.display!.equipixel!,
              ),
              child: const Text(
                "Relancer.",
                style: TextStyle(color: Colors.black),
              ),
            ),
          ],
        ),
      );
    } else {
      return TextButton(
        style: ButtonStyle(
          minimumSize:
              WidgetStateProperty<Size>.fromMap(<WidgetStatesConstraint, Size>{
                WidgetState.any: Size(
                  gl.onCatalogueWidth * gl.display!.equipixel!,
                  10 * gl.display!.equipixel!,
                ),
              }),
        ),
        onPressed: () async {
          setState(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          gl.rebuildSwitcherCatalogueButtons(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          gl.rebuildOfflineCatalogue(() {
            gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
          });
          downloadId = await fD?.downloadFile(widget.layer.key);
          setState(() {
            _layerKeyToDownloadId[widget.layer.key] = downloadId!;
            _downloadIdToStateFunction[downloadId!] =
                (Function f) => setState(() {
                  f();
                });
          });
        },
        child: Row(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            Icon(
              Icons.download,
              size: gl.onCatalogueIconSize * gl.display!.equipixel!,
              color: Colors.black,
            ),
            Container(
              constraints: BoxConstraints(maxWidth: 5 * gl.display!.equipixel!),
            ),
            Container(
              constraints: BoxConstraints(
                maxWidth: 60 * gl.display!.equipixel!,
              ),
              child: const Text(
                "La couche peut être téléchargée pour l'utilisation hors ligne.",
                style: TextStyle(color: Colors.black),
              ),
            ),
          ],
        ),
      );
    }
  }

  @override
  void initState() {
    String id = "null";
    if (_layerKeyToDownloadId[widget.layer.key] != null) {
      id = _layerKeyToDownloadId[widget.layer.key]!;
      downloadId = id;
      _downloadIdToStateFunction[id] = (Function f) {
        setState(() {
          f();
        });
      };
    }
    super.initState();
  }

  @override
  void dispose() {
    _downloadIdToStateFunction[downloadId!] = (Function f) {
      f();
    };
    super.dispose();
  }
}

Future<bool> fileExists(String path) async {
  final File file = File(path);
  return file.exists();
}

Future<bool> fileDelete(String path) async {
  final File file = File(path);
  if (await file.exists()) {
    file.delete();
  }
  return fileExists(path);
}

@pragma('vm:entry-point')
class ForestimatorDownloader {
  final Map<String, String> layerToId = {};
  final ReceivePort _port = ReceivePort();
  final List<Map> downloadData = [];

  ForestimatorDownloader() {
    FlutterDownloader.registerCallback(downloadCallback, step: 10);
    _listenToDownloader();
  }

  @pragma('vm:entry-point')
  static void downloadCallback(String id, int status, int progress) {
    final SendPort send =
        IsolateNameServer.lookupPortByName('downloader_send_port')!;
    send.send([id, status, progress]);
  }

  void dispose() {
    IsolateNameServer.removePortNameMapping('downloader_send_port');
  }

  Future<String?> downloadFile(String layerKey) async {
    late String? taskId;
    FlutterDownloader.registerCallback(downloadCallback, step: 10);
    if (!(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
      taskId = await FlutterDownloader.enqueue(
        url:
            "${gl.queryApiRastDownload}/${gl.dico.getLayerBase(layerKey).mCode}",
        fileName: gl.dico.getLayerBase(layerKey).mNomRaster,
        savedDir: gl.dico.docDir.path,
        showNotification: false,
        openFileFromNotification: false,
        timeout: 180000,
      );
    }
    _downloadIdToLayerKey[taskId!] = layerKey;
    _downloadIdToLayerName[taskId] = gl.dico.getLayerBase(layerKey).mNom;
    layerToId[layerKey] = taskId;
    return taskId;
  }

  void _listenToDownloader() {
    IsolateNameServer.registerPortWithName(
      _port.sendPort,
      'downloader_send_port',
    );
    _port.listen((dynamic data) {
      String idListened = data[0];
      String layerKey = "", layerName = "";
      Function widgetState = (f) {
        f();
      };
      if (_downloadIdToStateFunction[idListened] != null) {
        widgetState = _downloadIdToStateFunction[idListened]!;
      }
      layerKey = _downloadIdToLayerKey[idListened]!;
      layerName = _downloadIdToLayerName[idListened]!;

      DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
      if (DownloadTaskStatus.enqueued == status) {
        gl.print("Downloader: new download launched.");
        widgetState(() {
          gl.dico.getLayerBase(layerKey).mInDownload = true;
        });
      }
      if (status == DownloadTaskStatus.complete) {
        gl.print("Downloader: download completed.");
        BuildContext context = gl.notificationContext!;
        PopupDownloadSuccess(context, layerName);
        widgetState(() {
          gl.dico.getLayerBase(layerKey).mOffline = true;
          gl.dico.getLayerBase(layerKey).mInDownload = false;
        });
        gl.rebuildSwitcherCatalogueButtons(() {});
        gl.rebuildOfflineCatalogue(() {});
      }
      if (DownloadTaskStatus.failed == status) {
        gl.print("Downloader: download failed.");
        BuildContext context = gl.notificationContext!;

        PopupDownloadFailed(context, layerName);
        widgetState(() {
          gl.dico.getLayerBase(layerKey).mOffline = false;
          gl.dico.getLayerBase(layerKey).mInDownload = false;
        });
      }
      return;
    });
  }

  void getAllDownloads() async {
    downloadData.clear();
    List<DownloadTask>? getTasks = await FlutterDownloader.loadTasks();
    getTasks?.forEach((task) {
      Map map = {};
      map['status'] = task.status;
      map['progress'] = task.progress;
      map['id'] = task.taskId;
      map['filename'] = task.filename;
      map['savedDirectory'] = task.savedDir;
      downloadData.add(map);
    });
  }

  int getProgress(String? id) {
    for (Map entry in downloadData) {
      if (entry["id"] == id) return entry["progress"];
    }
    return 0;
  }
}
