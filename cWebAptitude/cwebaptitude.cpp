#pragma once
#include "cwebaptitude.h"
//#include "auth.h"

extern bool globTest;

cWebAptitude::cWebAptitude(AuthApplication *app, Auth::AuthWidget* authWidget_)
    : WContainerWidget(),authWidget(authWidget_),mDico(app->getDico())
{
    std::cout << "cWebApt\n" << std::endl;
    GDALAllRegister();
    mApp = app;
    mApp->setLoadingIndicator(cpp14::make_unique<Wt::WOverlayLoadingIndicator>());
    mApp->loadingIndicator()->setMessage(tr("defaultLoadingI"));

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
    menuitem_login = navbar_menu->addItem(mApp->isLoggedIn()?"resources/user_icon_logout.png":"resources/user_icon_149851.png","");
    menuitem_login->clicked().connect([=] {
        if(mApp->isLoggedIn()){
            menuitem_login->setIcon("resources/user_icon_149851.png");
            mApp->logout();
        }else{
            //authWidget->addStyleClass("visible"); // ne fonctionne qu'une fois ! bizarre...workaround avec injection de JS direct OK !
            //authWidget->removeStyleClass("nonvisible");
            std::string JScommand("$('.Wt-auth-login-container').removeClass('nonvisible').addClass('visible');");
            mApp->doJavaScript(JScommand);
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
    top_stack->setOverflow(Overflow::Auto);
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
    // load RESULT page TODO


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
    //content_couches->setOverflow(Overflow::Auto);
    content_couches->addStyleClass("content_couches");
    //load_content_couches(content_couches); // moved after mGroupL initialization !! non c'est juste la création du panier qu'il faut mettre après, le reste (création conteneur et layout) je dois le faire ici pour avoir mes Menuitem avant de créer class dialogu
    auto layout = content_couches->setLayout(cpp14::make_unique<WHBoxLayout>());
    content_couches->setPadding(0);
    layout->setContentsMargins(0,0,0,0);

    auto menu_gauche = layout->addWidget(cpp14::make_unique<WContainerWidget>());
    auto content_panier = layout->addWidget(cpp14::make_unique<WContainerWidget>());

    menu_gauche->setWidth(60);
    menu_gauche->addStyleClass("menu_gauche");

    content_panier->addWidget(Wt::cpp14::make_unique<Wt::WTemplate>(tr("panier.header")));
    content_panier->setWidth("100%");

    auto menu = menu_gauche->addWidget(Wt::cpp14::make_unique<WMenu>());
    //menu->setStyleClass("nav nav-pills nav-stacked");
    menu->setStyleClass("nav-stacked");
    menu->addStyleClass("nav-apt");

    menuitem_panier = menu->addItem("resources/right_angle_circle_icon_149877.png","");
    menuitem_panier->clicked().connect([=] {
        std::cout << content_couches->width().value() << std::endl;
        if(content_couches->width().value()>60 || content_couches->width().value()==-1){
            content_couches->setWidth(60);
            menuitem_panier->setIcon("resources/right_angle_circle_icon_149877d.png");
            content_panier->hide();
        }else{
            content_couches->setWidth(400);
            menuitem_panier->setIcon("resources/right_angle_circle_icon_149877.png");
            content_panier->show();
        }
        //menuitem_panier->renderSelected(false);

    });
    menuitem_panier->setToolTip(tr("menu.button.tooltip.panier_collapse"));
    // bouton catalogue
    menuitem_catalog = menu->addItem("resources/warehouse_check_icon_149849.png","");
    menuitem_catalog->setToolTip(tr("menu.button.tooltip.catalog"));
    // bouton cadastre
    menuitem_cadastre = menu->addItem("resources/incoming_inspection_icon_149926.png","");
    menuitem_cadastre->setToolTip(tr("menu.button.tooltip.cadastre"));
    // bouton legende
    menuitem_legend = menu->addItem("resources/product_bom_icon_149894.png","");
    menuitem_legend->setToolTip(tr("menu.button.tooltip.legend"));
    // bouton info point
    menuitem_simplepoint = menu->addItem("resources/position-logo.png","");
    menuitem_simplepoint->setToolTip(tr("menu.button.tooltip.point"));
    // bouton analyse
    menuitem_analyse = menu->addItem("resources/stat-logo.png","");
    menuitem_analyse->setToolTip(tr("menu.button.tooltip.surf"));

    /*  DIALOGS info_point-légende-analyse-catalogue-cadastre */

    // info point
    dialog_info = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Info ponctuelle",menuitem_simplepoint));

    auto content_info = dialog_info->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    //content_info->setOverflow(Overflow::Auto);
    content_info->addStyleClass("content_info");

    // analyse
    dialog_anal = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Analyse surfacique",menuitem_analyse));


    //auto content_anal = dialog_anal->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    //content_anal->setOverflow(Overflow::Auto);
    //content_anal->addStyleClass("content_anal");

    // catalogue
    dialog_catalog = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Catalogue des couches",menuitem_catalog));

    auto content_catalog = dialog_catalog->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    content_catalog->addStyleClass("content_catalog");

    // cadastre
    dialog_cadastre = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Recherche cadastrale",menuitem_cadastre));
    // legende
    dialog_legend = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Légende",menuitem_legend));
    mLegendW = dialog_legend->contents()->addWidget(cpp14::make_unique<WContainerWidget>());
    //mLegendW->setOverflow(Overflow::Auto);
    mLegendW->addStyleClass("content_legend");


    widgetCadastre * content_cadastre;
    content_cadastre = dialog_cadastre->contents()->addWidget(cpp14::make_unique<widgetCadastre>(mDico->mCadastre.get()));
    content_cadastre->addStyleClass("content_cadastre");


    //stack_info = content_info_->addWidget(cpp14::make_unique<WStackedWidget>());
    //stack_info = dialog_cont->contents()->addWidget(cpp14::make_unique<WStackedWidget>());
    //stack_info->setOverflow(Overflow::Auto);
    //stack_info->addStyleClass("content_info_stack");

    
    mSimplepointW = content_info;
    mGroupLayerW  = content_catalog;
    mGroupLayerW->setStyleClass("content_GL");

    /* CHARGE ONGLET COUCHES & SIMPLEPOINT */
    printf("create GL\n");
    mGroupL = new groupLayers(mApp,this);
    //load_content_couches(content_couches);

    mPanier = content_panier->addWidget(Wt::cpp14::make_unique<panier>(mApp, this));
    mGroupL->updateGL();
    // des couches que l'on souhaite voir dans le panier dès le départ
    mGroupL->clickOnName("IGN");

    statWindow * page_camembert = top_stack->addNew<statWindow>(mGroupL);

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    mPA = dialog_anal->contents()->addWidget(cpp14::make_unique<parcellaire>(mGroupL,mApp,page_camembert));
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

    //cadastre

    content_cadastre->sendPolygone().connect(std::bind(&parcellaire::polygoneCadastre,mPA,std::placeholders::_1));

    mApp->internalPathChanged().connect(this, &cWebAptitude::handlePathChange);

    // force first route
    handlePathChange();

}

/*
void cWebAptitude::load_content_couches(WContainerWidget * content){

    // TODO charger le panier
}
*/


/*
 * Redirections en fonction du internal path
 * (ne pas oublier de compléter dans le main.cpp si internalpath changés - S.Q.)
 */
void cWebAptitude::handlePathChange()
{

    std::size_t found = mApp->internalPath().find("/documentation");
    if (mApp->internalPath() == "/documentation" | found!=std::string::npos){
        top_stack->setCurrentIndex(0);
        menuitem_documentation->select();
        //sub_stack->setCurrentIndex(1);
        navigation->setTitle(tr("titre.presentation"));
        mApp->addMetaHeader("description", tr("desc.pres"), "fr");
        /*}else if (m_app->internalPath() == "/home"){
        top_stack->setCurrentIndex(0);*/
        mApp->addMetaHeader("description", tr("desc.home"), "fr");

        showDialogues(0);
    }else if (mApp->internalPath() == "/cartographie" || mApp->internalPath() == "/"){
        top_stack->setCurrentIndex(1);
        //stack_info->setCurrentIndex(1);
        menuitem_app->select();
        //sub_stack->setCurrentIndex(0);
        navigation->setTitle(tr("titre.carto"));
        mApp->addMetaHeader("description", tr("desc.carto"), "fr");
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

         showDialogues(1);
    }else if (mApp->internalPath() == "/resultat"){
        top_stack->setCurrentIndex(2);
        //sub_stack->setCurrentIndex(2);
         showDialogues(0);
    }else if (mApp->internalPath() == "/parametres"){
        mApp->doJavaScript("alert('Pas encore implémenté...')");
         showDialogues(0);
    }else{
        std::cout << "m_app->internalPath() " << mApp->internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << mApp->internalPath() << std::endl;
    }
    //m_app->removeMetaHeader(Wt::MetaHeaderType::Meta, "robots");
    //m_app->addMetaHeader("robots", "index, follow", "fr");

    // TODO css min-size [menu_analyse] display:none if width<900
    // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;

}


dialog::dialog(const WString& windowTitle,Wt::WMenuItem * aMenu):WDialog(windowTitle),mMenu(aMenu),mShow(0){

    setResizable(true);
    setModal(false);
    setMaximumSize(700,800);
    contents()->setOverflow(Overflow::Scroll);
    setClosable(true);

   // faire les liens entre le boutton menu et l'affichage de la fenetre
    finished().connect([=] {mMenu->decorationStyle().setBackgroundColor(col_not_sel); mShow=0;});

    mMenu->clicked().connect([=] {
        if(this->isVisible()){
            this->hide();
            mMenu->decorationStyle().setBackgroundColor(col_not_sel);
            mShow=0;
        }else{
            this->show();
            mMenu->decorationStyle().setBackgroundColor(col_sel);
            mShow=1;
        }
    });
}
