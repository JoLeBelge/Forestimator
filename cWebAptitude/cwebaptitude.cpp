#pragma once
#include "cwebaptitude.h"
//#include "auth.h"

//bool ModeExpert(0);

cWebAptitude::cWebAptitude(AuthApplication *app, Auth::AuthWidget* authWidget_)
    : WContainerWidget(),authWidget(authWidget_)
{
    //ModeExpert=0;// un peu sale comme procédé, car le login d'un expert va mettre la var Globale ModeExpert=1 et toutes les nouvelles instances de l'application (d'autre utilisateurs) vont également être en mode expert lool. Solution; remettre la var à 0 lors de chaque nouvelle app
    m_app = app;
    m_app->setLoadingIndicator(cpp14::make_unique<Wt::WOverlayLoadingIndicator>());
     m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));
    //m_app->loadingIndicator()->setStyleClass("loading-indicator"); // c'est moche, je vais laisser Sam gerer ça
    std::string aBD=loadBDpath();
    mDico=new cDicoApt(aBD);
    color col= mDico->Apt2col(2);
    WCssDecorationStyle T;
    T.setForegroundColor(WColor("black"));
    //T.setBackgroundImage("data/img/T.png");
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".T", T);
    col=mDico->Apt2col(1);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".O", T);
    col=mDico->Apt2col(3);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".TE", T);
    col=mDico->Apt2col(4);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".E", T);
    mStackInfoPtr=new stackInfoPtr();

    addStyleClass("cWebAptitude");
    setMargin(0);
    setPadding(0);
    std::unique_ptr<WContainerWidget> container_2 = cpp14::make_unique<WContainerWidget>();
    WVBoxLayout * layoutGlobal = container_2->setLayout(cpp14::make_unique<WVBoxLayout>());
    layoutGlobal->setContentsMargins(0,0,0,0);

    container_2->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    container_2->addStyleClass("carto_div");

    WVBoxLayout * layout = setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    layout->setContentsMargins(0,0,0,0);
    top_stack  = layout->addWidget(Wt::cpp14::make_unique<Wt::WStackedWidget>());
    top_stack->setMargin(0);

    /*	HOME PAGE	*/
    auto container_home = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    container_home->setStyleClass("home_div");
    container_home->setOverflow(Wt::Overflow::Auto);
    container_home->setMargin(Wt::WLength::Auto);
    container_home->setMargin(20,Wt::Side::Top);
    container_home->addNew<Wt::WText>("<h1 style='color:white;height: 37px;font-size: 3em;max-width: 1000px;text-align: center;'>Forestimator</h1>");
    container_home->decorationStyle().setBackgroundImage(Wt::WLink("resources/DJI_0497_3.png"), None);
    // add menu as push buttons!
    Wt::WPushButton *b_pres  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Présentation"));
    container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    Wt::WPushButton *b_carto = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Cartographie"));
    container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    Wt::WPushButton *b_point = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Info ponctuelle"));
    container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    Wt::WPushButton *b_anal  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>("Analyse"));
    container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    b_login  = container_home->addWidget(Wt::cpp14::make_unique<Wt::WPushButton>(m_app->isLoggedIn()?"Se déconnecter":"Se connecter"));
    container_home->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    b_pres ->setStyleClass("btn btn-success");
    b_carto->setStyleClass("btn btn-success");
    b_anal ->setStyleClass("btn btn-success");
    b_login->setStyleClass("btn btn-success");
    b_point->setStyleClass("btn btn-success");
    b_pres ->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/presentation"));
    b_carto->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/cartographie"));
    b_anal ->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/analyse"));
    b_point->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/point"));
    b_login ->clicked().connect([=] {
        if(m_app->isLoggedIn()){
            b_login->setText("Se connecter");
            m_app->logout();
            // toto enlever les extents dans carto
        }else{
            printf("event");
            //authWidget->addStyleClass("visible"); // ne fonctionne qu'une fois ! bizarre...workaround avec injection de JS direct OK !
            //authWidget->removeStyleClass("nonvisible");
            std::string JScommand("$('.Wt-auth-login-container').removeClass('nonvisible').addClass('visible');");
            m_app->doJavaScript(JScommand);
        }
    });
    authWidget->addStyleClass("nonvisible");

    WContainerWidget *c_img1 = container_home->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    Wt::WImage *img1 = c_img1->addNew<Wt::WImage>(Wt::WLink("resources/uliege.png"));
    img1->setAlternateText("ULiege logo");
    c_img1->addStyleClass("logo_left");

    WContainerWidget *c_img2 = container_home->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    Wt::WImage *img2 = c_img2->addNew<Wt::WImage>(Wt::WLink("resources/fpb.png"));
    c_img2->addStyleClass("logo_right");
    /*	FIN HOME PAGE	*/

    /*	NAVIGATION BAR	*/
    navigation = layoutGlobal->addWidget(cpp14::make_unique<WNavigationBar>());
    //navigation->setTitle("   <strong>Forestimator - cartographie</strong>   "); // WLink(LinkType::InternalPath, "/home"));
    navigation->setResponsive(true);
    navigation->addStyleClass("carto_menu");

    /*
     * Création d'un stack sous la barre de navigation
     * 0 : map et couche et legende et analyse
     * 1 : statistique parcellaire
     * 2 : présentation
     */
    sub_stack  = layoutGlobal->addWidget(cpp14::make_unique<WStackedWidget>());
    // page principale
    WContainerWidget * page_carto = sub_stack->addNew<WContainerWidget>();
    // page de statistique
    statWindow * page_camembert = sub_stack->addNew<statWindow>(mDico);
    //WContainerWidget * page_camembert = sub_stack->addNew<WContainerWidget>();
    sub_stack->addNew<presentationPage>();


    /* MAP div */
    auto layout_carto = page_carto->setLayout(cpp14::make_unique<WHBoxLayout>());
    page_carto->layout()->setContentsMargins(0,0,0,0);
    page_carto->setHeight("100%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    page_carto->setOverflow(Overflow::Visible); // non pas d'overflow pour la carte, qui est dans page_carto

    /* Partie droite couches-légende-analyse */
    auto content_info = cpp14::make_unique<WContainerWidget>();
    WContainerWidget * content_info_ = content_info.get();
    //WVBoxLayout * layout_info = content_info_->setLayout(cpp14::make_unique<WVBoxLayout>());
    content_info_->setOverflow(Overflow::Auto);
    content_info_->setWidth("30%"); // TODO CSS resize @min-width
    //content_info_->setMinimumSize(400,0);
    content_info_->addStyleClass("content_info");

    /* 	SOUS-MENU droite    */
    /* Create a stack where the contents will be located.
     *
     * Index de mStackInfoPtr->stack_info
     * 0 : point (analyse simple par double clic on map)
     * 1 : analyse
     * 2 : carto
     *
     * */
    mStackInfoPtr->stack_info = content_info_->addWidget(cpp14::make_unique<WStackedWidget>());
    //mStackInfoPtr->stack_info->setOverflow(Overflow::Auto);
    mStackInfoPtr->stack_info->addStyleClass("content_info_stack");
    std::unique_ptr<WMenu> menu_ = cpp14::make_unique<WMenu>();
    WMenu * right_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);
    auto bh = right_menu->addItem("resources/purchasing_management_icon_149886.png","");
    bh->setLink(WLink(LinkType::InternalPath, "/home"));
    bh->setToolTip("Accueil");
    mStackInfoPtr->menuitem_presentation = right_menu->addItem("resources/problem_analysis_icon_149897.png","");
    mStackInfoPtr->menuitem_presentation->setLink(WLink(LinkType::InternalPath, "/presentation"));
    mStackInfoPtr->menuitem_presentation->setToolTip("Présentation");
    mStackInfoPtr->menuitem_cartes = right_menu->addItem("resources/layers_filled_icon_149920.png","");
    mStackInfoPtr->menuitem_cartes->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    mStackInfoPtr->menuitem_cartes->setToolTip("Cartographie");
    mStackInfoPtr->menuitem_simplepoint = right_menu->addItem("resources/process_icon_149895.png","");
    mStackInfoPtr->menuitem_simplepoint->setLink(WLink(LinkType::InternalPath, "/point"));
    mStackInfoPtr->menuitem_simplepoint->setToolTip("Info ponctuelle");
    mStackInfoPtr->menuitem_analyse = right_menu->addItem("resources/data_analysis_icon_149953.png","");
    mStackInfoPtr->menuitem_analyse->setLink(WLink(LinkType::InternalPath, "/analyse"));
    mStackInfoPtr->menuitem_analyse->setToolTip("Analyse");
    right_menu->addItem("resources/configuration_center_icon_149956.png","")
            ->setLink(WLink(LinkType::InternalPath, "/parametres"));
    

    mStackInfoPtr->mSimplepointW = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>()); // index 0
    mStackInfoPtr->mGroupLayerW  = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>()); // index 1
    mStackInfoPtr->mGroupLayerW->setStyleClass("content_GL");

    /*	MAPS	*/
    printf("create map\n");
    auto map = cpp14::make_unique<WOpenLayers>(mDico);
    mMap = map.get();
    mMap->setWidth("70%");
    mMap->setMinimumSize(400,0);
    mMap->setOverflow(Overflow::Visible);

    /* CHARGE ONGLET COUCHES & SIMPLEPOINT */
    //printf("create GL\n");
    mGroupL = new groupLayers(mDico,mMap,m_app,mStackInfoPtr);

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    //mPA = new parcellaire(content_analyse,mGroupL,m_app,page_camembert);
    mPA = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<parcellaire>(mGroupL,m_app,page_camembert)); // index 2
    mPA->addStyleClass("content_analyse");

    // maintenant que tout les objets sont crées, je ferme la connection avec la BD sqlite3, plus propre
    mDico->closeConnection();

    /*	ACTIONS	: on connect les events aux méthodes	*/
    // dans wt_config, mettre à 500 milliseconde au lieu de 200 pour le double click
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));
    mMap->polygId().connect(std::bind(&parcellaire::computeStatAndVisuSelectedPol,mPA, std::placeholders::_1));
    //mMap->getMapExtendSignal().connect(std::bind(&WOpenLayers::updateMapExtend,mMap,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    // je divise la fenetre en 2 dans la largeur pour mettre la carte à gauche et à droite une fenetre avec les infos des couches
    layout_carto->addWidget(std::move(map), 0);
    layout_carto->addWidget(std::move(content_info), 1);
    top_stack->addWidget(std::move(container_home));
    top_stack->addWidget(std::move(container_2));
    top_stack->setCurrentIndex(0);

    m_app->internalPathChanged().connect(this, &cWebAptitude::handlePathChange);

    // force first route
    handlePathChange(); // force first route
}

