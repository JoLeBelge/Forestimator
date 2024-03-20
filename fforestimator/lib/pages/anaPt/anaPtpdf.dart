import 'package:fforestimator/pages/anaPt/anaPtpage.dart';
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'dart:io';
import 'package:pdf/widgets.dart' as pw;
import 'package:fforestimator/dico/ess.dart';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/services.dart';
import 'package:path_provider/path_provider.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;

class exportPdfDialog extends StatefulWidget {
  List<layerAnaPt> requestedLayers = [];
  @override
  exportPdfDialog(this.requestedLayers);
  State<exportPdfDialog> createState() => _exportPdfDialogState();
}

class _exportPdfDialogState extends State<exportPdfDialog> {
  late String _filename;
  @override
  Widget build(BuildContext context) {
    return Scaffold(
        backgroundColor: Colors.grey[200],
        appBar: AppBar(
          title: Text("Export d'un pdf"),
        ),
        body: Center(
          child: TextField(
            onChanged: (value) {
              setState(() {
                _filename = value;
              });
            },
            onSubmitted: (value) {
              makePdf(widget.requestedLayers, _filename);
            },
            decoration: InputDecoration(
              hintText: 'Choisisez un nom pour le fichier pdf',
            ),
          ),
        ));
  }
}

Future makePdf(List<layerAnaPt> layers, String fileName) async {
  final pdf = pw.Document();
  final imageLogo = pw.MemoryImage((await rootBundle
          .load('assets/images/GRF_nouveau_logo_uliege-retina.jpg'))
      .buffer
      .asUint8List());
  final now = DateTime.now();

  pdf.addPage(pw.Page(build: (context) {
    return pw.Column(children: [
      pw.Row(
        mainAxisAlignment: pw.MainAxisAlignment.spaceBetween,
        children: [
          pw.Column(
            children: [
              pw.Text("Analyse ponctuelle Forestimator"),
              pw.SizedBox(height: 5),
              pw.Text("Réalisé sans connextion internet, le " + now.toString()),
              pw.SizedBox(height: 5),
              pw.Text("X:" + gl.pt.x.toInt.toString()),
              pw.Text("Y:" + gl.pt.y.toInt.toString()),
            ],
            crossAxisAlignment: pw.CrossAxisAlignment.start,
          ),
          pw.SizedBox(
            height: 150,
            width: 150,
            child: pw.Image(imageLogo),
          )
        ],
      ) // first row
    ]);
  }));
  String? dir = await getDownloadPath();
  File out = File(dir! + "/" + fileName);
  out.writeAsBytes(await pdf.save(), flush: true);
  print("make pdf is done, written on " + out.path.toString());
}

Future<String?> getDownloadPath() async {
  Directory? directory;
  try {
    if (Platform.isIOS | Platform.isLinux) {
      directory = await getApplicationDocumentsDirectory();
    } else {
      directory = Directory('/storage/emulated/0/Download');
      // Put file in global download folder, if for an unknown reason it didn't exist, we fallback
      // ignore: avoid_slow_async_io
      if (!await directory.exists())
        directory = await getExternalStorageDirectory();
    }
  } catch (err, stack) {
    print("Cannot get download folder path");
  }
  return directory?.path;
}
