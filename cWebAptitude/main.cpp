/*
 * Copyright (C) 2008 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>
#include <stdio.h>
#include <sqlite3.h>
#include <algorithm>
#include "cwebaptitude.h"


#include <Wt/WContainerWidget.h>
#include <Wt/WMenuItem.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPopupMenuItem.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WEnvironment.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WToolBar.h>
#include <Wt/WTemplate.h>
#include <Wt/WString.h>
#include <Wt/WCheckBox.h>
#include <Wt/WLeafletMap.h>
#include <sys/stat.h>

// attention, je n'ai jamais réussi à paramètrer deux docroot donc je dois tout mettre dans un seul et unique!
// ./WebAptitude --http-address=0.0.0.0 --http-port=8085 --deploy-path=/WebAptitude --docroot="/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/build-WebAptitude/"

using namespace std;

std::unique_ptr<WApplication> createApplication(const WEnvironment& env)
{
    std::unique_ptr<WApplication> app
            = cpp14::make_unique<WApplication>(env);
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    app->messageResourceBundle().use(WApplication::appRoot() + "./data/WebAptitude");
    app->setTitle("Aptitude");
    // thème bootstrap 3
    auto theme = std::make_shared<Wt::WBootstrapTheme>();
    theme->setVersion(Wt::BootstrapVersion::v3);
    //spécifier ça également dans wt_option.xml sinon ne fonctionne pas
    theme->setResponsive(true);
    app->setTheme(theme);
    // load the default bootstrap3 (sub-)theme (nécessaire en plus de theme->setVersion)
    app->useStyleSheet("style/bootstrap-theme.min.css");
    // tout les style de wt gallery
    app->useStyleSheet("style/everywidget.css");
    app->useStyleSheet("style/dragdrop.css");
    app->useStyleSheet("style/combostyle.css");
    app->useStyleSheet("style/pygments.css");
    //app->useStyleSheet("style/layout.css");
    app->useStyleSheet("style/filedrop.css");
    //app->useStyleSheet("style/home.css");
    app->useStyleSheet("style/wt.css");
    app-> useStyleSheet("style/form.css");
    app-> useStyleSheet("resources/themes/polished/wt.css");
    app->useStyleSheet("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/css/ol.css");
    app->enableUpdates();

    WCssDecorationStyle EssStyle;
    EssStyle.font().setSize(FontSize::Medium);
    EssStyle.setCursor(Cursor::PointingHand);
    EssStyle.setForegroundColor(WColor("gray"));
    app->styleSheet().addRule(".ess", EssStyle);

    EssStyle.setForegroundColor(WColor("black"));
    EssStyle.setTextDecoration(TextDecoration::Underline);
    EssStyle.setCursor(Cursor::Arrow);
    EssStyle.font().setWeight(FontWeight::Bold);
    app->styleSheet().addRule(".currentEss", EssStyle);

    WCssDecorationStyle BoldStyle;
    BoldStyle.font().setSize(FontSize::XSmall);
    BoldStyle.setForegroundColor(WColor("black"));
    BoldStyle.font().setWeight(FontWeight::Bold);
    app->styleSheet().addRule(".bold", BoldStyle);

    // init the OpenLayers javascript api
    //app->require("http://www.openlayers.org/api/OpenLayers.js");

    //std::string ol("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/build/ol.js");
    //app->require(ol);
    app->require("https://cdnjs.cloudflare.com/ajax/libs/proj4js/2.6.0/proj4.js");
    app->require("https://cdnjs.cloudflare.com/ajax/libs/proj4js/2.6.0/proj4-src.js");

    // il faut compiler
    //app->require("https://github.com/geotiffjs/geotiff.js");
    app->require("/geotiff.bundle.js");

    // finalement je n'utilise pas olGeoTiff
    // app->require("https://data.eox.at/geotiff.js-blog/04_multiband/olGeoTiff_07.js");
    // app->require("https://data.eox.at/geotiff.js-blog/04_multiband/dist/plotty.min.js");
    app->require("https://data.eox.at/geotiff.js-blog/04_multiband/dist/nouislider.js");
    app->require("https://data.eox.at/geotiff.js-blog/04_multiband/dist/geotiffjs/geotiff.browserify.js");
    app->require("https://data.eox.at/geotiff.js-blog/04_multiband/dist/ol-debug.js");

    //app->require("https://github.com/walkermatt/ol-layerswitcher/tree/master/src/ol-layerswitcher.js");

    auto layout = app->root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    std::unique_ptr<cWebAptitude> WebApt = Wt::cpp14::make_unique<cWebAptitude>(app.get());

    // checkbox
    //std::unique_ptr<WContainerWidget> cont1 = Wt::cpp14::make_unique<WContainerWidget>();

    //WCheckBox *checkBox =cont1->addWidget(cpp14::make_unique<WCheckBox>());
    //WOpenLayers * MyMap=WebApt->mMap ;
    //checkBox->changed().connect([=]{MyMap->addAptMap();});
    layout->addWidget(std::move(WebApt), 0);
    //layout->addWidget(std::move(cont1), 0);
    //Wt::WLeafletMap aLM;
    //aLM.panTo();

    return app;
}

int main(int argc, char **argv)
{
    return WRun(argc, argv, &createApplication);
}

