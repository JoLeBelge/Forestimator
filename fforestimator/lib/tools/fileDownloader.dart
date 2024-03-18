import 'dart:io';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';
import 'package:path/path.dart';

class FileDownloader extends StatefulWidget {
  final LayerTile layer;
  const FileDownloader(this.layer, {super.key});

  @override
  State<FileDownloader> createState() => _FileDownloaderState();
}

class _FileDownloaderState extends State<FileDownloader> {
  @override
  Widget build(BuildContext context) {
    return (Platform.isWindows || Platform.isLinux || Platform.isMacOS)
        ? Container(
            color: gl.colorBackground,
            constraints: BoxConstraints(
                minWidth: MediaQuery.of(context).size.width * 1,
                maxWidth: MediaQuery.of(context).size.width * 1,
                minHeight: MediaQuery.of(context).size.height * .1,
                maxHeight: MediaQuery.of(context).size.height * .1),
            child: const Text("Downloads are not supported yet."),
          )
        : IconButton(
            onPressed: () async {
              await _downloadFile();
            },
            icon: Icon(Icons.download));
    Container(
      constraints: BoxConstraints(
          minWidth: MediaQuery.of(context).size.width * 1,
          maxWidth: MediaQuery.of(context).size.width * 1,
          minHeight: MediaQuery.of(context).size.height * .15,
          maxHeight: MediaQuery.of(context).size.height * .15),
      child: LinearProgressIndicator(
        value: 0.5,
        semanticsLabel: 'Linear progress indicator',
      ),
    );
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
}
