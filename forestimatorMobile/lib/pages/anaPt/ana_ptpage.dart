import 'dart:io';
import 'dart:math';

import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:fforestimator/pages/anaPt/ana_ptpdf.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/myicons.dart';
import 'package:go_router/go_router.dart';
import 'package:path_provider/path_provider.dart';

class AnaPtpage extends StatefulWidget {
  final List<LayerAnaPt> requestedLayers;
  const AnaPtpage(this.requestedLayers, {super.key});

  @override
  State<AnaPtpage> createState() => _AnaPtpageState();
}

class _AnaPtpageState extends State<AnaPtpage> {
  late TextEditingController controllerPdfName;
  late TextEditingController controllerLocationName;
  @override
  Widget build(BuildContext context) {
    gl.anaPtPageContext = context;
    // pour la construction du tableau d'aptitude
    AptsFEE apts = AptsFEE(widget.requestedLayers);
    PropositionGS aptsGS = PropositionGS(widget.requestedLayers);
    double boxheight = 12.0 * gl.display.equipixel;
    double boxwidth = 80.0 * gl.display.equipixel;
    double iconBoxWidth = 15.0 * gl.display.equipixel;
    double sizeIcon = gl.iconSizeS * gl.display.equipixel;
    double sizeFontTitle = gl.fontSizeL * gl.display.equipixel;
    double sizeFontNormal = gl.fontSizeM * gl.display.equipixel;
    double sizeFontSmall = gl.fontSizeS * gl.display.equipixel;
    return Scaffold(
      backgroundColor: Colors.grey[200],
      appBar: AppBar(
        title: Container(
          alignment: Alignment.centerLeft,
          width: boxwidth,
          height: boxheight,
          child: Text(
            "Analyse pour cette position",
            overflow: TextOverflow.ellipsis,
            style: TextStyle(
              fontWeight: FontWeight.normal,
              fontSize: sizeFontTitle,
            ),
          ),
        ),
      ),
      body: SingleChildScrollView(
        physics: ScrollPhysics(),
        child: Column(
          children: [
            Text(
              gl.offlineMode
                  ? "Analyse réalisée hors-ligne"
                  : "Analyse réalisée en ligne",
              style: TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: sizeFontNormal,
              ),
            ),
            handlePermissionForStorage(
              child: SizedBox(
                child:
                    getStorage()
                        ? Row(
                          mainAxisAlignment: MainAxisAlignment.start,
                          children: [
                            Container(
                              alignment: Alignment.center,
                              width: iconBoxWidth,
                              child: Icon(
                                Icons.picture_as_pdf,
                                size: sizeIcon,
                                color: Colors.black,
                              ),
                            ),

                            TextButton(
                              onPressed: () async {
                                bool isPermitted = true;
                                if (isPermitted) {
                                  List<String>? l = await openDialog();
                                  String? pdf = l?.elementAt(0);
                                  String? locationName = l?.elementAt(1);
                                  if (pdf != null && pdf.isEmpty) {
                                    pdf = "analysePonctuelleForestimator.pdf";
                                  }
                                  if (pdf!.length < 4 ||
                                      pdf.substring(pdf.length - 4) != ".pdf") {
                                    pdf = "$pdf.pdf";
                                  }
                                  if (locationName!.isEmpty) {
                                    locationName = "une position";
                                  }
                                  String dir = "/storage/emulated/0/Download";
                                  if (Platform.isIOS) {
                                    dir =
                                        (await getApplicationDocumentsDirectory())
                                            .path;
                                  }
                                  makePdf(
                                    widget.requestedLayers,
                                    pdf,
                                    dir,
                                    locationName,
                                  );
                                  // confirmation que le pdf a été créé
                                  //PopupPDFSaved(gl.anaPtPageContext!, pdf);
                                }
                              },
                              child: Text(
                                "Saufgardez cette analyse sur votre appareil",
                                overflow: TextOverflow.ellipsis,
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: sizeFontSmall,
                                  fontWeight: FontWeight.normal,
                                ),
                              ),
                            ),
                          ],
                        )
                        : SizedBox(),
              ),
              refreshParentWidgetTree: setState,
            ),
            _anaPtListLayers(context, widget.requestedLayers),
            if (apts.ready)
              Card(
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                  side: BorderSide(
                    color: Color.fromRGBO(205, 225, 138, 1.0),
                    width: 2.0,
                  ),
                ),
                child: _tabAptFEE(context, apts),
              ),

            if (aptsGS.ready)
              Card(
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadiusGeometry.circular(12.0),
                  side: BorderSide(
                    color: Color.fromRGBO(205, 225, 138, 1.0),
                    width: 2.0,
                  ),
                ),
                child: _tabPropositionCS(context, aptsGS),
              ),
          ],
        ),
      ),
    );
  }

  Future<List<String>?> openDialog() => showDialog<List<String>>(
    context: context,
    builder:
        (context) => AlertDialog(
          title: Text("Nom du pdf et de la localisation"),
          content: SingleChildScrollView(
            physics: ScrollPhysics(),
            child: Column(
              children: [
                TextField(
                  controller: controllerPdfName,
                  autofocus: true,
                  decoration: InputDecoration(
                    hintText: "analysePonctuelleForestimator.pdf",
                  ),
                ),
                TextField(
                  controller: controllerLocationName,
                  decoration: InputDecoration(hintText: "Ex: Point 5"),
                ),
              ],
            ),
          ),
          actions: [
            TextButton(onPressed: submit, child: Text("Générer le pdf")),
          ],
        ),
  );

  void submit() {
    Navigator.of(
      context,
      rootNavigator: true,
    ).pop([controllerPdfName.text, controllerLocationName.text]);
  }

  @override
  void initState() {
    super.initState();
    controllerPdfName = TextEditingController();
    controllerLocationName = TextEditingController();
  }
}