/*
 * Redirections en fonction du internal path
 *
 */
void cWebAptitude::handlePathChange()
{
    if (m_app->internalPath() == "/presentation"){
        top_stack->setCurrentIndex(1);
        mStackInfoPtr->menuitem_presentation->select();
        sub_stack->setCurrentIndex(2);
        navigation->setTitle("   <strong>Forestimator - présentation</strong>   ");
    }else if (m_app->internalPath() == "/home"){
        top_stack->setCurrentIndex(0);
    }else if (m_app->internalPath() == "/cartographie"){
        top_stack->setCurrentIndex(1);
        mStackInfoPtr->stack_info->setCurrentIndex(1);
        mStackInfoPtr->menuitem_cartes->select();
        sub_stack->setCurrentIndex(0);
        navigation->setTitle("   <strong>Forestimator - cartographie</strong>   ");
    }else if (m_app->internalPath() == "/analyse"){
        top_stack->setCurrentIndex(1);
        mStackInfoPtr->stack_info->setCurrentIndex(2);
        mStackInfoPtr->menuitem_analyse->select();
        sub_stack->setCurrentIndex(0);
        navigation->setTitle("   <strong>Forestimator - analyse</strong>   ");
    }else if (m_app->internalPath() == "/point"){
        top_stack->setCurrentIndex(1);
        mStackInfoPtr->stack_info->setCurrentIndex(0);
        mStackInfoPtr->menuitem_simplepoint->select();
        sub_stack->setCurrentIndex(0);
        navigation->setTitle("   <strong>Forestimator - info ponctuelle</strong>   ");
    }else if (m_app->internalPath() == "/resultat"){
        top_stack->setCurrentIndex(1);
        sub_stack->setCurrentIndex(1);
    }else if (m_app->internalPath() == "/parametres"){
        m_app->doJavaScript("alert('Pas encore implémenté...')");

    }else{
        //
        std::cout << "m_app->internalPath() " << m_app->internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << m_app->internalPath() << std::endl;
    }

    // TODO css min-size [menu_analyse] display:none if width<900
    // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;

}
