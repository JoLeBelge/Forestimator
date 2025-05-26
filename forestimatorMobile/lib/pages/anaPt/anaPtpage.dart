import 'package:fforestimator/dico/dicoApt.dart';
import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:fforestimator/pages/anaPt/anaPtpdf.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:fforestimator/myicons.dart';
import 'package:go_router/go_router.dart';
import 'package:fforestimator/tools/notification.dart';

class AnaPtpage extends StatefulWidget {
  final List<layerAnaPt> requestedLayers;
  const AnaPtpage(this.requestedLayers, {super.key});

  @override
  State<AnaPtpage> createState() => _AnaPtpageState();
}

class _AnaPtpageState extends State<AnaPtpage> {
  late TextEditingController controllerPdfName;
  late TextEditingController controllerLocationName;
  @override
  Widget build(BuildContext context) {
    // pour la construction du tableau d'aptitude
    aptsFEE apts = aptsFEE(widget.requestedLayers);
    propositionGS aptsGS = propositionGS(widget.requestedLayers);

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
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
            ),
            IconButton(
              icon: const Icon(Icons.picture_as_pdf, size: 28),
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
                  PopupPDFSaved(context, pdf);
                }
              },
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
  final layerAnaPt data;
  const LayerAnaPtListTile({super.key, required this.data});

  @override
  Widget build(BuildContext context) {
    layerBase l = gl.dico.getLayerBase(data.mCode);
    return ListTile(
      leading: switch (l.mGroupe) {
        "ST" => Icon(CustomIcons.montain),
        "PEUP" => Icon(CustomIcons.forest),
        "CS" => Icon(CustomIcons.mountains),
        "REF" => Icon(Icons.location_on),
        _ => Icon(Icons.location_on),
      },
      title: Text(l.mNom),
      subtitle: Row(
        children: <Widget>[
          Text(
            l.getValLabel(data.mRastValue),
            style: TextStyle(fontSize: 16.0, fontWeight: FontWeight.w500),
          ),
          SizedBox(width: 20),
          if (l.getValColor(data.mRastValue).value != 4294967295)
            CircleAvatar(
              radius: 10,
              backgroundColor: l.getValColor(data.mRastValue),
            ),
        ],
      ),
      onTap: () {
        if (l.hasDoc()) {
          GoRouter.of(context).push(
            '/${gl.basePathbranchB}/${l.getFicheRoute(us: data.mRastValue)}/0',
          );
        }
      },
    );
  }
}

Widget _tabAptFEE(BuildContext context, aptsFEE apts) {
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
            Container(
              child: TabBar(
                tabs: [
                  Tab(text: "Optimum"),
                  Tab(text: "Tolérance"),
                  Tab(text: "Tolérance élargie"),
                ],
              ),
            ),
            Container(
              //Add this to give height
              height: MediaQuery.of(context).size.height,
              child: TabBarView(
                children: [
                  essencesListView(apts: apts, codeApt: 1),
                  essencesListView(apts: apts, codeApt: 2),
                  essencesListView(apts: apts, codeApt: 3),
                ],
              ),
            ),
          ],
        ),
      ),
    ],
  );
}

class essencesListView extends StatelessWidget {
  final aptsFEE apts;
  final int codeApt;
  essencesListView({required this.apts, required this.codeApt});
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
            gl.dico.getEss(code.elementAt(index)).mF_R == 1
                ? CustomIcons.tree
                : Icons.forest_outlined,
          ),
          title: Text(gl.dico.getEss(code.elementAt(index)).mNomFR),
          subtitle:
              codeApt != mEss[code.elementAt(index)]
                  ? Text(gl.dico.AptLabel(mEss[code.elementAt(index)]!))
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
            GoRouter.of(context).push(
              "/${gl.basePathbranchB}/${gl.dico.getEss(code.elementAt(index)).getFicheRoute()}",
            );
          },
        );
      },
    );
  }
}

Widget _tabPropositionCS(BuildContext context, propositionGS apts) {
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
            Container(
              child: TabBar(
                tabs: [
                  Tab(text: gl.dico.vulnerabiliteLabel(1)),
                  Tab(text: gl.dico.vulnerabiliteLabel(2)),
                  Tab(text: gl.dico.vulnerabiliteLabel(3)),
                  Tab(text: gl.dico.vulnerabiliteLabel(4)),
                  //  Tab(text: "Déconseillé"),
                  //  Tab(text: "Très fortement déconseillé"),
                ],
              ),
            ),
            Container(
              //Add this to give height
              height: MediaQuery.of(context).size.height,
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
  final propositionGS apts;
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
            gl.dico.getEss(code.elementAt(index - 1)).mF_R == 1
                ? CustomIcons.tree
                : Icons.forest_outlined,
          ),
          title: Text(gl.dico.getEss(code.elementAt(index - 1)).mNomFR),
        );
      },
    );
  }
}

Widget _anaPtListLayers(
  BuildContext context,
  List<layerAnaPt> requestedLayers,
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
