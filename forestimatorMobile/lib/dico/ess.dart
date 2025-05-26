import 'package:tuple/tuple.dart';

import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/dico/dicoApt.dart';

class Ess {
  late String mCode;
  late String mNomFR;
  late int mF_R;
  late String mPrefix;

  // aptitude ecograme : clé chaine charactère ; c'est la combinaison ntxnh du genre "A2p5" ou "Mm4
  Map<int, Map<String, int>> mEcoVal;
  // aptitude pour chaque zone bioclim
  Map<int, int> mAptZbio;
  // aptitude pour catalogue de station
  // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station (id + variante) Value ; aptitude
  Map<int, Map<Tuple2<int, String>, int>> mAptCS;
  // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station (id + variante) Value ; risque climatique (court et long terme)
  Map<int, Map<Tuple2<int, String>, int>> mRisqueCS;
  // clé ; zone bioclim/ région. Value ; une map -> clé ; id situation topo. valeur ; code risque
  Map<int, Map<int, int>> mRisqueTopo;

  String getFicheRoute({bool complete = false}) {
    if (complete) {
      return "/" + gl.basePathbranchB + "/fiche-esssence/" + mCode;
    }
    return 'fiche-esssence/' + mCode;
  }

  bool hasFEEapt() {
    return mAptZbio.length == 10;
  }

  bool hasCSapt() {
    return mAptCS.isNotEmpty;
  }

  int getApt(int aZbio) {
    int aRes = 0;
    if (mAptZbio.containsKey(aZbio)) {
      aRes = mAptZbio[aZbio]!;
    }
    return aRes;
  }

  int getAptCS(int aZbio, int US, {String aVar = ''}) {
    int aRes = 0;
    if (mAptCS.containsKey(aZbio)) {
      Map<Tuple2<int, String>, int> Apt = mAptCS[aZbio]!;
      // on prend par défaut la station majoritaire
      String variante = gl.dico.getStationMaj(aZbio, US);
      Tuple2<int, String> aUSkey = Tuple2(US, variante);
      if (Apt.containsKey(aUSkey)) {
        aRes = Apt[aUSkey]!;
      }
      // si l'utilisateur a renseigné une variante avec l'argument aVar:
      if (aVar != '') {
        aUSkey = Tuple2(US, aVar);
        if (Apt.containsKey(aUSkey)) {
          aRes = Apt[aUSkey]!;
        }
      }
      // risque climatique
      int aClim = getCSClim(aZbio, US, aVar: aVar);
      // on regroupe aptitude et risque clim
      if (aRes == 4) {
        aRes = 13;
      } else if (aRes != 0) {
        aRes = (aRes - 1) * 4 + aClim;
      }
      // maintenant on regroupe certaine classes ensemble
      aRes = gl.lutVulnerabiliteCS[aRes]!;
    }
    return aRes;
  }

  int getCSClim(int zbio, int US, {String aVar = ''}) {
    int aRes = 9;
    if (zbio == 1) {
      zbio = 2;
    }
    if (mRisqueCS.containsKey(zbio)) {
      Map<Tuple2<int, String>, int> clim = mRisqueCS[zbio]!;
      // on prend par défaut la station majoritaire
      String variante = gl.dico.getStationMaj(zbio, US);
      Tuple2<int, String> aUSkey = Tuple2(US, variante);

      if (clim.containsKey(aUSkey)) {
        aRes = clim[aUSkey]!;
      }
      // si l'utilisateur a renseigné une variante avec l'argument aVar:
      if (aVar != '') {
        aUSkey = Tuple2(US, aVar);
        if (clim.containsKey(aUSkey)) {
          aRes = clim[aUSkey]!;
        }
      }
    }
    return aRes;
  }

