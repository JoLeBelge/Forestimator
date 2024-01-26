import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:flutter/services.dart';

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
    getLayers().then((mylist) {
      for (layerBase l in mylist) {
        print(l.toString());
      }
    });
  }

  Future<List<layerBase>> getLayers() async {
    List<layerBase> res = [];
    List<Map<String, dynamic>> result =
        await db.query('fichiersGIS', where: 'groupe IS NOT NULL AND expert=0');
    for (var row in result) {
      res.add(layerBase.fromMap(row));
    }
    result = await db.query('layerApt');
    for (var row in result) {
      res.add(layerBase.fromMap(row));
    }
    for (layerBase l in res) {
      await l.fillLayerDico(this);
    }
    return res;
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
