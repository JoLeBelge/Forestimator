import 'package:http/http.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:flutter/services.dart';
import 'package:fforestimator/dico/ess.dart';
import 'dart:async';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/pages/anaPt/onePixGeotifDecoder.dart';
import 'package:proj4dart/proj4dart.dart' as proj4;

class aptitude {
  late int mCodeNum;
  String mLabelApt;
  late String mCode;
//String? mEquiv;
  late int mEquCodeNonContr;
  late int mAptContraigante;
  int? mOrdreContrainte;
  late int mSurcote;
  late int mSouscote;

  aptitude.fromMap(final Map<String, dynamic> map)
      : mCodeNum = map['Num'],
        mLabelApt = map['Aptitude'],
        mCode = map['Code_Aptitude'],
        mEquCodeNonContr = map['EquCodeNonContr'],
        mAptContraigante = map['Equiv2Code'],
        mOrdreContrainte = map['OrdreContrainte'],
        mSurcote = map['surcote'],
        mSouscote = map['souscote'];
}

class risque {
  late int mCode;
  String? mRisque;
  late int mCategorie;
  risque.fromMap(final Map<String, dynamic> map)
      : mCode = map['code'],
        mRisque = map['risque'],
        mCategorie = map['categorie'];
}

class vulnerabilite {
  late int mCode;
  String? mVulnerabilite;
  vulnerabilite.fromMap(final Map<String, dynamic> map)
      : mCode = map['raster_val'],
        mVulnerabilite = map['label'];
  //mCategorie = map['categorie'];
}

class station {
  late int mStationId;
  late int mZbio;
  late String mNomStationCarto;
  late String mNomVar;
  late bool mVarMaj;
  late String mVar;

  station.fromMap(final Map<String, dynamic> map)
      : mStationId = map['stat_id'],
        mZbio = map['ZBIO'],
        mNomStationCarto = map['Station_carto'],
        mNomVar = map['nom_var'] != null ? map['nom_var'] : '',
        mVarMaj = map['varMajoritaire'] == 1 ? true : false,
        mVar = map['var'] != null ? map['var'] : '';
}

class zbio {
  late int mCode;
  String? mNom;
  String? mCS_lay;
  int? mCSid;
  zbio.fromMap(final Map<String, dynamic> map)
      : mCode = map['Zbio'],
        mNom = map['Nom'],
        mCS_lay = map['CS_lay'],
        mCSid = map['CSid'];
}

class groupe_couche {
  late String mCode;
  late String mLabel;
  late bool mExpert;
  groupe_couche.fromMap(final Map<String, dynamic> map)
      : mCode = map['code'],
        mLabel = map['label'],
        mExpert = map['expert'] == 0 ? false : true;
}

