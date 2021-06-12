#pragma once
#include "cwebaptitude.h"
//#include "auth.h"

cWebAptitude::cWebAptitude(AuthApplication *app, Auth::AuthWidget* authWidget_)
    : WContainerWidget(),authWidget(authWidget_),mDico(app->getDico())
{
    std::cout << "cWebApt\n" << std::endl;
    GDALAllRegister();
    m_app = app;
    m_app->setLoadingIndicator(cpp14::make_unique<Wt::WOverlayLoadingIndicator>());
    m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));

    for (auto kv : mDico->colors){
        color col= kv.second;
        WCssDecorationStyle styleBgrd;
        styleBgrd.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
        if (col.dark()){styleBgrd.setForegroundColor(WColor("white"));}
        app->styleSheet().addRule(col.getStyleName(), styleBgrd);
        //std::cout << col.getStyleName() << "{" << styleBgrd.cssText() << "}" <<  std::endl;;
    }

    addStyleClass("cWebAptitude");
    setMargin(0);
    setPadding(0);

    /*	NAVIGATION BAR	*/
    navigation = this->addWidget(cpp14::make_unique<WNavigationBar>());
    navigation->setResponsive(true);
    navigation->addStyleClass("carto_menu");

    std::unique_ptr<WMenu> menu_ = cpp14::make_unique<WMenu>();
    WMenu * navbar_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);

    // menu app
    menuitem_app = navbar_menu->addItem("resources/layers_filled_icon_149920.png","");
    menuitem_app->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    menuitem_app->setToolTip(tr("menu.button.tooltip.carto"));
    // menu doc
    menuitem_documentation = navbar_menu->addItem("resources/problem_analysis_icon_149897.png","");
    menuitem_documentation->setLink(WLink(LinkType::InternalPath, "/documentation"));
    menuitem_documentation->setToolTip(tr("menu.button.tooltip.doc"));
    // menu login
    menuitem_login = navbar_menu->addItem("resources/user_icon_149851.png","");
    menuitem_login->clicked().connect([=] {
        if(m_app->isLoggedIn()){
            menuitem_login->setIcon("resources/user_icon_149851.png");
            m_app->logout();
        }else{
            //authWidget->addStyleClass("visible"); // ne fonctionne qu'une fois ! bizarre...workaround avec injection de JS direct OK !
            //authWidget->removeStyleClass("nonvisible");
            std::string JScommand("$('.Wt-auth-login-container').removeClass('nonvisible').addClass('visible');");
            m_app->doJavaScript(JScommand);
        }
    });
    menuitem_login->setToolTip(tr("menu.button.tooltip.login"));

    // pas encore implémenté
    /*navbar_menu->addItem("resources/configuration_center_icon_149956.png","")
            ->setLink(WLink(LinkType::InternalPath, "/parametres"));
    */


    // main stack : DOC and MAP and RESULTS divs
    top_stack  = this->addWidget(Wt::cpp14::make_unique<Wt::WStackedWidget>());
    top_stack->setMargin(0);
    top_stack->setHeight("100%");
    // load DOC page
    top_stack->addNew<presentationPage>(mDico);
    // load MAP page
    std::unique_ptr<WContainerWidget> content_app = cpp14::make_unique<WContainerWidget>();
    content_app->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    content_app->addStyleClass("carto_div");
    WHBoxLayout * layout_app = content_app->setLayout(cpp14::make_unique<WHBoxLayout>());
    layout_app->setContentsMargins(0,0,0,0);
    content_app->setHeight("100%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    content_app->setOverflow(Overflow::Visible); // non pas d'overflow pour la carte, qui est dans page_carto
    top_stack->addWidget(std::move(content_app));
    top_stack->setCurrentIndex(1);


    /*** page principale application map ***/

    /*	MAP	*/
    std::cout << "create map" << std::endl;
    auto map = cpp14::make_unique<WOpenLayers>(mDico);
    mMap = map.get();
    mMap->setWidth("100%");
    mMap->setMinimumSize(400,0);
    mMap->setOverflow(Overflow::Visible);
    layout_app->addWidget(std::move(map), 0);

    /*  Panel droit avec boutons et couches selectionnees */
    auto content_couches = layout_app->addWidget(cpp14::make_unique<WContainerWidget>());
    content_couches->setId("content_couches");
    content_couches->setOverflow(Overflow::Auto);
    content_couches->addStyleClass("content_couches");
    load_content_couches(content_couches);

    /*  DIALOGS info_point-légende-analyse */
    dialog_info = layout_app->addChild(Wt::cpp14::make_unique<Wt::WDialog>("info"));
    dialog_info->setResizable(true);
    dialog_info->setModal(false);
    dialog_info->setMaximumSize(1000,1000);

    auto content_info = dialog_info->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    content_info->setOverflow(Overflow::Auto);
    content_info->addStyleClass("content_info");

    dialog_anal = layout_app->addChild(Wt::cpp14::make_unique<Wt::WDialog>("anal"));
    dialog_anal->setResizable(true);
    dialog_anal->setModal(false);
    dialog_anal->setMaximumSize(1000,1000);

    //auto content_anal = dialog_anal->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    //content_anal->setOverflow(Overflow::Auto);
    //content_anal->addStyleClass("content_anal");

    dialog_catalog = layout_app->addChild(Wt::cpp14::make_unique<Wt::WDialog>("catalog"));
    dialog_catalog->setResizable(true);
    dialog_catalog->setModal(false);
    dialog_catalog->setMaximumSize(1000,1000);

    auto content_catalog = dialog_catalog->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    content_catalog->setOverflow(Overflow::Auto);
    content_catalog->addStyleClass("content_catalog");

    //stack_info = content_info_->addWidget(cpp14::make_unique<WStackedWidget>());
    //stack_info = dialog_cont->contents()->addWidget(cpp14::make_unique<WStackedWidget>());
    //stack_info->setOverflow(Overflow::Auto);
    //stack_info->addStyleClass("content_info_stack");

    
    mSimplepointW = content_info;
    mGroupLayerW  = content_catalog;
    mGroupLayerW->setStyleClass("content_GL");



    /* CHARGE ONGLET COUCHES & SIMPLEPOINT */
    printf("create GL\n");
    //mDico->openConnection();
    mGroupL = new groupLayers(mDico,mMap,m_app,this);
    //mDico->closeConnection();// avant, GL et stat windows avaient tout deux besoin d'ouvrir la connection du dico pour créer des layers

    statWindow * page_camembert = top_stack->addNew<statWindow>(mGroupL);

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    mPA = dialog_anal->contents()->addWidget(cpp14::make_unique<parcellaire>(mGroupL,m_app,page_camembert));
    mPA->addStyleClass("content_analyse");

    /*	ACTIONS	: on connect les events aux méthodes	*/
    // dans wt_config, mettre à 500 milliseconde au lieu de 200 pour le double click
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));
    //
    mMap->xySelect().connect(std::bind(&parcellaire::selectPolygon,mPA, std::placeholders::_1,std::placeholders::_2));
    //mMap->polygId().connect(std::bind(&parcellaire::computeStatAndVisuSelectedPol,mPA, std::placeholders::_1));
    //mMap->getMapExtendSignal().connect(std::bind(&WOpenLayers::updateMapExtend,mMap,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    //layout_carto->addWidget(std::move(content_info), 1);
    //dialog_cont->contents()->addChild(std::move(content_info));
    //dialog_cont->contents()->addNew<Wt::WLabel>("Cell location (A1..Z999)");
    //top_stack->addWidget(std::move(container_home));


    m_app->internalPathChanged().connect(this, &cWebAptitude::handlePathChange);

    // force first route
    handlePathChange();

}


