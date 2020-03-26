#include "wopenlayers.h"

WOpenLayers::WOpenLayers(WContainerWidget *parent, cDicoApt *aDico):xy_(this,"1.0"),mDico(aDico)
{
  resize(640, 480);
  setId("map");//sans ça le script js ne sert à rien car ne vise aucun objet cible
  std::ifstream t(mDico->Files()->at("initOL"));
  std::stringstream ss;
  ss << t.rdbuf();
  doJavaScript(ss.str());
  //std::cout << " js Ref " << jsRef() << std::endl;
  setJS_click();
}

// permet de récuper les coodonnées de la carte dans wt lors d'un click dessus
void WOpenLayers::setJS_click(){    
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
