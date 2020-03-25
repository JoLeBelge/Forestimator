#include "wopenlayers.h"

WOpenLayers::WOpenLayers(WContainerWidget *parent):xy_(this,"1.0")
{
  resize(640, 480);
  setId("map");//sans ça le script js ne sert à rien car ne vise aucun objet cible
  //std::ifstream t("/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/cWebAptitude/data/tile.js");
  std::ifstream t("/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/cWebAptitude/data/Test1.js");
  std::stringstream ss;
  ss << t.rdbuf();
  doJavaScript(ss.str());
  //std::cout << " js Ref " << jsRef() << std::endl;

  setJS_click();
}

void WOpenLayers::addAptMap(std::string aCodeEs, std::string aCodeClassifST){
    std::stringstream ss;
    std::string aFileIn("/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/cWebAptitude/data/add_layer.js");
    std::ifstream in(aFileIn);
    std::string aTmp(aFileIn+".tmp");
    std::ofstream out(aTmp);
    // remplace l'url des tuiles par celui de l'essence actuelle:

    std::string aFind("CODE_ES");
    std::string aFind2("CLASSIF_ST");
    std::string line;
    //size_t len = aFind.length();
    while (getline(in, line))
    {
        boost::replace_all(line,aFind,aCodeEs);
        boost::replace_all(line,aFind2,aCodeClassifST);
        out << line << "\n";
    }
    in.close();
    out.close();

    in.open(aFileIn+".tmp");
    ss << in.rdbuf();
    doJavaScript(ss.str());


}

// c'est chiant car avec la couches des communes qui dois être tout le temps visible, la gestion de l'affichage des couches devient un peu complexe.

void WOpenLayers::displayMap(std::string aNomCarteJS, bool isVisible){
    //std::stringstream ss;
    std::string aBool("true");
    if (!isVisible){aBool="false";}
    // fonctionne pas, devrait pour vraiment fonctionner mettre à false les setVisible des autres couches
    //std::string JScommand (aNomCarteJS+".setVisible("+aBool+");");

    std::string JScommand("groupe = new ol.layer.Group({layers:[IGN, communes]});IGN.setVisible(true);map.setLayerGroup(groupe);");
    doJavaScript(JScommand);
}
void WOpenLayers::setJS_click(){
    //slot.setJavaScript("console.log('toto\ntoto\ntoto');");
    slot.setJavaScript
           ("function getXY(evt){"
            "var e = window.event;"
            "console.log('window event '+ e.x + ',' +e.y);"
            "var f = map.getEventCoordinate(e);"
           // "var n = f[0];"
            "console.log('suspence...'+f);"
            "if (f != null) {"
            + xy_.createCall({"f[0]","f[1]"}) +
            "}}"
            );
}