  int getAptHT(
    int aCodeNT,
    int aCodeNH,
    int aZbio, {
    bool hierarchique = true,
    int aTopo = 666,
  }) {
    int aRes = 12; // indéterminé zone batie ; par défaut
    if (aCodeNT == 0 && aCodeNH != 0) {
      aRes = 11;
      return aRes;
    }
    if (!mAptZbio.containsKey(aZbio)) {
      aRes = 11;
      return aRes;
    } // hors belgique ; Indéterminé mais pas zone batie
    String codeNTNH = "h" + aCodeNH.toString() + "t" + aCodeNT.toString();
    if (mEcoVal.containsKey(aZbio)) {
      if (mEcoVal[aZbio]!.containsKey(codeNTNH)) {
        aRes = mEcoVal[aZbio]![codeNTNH]!;
      }
    }
    if (hierarchique && mAptZbio.containsKey(aZbio)) {
      int aZbioApt = mAptZbio[aZbio]!;

      if (aTopo != 666) {
        // c'est sur cette aptitude que l'on applique un facteur de correction
        aZbioApt = corrigAptBioRisqueTopo(aZbioApt, aTopo, aZbio);
      }
      if (gl.dico.AptContraignante(aRes) < gl.dico.AptContraignante(aZbioApt)) {
        aRes = aZbioApt;
      }
    }
    return aRes;
  }

  int corrigAptBioRisqueTopo(int aptBio, int topo, int zbio) {
    int aRes = aptBio;
    int risque = 0;
    if (mRisqueTopo.containsKey(zbio) && mRisqueTopo[zbio]!.containsKey(topo)) {
      risque = mRisqueTopo[zbio]![topo]!;
    }
    int catRisque = gl.dico.risqueCat(risque);
    // situation favorable
    if (catRisque == 1) {
      aRes = gl.dico.AptSurcote(aptBio);
      return aRes;
    }
    // risque élevé et très élevé
    if (catRisque == 3) {
      aRes = gl.dico.AptSouscote(aptBio);
    }
    // attention, pour résineux, pas de Tolérance Elargie --> exclusion
    if (aRes == 3 && mF_R == 2) {
      aRes = 4;
    }
    //std::cout << "Décote l'aptitude " << mDico->code2Apt(apt) << " vers " << mDico->code2Apt(aRes) << std::endl;}
    return aRes;
  }

  int getRisque(int zbio, int topo) {
    int aRes = 0;
    if (mRisqueTopo.containsKey(zbio) && mRisqueTopo[zbio]!.containsKey(topo)) {
      aRes = mRisqueTopo[zbio]![topo]!;
    }
    return aRes;
  }

  String getNameAndPrefix() {
    return mPrefix + mNomFR;
  }

  Ess.fromMap(final Map<String, dynamic> map)
    : mCode = map['Code_FR'],
      mNomFR = map['Ess_FR'],
      mPrefix = map['prefix'],
      mF_R = map['FeRe'],
      mEcoVal = {},
      mAptZbio = {},
      mAptCS = {},
      mRisqueTopo = {},
      mRisqueCS = {};

