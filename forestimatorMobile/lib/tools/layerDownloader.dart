import 'dart:io';
import 'dart:isolate';
import 'dart:ui';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:fforestimator/tools/handlePermissions.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';
import 'package:path/path.dart';

final Map<String, _LayerDownloaderState> _downloadIdToWidget = {};
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

class LayerDownloader extends StatefulWidget {
  final LayerTile layer;
  LayerDownloader(this.layer, {super.key});

  @override
  State<LayerDownloader> createState() => _LayerDownloaderState();
}

@pragma('vm:entry-point')
class _LayerDownloaderState extends State<LayerDownloader> {
  static final Map<String, double> _downloadStates = {};

  static List<Map> downloadData = [];
  final ReceivePort _port = ReceivePort();

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
    /*while (_isDownloading) {
      int progress = getProgress(_taskIDToLayerCode[widget.layer.key]);
      print(progress);
      if (progress == 100) {
        setState(() {
          _isDownloading = false;
          gl.dico.checkLayerBaseOfflineRessource();
          gl.dico.getLayerBase(widget.layer.key).mOffline = true;
        });
      } else {
        setState(() {
          _isDownloading = true;
        });
      }
    }*/

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
                  _downloadStates[widget.layer.key] == 0.0;
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
              _downloadStates[widget.layer.key] = 0.01;
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

  void getAllDownloads() async {
    downloadData = [];
    List<DownloadTask>? getTasks = await FlutterDownloader.loadTasks();
    getTasks?.forEach((_task) {
      Map _map = Map();
      _map['status'] = _task.status;
      _map['progress'] = _task.progress;
      _map['id'] = _task.taskId;
      _map['filename'] = _task.filename;
      _map['savedDirectory'] = _task.savedDir;
      downloadData.add(_map);
    });
  }

  int getProgress(String? id) {
    for (Map entry in downloadData) {
      if (entry["id"] == id) return entry["progress"];
    }
    return 0;
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
    getAllDownloads();
  }

}

@pragma('vm:entry-point')
class ForestimatorDownloader {
  static Map<String, String> layerToId = {};
  final ReceivePort _port = ReceivePort();

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
    layerToId[layerKey] = taskId!;
    return taskId;
  }

  void _listenToDownloader() {
    IsolateNameServer.registerPortWithName(
      _port.sendPort,
      'downloader_send_port',
    );
    _port.listen((dynamic data) {
      String idListened = data[0];
      print("${data[2]}");
      _LayerDownloaderState downloader = _downloadIdToWidget[idListened]!;

      DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
      if (DownloadTaskStatus.enqueued == status) {
        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(downloader.widget.layer.key).mInDownload = true;
        });
      }
      if (status == DownloadTaskStatus.complete) {
        BuildContext context = gl.notificationContext!;

        showDialog(
          context: context,
          builder: (BuildContext context) {
            return AlertDialog(
              title: Text("Téléchargement"),
              content: Text(
                "${downloader.widget.layer.name} a été téléchargée avec succès.",
              ),
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

        gl.rebuildOfflineView(() {
          gl.dico.getLayerBase(downloader.widget.layer.key).mOffline = true;
          gl.dico.getLayerBase(downloader.widget.layer.key).mInDownload = false;
        });
        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(downloader.widget.layer.key).mInDownload = false;
          gl.dico.getLayerBase(downloader.widget.layer.key).mOffline = true;
        });
      }
      if (DownloadTaskStatus.failed == status) {
        //download failed
        BuildContext context = gl.notificationContext!;

        showDialog(
          context: context,
          builder: (BuildContext context) {
            return AlertDialog(
              title: Text("Problèmes de connexion"),
              content: Text(
                "${downloader.widget.layer.name} n'a pas été téléchargée.",
              ),
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

        gl.refreshWholeCatalogueView(() {
          gl.dico.getLayerBase(downloader.widget.layer.key).mInDownload = false;
          gl.dico.getLayerBase(downloader.widget.layer.key).mOffline = false;
        });
      }
      return;
    });
  }
}
