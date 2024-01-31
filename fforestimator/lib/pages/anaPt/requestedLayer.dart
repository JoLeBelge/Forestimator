import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/dico/ess.dart';

// une classe pour lire les résultats de l'analyse ponctuelle online
class layerAnaPt {
  late String mCode;
  late bool mFoundLayer, mFoundRastFile;
  late int mRastValue;
  late String mValue;

  layerAnaPt.fromMap(final Map<String, dynamic> map)
      : mCode = map['layerCode'],
        mFoundLayer = map['foundLayer'],
        mFoundRastFile = map['foundRastFile'],
        mRastValue = map["rastValue"],
        mValue = map["value"];
}

// listing de toutes les aptitudes pour création du tableau d'aptitude
class aptsFEE {
  Map<String, int> mApts = {};
  late int NT, NH, ZBIO, Topo;
  late bool ready;

  aptsFEE(List<layerAnaPt> layersAnaP) {
    int test = 0;
    for (layerAnaPt l in layersAnaP) {
      if (l.mCode == "ZBIO" && l.mFoundRastFile) {
        ZBIO = l.mRastValue;
        if (gl.dico.getLayerBase("ZBIO").mDicoVal.containsKey(ZBIO)) {
          ++test;
        }
      }
      if (l.mCode == "NT" && l.mFoundRastFile) {
        NT = l.mRastValue;
        if (gl.dico.getLayerBase("NT").mDicoVal.containsKey(NT)) {
          ++test;
        }
      }
      if (l.mCode == "NH" && l.mFoundRastFile) {
        NH = l.mRastValue;
        if (gl.dico.getLayerBase("NH").mDicoVal.containsKey(NH)) {
          ++test;
        }
      }
      if (l.mCode == "Topo" && l.mFoundRastFile) {
        Topo = l.mRastValue;
        if (gl.dico.getLayerBase("Topo").mDicoVal.containsKey(Topo)) {
          ++test;
        }
      }
    }

    if (test == 4) {
      ready = true;
    } else {
      ready = false;
    }

    //Ess ep = gl.dico.getEss("EP"); // situation de compensation pour l'epicea*/
    ZBIO = 3;
    NT = -1 + 10;
    NH = 1 + 10;
    Topo = 1;
    ready = true;

    if (ready) {
      /*print("ZBIO " +
          ZBIO.toString() +
          " NT " +
          NT.toString() +
          " NH " +
          NH.toString() +
          " Topo " +
          Topo.toString());*/
      for (Ess es in gl.dico.mEssences.values) {
        if (es.hasFEEapt()) {
          mApts.addEntries({
            es.mCode: es.getAptHT(NT, NH, ZBIO, hierarchique: true, aTopo: Topo)
          }.entries);

          /*  print("ess " + es.mNomFR!);
          print("ess apt zbio " + es.getApt(ZBIO).toString());
          print("ess apt HT " +
              es.getAptHT(NT, NH, ZBIO, hierarchique: false).toString());
          print("ess apt finale " +
              es
                  .getAptHT(NT, NH, ZBIO, hierarchique: true, aTopo: Topo)
                  .toString());*/
        }
      }
      //
    }
  }
}
