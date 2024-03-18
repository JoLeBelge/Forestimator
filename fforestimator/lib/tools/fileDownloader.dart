import 'dart:io';
import 'package:fforestimator/pages/catalogueView/layerTile.dart';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_downloader/flutter_downloader.dart';

class FileDownloader extends StatefulWidget {
  final LayerTile layer;
  const FileDownloader(this.layer, {super.key});

  @override
  State<FileDownloader> createState() => _FileDownloaderState();
}

class _FileDownloaderState extends State<FileDownloader> {
  @override
  Widget build(BuildContext context) {
    late Future<String?> taskId;
    if (!(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
      taskId = FlutterDownloader.enqueue(
        url: 	gl.queryApiRastDownload+ "/"+ widget.layer.key,
        //url: 	gl.queryApiRastDownload,
        fileName: gl.dico.getLayerBase(widget.layer.key).mNomRaster,
        headers: {}, // optional: header send with url (auth token etc)
        savedDir:"/storage/emulated/0/Download",
        //    gl.dico.docDir.path,
        showNotification:
            true, // show download progress in status bar (for Android)
        openFileFromNotification:
            true, // click on notification to open downloaded file (for Android)
      );
    }

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
        : Container(
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
}