class layerBase {
  late String mNom, mNomCourt, mNomRaster;
  late bool mExpert, mVisu, mOffline, mIsDownloadableRW;
  late String mCode;
  late String mUrl, mWMSLayerName, mWMSattribution, mTypeGeoservice;
  late String mGroupe;
  late String mCategorie;
  late String mTypeVar;
  late double mGain;
  late String mPdfName;
  late String mLogoAttributionFile;
  late int mPdfPage;
  late double mRes;
  String? nom_field_raster, nom_field_value, nom_dico, condition;
  Map<int, String> mDicoVal; // valeur raster vers signification
  Map<int, Color> mDicoCol; // valeur raster vers couleur
  late bool mUsedForAnalysis;
  late int mBits;
  //String mNomFile,mDir; mPathQml mPathRaster,

// frommap avec liste d'instanciation, inspiré de https://medium.com/@lumeilin/using-sqlite-in-flutter-59b27b099123
// named constructor
  layerBase.fromMap(final Map<String, dynamic> map)
      : mNom = map['NomComplet'],
        mCode = map['Code'],
        mNomCourt = map['NomCourt'],
        //mPathRaster = map['Dir3'], //+ '/' + map['Nom'], // pas si simple, chemin d'accès sur le mobile. Utile que si bulk download des raster
        mExpert = map['expert'] == 0 ? false : true,
        mVisu = map['visu'] == 0 ? false : true,
        mGroupe = map['groupe'],
        mUrl = map['WMSurl'] == null ? "" : map['WMSurl'],
        mWMSLayerName = map['WMSlayer'] == null ? "" : map['WMSlayer'],
        mWMSattribution =
            map['WMSattribution'] == null ? "" : map['WMSattribution'],
        mTypeGeoservice =
            map['typeGeoservice'] == null ? "" : map['typeGeoservice'],
        mCategorie = map['Categorie'],
        nom_field_raster = map['nom_field_raster'],
        nom_field_value = map['nom_field_value'],
        nom_dico = map['nom_dico'],
        condition = map['condition'],
        mTypeVar = map['TypeVar'],
        mGain = map['gain'] == null ? 66.6 : map['gain'],
        mPdfPage = map['pdfPage'] == null ? 0 : map['pdfPage'] - 1,
        mPdfName = map['pdfName'] == null ? "" : map['pdfName'],
        mRes = map['res'] == null ? 0.0 : map['res'],
        mNomRaster = map['Nom'],
        mDicoVal = {},
        mDicoCol = {},
        mOffline = false,
        mBits = map['Bits'] == null ? 8 : map['Bits'],
        mUsedForAnalysis = false {
    mIsDownloadableRW = mRes >= 10 ? true : false;
    mLogoAttributionFile = logoAttributionFile(mWMSattribution);
  }

  String logoAttributionFile(String mWMSattribution) {
    String aRes = "";
    switch (mWMSattribution) {
      case "Service Public de la Wallonie":
        aRes = 'assets/images/spw_fr_LR.png';
        break;
      case "IGN/NGI":
        aRes = 'assets/images/LOGO_NGI_LR.jpg';
        break;
      case "UCL":
        aRes = 'assets/images/UCLouvain_logo.png';
        break;
      case "Gembloux Agro-Bio Tech":
        aRes = 'assets/images/uLIEGE_Gembloux_AgroBioTech_Logo_CMJN_pos.png';
        break;
    }
    return aRes;
  }

  layerBase()
      : mNom = '',
        mCode = 'toto',
        mNomCourt = '',
        mExpert = true,
        mGroupe = '',
        mUrl = '',
        mWMSLayerName = '',
        mWMSattribution = '',
        mCategorie = '',
        nom_field_raster = '',
        nom_field_value = '',
        nom_dico = '',
        condition = '',
        mTypeVar = '',
        mGain = 0,
        mPdfName = '',
        mPdfPage = 1,
        mDicoVal = {},
        mDicoCol = {},
        mRes = 0.0,
        mUsedForAnalysis = false;

  bool hasDoc() {
    return mPdfName != "";
  }

  String getFicheRoute({int us = 0}) {
    // cas particulier du GSA ou j'ai un pdf différent par station
    if (mCode == "CS_A") {
      return gl.dico.getStationPdf(us);
    } else {
      return "documentation/" + mCode;
    }
  }

  String getEssCode() {
    String es = "toto";
    if (mGroupe == "APT_FEE" || mGroupe == "APT_CS") {
      es = mCode.substring(0, 2);
    }
    return es;
  }

  String getValLabel(int aRastValue) {
    String aRes = "";
    // attention aux MNH qui ont à la fois un dico pour la légende couleur, et à la fois un gain (pour la vrai valeur)
    if (mTypeVar == 'Continu' && mGain != 66.6) {
      double d = aRastValue * mGain;
      aRes = d.toStringAsFixed(1);
      return aRes;
    }
    if (mDicoVal.containsKey(aRastValue)) {
      aRes = mDicoVal[aRastValue]!;
      return aRes;
    }

    return aRes;
  }

  List<int> getDicoValForLegend() {
    // garder uniquement les valeurs pour lesquelles on a à la fois la légende texte et la couleur associée
    List<int> l1 = List<int>.from(mDicoVal.keys);
    List<int> l2 = List<int>.from(mDicoCol.keys);
    l1.removeWhere((item) => !l2.contains(item));
    return l1;
  }

