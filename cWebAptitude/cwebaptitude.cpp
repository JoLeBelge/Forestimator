#include "cwebaptitude.h"

bool ModeExpert(0);

cWebAptitude::cWebAptitude(WApplication* app)
    : WContainerWidget()
{
    ModeExpert=0;// un peu sale comme procédé, car le login d'un expert va mettre la var Globale ModeExpert=1 et toutes les nouvelles instances de l'application (d'autre utilisateurs) vont également être en mode expert lool. Solution; remettre la var à 0 lors de chaque nouvelle app
    m_app = app;
    std::string aBD=loadBDpath();
    mDico=new cDicoApt(aBD);
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

    /*	NAVIGATION BAR	*/
    auto navigation = layoutGlobal->addWidget(cpp14::make_unique<WNavigationBar>());
    //navigation->setTitle("   <strong>Forestimator</strong>   ", WLink(LinkType::InternalPath, "/home"));
    navigation->setResponsive(true);
    navigation->addStyleClass("carto_menu");

    // Setup a Left-aligned menu. Remplace le menu de droite.
    auto left_menu = cpp14::make_unique<WMenu>();
    auto left_menu_ = navigation->addMenu(std::move(left_menu));
    // Create a popup submenu pour retour à la page de présentation ou accès au mode expert
    auto popupPtr = Wt::cpp14::make_unique<Wt::WPopupMenu>();
    auto popup = popupPtr.get();
    menuitem_presentation = popup->addItem("Présentation");
    menuitem_presentation->setLink(WLink(LinkType::InternalPath, "/presentation"));
    WMenuItem * menuitem_login = popup->addItem("Mode expert");
    menuitem_login->clicked().connect([=] {this->login();});
    auto item = Wt::cpp14::make_unique<Wt::WMenuItem>("Forestimator");
    item->setMenu(std::move(popupPtr));
    left_menu_->addItem(std::move(item));
    //menuitem_presentation = left_menu_->addItem("Présentation");
    //menuitem_presentation->setLink(WLink(LinkType::InternalPath, "/presentation"));
    menuitem_carto = left_menu_->addItem("Cartographie");
    menuitem_carto->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    menuitem_analyse = left_menu_->addItem("Analyse");
    menuitem_analyse->setLink(WLink(LinkType::InternalPath, "/analyse"));

    /*
         * création d'un stack sous la barre de navigation
         * page 1 ; map et couche et legende et analyse
         * page 2 ; statistique parcellaire
         */
    sub_stack  = layoutGlobal->addWidget(cpp14::make_unique<WStackedWidget>());
    // page principale
    WContainerWidget * page_carto = sub_stack->addNew<WContainerWidget>();
    // page de statistique
    WContainerWidget * page_camembert = sub_stack->addNew<WContainerWidget>();

    /* MAP div */
    auto layout_carto = page_carto->setLayout(cpp14::make_unique<WHBoxLayout>());
    page_carto->layout()->setContentsMargins(0,0,0,0);

    //std::unique_ptr<WContainerWidget> page_carto = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    //auto layout_carto = page_carto->setLayout(cpp14::make_unique<WHBoxLayout>());
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
    // Create a stack where the contents will be located.
    //mStackInfoPtr->stack_info = layout_info->addWidget(cpp14::make_unique<WStackedWidget>());
    mStackInfoPtr->stack_info = content_info_->addWidget(cpp14::make_unique<WStackedWidget>());
    //mStackInfoPtr->stack_info->setOverflow(Overflow::Auto);
    mStackInfoPtr->stack_info->addStyleClass("content_info_stack");
    std::unique_ptr<WMenu> menu_ = cpp14::make_unique<WMenu>();
    WMenu * right_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);
    mStackInfoPtr->menuitem2_cartes = right_menu->addItem("Couches");
    mStackInfoPtr->menuitem2_legend = right_menu->addItem("Légende");
    mStackInfoPtr->menuitem2_analyse = right_menu->addItem("Analyse");
    mStackInfoPtr->menuitem2_cartes->clicked().connect([=] {mStackInfoPtr->stack_info->setCurrentIndex(2);});
    mStackInfoPtr->menuitem2_legend->clicked().connect([=] {mStackInfoPtr->stack_info->setCurrentIndex(0);});
    mStackInfoPtr->menuitem2_analyse->clicked().connect([=] {mStackInfoPtr->stack_info->setCurrentIndex(1);});
    
    //auto content_legend = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    mStackInfoPtr->mLegendW = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>());

    auto content_analyse = mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    content_analyse->addStyleClass("content_analyse");

    /*	MAPS	*/
    printf("create map\n");
    auto map = cpp14::make_unique<WOpenLayers>(mDico);
    mMap = map.get();
    mMap->setWidth("70%");
    mMap->setMinimumSize(400,0);
    mMap->setOverflow(Overflow::Visible);
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

    //auto content_GL = stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    mStackInfoPtr->mGroupLayerW= mStackInfoPtr->stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    //content_GL->setStyleClass("content_GL");
    mStackInfoPtr->mGroupLayerW->setStyleClass("content_GL");

    /* CHARGE ONGLET COUCHES & LEGENDE */
    //printf("create GL\n");
    mGroupL = new groupLayers(mDico,mMap,m_app,mStackInfoPtr);

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    mPA = new parcellaire(content_analyse,mGroupL,m_app,page_camembert);


    // first route TODO check si necessaire
    /* redondant avec appel num 1 de handlepathchange()
    printf("route : %s\n",m_app->internalPath().c_str());
    if (m_app->internalPath() == "/analyse"){
        menuitem_analyse->addStyleClass("active");
        menuitem2_analyse->addStyleClass("active");
        //menuitem2_analyse->toggleStyleClass("notvisible",true,);
        menuitem2_analyse->setHidden(false);
        stack_info->setCurrentIndex(1);
        mMap->setWidth("60%");
        // TODO css min-size [menu_analyse] display:none if width<900
    }else{
        menuitem_carto->addStyleClass("active");
        stack_info->setCurrentIndex(2);
        menuitem2_cartes->addStyleClass("active");
        //menuitem2_analyse->toggleStyleClass("notvisible",true);
        menuitem2_analyse->setHidden(true);
        // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;
    }
    */

    // maintenant que tout les objets sont crées, je ferme la connection avec la BD sqlite3, plus propre
    mDico->closeConnection();

    /*	ACTIONS		*/

    // reviens sur l'onglet légende si on est sur l'onglet parcellaire
   /* mMap->doubleClicked().connect([this]{mStackInfoPtr->menuitem2_legend->select();
         // select ; ne fait pas bien son job, bug car affiche à la fois le conteneur légende et en dessous le conteneur stack de group layer. Il faut donc en plus la ligne suivante
    mStackInfoPtr->stack_info->setCurrentIndex(0);});
    */
    // et dans wt_config, mettre à 500 milliseconde au lieu de 200 pour le double click
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));

    mMap->polygId().connect(std::bind(&parcellaire::computeStatAndVisuSelectedPol,mPA, std::placeholders::_1));
    


    //mMap->getMapExtendSignal().connect(std::bind(&WOpenLayers::updateMapExtend,mMap,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    // je divise la fenetre en 2 dans la largeur pour mettre la carte à gauche et à droite une fenetre avec les infos des couches
    layout_carto->addWidget(std::move(map), 0);

    layout_carto->addWidget(std::move(content_info), 1);

    top_stack->addWidget(std::move(container_home));
    top_stack->addWidget(std::move(container_2));
    top_stack->setCurrentIndex(0);

    /*m_app->internalPathChanged().connect([=] {
        handlePathChange();
    });
    */
    // un peu plus élégant que lambda même si ça a le même comportement
    m_app->internalPathChanged().connect(this, &cWebAptitude::handlePathChange);

    // force first route
    //m_app->setInternalPath("/presentation");
    handlePathChange(); // force first route

}

