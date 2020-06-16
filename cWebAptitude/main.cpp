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
#include <Wt/WCheckBox.h>
#include <Wt/WLeafletMap.h>
#include <sys/stat.h>

/* attention, je n'ai jamais réussi à paramètrer deux docroot donc je dois tout mettre dans un seul et unique!
 *
 * ./WebAptitude --http-address=0.0.0.0 --http-port=8085 --deploy-path=/WebAptitude --docroot="./" --config="/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude/wt_config.xml"
 * Current arg :
 * ./WebAptitude --deploy-path=/ --docroot "/data1/Forestimator/build-WebAptitude;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
 * ./WebAptitude --deploy-path=/ --docroot "/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
*/

cWebAptitude * _webapt;

using namespace std;



/*
 * Redirections en fonction du internal path
 *
 */
void handlePathChange(Wt::WStackedWidget *stack)
{
    Wt::WApplication *app = Wt::WApplication::instance();
    printf("path change\n");
    // TODO corriger les affichages
    if (app->internalPath() == "/presentation")
		std::cout << "presentation " << std::endl;
    else if (app->internalPath() == "/cartographie"){
		stack->setCurrentIndex(1);
		_webapt->stack_info->setCurrentIndex(2);
		_webapt->menuitem2_analyse->setHidden(true);	
	}else if (app->internalPath() == "/analyse"){
		stack->setCurrentIndex(1);
		_webapt->stack_info->setCurrentIndex(1);
		_webapt->menuitem2_analyse->setHidden(false);	
		_webapt->menuitem2_analyse->select();
    }else
		stack->setCurrentIndex(0);
}


std::unique_ptr<WApplication> createApplication(const WEnvironment& env)
{
    printf("createApplication\n");

    std::unique_ptr<WApplication> app = cpp14::make_unique<WApplication>(env);
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    //app->appRoot()std::cout << "app->docRoot() " << app->docRoot() << std::endl;
    app->messageResourceBundle().use(app->docRoot() + "/data/forestimator");
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
    //std::string ol("data/js/v6.3.1-dist/ol.js");
    std::string ol("data/js/ol4.6.4-debug.js");
    app->require(ol);
    app->useStyleSheet("data/js/v6.3.1-dist/ol.css");
    app->require("data/js/proj4js-2.6.1/dist/proj4.js");
    app->require("data/js/proj4js-2.6.1/dist/proj4-src.js");
	// CSS custom pour faire beau
	app->useStyleSheet("style/style.css");

    
	auto layout = app->root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
	std::cout << "s1 " << std::endl;
	
	Wt::WStackedWidget *stack  = layout->addWidget(Wt::cpp14::make_unique<Wt::WStackedWidget>());
	
    std::unique_ptr<cWebAptitude> WebApt = Wt::cpp14::make_unique<cWebAptitude>(app.get());
    //_webapt = new cWebAptitude(app.get()); // TODO dangling :)
    _webapt = WebApt.get();

	/*	HOME PAGE	*/
	auto container_home = Wt::cpp14::make_unique<Wt::WContainerWidget>();
	container_home->setStyleClass("home_div");
	container_home->setOverflow(Wt::Overflow::Auto);
	container_home->setMargin(Wt::WLength::Auto);
	container_home->setMargin(20,Wt::Side::Top);
	container_home->addNew<Wt::WText>("<h1 style='color:white;height: 37px;font-size: 3em;max-width: 800px;text-align: center;'>Forestimator</h1>");
	container_home->decorationStyle().setBackgroundImage(Wt::WLink("resources/bg.png"), None);
	// add menu as push buttons!
	Wt::WPushButton *b_pres  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Présentation"));
	container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
	Wt::WPushButton *b_carto = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Cartographie"));
	container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
	Wt::WPushButton *b_anal  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Analyse"));
	container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
	Wt::WPushButton *b_simu  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Simulation"));
	container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
	b_pres->setStyleClass("btn btn-success");
	b_carto->setStyleClass("btn btn-success");
	b_anal->setStyleClass("btn btn-success");
	b_simu->setStyleClass("btn btn-success");
	b_pres ->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/presentation"));
	b_carto->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/cartographie"));
	b_anal ->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/analyse"));
	b_simu ->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/simulation"));
	
	WContainerWidget *c_img1 = container_home->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
	Wt::WImage *img1 = c_img1->addNew<Wt::WImage>(Wt::WLink("resources/uliege.png"));
	img1->setAlternateText("ULiege logo");
	c_img1->addStyleClass("logo_left");

	WContainerWidget *c_img2 = container_home->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
	Wt::WImage *img2 = c_img2->addNew<Wt::WImage>(Wt::WLink("resources/fpb.png"));
	c_img2->addStyleClass("logo_right");
	/*	FIN HOME PAGE	*/	
	

	stack->addWidget(std::move(container_home));
    stack->addWidget(std::move(WebApt));
	stack->setCurrentIndex(0);
	
    app->enableUpdates();
    
    app->internalPathChanged().connect([=] {
    	handlePathChange(stack);
	});
    handlePathChange(stack); // force first route

    return app;
}


int main(int argc, char **argv)
{
    return WRun(argc, argv, &createApplication);
}