void cWebAptitude::load_content_couches(WContainerWidget * content){
    auto layout = content->setLayout(cpp14::make_unique<WHBoxLayout>());

    auto menu_gauche = layout->addWidget(cpp14::make_unique<WContainerWidget>());
    auto content_panier = layout->addWidget(cpp14::make_unique<WContainerWidget>());

    menu_gauche->setWidth(60);
    menu_gauche->addStyleClass("menu_gauche");

    content_panier->addWidget(Wt::cpp14::make_unique<Wt::WText>("coucoucoucouc ocuc"));
    content_panier->setWidth("100%");

    auto menu = menu_gauche->addWidget(Wt::cpp14::make_unique<WMenu>());
    //menu->setStyleClass("nav nav-pills nav-stacked");
    menu->setStyleClass("nav-stacked");

    menuitem_panier = menu->addItem("resources/right_angle_circle_icon_149877d.png","");
    menuitem_panier->clicked().connect([=] {
        std::cout << content->width().value() << std::endl;
        if(content->width().value()>70 || content->width().value()==-1){
            content->setWidth(70);
            content_panier->hide();
        }else{
            content->setWidth(300);
            content_panier->show();
        }
        //menuitem_panier->renderSelected(false);

    });
    menuitem_panier->setToolTip(tr("menu.button.tooltip.panier_collapse"));

    menuitem_simplepoint = menu->addItem("resources/position-logo.png","");
    menuitem_simplepoint->clicked().connect([=] {
        if(dialog_info->isVisible()){
            dialog_info->hide();
            menuitem_simplepoint->decorationStyle().setBackgroundColor(Wt::WColor(0,220,0,0));
        }else{
            dialog_info->show();
            menuitem_simplepoint->decorationStyle().setBackgroundColor(Wt::WColor(255,255,255,0));
        }
        //menuitem_simplepoint->renderSelected(dialog_info->isVisible());
    });
    menuitem_simplepoint->setToolTip(tr("menu.button.tooltip.point"));

    menuitem_analyse = menu->addItem("resources/stat-logo.png","");
    menuitem_analyse->clicked().connect([=] {
        if(dialog_anal->isVisible()){
            dialog_anal->hide();
        }else{
            dialog_anal->show();
        }
        //menuitem_analyse->renderSelected(dialog_info->isVisible());
    });
    menuitem_analyse->setToolTip(tr("menu.button.tooltip.surf"));

    // TODO menu catalog

    // TODO charger le panier
}


