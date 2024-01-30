import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:flutter/services.dart';
import 'package:tuple/tuple.dart';

class Ess {
  String? mCode;
  String? mNomFR;
  int? mF_R;
  String? mPrefix;

  // aptitude ecograme : clé chaine charactère ; c'est la combinaison ntxnh du genre "A2p5" ou "Mm4
  Map<int, Map<String, int>> mEcoVal;
  // aptitude pour chaque zone bioclim
  Map<int, int> mAptZbio;
  // aptitude pour catalogue de station
  // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station (id + variante) Value ; aptitude
  Map<int, Map<Tuple2<int, String>, int>> mAptCS;
  // clé ; zone bioclim/ région. Value ; une map -> clé ; id situation topo. valeur ; code risque
  Map<int, Map<int, int>> mRisqueTopo;

  Ess.fromMap(final Map<String, dynamic> map)
      : mCode = map['Code_FR'],
        mNomFR = map['Ess_FR'],
        mPrefix = map['prefix'],
        mF_R = map['FeRe'],
        mEcoVal = {},
        mAptZbio = {},
        mAptCS = {},
        mRisqueTopo = {};

  Future<void> fillApt(dicoAptProvider dico) async {
    // aptitude hydro-trophique ; une matrice par zbioclimatique
    String myquery =
        'SELECT CodeNTNH,"1","2","3","4","5","6","7","8","9","10" FROM AptFEE WHERE CODE_ESSENCE="' +
            mCode.toString() +
            '";';
    List<Map<String, dynamic>> aAptEco = await dico.db.rawQuery(myquery);
    for (int zbio = 1; zbio <= 10; zbio++) {
      Map<String, int> EcoOneZbio = {};
      for (var r in aAptEco) {
        String apt = r[zbio.toString()];
        String codeNTNH = dico.code2NTNH(r['CodeNTNH']);
        // convertion apt code Str vers code integer
        int codeApt = dico.Apt(apt);
        EcoOneZbio.addEntries({codeNTNH: codeApt}.entries);
      }
      mEcoVal[zbio] = EcoOneZbio;
    }

    // aptitude climatique ; une aptitude pour chacune des 10 zones climatiques
    myquery =
        'SELECT "1","2","3","4","5","6","7","8","9","10" FROM AptFEE_ZBIO WHERE CODE_ESSENCE="' +
            mCode.toString() +
            '";';
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
        'SELECT Secteurfroid,Secteurneutre,Secteurchaud,Fond_vallee,SF_Ardenne,FV_Ardenne FROM Risque_topoFEE WHERE Code_Fr="' +
            mCode.toString() +
            '";';
    List<Map<String, dynamic>> rTopo = await dico.db.rawQuery(myquery);
    for (int zbio = 1; zbio <= 10; zbio++) {
      Map<int, int> rTopoOneZbio = {};
      for (var r in rTopo) {
        int codeRisque1 = dico.Risque(r['Secteurfroid']);
        int codeRisque4 = dico.Risque(r['Fond_vallee']);
        int codeRisque2 = dico.Risque(r['Secteurneutre']);
        int codeRisque3 = dico.Risque(r['Secteurchaud']);
        if (zbio == 1 || zbio == 2 || zbio == 10) {
          // pour l'ardenne
          if (r['SF_Ardenne'] != null) {
            codeRisque1 = dico.Risque(r['SF_Ardenne']);
          }
          if (r['FV_Ardenne'] != null) {
            codeRisque4 = dico.Risque(r['FV_Ardenne']);
          }
        }
        rTopoOneZbio.addEntries({
          1: codeRisque1,
          2: codeRisque2,
          3: codeRisque3,
          4: codeRisque4
        }.entries);
      }
      mRisqueTopo[zbio] = rTopoOneZbio;
    }

    // aptitude CS
    for (int zbio = 1; zbio <= 10; zbio++) {
      myquery = "SELECT stat_id," +
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
              aptCSOneZbio
                  .addEntries({Tuple2(station, variante): codeApt}.entries);
            }
          }
          mAptCS[zbio] = aptCSOneZbio;
        }
      } catch (e) {}
    }
  } // fin fillApt
}

class aptitude {
  late int mCodeNum;
  String? mLabelApt;
  String? mCode;
//String? mEquiv;
  int? mEquCodeNonContr;
  int? mEquiv;
  int? mOrdreContrainte;
  int? mSurcote;
  int? mSouscote;

  aptitude.fromMap(final Map<String, dynamic> map)
      : mCodeNum = map['Num'],
        mLabelApt = map['Aptitude'],
        mCode = map['Code_Aptitude'],
        mEquCodeNonContr = map['EquCodeNonContr'],
        mEquiv = map['Equiv2Code'],
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

class layerBase {
  String? mNom, mNomCourt;
  bool? mExpert; // l'app mobile n'as pas besoin de cette info, non?
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

  Future<void> fillLayerDico(dicoAptProvider dico) async {
    if (mCategorie != 'Externe' && nom_dico != null) {
      String myquery = 'SELECT ' +
          nom_field_raster.toString() +
          ' as rast, ' +
          nom_field_value.toString() +
          ' as val, col FROM ' +
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
  late Database db;
  Map<String, Color> colors = {};
  Map<String, layerBase> mLayerBases = {};
  Map<String, Ess> mEssences = {};
  List<aptitude> mAptitudes = [];
  List<risque> mRisques = [];
  List<zbio> mZbio = [];
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
      //print(mLayerBases[code].toString());
    }
    // lecture du dico aptitude
    result = await db.query('dico_apt');
    for (var row in result) {
      mAptitudes.add(aptitude.fromMap(row));
    }
    // lecture du dico risque
    result = await db.query('dico_risque');
    for (var row in result) {
      mRisques.add(risque.fromMap(row));
    }
    result = await db.query('dico_zbio');
    for (var row in result) {
      mZbio.add(zbio.fromMap(row));
    }
    // dico_NTNH
    result = await db.rawQuery('SELECT ID,concat2 FROM dico_NTNH;');
    for (var row in result) {
      dico_code2NTNH[row['ID']] = row['concat2'];
    }

    // lecture des essences
    result = await db.query('dico_essences');
    for (var row in result) {
      mEssences[row['Code_FR']] = Ess.fromMap(row);
      await mEssences[row['Code_FR']]?.fillApt(this);
    }

    db.close();
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

  int Risque(String aStr) {
    int aRes = 0;
    for (risque r in mRisques) {
      if (r.mRisque == aStr) {
        aRes = r.mCode;
        break;
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
