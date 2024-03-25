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
  const LayerDownloader(this.layer, {super.key});

  @override
  State<LayerDownloader> createState() => _LayerDownloaderState();
}

class _LayerDownloaderState extends State<LayerDownloader> {
  static double downloadState = 0.0;
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
    if (gl.dico.getLayerBase(widget.layer.key).mOffline ||
        downloadState == 1.0) {
      return Row(children: [
        IconButton(
            onPressed: () async {
              await fileDelete(join(gl.dico.docDir.path,
                  gl.dico.getLayerBase(widget.layer.key).mNomRaster));
              setState(() {
                gl.dico.getLayerBase(widget.layer.key).mOffline = false;
                downloadState == 0.0;
              });
            },
            icon: const Icon(Icons.delete)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text("La couche est enregistré."))
      ]);
    } else if (downloadState == 0.0) {
      return Row(children: [
        IconButton(
            onPressed: () async {
              _downloadFile();
              setState(() {
                downloadState = 0.001;
              });
            },
            icon: const Icon(Icons.download)),
        Container(
            constraints: const BoxConstraints(
                maxWidth: 256, minWidth: 48, maxHeight: 48, minHeight: 48),
            child: const Text(
                "La couche peut être téléchargé pour l'utilisation hors ligne."))
      ]);
    } else {
      final tasks = FlutterDownloader.loadTasksWithRawQuery(
          query: "SELECT * FROM task WHERE status=3");
      print(tasks.toString());

      return Container(
        constraints: BoxConstraints(
            minWidth: MediaQuery.of(context).size.width * 1,
            maxWidth: MediaQuery.of(context).size.width * 1,
            minHeight: MediaQuery.of(context).size.height * .15,
            maxHeight: MediaQuery.of(context).size.height * .15),
        child: LinearProgressIndicator(
          value: downloadState,
          semanticsLabel: 'Linear progress indicator',
        ),
      );
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
    _port.listen((dynamic data) {
      String id = data[0];
      DownloadTaskStatus status = DownloadTaskStatus.fromInt(data[1]);
      int progress = data[2];
      setState((){ });
    });

    FlutterDownloader.registerCallback(downloadCallback);
  }

  @override
  void dispose() {
    IsolateNameServer.removePortNameMapping('downloader_send_port');
    super.dispose();
  }

  @pragma('vm:entry-point')
  static void downloadCallback(String id, int status, int progress) {
    downloadState = (progress - 100) / 100;
    print(downloadState);

    final SendPort? send =
        IsolateNameServer.lookupPortByName('downloader_send_port');
    send?.send([id, status, progress]);
  }
}