/*
 * Redirections en fonction du internal path
 * (ne pas oublier de compléter dans le main.cpp si internalpath changés - S.Q.)
 */
void cWebAptitude::handlePathChange()
{


    std::size_t found = m_app->internalPath().find("/documentation");
    if (m_app->internalPath() == "/documentation" | found!=std::string::npos){
        top_stack->setCurrentIndex(0);
        menuitem_documentation->select();
        //sub_stack->setCurrentIndex(1);
        navigation->setTitle(tr("titre.presentation"));
        m_app->addMetaHeader("description", tr("desc.pres"), "fr");
    /*}else if (m_app->internalPath() == "/home"){
        top_stack->setCurrentIndex(0);*/
        m_app->addMetaHeader("description", tr("desc.home"), "fr");
    }else if (m_app->internalPath() == "/cartographie" || m_app->internalPath() == "/"){
        top_stack->setCurrentIndex(1);
        //stack_info->setCurrentIndex(1);
        menuitem_app->select();
        //sub_stack->setCurrentIndex(0);
        navigation->setTitle(tr("titre.carto"));
        m_app->addMetaHeader("description", tr("desc.carto"), "fr");
    /*}else if (m_app->internalPath() == "/analyse"){
        top_stack->setCurrentIndex(1);
        //stack_info->setCurrentIndex(2);
        menuitem_analyse->select();
        //sub_stack->setCurrentIndex(0);
        navigation->setTitle(tr("titre.ana.surf"));
        m_app->addMetaHeader("description", tr("desc.surf"), "fr");
    }else if (m_app->internalPath() == "/point"){
        top_stack->setCurrentIndex(1);
        //stack_info->setCurrentIndex(0);
        menuitem_simplepoint->select();
        //sub_stack->setCurrentIndex(0);
        navigation->setTitle(tr("titre.ana.point"));
        m_app->addMetaHeader("description", tr("desc.point"), "fr");*/
    }else if (m_app->internalPath() == "/resultat"){
        top_stack->setCurrentIndex(1);
        //sub_stack->setCurrentIndex(2);
    }else if (m_app->internalPath() == "/parametres"){
        m_app->doJavaScript("alert('Pas encore implémenté...')");
    }else{
        std::cout << "m_app->internalPath() " << m_app->internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << m_app->internalPath() << std::endl;
    }
    //m_app->removeMetaHeader(Wt::MetaHeaderType::Meta, "robots");
    //m_app->addMetaHeader("robots", "index, follow", "fr");

    // TODO css min-size [menu_analyse] display:none if width<900
    // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;

}
