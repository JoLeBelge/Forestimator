import 'dart:io';
import 'dart:isolate';
import 'dart:ui';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';
import 'package:path/path.dart';

class LayerDownloader extends StatefulWidget {
  final LayerTile layer;
  final Function rebuildWidgetTree;
  const LayerDownloader(this.layer, this.rebuildWidgetTree, {super.key});

  @override
  State<LayerDownloader> createState() => _LayerDownloaderState();
}

class _LayerDownloaderState extends State<LayerDownloader> {
  static Map<String, double> _downloadStates = {};
  static Map<String?, String> _taskIDToLayerCode = {};
  ReceivePort _port = ReceivePort();

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
    if (_downloadStates[widget.layer.key] == null) {
      _downloadStates[widget.layer.key] = 0.0;
    } else if (_downloadStates[widget.layer.key]! > 0.0 &&
        _downloadStates[widget.layer.key]! < 1.0) {
      return LinearProgressIndicator(value: _downloadStates[widget.layer.key]);
    }
    if (gl.dico.getLayerBase(widget.layer.key).mOffline) {
      return Row(children: [
        IconButton(
            onPressed: () async {
              await fileDelete(join(gl.dico.docDir.path,
                      gl.dico.getLayerBase(widget.layer.key).mNomRaster))
                  .whenComplete(() {
                widget.rebuildWidgetTree(() {
                  gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                  _downloadStates[widget.layer.key] == 0.0;
                });
              });
            },
            icon: const Icon(Icons.delete)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text("La couche est enregistré."))
      ]);
    } else {
      return Row(children: [
        IconButton(
            onPressed: () async {
              String? it = await _downloadFile();
              setState(() {
                _downloadStates[widget.layer.key] = 0.001;
                _taskIDToLayerCode[it] = widget.layer.key;
              });
            },
            icon: const Icon(Icons.download)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text(
                "La couche peut être téléchargé pour l'utilisation hors ligne."))
      ]);
    }
  }

  Future<String?> _downloadFile() async {
    late Future<String?> taskId;
    if (!(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
      taskId = FlutterDownloader.enqueue(
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

    IsolateNameServer.registerPortWithName(
        _port.sendPort, 'downloader_send_port');
    _port.listen((dynamic data) async {
      String id = data[0];
      DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
      int progress = data[2];
      print(progress);
      if (progress > 100) {
        progress -= 100;
      }
      widget.rebuildWidgetTree((){});
      setState(() {
        if (status == DownloadTaskStatus.running) {
          _downloadStates[_taskIDToLayerCode[id]!] = progress / 100.0;
        }
        if (status == DownloadTaskStatus.complete) {
          gl.dico.getLayerBase(_taskIDToLayerCode[id]!).mOffline = true;
          _downloadStates[_taskIDToLayerCode[id]!] = 1.0;
        }
      });
    });
    FlutterDownloader.registerCallback(
      downloadCallback,
      step: 1,
    );
  }

  @override
  void dispose() {
    IsolateNameServer.removePortNameMapping('downloader_send_port');
    super.dispose();
  }

  @pragma('vm:entry-point')
  static void downloadCallback(String id, int status, int progress) {
    final SendPort? send =
        IsolateNameServer.lookupPortByName('downloader_send_port');
    send?.send([id, status, progress]);
  }
}
