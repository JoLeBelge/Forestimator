#include "wopenlayers.h"

WOpenLayers::WOpenLayers(cDicoApt *aDico):xy_(this,"1.0"),mDico(aDico)
{
  resize(640, 480);
  setId("map");//sans ça le script js ne sert à rien car ne vise aucun objet cible
  std::ifstream t(mDico->File("initOL"));
  std::stringstream ss;
  ss << t.rdbuf();
  doJavaScript(ss.str());
  //std::cout << " js Ref " << jsRef() << std::endl;
  setJS_click();

}

// permet de récuper les coodonnées de la carte dans wt lors d'un click dessus + dessine un point là ou l'utilisateur a cliqué
void WOpenLayers::setJS_click(){    
    slot.setJavaScript
           ("function getXY(evt){"
            "var e = window.event;"
            //"console.log('window event '+ e.x + ',' +e.y);"
            "var f = map.get"
            "EventCoordinate(e);"
            // source ; c'est la source pour la couche de point "station", càd celle qui affiche là ou l'utilisateur à double-cliqué
            "source.clear();"
            "source.addFeature(new ol.Feature({geometry: new ol.geom.Point([f[0], f[1]])}));"
           // "var n = f[0];"
           // "console.log('suspence...'+f);"
            "if (f != null) {"
            + xy_.createCall({"f[0]","f[1]"}) +
            "}}"

            );
}