/*
 * Redirections en fonction du internal path
 *
 */
void cWebAptitude::handlePathChange()
{
    //printf("path change\n");
    //std::cout << "m_app->internalPath() " << m_app->internalPath() << std::endl;
    // TODO corriger les affichages
    if (m_app->internalPath() == "/presentation"){
        // pour l'instant, n'existe pas, on retourne à home
        top_stack->setCurrentIndex(0);
    }else if (m_app->internalPath() == "/home"){
        top_stack->setCurrentIndex(0);
    }else if (m_app->internalPath() == "/cartographie"){
        top_stack->setCurrentIndex(1);
        mStackInfoPtr->stack_info->setCurrentIndex(2);

        menuitem_presentation->removeStyleClass("active");
        menuitem_carto->addStyleClass("active");
        menuitem_analyse->removeStyleClass("active");

         mStackInfoPtr->menuitem2_analyse->setHidden(true);
         mStackInfoPtr->menuitem2_legend->setHidden(false);
         mStackInfoPtr->menuitem2_cartes->setHidden(false);
        sub_stack->setCurrentIndex(0);
    }else if (m_app->internalPath() == "/analyse"){
        top_stack->setCurrentIndex(1);
         mStackInfoPtr->stack_info->setCurrentIndex(1);

        menuitem_presentation->removeStyleClass("active");
        menuitem_carto->removeStyleClass("active");
        menuitem_analyse->addStyleClass("active");

         mStackInfoPtr->menuitem2_analyse->setHidden(false);
         mStackInfoPtr->menuitem2_legend->setHidden(false);
         mStackInfoPtr->menuitem2_cartes->setHidden(false);
         mStackInfoPtr->menuitem2_analyse->select();
        sub_stack->setCurrentIndex(0);
    }else if (m_app->internalPath() == "/resultat"){
        top_stack->setCurrentIndex(1);
        sub_stack->setCurrentIndex(1);

        menuitem_presentation->removeStyleClass("active");
        menuitem_carto->removeStyleClass("active");
        menuitem_analyse->removeStyleClass("active");

         mStackInfoPtr->menuitem2_analyse->setHidden(true);
         mStackInfoPtr->menuitem2_legend->setHidden(true);
         mStackInfoPtr->menuitem2_cartes->setHidden(true);

    }else{
        //
        std::cout << "m_app->internalPath() " << m_app->internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << m_app->internalPath() << std::endl;
    }
}