  Color getValColor(int aRastValue) {
    Color col = HexColor('#FFFFFF');
    if (mDicoCol.containsKey(aRastValue)) {
      col = mDicoCol[aRastValue]!;
    }
    return col;
  }

  Future<void> fillLayerDico(dicoAptProvider dico) async {
    if (mCategorie != 'Externe' && nom_dico != null) {
      String myquery = 'SELECT ' +
          nom_field_raster.toString() +
          ' as rast, ' +
          nom_field_value.toString() +
          ' as val, "col" FROM ' +
          nom_dico.toString();
      if (condition != null) {
        myquery += ' WHERE ' + condition.toString();
      }
      myquery += ';';
      List<Map<String, dynamic>> adicoval = await dico.db.rawQuery(myquery);

      for (var r in adicoval) {
        // int DN = int.parse(r['rast']);
        mDicoVal[r['rast']] = r['val'].toString();
        if (adicoval.length < 300) {
          // sinon cnsw prends trop de temps, une couleur pour les 6000 sigles
          if (r['col'] == null) {
            // c'est le cas pour dico_MNT par exemple, mais pour CNSW également
            // print("couleur null dans table ${nom_dico}");
          } else {
            // test si c'est un code hexa ou un nom de couleur. Si null, le toString renvoie 'null'. pas très pratique évidemment
            String colcode = r['col'].toString(); // ?? '#FFFFFF';
            if (colcode.substring(0, 1) == '#') {
              mDicoCol[r['rast']] = HexColor(colcode);
            } else if (dico.colors.containsKey(colcode)) {
              mDicoCol[r['rast']] =
                  dico.colors[colcode] ?? Color.fromRGBO(255, 255, 255, 1.0);
              // } else {
              //print("couleur ${colcode} n'est pas définie dans le dico.colors");
            }
          }
        }
      }
    }
  }

  @override
  String toString() {
    String res = "layerbase code ${mCode}, name ${mNom}, dicoVal size " +
        mDicoVal.length.toString() +
        ' dicoCol size ' +
        mDicoCol.length.toString();
    return res;
  }

  void setHasOffline(bool offline) {
    mOffline = true;
  }

  Future<int> getValXY(proj4.Point pt) async {
    final File fileIm = File("${gl.dico.docDir?.path}/${mNomRaster}");
    onePixGeotifDecoder myDecoder = onePixGeotifDecoder(x: pt.x, y: pt.y);
    Uint8List bytes = await fileIm.readAsBytes();
    return myDecoder.getVal(bytes);
  }
}

class dicoAptProvider {
  bool finishedLoading = false;
  late Database db;
  Map<String, Color> colors = {};
  Map<String, layerBase> mLayerBases = {};
  Map<String, Ess> mEssences = {};
  List<aptitude> mAptitudes = [];
  List<vulnerabilite> mVulnerabilite =
      []; // carte recommandation CS = carte de vulnerabilite
  List<risque> mRisques =
      []; // attention, risque Topo FEE, pas risque Climatique CS
  List<zbio> mZbio = [];
  List<groupe_couche> mGrCouches = [];
  List<station> mStations = [];
  Map<int, String> dico_code2NTNH = {};
  late Directory docDir;

  Future<void> init() async {
    //final dbPath = await getDatabasesPath(); plante sous android

    
    docDir = await getApplicationDocumentsDirectory();
    final path = join(await getDatabasesPath(), "db/fforestimator.db");
    var exists = await databaseExists(path);
    
    if (!exists){
      // pour  maj à chaque fois des assets car sinon quand je fait une maj de l'app sur google play store et que la BD a changé, c'est l'ancienne bd qui est utilisée vu qu'elle existe déjà

      try {
        await Directory(dirname(path)).create(recursive: true);
      } catch (_) {}

      // Create the writable database file from the bundled  (asset bulk) fforestimator.db database file:
      // the bundled resource itself can't be directly opened as a file on Android -> c'est bien dommage
      ByteData data =
          await rootBundle.load(url.join("assets", "db/fforestimator.db"));
      List<int> bytes =
          data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);
      await File(path).writeAsBytes(bytes, flush: true);
    }
    