class LayerAnaPtListTile extends StatelessWidget {
  final LayerAnaPt data;
  const LayerAnaPtListTile({super.key, required this.data});

  Widget _leadingSymbol(IconData icon) {
    return Container(
      alignment: Alignment.centerLeft,
      constraints: BoxConstraints(
        maxWidth: gl.display.equipixel * gl.iconSizeS + 11,
        maxHeight: gl.display.equipixel * gl.iconSizeS + 11,
      ),
      child: Icon(
        icon,
        size: gl.display.equipixel * gl.iconSizeS,
        color: Colors.black,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    double boxheight = 12.0 * gl.display.equipixel;
    double boxwidth = 65.0 * gl.display.equipixel;
    LayerBase layer = gl.dico.getLayerBase(data.mCode);
    return Container(
      constraints: BoxConstraints(
        maxHeight: boxheight * 1.6,
        minHeight: boxheight * 1,
      ),

      child: ListTile(
        leading: switch (layer.mGroupe) {
          "ST" => _leadingSymbol(CustomIcons.mountain),
          "PEUP" => _leadingSymbol(CustomIcons.forest),
          "CS" => _leadingSymbol(CustomIcons.mountains),
          "REF" => _leadingSymbol(CustomIcons.map),
          _ => _leadingSymbol(CustomIcons.soil),
        },
        trailing:
            (layer.getValColor(data.mRastValue).toARGB32() != 4294967295)
                ? CircleAvatar(
                  radius: 10,
                  backgroundColor: layer.getValColor(data.mRastValue),
                )
                : null,
        title: Text(
          layer.mNom,
          style: TextStyle(
            fontSize: gl.display.equipixel * gl.fontSizeS,
            fontWeight: FontWeight.w500,
            color: Colors.black54,
          ),
        ),
        subtitle: Row(
          children: <Widget>[
            SizedBox(
              width: boxwidth,
              height: boxheight,
              child: Text(
                layer.getValLabel(data.mRastValue),
                style: TextStyle(
                  fontSize: gl.display.equipixel * gl.fontSizeS,
                  fontWeight: FontWeight.w500,
                  color: Colors.black,
                ),
              ),
            ),
          ],
        ),
        onTap: () {
          if ((layer.hasDoc() && data.mCode != "CS_A") ||
              (layer.hasDoc() &&
                  data.mCode == "CS_A" &&
                  data.mRastValue < 99)) {
            GoRouter.of(
              context,
            ).push('/${layer.getFicheRoute(us: data.mRastValue)}/0');
          }
        },
      ),
    );
  }
}

Widget _tabAptFEE(BuildContext context, AptsFEE apts) {
  return Column(
    children: [
      Center(
        child: Text(
          "Aptitude du Fichier Ecologique des Essences",
          style: TextStyle(
            fontSize: gl.display.equipixel * gl.fontSizeM,
            fontWeight: FontWeight.w600,
            color: Colors.black,
          ),
          textAlign: TextAlign.center,
        ),
      ),
      DefaultTabController(
        length: 3,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            TabBar(
              tabs: [
                Tab(text: "Optimum"),
                Tab(text: "Tolérance"),
                Tab(text: "Tolérance élargie"),
              ],
            ),
            Container(
              constraints: BoxConstraints(
                maxHeight:
                    max(
                      max(apts.getListEss(1).length, apts.getListEss(2).length),
                      apts.getListEss(3).length,
                    ) *
                    gl.display.equipixel *
                    15,
              ),
              child: TabBarView(
                children: [
                  EssencesListView(apts: apts, codeApt: 1),
                  EssencesListView(apts: apts, codeApt: 2),
                  EssencesListView(apts: apts, codeApt: 3),
                ],
              ),
            ),
          ],
        ),
      ),
    ],
  );
}