void cWebAptitude::login(){


    Wt::WDialog * dialogPtr =  this->addChild(Wt::cpp14::make_unique<Wt::WDialog>("Se connecter en mode expert"));

    Wt::WPushButton *ok =
            dialogPtr->footer()->addNew<Wt::WPushButton>("Connexion");
    ok->setDefault(true);

    Wt::WPushButton * annuler =
            dialogPtr->footer()->addNew<Wt::WPushButton>("Annuler");
    annuler->setDefault(false);

    Wt::WLineEdit * username = dialogPtr->contents()->addNew<Wt::WLineEdit>();
    username->setPlaceholderText("Utilisateur");
    dialogPtr->contents()->addNew<Wt::WBreak>();
    Wt::WLineEdit * password = dialogPtr->contents()->addNew<Wt::WLineEdit>();
    password->setPlaceholderText("Mot de passe");
    dialogPtr->contents()->addNew<Wt::WBreak>();
    Wt::WText *out = dialogPtr->contents()->addNew<Wt::WText>("");
    out->addStyleClass("help-block");
    //username->setValidator(std::make_shared<Wt::WIntValidator>(0, 130));
    /*
       * Accept the dialog
       */
    ok->clicked().connect([=] {
        // controle du contenu username et password
        if (password->text()=="gef" & username->text() =="gef"){
        ModeExpert=1;
        // je recrée les essences donc j'ai besoin d'une connection à la BD
        mDico->openConnection();
        // update grouplayer
        mGroupL->updateGL();
        mDico->closeConnection();
        mPA->update();

        auto messageBox = dialogPtr->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                      "Mode Expert",
                                      "<p> Mode expert activé, vous avez accès aux information liées aux catalogues de station (aptitudes, potentiel sylvicole, habitats)</p>",
                                      Wt::Icon::Information,
                                      Wt::StandardButton::Ok));

        messageBox->setModal(false);
        messageBox->buttonClicked().connect([=] {
            dialogPtr->removeChild(messageBox);
        });
        messageBox->show();

        dialogPtr->accept();
        } else {
            out->setText("Utilisateur et/ou mot de passe invalide.");
        }
    });

    if (ModeExpert){
        ok->disable();
        username->hide();
        password->hide();
        out->setText("Mode Expert déjà actif.");
    }

    annuler->clicked().connect([=]{dialogPtr->reject();});
    dialogPtr->show();
}
