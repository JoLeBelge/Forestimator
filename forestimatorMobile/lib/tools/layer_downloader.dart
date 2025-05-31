import 'dart:io';
import 'dart:isolate';
import 'dart:ui';
import 'package:downloadsfolder/downloadsfolder.dart';
import 'package:fforestimator/pages/catalogueView/layer_tile.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import 'package:fforestimator/tools/notification.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';

final Map<String, _LayerDownloaderState> _downloadIdToWidget = {};
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
  bool listenerInitialized = false;
  dynamic buildContextNotifications;
  String? downloadId = "";

  @override
  Widget build(BuildContext context) {
    return handlePermissionForStorage(
      child: buildDL(context),
      refreshParentWidgetTree: setState,
    );
  }

  Widget buildDL(BuildContext context) {
    buildContextNotifications = context;
    if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
      return Container(
        color: gl.colorBackground,
        constraints: BoxConstraints(
          minWidth: MediaQuery.of(context).size.width * 1,
          maxWidth: MediaQuery.of(context).size.width * 1,
          minHeight: MediaQuery.of(context).size.height * .1,
          maxHeight: MediaQuery.of(context).size.height * .1,
        ),
        child: const Text("Downloads are not supported yet."),
      );
    }

    if (gl.dico.getLayerBase(widget.layer.key).mOffline) {
      return Row(
        children: [
          IconButton(
            onPressed: () async {
              await fileDelete(
                join(
                  gl.dico.docDir.path,
                  gl.dico.getLayerBase(widget.layer.key).mNomRaster,
                ),
              ).whenComplete(() {
                setState(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                  gl.removeFromOfflineList(widget.layer.key);
                });
                gl.refreshWholeCatalogueView(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                });
                gl.rebuildOfflineView(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                });
              });
            },
            icon: const Icon(Icons.delete),
          ),
          Container(
            constraints: const BoxConstraints(
              maxWidth: 256,
              minWidth: 48,
              maxHeight: 48,
              minHeight: 48,
            ),
            child: const Text("La couche est enregistrée."),
          ),
        ],
      );
    } else if (gl.dico.getLayerBase(widget.layer.key).mInDownload) {
      return Row(
        children: [
          IconButton(
            onPressed: () async {
              FlutterDownloader.cancel(taskId: downloadId!);
              setState(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              gl.refreshWholeCatalogueView(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              gl.rebuildOfflineView(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              downloadId = await fD?.downloadFile(widget.layer.key);
              setState(() {
                _downloadIdToWidget[downloadId!] = this;
                print(_downloadIdToWidget[downloadId!]);
              });
            },
            icon: const Icon(Icons.repeat_rounded),
          ),
          Container(
            constraints: const BoxConstraints(
              maxWidth: 256,
              minWidth: 48,
              maxHeight: 48,
              minHeight: 48,
            ),
            child: const Text("Réessayer."),
          ),
        ],
      );
    } else {
      return Row(
        children: [
          IconButton(
            onPressed: () async {
              setState(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              gl.refreshWholeCatalogueView(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              gl.rebuildOfflineView(() {
                gl.dico.getLayerBase(widget.layer.key).mInDownload = true;
              });
              downloadId = await fD?.downloadFile(widget.layer.key);
              setState(() {
                _downloadIdToWidget[downloadId!] = this;
                print(_downloadIdToWidget[downloadId!]);
              });
            },
            icon: const Icon(Icons.download),
          ),
          Container(
            constraints: const BoxConstraints(
              maxWidth: 256,
              minWidth: 48,
              maxHeight: 48,
              minHeight: 48,
            ),
            child: const Text(
              "La couche peut être téléchargée pour l'utilisation hors ligne.",
            ),
          ),
        ],
      );
    }
  }

  static Future<bool> fileExists(String path) async {
    final File file = File(path);
    return file.exists();
  }

  static Future<bool> fileDelete(String path) async {
    final File file = File(path);
    if (await file.exists()) {
      file.delete();
    }
    return fileExists(path);
  }

  @override
  void initState() {
    super.initState();
  }
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
      if (_downloadIdToWidget[idListened] != null) {
        layerKey = _downloadIdToWidget[idListened]!.widget.layer.key;
        layerName = _downloadIdToWidget[idListened]!.widget.layer.name;
      } else {
        layerKey = _downloadIdToLayerKey[idListened]!;
        layerName = _downloadIdToLayerName[idListened]!;
      }

      DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
      if (DownloadTaskStatus.enqueued == status) {
        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(layerKey).mInDownload = true;
        });
      }
      if (status == DownloadTaskStatus.complete) {
        BuildContext context = gl.notificationContext!;
        PopupDownloadSuccess(context, layerName);

        gl.rebuildOfflineView(() {
          gl.dico.getLayerBase(layerKey).mOffline = true;
          gl.dico.getLayerBase(layerKey).mInDownload = false;
        });
        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(layerKey).mInDownload = false;
          gl.dico.getLayerBase(layerKey).mOffline = true;
        });
      }
      if (DownloadTaskStatus.failed == status) {
        BuildContext context = gl.notificationContext!;

        PopupDownloadFailed(context, layerName);

        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(layerKey).mInDownload = false;
          gl.dico.getLayerBase(layerKey).mOffline = false;
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
