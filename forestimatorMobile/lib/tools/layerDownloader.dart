import 'dart:io';
import 'dart:isolate';
import 'dart:ui';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';
import 'package:path/path.dart';
import 'package:flutter_logs/flutter_logs.dart';

class LayerDownloader extends StatefulWidget {
  final LayerTile layer;
  const LayerDownloader(this.layer, {super.key});

  @override
  State<LayerDownloader> createState() => _LayerDownloaderState();
}

class _LayerDownloaderState extends State<LayerDownloader> {
  static Map<String, double> _downloadStates = {};
  //static Map<String?, String> _taskIDToLayerCode = {};
  //ReceivePort _port = ReceivePort();
  bool _isDownloading = false;

  @override
  Widget build(BuildContext context) {
    if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
      return Container(
        color: gl.colorBackground,
        constraints: BoxConstraints(
            minWidth: MediaQuery.of(context).size.width * 1,
            maxWidth: MediaQuery.of(context).size.width * 1,
            minHeight: MediaQuery.of(context).size.height * .1,
            maxHeight: MediaQuery.of(context).size.height * .1),
        child: const Text("Downloads are not supported yet."),
      );
    }
    /* if (_downloadStates[widget.layer.key] == null) {
      _downloadStates[widget.layer.key] = 0.0;
    } else if (_downloadStates[widget.layer.key]! != 0.0 &&
        _downloadStates[widget.layer.key]! != 1.0) {
      return Container(
          color: gl.colorBackground,
          constraints: BoxConstraints(
              minWidth: MediaQuery.of(context).size.width * 1,
              maxWidth: MediaQuery.of(context).size.width * 1,
              minHeight: MediaQuery.of(context).size.height * .1,
              maxHeight: MediaQuery.of(context).size.height * .1),
          child: LinearProgressIndicator(
            value: _downloadStates[widget.layer.key]! < 1.0 &&
                    _downloadStates[widget.layer.key]! > 0.0
                ? _downloadStates[widget.layer.key]!
                : 0.5,
          ));
    }*/
    if (gl.dico.getLayerBase(widget.layer.key).mOffline) {
      return Row(children: [
        IconButton(
            onPressed: () async {
              await fileDelete(join(gl.dico.docDir.path,
                      gl.dico.getLayerBase(widget.layer.key).mNomRaster))
                  .whenComplete(() {
                setState(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                  _downloadStates[widget.layer.key] == 0.0;
                  gl.removeFromOfflineList(widget.layer.key);
                });
                gl.refreshWholeCatalogueView(() {});
                gl.rebuildOfflineView(() {});
              });
            },
            icon: const Icon(Icons.delete)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text("La couche est enregistrée."))
      ]);
    } else if (_isDownloading) {
      return Container(
          constraints: const BoxConstraints(
              maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
          child: const Text("en téléchargement."));
    } else {
      return Row(children: [
        IconButton(
            onPressed: () async {
              var it = await _downloadFile();
              //_downloadStates[widget.layer.key] = 0.01;
              //_taskIDToLayerCode[it] = widget.layer.key;
              _isDownloading = true;
              setState(() {
                //gl.offlineMode = gl.offlineMode;
              });
            },
            icon: const Icon(Icons.download)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text(
                "La couche peut être téléchargée pour l'utilisation hors ligne."))
      ]);
    }
  }

  Future<String?> _downloadFile() async {
    //late Future<String?> taskId;
    late String? taskId;
    if (!(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
      taskId = await FlutterDownloader.enqueue(
        url: gl.queryApiRastDownload +
            "/" +
            gl.dico.getLayerBase(widget.layer.key).mCode,
        fileName: gl.dico.getLayerBase(widget.layer.key).mNomRaster,
        savedDir: gl.dico.docDir.path,
        showNotification: false,
        openFileFromNotification: false,
        timeout: 15000,
      );
    }

    return taskId;
  }

  /*void _bindBackgroundIsolate() {
    IsolateNameServer.registerPortWithName(
        _port.sendPort, 'downloader_send_port');
    _port.listen((dynamic data) {
      String id = data[0];
      print("inside bindBackgroundIsolate!!");
      if (_taskIDToLayerCode[id] == widget.layer.key) {
        DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
        double progress = (data[2] / -409600);
        /*if (progress > 100) {
        progress -= 100;
      } else if (progress < 0) {
        progress = 0;
      }*/
        if (status == DownloadTaskStatus.enqueued) {
          FlutterLogs.logInfo("download", "started", "Download enqueued");
        } else if (status == DownloadTaskStatus.running) {
          setState(() {
            _downloadStates[_taskIDToLayerCode[id]!] = progress;

            /// 100.0;
          });
          FlutterLogs.logInfo("download", "running",
              "Download running " + (progress).toString()); // / 100.0
        } else if (status == DownloadTaskStatus.complete) {
          FlutterLogs.logInfo("download", "completed", "Download finished");
          setState(() {
            _downloadStates[_taskIDToLayerCode[id]!] = 1.0;
            gl.dico.getLayerBase(_taskIDToLayerCode[id]!).mOffline = true;
            gl.addToOfflineList(_taskIDToLayerCode[id]!);
            gl.dico.checkLayerBaseOfflineRessource();
          });
          gl.refreshWholeCatalogueView(() {});
          gl.rebuildOfflineView(() {});
        } else if (status == DownloadTaskStatus.failed) {
          setState(() {
            gl.dico.getLayerBase(_taskIDToLayerCode[id]!).mOffline = false;
            _downloadStates[_taskIDToLayerCode[id]!] = 0.0;
          });
          FlutterLogs.logInfo("download", "FAILED", "Download failed");
        } else {
          FlutterLogs.logInfo("download", "NOT IMPLEMENTED",
              "DownloadTaskStatus. is not implemented");
        }
      }
      //FlutterDownloader.registerCallback(downloadCallback);
    });
  }*/

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
    //This part sucks. its executed at 'progress' = 0 -409600 100

    /*register callback after start download!!
    if (Platform.isAndroid || Platform.isIOS) {
      FlutterDownloader.registerCallback(
        downloadCallback,
        step: 50,
      );
    }*/
  }

  /* @pragma('vm:entry-point')
  static void downloadCallback(String id, int status, int progress) {
    //print("callback here and there");
    final SendPort send =
        IsolateNameServer.lookupPortByName('downloader_send_port')!;
    //if (send != null) {
    send.send([id, status, progress]);
    //}
  }*/

  @override
  void dispose() {
    //IsolateNameServer.removePortNameMapping('downloader_send_port');
    super.dispose();
  }
}
