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

#include <Wt/WApplication.h>
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
#include <sys/stat.h>

/* attention, je n'ai jamais réussi à paramètrer deux docroot donc je dois tout mettre dans un seul et unique!
 *
 * ./WebAptitude --http-address=0.0.0.0 --http-port=8085 --deploy-path=/WebAptitude --docroot="./" --config="/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude/wt_config.xml"
 * Current arg :
 * ./WebAptitude --deploy-path=/ --docroot "/data1/Forestimator/build-WebAptitude;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
 * sudo ./WebAptitude --deploy-path=/ --docroot "/home/sam/master_chatmetaleux/Forestimator/data;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
 * ./WebAptitude --deploy-path=/ --docroot "/home/lisein/Documents/carteApt/Forestimator/data/;/favicon.ico,/js,/jslib,/resources,/style,/tmp,/data,/Tuiles" --http-port 8085 --http-addr 0.0.0.0
*/


using namespace std;


std::unique_ptr<WApplication> createApplication(const WEnvironment& env)
{
    printf("createApplication\n");

    std::unique_ptr<WApplication> app = cpp14::make_unique<WApplication>(env);
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    //app->appRoot()std::cout << "app->docRoot() " << app->docRoot() << std::endl;
    app->messageResourceBundle().use(app->docRoot() + "/forestimator");
    app->setTitle("Forestimator");
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
    app->useStyleSheet("style/form.css");
    app->useStyleSheet("resources/themes/polished/wt.css");
    //app->useStyleSheet("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/css/ol.css");

    WCssDecorationStyle EssStyle;
    EssStyle.font().setSize(FontSize::Medium);
    EssStyle.setCursor(Cursor::PointingHand);
    EssStyle.setForegroundColor(WColor("gray"));
    app->styleSheet().addRule(".ess", EssStyle);

    EssStyle.font().setSize(FontSize::Smaller);
    app->styleSheet().addRule(".tree", EssStyle);
    EssStyle.font().setSize(FontSize::Medium);

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
    // avec ol 6, la carte met du temps à s'afficher. je reste à ol 4 pour le moment.
    //std::string ol("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/build/ol.js");
    //std::string ol("jslib/v6.3.1-dist/ol.js");
    std::string ol("jslib/ol4.6.4-debug.js");
    app->require(ol);
    app->useStyleSheet("jslib/v6.3.1-dist/ol.css");
    app->require("jslib/proj4js-2.6.1/dist/proj4.js");
    app->require("jslib/proj4js-2.6.1/dist/proj4-src.js");
	// CSS custom pour faire beau
	app->useStyleSheet("style/style.css");

    app->enableUpdates();

    auto layout = app->root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    std::unique_ptr<cWebAptitude> WebApt = Wt::cpp14::make_unique<cWebAptitude>(app.get());
    layout->addWidget(std::move(WebApt), 0);
    //layout->setContentsMargins(0,0,0,0);
    //app->root()->setPadding(0);
    app->root()->addStyleClass("layout_main");

    return app;
}


int main(int argc, char **argv)
{
    return WRun(argc, argv, &createApplication);
}