  Future<void> fillApt(dicoAptProvider dico) async {
    // aptitude hydro-trophique ; une matrice par zbioclimatique
    String myquery =
        "SELECT CodeNTNH,`1`,`2`,`3`,`4`,`5`,`6`,`7`,`8`,`9`,`10` FROM AptFEE WHERE CODE_ESSENCE='" +
        mCode.toString() +
        "';";
    List<Map<String, dynamic>> aAptEco = await dico.db.rawQuery(myquery);
    for (int zbio = 1; zbio <= 10; zbio++) {
      Map<String, int> EcoOneZbio = {};
      for (var r in aAptEco) {
        String apt = r[zbio.toString()];
        String codeNTNH = dico.code2NTNH(r['CodeNTNH']);
        // convertion apt code Str vers code integer
        int codeApt = dico.Apt(apt);
        /*if (mCode == "CR" && zbio == 2) {
          print(r['CodeNTNH'].toString() +
              "= " +
              codeNTNH +
              " , aptitude = " +
              apt +
              " soit code " +
              codeApt.toString());
        }*/
        EcoOneZbio.addEntries({codeNTNH: codeApt}.entries);
      }
      mEcoVal[zbio] = EcoOneZbio;
    }

    // aptitude climatique ; une aptitude pour chacune des 10 zones climatiques
    myquery =
        "SELECT `1`,`2`,`3`,`4`,`5`,`6`,`7`,`8`,`9`,`10` FROM AptFEE_ZBIO WHERE CODE_ESSENCE='" +
        mCode.toString() +
        "';";
    List<Map<String, dynamic>> aAptZbio = await dico.db.rawQuery(myquery);
    for (var r in aAptZbio) {
      for (int zbio = 1; zbio <= 10; zbio++) {
        String apt = r[zbio.toString()];
        // convertion apt code Str vers code integer
        int codeApt = dico.Apt(apt);
        mAptZbio.addEntries({zbio: codeApt}.entries);
      }
    }

    // risque topographique ; permet de compenser une aptitude en bien (surcote) ou en mal (souscote)
    myquery =
        "SELECT Secteurfroid,Secteurneutre,Secteurchaud,Fond_vallee,SF_Ardenne,FV_Ardenne FROM Risque_topoFEE WHERE Code_Fr='" +
        mCode.toString() +
        "';";
    List<Map<String, dynamic>> rTopo = await dico.db.rawQuery(myquery);
    for (int zbio = 1; zbio <= 10; zbio++) {
      Map<int, int> rTopoOneZbio = {};
      for (var r in rTopo) {
        int codeRisque1 = dico.getRisque(r['Secteurfroid']);
        int codeRisque4 = dico.getRisque(r['Fond_vallee']);
        int codeRisque2 = dico.getRisque(r['Secteurneutre']);
        int codeRisque3 = dico.getRisque(r['Secteurchaud']);
        if (zbio == 1 || zbio == 2 || zbio == 10) {
          // pour l'ardenne
          if (r['SF_Ardenne'] != null) {
            codeRisque1 = dico.getRisque(r['SF_Ardenne']);
          }
          if (r['FV_Ardenne'] != null) {
            codeRisque4 = dico.getRisque(r['FV_Ardenne']);
          }
        }
        rTopoOneZbio.addEntries(
          {
            1: codeRisque1,
            2: codeRisque2,
            3: codeRisque3,
            4: codeRisque4,
          }.entries,
        );
      }
      mRisqueTopo[zbio] = rTopoOneZbio;
    }

    // aptitude CS
    for (int zbio = 1; zbio <= 10; zbio++) {
      myquery =
          "SELECT stat_id," +
          mCode.toString() +
          ",var FROM AptCS WHERE ZBIO=" +
          zbio.toString() +
          ";";
      try {
        List<Map<String, dynamic>> aptCS = await dico.db.rawQuery(myquery);
        if (aptCS.length > 0) {
          Map<Tuple2<int, String>, int> aptCSOneZbio = {};
          for (var r in aptCS) {
            if (r[mCode.toString()] != null) {
              int codeApt = dico.Apt(r[mCode.toString()]);
              String variante = "";
              r['var'] != null ? variante = r['var'] : variante = "";
              int station = r['stat_id'];
              aptCSOneZbio.addEntries(
                {Tuple2(station, variante): codeApt}.entries,
              );
            }
          }
          mAptCS[zbio] = aptCSOneZbio;
        }
      } catch (e) {}
      // risque climatique CS - attention ça n'as rien à voir avec le risque de la situation topographique (FEE)
      myquery =
          "SELECT stat_id," +
          mCode.toString() +
          ",var FROM AptCSClim WHERE ZBIO=" +
          zbio.toString() +
          ";";
      try {
        List<Map<String, dynamic>> risqueCS = await dico.db.rawQuery(myquery);
        if (risqueCS.length > 0) {
          Map<Tuple2<int, String>, int> risqueCSOneZbio = {};
          for (var r in risqueCS) {
            if (r[mCode.toString()] != null) {
              int risque = r[mCode.toString()]; //int.parse(mCode);
              String variante = "";
              r['var'] != null ? variante = r['var'] : variante = "";
              int station = r['stat_id'];
              risqueCSOneZbio.addEntries(
                {Tuple2(station, variante): risque}.entries,
              );
            }
          }
          mRisqueCS[zbio] = risqueCSOneZbio;
        }
      } catch (e) {}
    }
  } // fin fillApt
}