    db = await openDatabase(path, version: 1, readOnly: true  );
    /*
    List<Map<String, dynamic>> result = await db.query('dico_color');
    for (var r in result) {
      colors[r['Col']] = Color.fromRGBO(r['R'], r['G'], r['B'], 1.0);
    }
    result = await db.query('dico_colGrey');
    for (var r in result) {
      colors[r['Col']] = Color.fromRGBO(r['R'], r['G'], r['B'], 1.0);
    }
    result = await db.query('dico_viridisColors');
    for (var r in result) {
      colors[r['id'].toString()] = HexColor(r['hex']);
    }
    // lecture des layerbase
    result = await db.query('fichiersGIS', where: 'groupe IS NOT NULL');
    for (var row in result) {
      mLayerBases[row['Code']] = layerBase.fromMap(row);
    }
    result = await db.query('layerApt');
    for (var row in result) {
      mLayerBases[row['Code']] = layerBase.fromMap(row);
    }
    for (String code in mLayerBases.keys) {
      await mLayerBases[code]?.fillLayerDico(this);
      //print(mLayerBases[code].toString());
    }
    result = await db.query('dico_apt');
    for (var row in result) {
      mAptitudes.add(aptitude.fromMap(row));
    }
    result = await db.query('dico_risque');
    for (var row in result) {
      mRisques.add(risque.fromMap(row));
    }
    result = await db.query('dico_zbio');
    for (var row in result) {
      mZbio.add(zbio.fromMap(row));
    }
    result = await db.query('dico_recommandation');
    for (var row in result) {
      mVulnerabilite.add(vulnerabilite.fromMap(row));
    }
    result = await db.rawQuery('SELECT ID,concat2 FROM dico_NTNH;');
    for (var row in result) {
      dico_code2NTNH[row['ID']] = row['concat2'];
    }
    result = await db.query('groupe_couche');
    for (var row in result) {
      mGrCouches.add(groupe_couche.fromMap(row));
    }
    result = await db.query('dico_station', where: 'stat_id=stat_num');
    for (var row in result) {
      mStations.add(station.fromMap(row));
    }

    // lecture des essences
    result = await db.query('dico_essences');
    for (var row in result) {
      mEssences[row['Code_FR']] = Ess.fromMap(row);
      await mEssences[row['Code_FR']]?.fillApt(this);
    }

    db.close();
    finishedLoading = true;

    checkLayerBaseOfflineRessource();
    checkLayerBaseForAnalysis();
    */
  }

  Ess getEss(String aCode) {
    if (mEssences.containsKey(aCode)) {
      return mEssences[aCode]!;
    } else {
      throw "oops no essences " + aCode;
    }
  }

  List<Ess> getFEEess() {
    return mEssences.values.where((i) => i.hasFEEapt()).toList();
  }

  List<layerBase> getLayersWithDoc() {
    return mLayerBases.values.where((i) => i.mPdfName != "").toList();
  }

  List<layerBase> getLayersOffline() {
    List<layerBase> that = mLayerBases.values.where((i) => i.mOffline).toList();
    if (that.isEmpty) {
      return [layerBase()..mCode = gl.defaultLayer];
    }
    return that;
  }

  layerBase getLayerBase(String aCode) {
    if (mLayerBases.containsKey(aCode)) {
      return mLayerBases[aCode]!;
    } else {
      print("oops no layerBase " + aCode);
      return layerBase();
      //throw "oops no layerBase " + aCode;
    }
  }

  int Apt(String codeAptStr) {
    int aRes = 777;
    for (aptitude apt in mAptitudes) {
      if (apt.mCode == codeAptStr) {
        aRes = apt.mCodeNum;
      }
    }
    return aRes;
  }

  String AptLabel(int codeApt) {
    String aRes = "";
    for (aptitude apt in mAptitudes) {
      if (apt.mCodeNum == codeApt) {
        aRes = apt.mLabelApt;
      }
    }
    return aRes;
  }

  String code2NTNH(int aCode) {
    String aRes = "ND";
    dico_code2NTNH.forEach((k, v) {
      if (k == aCode) {
        aRes = v;
      }
    });
    return aRes;
  }

  String vulnerabiliteLabel(int aCode) {
    String aRes = "";
    for (vulnerabilite v in mVulnerabilite) {
      if (v.mCode == aCode) {
        aRes = v.mVulnerabilite!;
      }
    }
    return aRes;
  }

  int getRisque(String aStr) {
    int aRes = 0;
    for (risque r in mRisques) {
      if (r.mRisque == aStr) {
        aRes = r.mCode;
        break;
      }
    }
    return aRes;
  }

  int risqueCat(int aCode) {
    int aRes = 0;
    for (risque r in mRisques) {
      if (r.mCode == aCode) {
        aRes = r.mCategorie;
        break;
      }
    }
    return aRes;
  }

  int AptSurcote(int aCode) {
    int aRes = aCode;
    for (aptitude apt in mAptitudes) {
      if (apt.mCodeNum == aCode) {
        aRes = apt.mSurcote;
        break;
      }
    }
    return aRes;
  }

  int AptSouscote(int aCode) {
    int aRes = aCode;
    for (aptitude apt in mAptitudes) {
      if (apt.mCodeNum == aCode) {
        aRes = apt.mSouscote;
        break;
      }
    }
    return aRes;
  }

  int AptContraignante(int aCode) {
    int aRes = 0;
    for (aptitude apt in mAptitudes) {
      if (apt.mCodeNum == aCode) {
        aRes = apt.mAptContraigante;
        break;
      }
    }
    return aRes;
  }

  int AptNonContraignante(int aCode) {
    int aRes = 0;
    for (aptitude apt in mAptitudes) {
      if (apt.mCodeNum == aCode) {
        aRes = apt.mEquCodeNonContr;
        break;
      }
    }
    return aRes;
  }

  int zbio2CSid(int aCode) {
    int aRes = 0;
    for (zbio z in mZbio) {
      if (z.mCode == aCode) {
        aRes = z.mCSid!;
        break;
      }
    }
    return aRes;
  }

  String getStationMaj(int zbio, int US) {
    String aRes = "";
    int zbioKey = zbio2CSid(zbio);
    for (station st in mStations) {
      if (st.mZbio == zbioKey && st.mStationId == US && st.mVarMaj) {
        aRes = st.mVar;
        break;
      }
    }
    return aRes;
  }

  List<String> getAllStationFiches() {
    // en l'état, uniquement fonctionnel pour l'Ardenne
    List<station> that =
        mStations.where((i) => i.mVarMaj & (i.mZbio == 1)).toList();
    List<String> aRes =
        that.map((item) => getStationPdf(item.mStationId)).toList();
    return aRes;
  }

  String getStationPdf(int us) {
    return "US-A" + us.toString() + ".pdf";
  }

  Future<void> checkLayerBaseOfflineRessource() async {
    for (layerBase l in mLayerBases.values) {
      File file = File(getRastPath(l.mCode));
      if (await file.exists() == true) {
        l.setHasOffline(true);
      }
    }
    return;
  }

  void checkLayerBaseForAnalysis() async {
    for (layerBase l in mLayerBases.values) {
      File file = File(getRastPath(l.mCode));
      if (l.mGroupe != "APT_CS" &&
          l.mGroupe != "APT_FEE" &&
          l.mCategorie != "Externe") {
        l.mUsedForAnalysis = true;
      }
    }
    return;
  }

  String getRastPath(String aLayerCode) {
    layerBase l = getLayerBase(aLayerCode);
    return "${docDir?.path}/${l.mNomRaster}";
  }
}

class HexColor extends Color {
  static int _getColorFromHex(String hexColor) {
    hexColor = hexColor.toUpperCase().replaceAll("#", "");
    if (hexColor.length == 6) {
      hexColor = "FF" + hexColor;
    }
    return int.parse(hexColor, radix: 16);
  }

  HexColor(final String hexColor) : super(_getColorFromHex(hexColor));
}
