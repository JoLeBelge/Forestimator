import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/dico/ess.dart';

// une classe pour lire les résultats de l'analyse ponctuelle online
class LayerAnaPt {
  late String mCode;
  late bool mFoundLayer, mFoundRastFile;
  late int mRastValue;
  late String mValue;

  LayerAnaPt({required this.mCode, required this.mRastValue}) {
    mValue = gl.dico.getLayerBase(mCode).getValLabel(mRastValue);
    mFoundLayer = true;
    mFoundRastFile = true;
  }

  LayerAnaPt.fromMap(final Map<String, dynamic> map)
    : mCode = map['layerCode'],
      mFoundLayer = map['foundLayer'],
      mFoundRastFile = map['foundRastFile'],
      mRastValue = map["rastValue"],
      mValue = map["value"];
}

// listing de toutes les aptitudes pour création du tableau d'aptitude
class AptsFEE {
  // key=code essence. val = code aptitude (numérique)
  Map<String, int> mApts = {};
  Map<String, bool> mCompensations = {};
  late int nt, nh, zbio, topo;
  late bool ready;

  Map<String, int> getListEss(int codeApt) {
    Map<String, int> aRes = {};
    for (String esCode in mApts.keys) {
      if (gl.dico.aptContraignante(mApts[esCode]!) == codeApt) {
        aRes.addEntries({esCode: mApts[esCode]!}.entries);
      }
    }
    return aRes;
  }

  AptsFEE(List<LayerAnaPt> layersAnaP) {
    int test = 0;
    for (LayerAnaPt l in layersAnaP) {
      if (l.mCode == "ZBIO" && l.mFoundRastFile) {
        zbio = l.mRastValue;
        if (gl.dico.getLayerBase("ZBIO").mDicoVal.containsKey(zbio)) {
          ++test;
        }
      }
      if (l.mCode == "NT" && l.mFoundRastFile) {
        nt = l.mRastValue;
        if (gl.dico.getLayerBase("NT").mDicoVal.containsKey(nt)) {
          ++test;
        }
      }
      if (l.mCode == "NH" && l.mFoundRastFile) {
        nh = l.mRastValue;
        if (gl.dico.getLayerBase("NH").mDicoVal.containsKey(nh)) {
          ++test;
        }
      }
      if (l.mCode == "Topo" && l.mFoundRastFile) {
        topo = l.mRastValue;
        if (gl.dico.getLayerBase("Topo").mDicoVal.containsKey(topo)) {
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
        int aptHT = es.getAptHT(nt, nh, zbio, hierarchique: true, aTopo: topo);
        mApts.addEntries({es.mCode: aptHT}.entries);
        bool aCompensation =
            topo != 2 && aptHT != es.getAptHT(nt, nh, zbio, hierarchique: true)
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
class PropositionGS {
  // key=code essence. val = code proposition
  Map<String, int> mApts = {};
  late int us, zbio;
  //late String aVariante;
  late bool ready;

  Map<String, int> getListEss(int codeApt) {
    Map<String, int> aRes = {};
    for (String esCode in mApts.keys) {
      if (mApts[esCode] == codeApt) {
        aRes.addEntries({esCode: mApts[esCode]!}.entries);
      }
    }
    return aRes;
  }

  PropositionGS(List<LayerAnaPt> layersAnaP) {
    int test = 0;
    for (LayerAnaPt l in layersAnaP) {
      if (l.mCode == "ZBIO" && l.mFoundRastFile) {
        zbio = l.mRastValue;
        if (gl.dico.getLayerBase("ZBIO").mDicoVal.containsKey(zbio)) {
          ++test;
        }
      }
      if (l.mCode == "CS_A" && l.mFoundRastFile) {
        us = l.mRastValue;
        if (gl.dico.getLayerBase("CS_A").mDicoVal.containsKey(us)) {
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
          int recom = es.getAptCS(zbio, us);
          mApts.addEntries({es.mCode: recom}.entries);
        }
      }
    }
  }
}
