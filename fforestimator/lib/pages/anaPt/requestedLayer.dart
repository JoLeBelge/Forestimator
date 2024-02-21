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
  // key=code essence. val = code aptitude (numérique)
  Map<String, int> mApts = {};
  Map<String, bool> mCompensations = {};
  late int NT, NH, ZBIO, Topo;
  late bool ready;

  Map<String, int> getListEss(codeApt) {
    Map<String, int> aRes = {};
    for (String esCode in mApts.keys) {
      if (gl.dico.AptContraignante(mApts[esCode]!) == codeApt) {
        aRes.addEntries({esCode: mApts[esCode]!}.entries);
      }
    }
    return aRes;
  }

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

    if (ready) {
      for (Ess es in gl.dico.getFEEess()) {
        int aptHT = es.getAptHT(NT, NH, ZBIO, hierarchique: true, aTopo: Topo);
        mApts.addEntries({es.mCode: aptHT}.entries);
        bool aCompensation =
            Topo != 2 && aptHT != es.getAptHT(NT, NH, ZBIO, hierarchique: true)
                ? true
                : false;
        mCompensations.addEntries({es.mCode: aCompensation}.entries);
        /*
          print("ess " + es.mNomFR!);
          print("ess apt zbio " + es.getApt(ZBIO).toString());
          print("ess apt HT " +
              es.getAptHT(NT, NH, ZBIO, hierarchique: false).toString());
          print("ess apt finale " +
              es
                  .getAptHT(NT, NH, ZBIO, hierarchique: true, aTopo: Topo)
                  .toString());*/
      }
    }
  }
}

// listing des propositions d'essences du Guide des stations
class propositionGS {
  // key=code essence. val = code proposition
  Map<String, int> mApts = {};
  late int US, ZBIO;
  //late String aVariante;
  late bool ready;

  Map<String, int> getListEss(codeApt) {
    Map<String, int> aRes = {};
    for (String esCode in mApts.keys) {
      if (mApts[esCode] == codeApt) {
        aRes.addEntries({esCode: mApts[esCode]!}.entries);
      }
    }
    return aRes;
  }

  propositionGS(List<layerAnaPt> layersAnaP) {
    int test = 0;
    for (layerAnaPt l in layersAnaP) {
      if (l.mCode == "ZBIO" && l.mFoundRastFile) {
        ZBIO = l.mRastValue;
        if (gl.dico.getLayerBase("ZBIO").mDicoVal.containsKey(ZBIO)) {
          ++test;
        }
      }
      if (l.mCode == "CS_A" && l.mFoundRastFile) {
        US = l.mRastValue;
        if (gl.dico.getLayerBase("CS_A").mDicoVal.containsKey(US)) {
          ++test;
        }
      }
    }

    if (test == 2) {
      ready = true;
    } else {
      ready = false;
    }

    if (ready) {
      for (Ess es in gl.dico.mEssences.values) {
        if (es.hasCSapt()) {
          int recom = es.getAptCS(ZBIO, US);
          mApts.addEntries({es.mCode: recom}.entries);
        }
      }
    }
  }
}
