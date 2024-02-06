import 'package:fforestimator/dico/dicoApt.dart';
import "package:flutter/material.dart";
import 'package:fforestimator/pages/anaPt/requestedLayer.dart';
import 'package:fforestimator/globals.dart' as gl;
import 'package:flutter_map/flutter_map.dart';

class anaPtpage extends StatefulWidget {
  List<layerAnaPt> requestedLayers = [];
  @override
  anaPtpage(this.requestedLayers);
  State<anaPtpage> createState() => _anaPtpageState();
}

class _anaPtpageState extends State<anaPtpage> {
  @override
  Widget build(BuildContext context) {
    int nb = 0;
    for (layerAnaPt l in widget.requestedLayers) {
      if (l.mFoundRastFile && l.mRastValue != 0) {
        nb = nb + 1;
      }
    }
    // pour la construction du tableau d'aptitude
    aptsFEE apts = aptsFEE(widget.requestedLayers);

    return Scaffold(
        backgroundColor: Colors.grey[200],
        appBar: AppBar(
          title: Text("analyse pour cette position"),
        ),
        body: SingleChildScrollView(
            physics: ScrollPhysics(),
            child: Column(children: [
              nb > 0
                  ? ListView.builder(
                      itemBuilder: (context, index) {
                        if (widget.requestedLayers[index].mFoundRastFile &&
                            widget.requestedLayers[index].mRastValue != 0) {
                          // si la valeur est 0 c'est du No Data -> on choisit de ne rien afficher
                          return layerAnaPtListTile(
                              data: widget.requestedLayers[index]);
                        }
                      },
                      itemCount: widget.requestedLayers.length,
                      shrinkWrap: true,
                      physics: NeverScrollableScrollPhysics(),
                    )
                  : Center(
                      child:
                          Text("aucune information disponible pour ce point"),
                    ),
              apts.ready ? _tabAptFEE(context, apts) : Container()
            ])));
  }
}

class layerAnaPtListTile extends StatelessWidget {
  final layerAnaPt data;
  layerAnaPtListTile({required this.data});

  @override
  Widget build(BuildContext context) {
    layerBase l = gl.dico.getLayerBase(data.mCode);
    return ListTile(
      leading: Container(
        child: CircleAvatar(
          radius: 25,
          backgroundColor: l.getValColor(data.mRastValue),
        ),
      ),
      title: Text(l.mNom),
      subtitle: Text(l.getValLabel(data.mRastValue)),
    );
  }
}

Widget _tabAptFEE(BuildContext context, aptsFEE apts) {
  return Column(children: [
    Container(
        color: Colors.amber,
        child: Text("Matrice d'aptitude du Fichier Ecologique des Essences")),
    DefaultTabController(
      length: 3,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: <Widget>[
          Container(
            child: TabBar(tabs: [
              Tab(text: "Optitum"),
              Tab(text: "Tolérance"),
              Tab(text: "Tolérance élargie"),
            ]),
          ),
          Container(
            //Add this to give height
            height: MediaQuery.of(context).size.height,
            child: TabBarView(children: [
              essencesListView(mEss: apts.getListEss(1)),
              essencesListView(mEss: apts.getListEss(2)),
              essencesListView(mEss: apts.getListEss(3)),
            ]),
          ),
        ],
      ),
    )
  ]);
}

class essencesListView extends StatelessWidget {
  final Map<String, int> mEss;
  essencesListView({required this.mEss});
  @override
  Widget build(BuildContext context) {
    return ListView.builder(
      itemCount: mEss.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index) {
        return ListTile(
          leading: Image.asset("assets/images/icon" +
              (gl.dico.getEss(mEss.keys.elementAt(index)).mF_R == 1
                  ? "Deciduous.png"
                  : "Conifer.png")),
          title: Text(gl.dico.getEss(mEss.keys.elementAt(index)).mNomFR),
        );
      },
    );
  }
}