class EssencesListView extends StatelessWidget {
  final AptsFEE apts;
  final int codeApt;

  const EssencesListView({
    super.key,
    required this.apts,
    required this.codeApt,
  });

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort(
      (a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR),
    );

    return ListView.builder(
      itemCount: mEss.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index) {
        return ListTile(
          leading: Icon(
            gl.dico.getEss(code.elementAt(index)).mFR == 1
                ? CustomIcons.tree
                : Icons.forest_outlined,
          ),
          title: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
          subtitle:
              codeApt != mEss[code.elementAt(index)]
                  ? Text(gl.dico.aptLabel(mEss[code.elementAt(index)]!))
                  : null,
          trailing:
              apts.mCompensations[code.elementAt(index)]!
                  ? IconButton(
                    icon: const Icon(Icons.balance_rounded),
                    onPressed: () {},
                    tooltip:
                        "La situation topographique provoque un effet de compensation (positif ou négatif) sur l'aptitude de cette essence",
                  )
                  : null,
          onTap: () {
            GoRouter.of(
              context,
            ).push("/${gl.dico.getEss(code.elementAt(index)).getFicheRoute()}");
          },
        );
      },
    );
  }
}

Widget _tabPropositionCS(BuildContext context, PropositionGS apts) {
  double sizeFontTitle = gl.fontSizeL * gl.display.equipixel;
  return Column(
    children: [
      Center(
        child: Text(
          "Propositions d'Essences du Guide des Stations",
          style: TextStyle(
            fontSize: sizeFontTitle,
            fontWeight: FontWeight.w600,
          ),
          textAlign: TextAlign.center,
        ),
      ),
      DefaultTabController(
        length: 4,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            TabBar(
              tabs: [
                Tab(text: gl.dico.vulnerabiliteLabel(1)),
                Tab(text: gl.dico.vulnerabiliteLabel(2)),
                Tab(text: gl.dico.vulnerabiliteLabel(3)),
                Tab(text: gl.dico.vulnerabiliteLabel(4)),
                //  Tab(text: "Déconseillé"),
                //  Tab(text: "Très fortement déconseillé"),
              ],
            ),

            Container(
              constraints: BoxConstraints(
                maxHeight:
                    max(
                      max(apts.getListEss(1).length, apts.getListEss(2).length),
                      apts.getListEss(3).length,
                    ) *
                    gl.display.equipixel *
                    15,
              ),
              child: TabBarView(
                children: [
                  EssencesListViewGS(apts: apts, codeApt: 1),
                  EssencesListViewGS(apts: apts, codeApt: 2),
                  EssencesListViewGS(apts: apts, codeApt: 3),
                  EssencesListViewGS(apts: apts, codeApt: 4),
                  //  EssencesListViewGS(apts: apts, codeApt: 5),
                  //  EssencesListViewGS(apts: apts, codeApt: 6),
                ],
              ),
            ),
          ],
        ),
      ),
    ],
  );
}

class EssencesListViewGS extends StatelessWidget {
  final PropositionGS apts;
  final int codeApt; // maintentant c'est plus un code de vulnérabilités

  const EssencesListViewGS({
    super.key,
    required this.apts,
    required this.codeApt,
  });

  @override
  Widget build(BuildContext context) {
    final Map<String, int> mEss = apts.getListEss(codeApt);
    // tri par ordre alphabétique des essences
    List<String> code = mEss.keys.toList();
    code.sort(
      (a, b) => gl.dico.getEss(a).mNomFR.compareTo(gl.dico.getEss(b).mNomFR),
    );
    return ListView.builder(
      itemCount: mEss.length + 1,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index) {
        if (index == 0) {
          // Add an extra item pour afficher la catégorie de recommandation d'essence
          return ListTile(
            /* leading: CircleAvatar(
              radius: 10,
              backgroundColor: Colors.purple,
            ),*/
            title: Text(gl.dico.vulnerabiliteLabel(codeApt)),
          );
        }
        return ListTile(
          leading: Icon(
            gl.dico.getEss(code.elementAt(index - 1)).mFR == 1
                ? CustomIcons.tree
                : Icons.forest_outlined,
          ),
          title: Text(gl.dico.getEss(code.elementAt(index - 1)).mNomFR),
          onTap: () {
            GoRouter.of(context).push(
              "/${gl.dico.getEss(code.elementAt(index - 1)).getFicheRoute()}",
            );
          },
        );
      },
    );
  }
}

Widget _anaPtListLayers(
  BuildContext context,
  List<LayerAnaPt> requestedLayers,
) {
  if (requestedLayers.isNotEmpty) {
    return ListView.builder(
      itemBuilder: (context, index) {
        return LayerAnaPtListTile(data: requestedLayers[index]);
      },
      itemCount: requestedLayers.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
    );
  } else {
    return Center(child: Text("Aucune information disponible pour ce point"));
  }
}
