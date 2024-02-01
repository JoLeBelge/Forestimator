import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:flutter/services.dart';
import 'package:fforestimator/dico/ess.dart';

class aptitude {
  late int mCodeNum;
  String? mLabelApt;
  late String mCode;
//String? mEquiv;
  int? mEquCodeNonContr;
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
  late String mNom, mNomCourt;
  bool? mExpert;
  String? mCode;
  String? mUrl, mWMSLayerName, mWMSattribution;
  String? mGroupe;
  String? mCategorie;
  num? mGain;
  String? nom_field_raster, nom_field_value, nom_dico, condition;
  Map<int, String> mDicoVal; // valeur raster vers signification
  Map<int, Color> mDicoCol; // valeur raster vers couleur
  //String mNomFile,mDir; mPathQml mPathRaster,

// frommap avec liste d'instanciation, inspiré de https://medium.com/@lumeilin/using-sqlite-in-flutter-59b27b099123

// named constructor
  layerBase.fromMap(final Map<String, dynamic> map)
      : mNom = map['NomComplet'],
        mCode = map['Code'],
        mNomCourt = map['NomCourt'],
        //mPathRaster = map['Dir3'], //+ '/' + map['Nom'], // pas si simple, chemin d'accès sur le mobile. Utile que si bulk download des raster
        mExpert = map['expert'] == 0 ? false : true,
        mGroupe = map['groupe'],
        mUrl = map['WMSurl'],
        mWMSLayerName = map['WMSlayer'],
        mWMSattribution = map['WMSattribution'],
        mCategorie = map['Categorie'],
        nom_field_raster = map['nom_field_raster'],
        nom_field_value = map['nom_field_value'],
        nom_dico = map['nom_dico'],
        condition = map['condition'],
        mGain = map['gain'],
        mDicoVal = {},
        mDicoCol = {};

  String getValLabel(int aRastValue) {
    String aRes = "";
    if (mDicoVal.containsKey(aRastValue)) {
      aRes = mDicoVal[aRastValue]!;
    }
    return aRes;
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
        if (r['col'] == null) {
          // c'est le cas pour dico_MNT par exemple
          print("couleur null dans table ${nom_dico}");
        } else {
          mDicoVal[r['rast']] = r['val'].toString();
          // test si c'est un code hexa ou un nom de couleur. Si null, le toString renvoie 'null'. pas très pratique évidemment
          String colcode = r['col'].toString(); // ?? '#FFFFFF';
          if (colcode.substring(0, 1) == '#') {
            mDicoCol[r['rast']] = HexColor(colcode);
          } else if (dico.colors.containsKey(colcode)) {
            mDicoCol[r['rast']] =
                dico.colors[colcode] ?? Color.fromRGBO(255, 255, 255, 1.0);
          } else {
            print("couleur ${colcode} n'est pas définie dans le dico.colors");
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
}

class dicoAptProvider {
  bool finishedLoading = false;
  late Database db;
  Map<String, Color> colors = {};
  Map<String, layerBase> mLayerBases = {};
  Map<String, Ess> mEssences = {};
  List<aptitude> mAptitudes = [];
  List<risque> mRisques = [];
  List<zbio> mZbio = [];
  List<groupe_couche> mGrCouches = [];
  List<station> mStations = [];
  Map<int, String> dico_code2NTNH = {};

  Future<void> init() async {
    //final dbPath = await getDatabasesPath(); plante sous android
    Directory docDir = await getApplicationDocumentsDirectory();
    final path = join(docDir.path, "fforestimator.db");
// Check if the database exists
    var exists = await databaseExists(path);

    if (!exists) {
      // Should happen only the first time you launch your application
      print("Creating new copy from asset");

      // Make sure the parent directory exists
      try {
        await Directory(dirname(path)).create(recursive: true);
      } catch (_) {}

      // Create the writable database file from the bundled  (asset bulk) fforestimator.db database file:
      ByteData data =
          await rootBundle.load(url.join("assets", "db/fforestimator.db"));
      List<int> bytes =
          data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);

      // Write and flush the bytes written
      await File(path).writeAsBytes(bytes, flush: true);
    } else {
      print("Opening existing database");
    }

    db = await openDatabase(path, version: 1, readOnly: true
        //onCreate: _onCreate,
        );
    // lecture des couleurs
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
    result =
        await db.query('fichiersGIS', where: 'groupe IS NOT NULL AND expert=0');
    for (var row in result) {
      mLayerBases[row['Code']] = layerBase.fromMap(row);
    }
    result = await db.query('layerApt');
    for (var row in result) {
      mLayerBases[row['Code']] = layerBase.fromMap(row);
    }
    for (String code in mLayerBases.keys) {
      await mLayerBases[code]?.fillLayerDico(this);
      print(mLayerBases[code].toString());
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
  }

  Ess getEss(String aCode) {
    if (mEssences.containsKey(aCode)) {
      return mEssences[aCode]!;
    } else {
      throw "oops no essences " + aCode;
    }
  }

  layerBase getLayerBase(String aCode) {
    if (mLayerBases.containsKey(aCode)) {
      return mLayerBases[aCode]!;
    } else {
      throw "oops no layerBase " + aCode;
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

  String code2NTNH(int aCode) {
    String aRes = "ND";
    dico_code2NTNH.forEach((k, v) {
      if (k == aCode) {
        aRes = v;
      }
    });
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
      if (st.mZbio == zbioKey && st.mVarMaj) {
        aRes = st.mVar;
      }
    }
    return aRes;
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
