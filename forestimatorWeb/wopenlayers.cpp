#include "wopenlayers.h"

WOpenLayers::WOpenLayers(cDicoApt *aDico) : xy_(this, "1.0"), mDico(aDico), slot(this), slot2(this), xySelect_(this, "2.0") //,polygId_(this,"1")
{
    setPadding(0);
    setMargin(0);
    // pour que layoutSizeChange fonctionne
    setLayoutSizeAware(1);

    setId("map"); // sans ça le script js ne sert à rien car ne vise aucun objet cible

    if (!exists(mDico->File("initOL")))
    {
        std::cout << "\n\n\n pas trouvé initOL.js (" << mDico->File("initOL") << " ), ça va planter!!! \n\n\n"
                  << std::endl;
    }
    else
    {
        std::ifstream t(mDico->File("initOL"));
        std::stringstream ss;
        ss << t.rdbuf();
        t.close();
        doJavaScript(ss.str());
        setToolTip(tr("tooltipMap1"));

        // slots

        // permet de récuper les coodonnées de la carte dans wt lors d'un click dessus + dessine un point là ou l'utilisateur a cliqué

        slot.setJavaScript("function getXY(owt,evt){"
                           "var e =  evt || window.event;"
                           "var f = map.getEventCoordinate(e);"
                           // source ; c'est la source pour la couche de point "station", càd celle qui affiche là ou l'utilisateur à double-cliqué
                           "source.clear();"
                           "source.addFeature(new ol.Feature({geometry: new ol.geom.Point([f[0], f[1]])}));"
                           "if (f != null) {" +
                           xy_.createCall({"f[0]", "f[1]"}) +
                           "}}");

        /* slot2.setJavaScript
                 ("function () {if (featuresSelect.getLength() > 0) {if (featuresSelect.item(0) !== 'undefined') {"
                  "if (featuresSelect.item(0).getId() !== null) {"+ polygId_.createCall({"featuresSelect.item(0).getId()"}) +
                  "console.log(featuresSelect.item(0).getId());"
                  "}"
                  "}}};"
                  );*/

        slot2.setJavaScript("function (x,y){"
                            "var f = map.getCoordinateFromPixel([x,y]);"
                            "if (f != null) {" +
                            xySelect_.createCall({"f[0]", "f[1]"}) +
                            "}}");

        slot3.setJavaScript("function projection(x,y){"
                            "var f = map.getCoordinateFromPixel([x,y]);"
                            "source.addFeature(new ol.Feature({geometry: new ol.geom.Point([f[0], f[1]])}));"
                            // source ; c'est la source pour la couche de point "station", càd celle qui affiche là ou l'utilisateur à double-cliqué
                            "source.clear();"
                            "source.addFeature(new ol.Feature({geometry: new ol.geom.Point([f[0], f[1]])}));"
                            "if (f != null) {" +
                            xy_.createCall({"f[0]", "f[1]"}) +
                            "}}");

        // actions

        this->doubleClicked().connect(this->slot);
        // click sur un polygone
        this->clicked().connect(std::bind(&WOpenLayers::filterMouseEvent, this, std::placeholders::_1));

        // mesure du temps entre touchstart et touchend pour savoir si c'est un longpress
        touchStarted().connect(this, &WOpenLayers::TouchStart);
        touchMoved().connect(this, &WOpenLayers::TouchMoved);
        touchEnded().connect(std::bind(&WOpenLayers::TouchEnd, this, std::placeholders::_1));

        // prevent default actions
        touchStarted().preventDefaultAction(true);
        touchMoved().preventDefaultAction(true);
        touchEnded().preventDefaultAction(true);
        // openfileBug
        t.close();
    }

    // le popup pour afficher la valeur de la couche en cours d'affichage
    Wt::WContainerWidget *popup = addNew<Wt::WContainerWidget>();
    popup->setId("popup");
    popup->setStyleClass("ol-popup");
    Wt::WLink link = Wt::WLink(""); // sert à rien en fait
    Wt::WContainerWidget *popupCloser = popup->addNew<Wt::WAnchor>(link);
    popupCloser->setId("popup-closer");
    popupCloser->setStyleClass("ol-popup-closer");
    Wt::WContainerWidget *popupContent = popup->addNew<Wt::WContainerWidget>();
    popupContent->setId("popup-content");
}
