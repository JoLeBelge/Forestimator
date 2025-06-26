import 'package:fforestimator/dico/dico_apt.dart';
import 'package:fforestimator/tools/handle_permissions.dart';
import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requested_layer.dart';
import 'package:fforestimator/pages/anaPt/ana_ptpdf.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/myicons.dart';
import 'package:go_router/go_router.dart';
import 'package:fforestimator/tools/notification.dart';

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

    return Scaffold(
      backgroundColor: Colors.grey[200],
      appBar: AppBar(title: Text("Analyse pour cette position")),
      body: SingleChildScrollView(
        physics: ScrollPhysics(),
        child: Column(
          children: [
            Text(
              gl.offlineMode
                  ? "Analyse réalisée hors-ligne"
                  : "Analyse réalisée en ligne",
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
            ),
            handlePermissionForStorage(
              child: TextButton(
                child:
                    getStorage()
                        ? Row(
                          mainAxisAlignment: MainAxisAlignment.start,
                          children: [
                            Container(
                              alignment: Alignment.center,
                              constraints: BoxConstraints(
                                maxWidth:
                                    MediaQuery.of(context).size.width * 0.075,
                              ),
                              child: Icon(
                                Icons.picture_as_pdf,
                                size: 28,
                                color: Colors.black,
                              ),
                            ),

                            Container(
                              alignment: Alignment.center,
                              constraints: BoxConstraints(
                                maxWidth:
                                    MediaQuery.of(context).size.width * 0.75,
                              ),
                              child: Text(
                                "Saufgardez cette analyse sur votre appareil",
                                style: TextStyle(color: Colors.black),
                              ),
                            ),
                          ],
                        )
                        : Row(),
                onPressed: () async {
                  //var deviceInfo = await DeviceInfoPlugin().androidInfo;
                  // Your app has write permission by default in all public directories on external storage. (Android 13+ )-> request retourne denied sans boite de dialogue
                  bool isPermitted = true;
                  //(Platform.isAndroid && deviceInfo.version.sdkInt > 32)
                  //  ? true
                  //  : await Permission.storage.request().isGranted;
                  if (isPermitted) {
                    //await Permission.manageExternalStorage
                    //   .request()
                    //  .isGranted ||
                    //await Permission.storage.request().isGranted) {
                    List<String>? l = await openDialog();
                    String? pdf = l?.elementAt(0);
                    String? locationName = l?.elementAt(1);
                    if (pdf!.isEmpty) {
                      pdf = "analysePonctuelleForestimator.pdf";
                    }
                    if (pdf.length < 4 ||
                        pdf.substring(pdf.length - 4) != ".pdf") {
                      pdf = "$pdf.pdf";
                    }
                    if (locationName!.isEmpty) {
                      locationName = "une position";
                    }
                    // création du pdf
                    //String? dir = await getDownloadDirectory().toString();
                    // String dir =
                    //  await getExternalStorageDirectories().toString();
                    String dir = "/storage/emulated/0/Download/";
                    makePdf(widget.requestedLayers, pdf, dir, locationName);
                    // confirmation que le pdf a été créé
                    PopupPDFSaved(gl.anaPtPageContext!, pdf);
                  }
                },
              ),
              refreshParentWidgetTree: setState,
            ),

            _anaPtListLayers(context, widget.requestedLayers),
            SizedBox(height: 15),
            if (apts.ready) Card(child: _tabAptFEE(context, apts)),
            SizedBox(height: 15),
            if (aptsGS.ready) Card(child: _tabPropositionCS(context, aptsGS)),
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

  Widget _leadingSymbol(var icon) {
    return Container(
      alignment: Alignment.center,
      constraints: BoxConstraints(
        maxWidth: MediaQuery.of(gl.notificationContext!).size.width * 0.075,
      ),
      child: Icon(
        icon,
        size: MediaQuery.of(gl.notificationContext!).size.width * 0.075,
        color: Colors.black,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    LayerBase layer = gl.dico.getLayerBase(data.mCode);
    return ListTile(
      leading: switch (layer.mGroupe) {
        "ST" => _leadingSymbol(CustomIcons.montain),
        "PEUP" => _leadingSymbol(CustomIcons.forest),
        "CS" => _leadingSymbol(CustomIcons.mountains),
        "REF" => _leadingSymbol(Icons.location_on),
        _ => _leadingSymbol(Icons.location_on),
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
          fontSize: 16.0,
          fontWeight: FontWeight.w500,
          color: Colors.black,
        ),
      ),
      subtitle: Row(
        children: <Widget>[
          SizedBox(
            width: MediaQuery.of(gl.notificationContext!).size.width * 0.6,
            height: MediaQuery.of(gl.notificationContext!).size.width * 0.1,
            child: Text(
              layer.getValLabel(data.mRastValue),
              style: TextStyle(fontSize: 16.0, fontWeight: FontWeight.w500),
            ),
          ),
        ],
      ),
      onTap: () {
        if ((layer.hasDoc() && data.mCode != "CS_A") ||
            (layer.hasDoc() && data.mCode == "CS_A" && data.mRastValue < 99)) {
          GoRouter.of(
            context,
          ).push('/${layer.getFicheRoute(us: data.mRastValue)}/0');
        }
      },
    );
  }
}

Widget _tabAptFEE(BuildContext context, AptsFEE apts) {
  return Column(
    children: [
      SizedBox(height: 10),
      Center(
        child: Text(
          "Aptitude du Fichier Ecologique des Essences",
          style: TextStyle(fontSize: 18.0, fontWeight: FontWeight.w600),
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
                maxHeight: MediaQuery.of(context).size.height,
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
  return Column(
    children: [
      SizedBox(height: 10),
      Center(
        child: Text(
          "Propositions d'Essences du Guide des Stations",
          style: TextStyle(fontSize: 18.0, fontWeight: FontWeight.w600),
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
                maxHeight: MediaQuery.of(context).size.height,
              ),
              //Add this to give height
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
  /* plus nécessaire car requestedLayers est purgé avant l'envoi à AnaPtpage
  int nb = 0;
  for (layerAnaPt l in requestedLayers) {
    if (l.mFoundRastFile && l.mRastValue != 0) {
      nb = nb + 1;
    }
  }*/

  if (requestedLayers.isNotEmpty) {
    return ListView.builder(
      itemBuilder: (context, index) {
        //if (requestedLayers[index].mFoundRastFile &&
        //  requestedLayers[index].mRastValue != 0) {

        return LayerAnaPtListTile(data: requestedLayers[index]);
        //}
      },
      itemCount: requestedLayers.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
    );
  } else {
    return Center(child: Text("Aucune information disponible pour ce point"));
  }
}
