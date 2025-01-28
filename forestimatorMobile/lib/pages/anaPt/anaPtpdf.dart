import 'package:fforestimator/dico/dicoApt.dart';
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:pdf/pdf.dart';
import 'dart:io';
import 'package:pdf/widgets.dart' as pw;
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter/services.dart';
import 'package:intl/intl.dart';

//import 'package:flutter_logs/flutter_logs.dart';

Future makePdf(List<layerAnaPt> layers, String fileName, String dir,
    String locationName) async {
  final pdf = pw.Document();
  final imageLogo = pw.MemoryImage((await rootBundle
          .load('assets/images/GRF_nouveau_logo_uliege-retina.jpg'))
      .buffer
      .asUint8List());
  final now = DateTime.now();

  pdf.addPage(pw.Page(build: (context) {
    return pw.Column(
        crossAxisAlignment: pw.CrossAxisAlignment.start,
        children: [
          pw.Row(
            mainAxisAlignment: pw.MainAxisAlignment.spaceBetween,
            children: [
              pw.Column(
                children: [
                  pw.Text("Analyse ponctuelle Forestimator",
                      style: pw.TextStyle(
                          fontSize: 18, color: PdfColor.fromHex("255f19"))),
                  pw.SizedBox(height: 30),
                  PaddedText((gl.offlineMode
                          ? "Réalisé en mode hors-ligne"
                          : "Réalisé avec connexion internet") +
                      " le " +
                      DateFormat('yyyy-MM-dd').format(now)),
                ],
                crossAxisAlignment: pw.CrossAxisAlignment.start,
              ),
              pw.SizedBox(
                height: 150,
                width: 150,
                child: pw.Image(imageLogo),
              )
            ],
          ), //first row
          PaddedText("Localisation: " + locationName, pad: 3),
          PaddedText("Coordonnée (EPSG:31370) ", pad: 3),
          PaddedText("X: " + gl.pt.x.toInt().toString(), pad: 3),
          PaddedText("Y: " + gl.pt.y.toInt().toString(), pad: 3),
          pw.SizedBox(height: 10),
          pw.Text("Couches cartographiques analysées",
              style: pw.TextStyle(fontSize: 16)),
          pw.SizedBox(height: 20),
          ...layers
              .where((i) => i.mRastValue != 0)
              .map<pw.Widget>((layerAnaPt a) {
            layerBase l = gl.dico.getLayerBase(a.mCode);
            return PaddedText(l.mNom + " : " + l.getValLabel(a.mRastValue));
          }).toList(),
        ]);
  }));

  if (dir != null) {
    File out = File(dir! + "/" + fileName);
    if (await out.exists()) {
      // on renomme le pdf
      int nb = 2;
      do {
        out = File(dir! +
            "/" +
            fileName.substring(0, fileName.length - 4) +
            nb.toString() +
            ".pdf");
        nb++;
        //print(out.path);
      } while (await out.exists());
    }
    out.writeAsBytes(await pdf.save(), flush: true);
    // FlutterLogs.logError("anaPt", "pdf", "pdf exported to. ${out.path}");
  } else {
    //FlutterLogs.logError("anaPt", "pdf", "downloadDirPath is null");
  }
}

pw.Widget PaddedText(
  final String text, {
  final pw.TextAlign align = pw.TextAlign.left,
  final double pad = 5.0,
}) =>
    pw.Padding(
      padding: pw.EdgeInsets.all(pad),
      child: pw.Text(
        text,
        textAlign: align,
      ),
    );
