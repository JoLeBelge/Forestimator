#include "cwebaptitude.h"

extern bool globTest;
std::string cookies_clientIDName="clientID";


/*
 * Redirections en fonction du internal path
 * (ne pas oublier de compléter dans le main.cpp si internalpath changés - S.Q.)
 */
void cWebAptitude::handlePathChange()
{
    bool baddLog(1);
    std::string docInternalP("/documentation");
    std::size_t found = internalPath().find(docInternalP);
    if (internalPath() == "/documentation" | found!=std::string::npos){
        top_stack->setCurrentIndex(0);
        menuitem_documentation->select();

        //navigation->setTitle(tr("titre.presentation"));
        // mApp->addMetaHeader("description", tr("desc.pres"), "fr");

        // une description différentes pour chaques page de documentation. Il faut préhalablement retirer la description, sinon ça n'aura pas d'effet
        // fonctionne pas si session avec javascript, mais pour les robots ça va quand-même. Sur mon navigateur, le meta descr c'est celui de la première page consultée de la session
        // check si j'ai un header description pour cette sous-section, sinon celui général à la page documentation
        if (globTest){std::cout << "handlepathChange documentation : "<< internalPath() << std::endl;}
        if (found!=std::string::npos && found + docInternalP.length()+1 < internalPath().size()){// test sur la longueur car il faut gérer le cas ou on a juste un backslach : /documentation/
            std::string sectionDoc=internalPath().erase(0, found + docInternalP.length()+1);
            // retirer l'éventuel baslach en fin de url
            if (sectionDoc.back()=='/'){
                //sectionDoc.pop_back();
                sectionDoc.erase(sectionDoc.size()-1);
            }

            //sectionDoc.erase(std::remove(sectionDoc.begin(), sectionDoc.end(), '/'), sectionDoc.end());
            if (globTest){std::cout << "section doc : "<< sectionDoc << std::endl;}
            changeHeader(sectionDoc);
        } else {
            changeHeader("documentation");
        }
        showDialogues(0);
    }else if (internalPath() == "/cartographie" || internalPath() == "/" || internalPath() == ""){
        top_stack->setCurrentIndex(1);
        menuitem_app->select();
        removeMetaHeader(MetaHeaderType::Meta,"description");
        addMetaHeader(MetaHeaderType::Meta,"description", Wt::WString::tr("meta.desc.carto"), "fr");
        showDialogues(1);
    }else if (internalPath() == "/resultat"){
        top_stack->setCurrentIndex(2);
        showDialogues(0);
    }else{
        std::cout << "m_app->internalPath() " << internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << internalPath() << std::endl;
        baddLog=0;
    }

    if (baddLog){addLog(internalPath());}

    // TODO css min-size [menu_analyse] display:none if width<900
    // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;

}


void cWebAptitude::changeHeader(std::string aSection){
    std::string message(WString::tr("meta.desc."+aSection).toUTF8());
    if (message.substr(0,2)!="??"){
        removeMetaHeader(MetaHeaderType::Meta,"description");
        addMetaHeader(MetaHeaderType::Meta,"description", message, "fr");
    }
    message=WString::tr("meta.titre."+aSection).toUTF8();
    if (message.substr(0,2)!="??"){
        removeMetaHeader(MetaHeaderType::Meta,"titre");
        addMetaHeader(MetaHeaderType::Meta,"titre", message, "fr");
    }

}


dialog::dialog(const WString& windowTitle, Wt::WMenuItem * aMenu, const WEnvironment *env):WDialog(windowTitle),mMenu(aMenu),mShow(0),env_(env){

    setResizable(true);
    setModal(false);

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
            //this->positionAt(widget,Wt::Orientation::Vertical);
            mShow=1;
            this->myshow();
            mMenu->decorationStyle().setBackgroundColor(col_sel);
            this->raiseToFront();
        }
    });
}


cWebAptitude::cWebAptitude(const Wt::WEnvironment& env, cDicoApt *dico)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db"),mDico(dico),mAnal(dico->File("docroot")+"analytics.db")
{
    //std::cout << "internal path : " << internalPath()<< std::endl;
    //
    //std::string ointernalPath(internalPath());
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    messageResourceBundle().use(docRoot() + "/forestimator");
    messageResourceBundle().use(docRoot() + "/forestimator-documentation");
    messageResourceBundle().use(docRoot() + "/forestimator-CS");

    // export de tout les messages html vers un fichier csv qui sera traduit en text avec ./html2text -from_encoding UTF8 -nobs -o /home/jo/app/Forestimator/data/tmp/Forestimator.txt /home/jo/app/Forestimator/data/tmp/texteForestimator.csv pour correction orthographique
    bool forestimator(0);
    if (globTest & 0){
        std::cout << " export des messages xml \n\n\n" << std::endl;
        std::ifstream theFile;
        std::string aFile(mDico->File("TMPDIR")+"texteForestimator.csv");
        if (forestimator){

            theFile.open(docRoot() + "/forestimator.xml");} else {
            aFile=mDico->File("TMPDIR")+"textePhytospy.csv";
            theFile.open("/home/jo/app/phytospy/data/phytoTool.xml");
            messageResourceBundle().use("/home/jo/app/phytospy/data/phytoTool");
        }

        std::ofstream aOut;
        aFile=mDico->File("TMPDIR")+"traductionPhytospy.xml";
        aOut.open(aFile,ios::out);
        xml_document<> doc;
        xml_node<> * root_node;
        std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc
        doc.parse<0>(&buffer[0]);
        // Find our root node
        root_node = doc.first_node("messages");
        for (xml_node<> * node = root_node->first_node("message"); node; node = node->next_sibling())
        {
            // il faudrait tester si l'attribut id existe, sinon plante. pour le moment c'est pas fonctionnel
            if (node->first_attribute("id")->value()){
                std::string aId(node->first_attribute("id")->value());
                if (0 & aId.find("Wt.Auth")==std::string::npos){
                    aOut << WText::tr(aId).toUTF8() ;
                    aOut <<"\n\n<br> <br/>" ;
                }
            } else {
                std::cout << "incorrect node " << std::endl;
            }
            //break;
        }
        std::cout << " premier fichier fait " << std::endl;
        doc.clear();
        theFile.close();
        if (forestimator){
            theFile.open(docRoot() + "/forestimator-documentation.xml");
            std::vector<char> buffer2((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
            buffer2.push_back('\0');
            // Parse the buffer using the xml file parsing library into doc
            doc.parse<0>(&buffer2[0]);
            // Find our root node
            root_node = doc.first_node("messages");
            for (xml_node<> * node = root_node->first_node("message"); node; node = node->next_sibling())
            {
                //std::cout << WText::tr(node->first_attribute("id")->value()).toUTF8() << "\n\n" << std::endl;
                std::string aId(node->first_attribute("id")->value());
                if (aId.find("Wt.Auth")==std::string::npos){
                    aOut << WText::tr(aId).toUTF8() ;
                    aOut <<"\n\n<br> <br/>" ;
                }
            }
        }
        std::cout << " ---------------- export done\n" << std::endl;
        aOut.close();
    }


    setTitle("Forestimator");
    loadStyles();

    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    root()->setMargin(0);
    root()->setPadding(0);

    //layout->addWidget(std::move(loadAuthWidget()));
    dialog_auth = layout->addChild(Wt::cpp14::make_unique<Wt::WDialog>("Connexion"));
    dialog_auth->setResizable(true);
    dialog_auth->setModal(true);
    dialog_auth->setMaximumSize(700,600);
    dialog_auth->setWidth(600);
    dialog_auth->setHeight(500);
    dialog_auth->contents()->setOverflow(Overflow::Scroll);
    dialog_auth->setClosable(true);
    dialog_auth->contents()->addWidget(std::move(loadAuthWidget()));

    Wt::WContainerWidget* contPrincipal = layout->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    std::cout << "cWebApt\n" << std::endl;
    contPrincipal->setContentAlignment(Wt::AlignmentFlag::Top);// j'ai mis ça sur deux ou trois conteneur car sinon j'ai un bug avec la taille de la page "catalogue station" qui augmente sa taille à l'infini (voir post sur e layout manager for a container which does not have a height that is constrained somehow, you need to specify AlignTop in the alignment flags of WContainerWidget::setLayout().
    GDALAllRegister();

    setLoadingIndicator(std::make_unique<Wt::WOverlayLoadingIndicator>());
    loadingIndicator()->setMessage(Wt::WString::tr("defaultLoadingI"));

    // std::cout << "nombre de couleurs: " << mDico->colors.size() << std::endl;
    for (const auto & kv : mDico->colors){
        // std::cout << kv.first << ", " << kv.second->cat() << std::endl;
        color * col=kv.second.get();
        //std::cout << "add getStyleName() " << col.getStyleName() << std::endl;
        WCssDecorationStyle styleBgrd;
        styleBgrd.setBackgroundColor(WColor(col->mR,col->mG,col->mB));
        if (col->dark()){styleBgrd.setForegroundColor(WColor("white"));}
        styleSheet().addRule(col->getStyleName(), styleBgrd);
    }

    contPrincipal->addStyleClass("cWebAptitude");
    contPrincipal->setMargin(0);
    contPrincipal->setPadding(0);

    /*	NAVIGATION BAR	*/
    navigation = contPrincipal->addWidget(std::make_unique<WNavigationBar>());
    navigation->setResponsive(true);
    navigation->addStyleClass("carto_menu");
    navigation->setTitle("<strong>Forestimator</strong>"); // PL request !

    std::unique_ptr<WMenu> menu_ = std::make_unique<WMenu>();
    WMenu * navbar_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);

    // menu app
    menuitem_app = navbar_menu->addItem("resources/layers_filled_icon_149920.png","");
    menuitem_app->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    menuitem_app->setToolTip(Wt::WString::tr("menu.button.tooltip.carto"));
    // menu doc
    menuitem_documentation = navbar_menu->addItem("resources/problem_analysis_icon_149897.png","");
    menuitem_documentation->setLink(WLink(LinkType::InternalPath, "/documentation"));
    menuitem_documentation->setToolTip(Wt::WString::tr("menu.button.tooltip.doc"));
    // menu login
    menuitem_login = navbar_menu->addItem(isLoggedIn()?"resources/user_icon_logout.png":"resources/user_icon_149851.png","");
    menuitem_login->setToolTip(WString::tr("menu.button.tooltip.login"));
    menuitem_login->clicked().connect([=] {
        if(isLoggedIn()){
            menuitem_login->setIcon("resources/user_icon_149851.png");
            logout();
            dialog_auth->hide();
        }else{
            showDialogues(0); // les autres fenetres
            dialog_auth->show();
            dialog_auth->raiseToFront();
        }
    });

    // main stack : DOC and MAP and RESULTS divs
    top_stack  = contPrincipal->addWidget(Wt::cpp14::make_unique<Wt::WStackedWidget>());
    top_stack->setMargin(0);
    //top_stack->setHeight("100%");
    top_stack->setOverflow(Overflow::Auto);
    top_stack->addStyleClass("stackfit");
    // load DOC page
    presentationP= top_stack->addNew<presentationPage>(mDico,this);
    // load MAP page
    std::unique_ptr<WContainerWidget> content_app = std::make_unique<WContainerWidget>();
    content_app->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    content_app->addStyleClass("carto_div");
    WHBoxLayout * layout_app = content_app->setLayout(std::make_unique<WHBoxLayout>());
    layout_app->setContentsMargins(0,0,0,0);
    content_app->setHeight("100%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    content_app->setOverflow(Overflow::Visible); // non pas d'overflow pour la carte, qui est dans page_carto
    top_stack->addWidget(std::move(content_app));
    top_stack->setCurrentIndex(1);
    // load RESULT page TODO


    /*** page principale application map ***/

    /*	MAP	*/
    if (globTest){std::cout << "create map" << std::endl;}
    auto map = std::make_unique<WOpenLayers>(mDico);
    mMap = map.get();
    mMap->setWidth("100%");
    mMap->setMinimumSize(400,0);
    mMap->setOverflow(Overflow::Visible);
    layout_app->addWidget(std::move(map), 0);

    /*  Panel droit avec boutons et couches selectionnees */
    auto content_couches = layout_app->addWidget(std::make_unique<WContainerWidget>());
    content_couches->setId("content_couches");
    //content_couches->setOverflow(Overflow::Auto);
    content_couches->addStyleClass("content_couches");
    //load_content_couches(content_couches); // moved after mGroupL initialization !! non c'est juste la création du panier qu'il faut mettre après, le reste (création conteneur et layout) je dois le faire ici pour avoir mes Menuitem avant de créer class dialogu
    auto layoutD = content_couches->setLayout(std::make_unique<WHBoxLayout>());
    content_couches->setPadding(0);
    layoutD->setContentsMargins(0,0,0,0);

    auto menu_gauche = layoutD->addWidget(std::make_unique<WContainerWidget>());
    WContainerWidget * content_panier = layoutD->addWidget(std::make_unique<WContainerWidget>());

    menu_gauche->setWidth(60);
    menu_gauche->addStyleClass("menu_gauche");

    content_panier->addNew<Wt::WText>(WString::tr("panier.header"));
    content_panier->setWidth("100%");
    //content_panier->setId("content_panier");
    content_panier->setOverflow(Overflow::Scroll);

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
    menuitem_panier->setToolTip(WString::tr("menu.button.tooltip.panier_collapse"));
    // bouton catalogue
    menuitem_catalog = menu->addItem("resources/warehouse_check_icon_149849.png","");
    menuitem_catalog->setToolTip(WString::tr("menu.button.tooltip.catalog"));
    // bouton cadastre
    menuitem_cadastre = menu->addItem("resources/incoming_inspection_icon_149926.png","");
    menuitem_cadastre->setToolTip(WString::tr("menu.button.tooltip.cadastre"));
    // bouton legende
    menuitem_legend = menu->addItem("resources/product_bom_icon_149894.png","");
    menuitem_legend->setToolTip(WString::tr("menu.button.tooltip.legend"));
    // bouton info point
    menuitem_simplepoint = menu->addItem("resources/position-logo.png","");
    menuitem_simplepoint->setToolTip(WString::tr("menu.button.tooltip.point"));
    // bouton analyse
    menuitem_analyse = menu->addItem("resources/stat-logo.png","");
    menuitem_analyse->setToolTip(WString::tr("menu.button.tooltip.surf"));

    /*  DIALOGS info_point-légende-analyse-catalogue-cadastre */

    // info point
    dialog_info = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Info ponctuelle",menuitem_simplepoint,&environment()));

    auto content_info = dialog_info->contents()->addWidget(std::make_unique<WContainerWidget>());
    //content_info->setOverflow(Overflow::Auto);
    content_info->addStyleClass("content_info");

    // analyse
    dialog_anal = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Analyse surfacique",menuitem_analyse,&environment()));

    //auto content_anal = dialog_anal->contents()->addWidget(std::make_unique<WContainerWidget>());
    //content_anal->setOverflow(Overflow::Auto);
    //content_anal->addStyleClass("content_anal");

    // catalogue
    dialog_catalog = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Catalogue des couches",menuitem_catalog,&environment()));

    auto content_catalog = dialog_catalog->contents()->addWidget(std::make_unique<WContainerWidget>());
    content_catalog->addStyleClass("content_catalog");

    // cadastre
    dialog_cadastre = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Recherche cadastrale",menuitem_cadastre,&environment()));
    // legende
    dialog_legend = layout_app->addChild(Wt::cpp14::make_unique<dialog>("Légende",menuitem_legend,&environment()));
    mLegendW = dialog_legend->contents()->addWidget(std::make_unique<WContainerWidget>());
    mLegendW->addStyleClass("content_legend");

    widgetCadastre * content_cadastre;
    content_cadastre = dialog_cadastre->contents()->addWidget(std::make_unique<widgetCadastre>(mDico->mCadastre.get(),this));
    content_cadastre->addStyleClass("content_cadastre");

    mSimplepointW = content_info;
    mGroupLayerW  = content_catalog;
    mGroupLayerW->setStyleClass("content_GL");

    /* CHARGE ONGLET COUCHES & SIMPLEPOINT */
    if (globTest){ printf("create GL\n");}
    mGroupL = new groupLayers(this);
    //load_content_couches(content_couches);
    if (globTest){ printf("done\n");}

    mPanier = content_panier->addWidget(Wt::cpp14::make_unique<panier>(this));

    statWindow * page_camembert = top_stack->addNew<statWindow>(mGroupL);

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    mPA = dialog_anal->contents()->addWidget(std::make_unique<parcellaire>(mGroupL,this,page_camembert));
    mPA->addStyleClass("content_analyse");

    mGroupL->updateGL();// updaGL utilise des pointeurs d'autres classe, donc je dois le faire après avoir instancier toutes les autres classes... StatWindows, SimplePoint et parcellaire
    // des couches que l'on souhaite voir dans le panier dès le départ
    mGroupL->clickOnName("IGN");

    /*	ACTIONS	: on connect les events aux méthodes	*/
    // dans wt_config, mettre à 500 milliseconde au lieu de 200 pour le double click
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));
    mMap->xySelect().connect(std::bind(&parcellaire::selectPolygon,mPA, std::placeholders::_1,std::placeholders::_2));
    //mMap->polygId().connect(std::bind(&parcellaire::computeStatAndVisuSelectedPol,mPA, std::placeholders::_1));
    //mMap->getMapExtendSignal().connect(std::bind(&WOpenLayers::updateMapExtend,mMap,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    //cadastre
    content_cadastre->sendPolygone().connect(std::bind(&parcellaire::polygoneCadastre,mPA,std::placeholders::_1,std::placeholders::_2));

    internalPathChanged().connect(this, &cWebAptitude::handlePathChange);
    // force first route
    //setInternalPath(ointernalPath);
    handlePathChange();

    root()->addStyleClass("layout_main");
    loaded_=true;

    /*
    std::string coName="invitationCF";
    if (!env.getCookie(coName)){
    Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                      "9ieme Carrefour forestier",
                                                      WString::tr("insertCF"),
                                                      Wt::Icon::None,
                                                      Wt::StandardButton::Ok));
    messageBox->setHeight("100%");
    messageBox->setWidth("50%");
    messageBox->contents()->setOverflow(Overflow::Scroll);
    messageBox->setModal(true);
    messageBox->buttonClicked().connect([=] {
        this->removeChild(messageBox);
        // une semaine
        Http::Cookie coCF(coName, "1", std::chrono::seconds(604800));
        this->setCookie(coCF);
    });
    messageBox->show();
    }*/
    clientIDcookies();
}

void cWebAptitude::loadStyles(){
    std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);
    // tout les style de wt gallery
    useStyleSheet("resources/themes/default/wt.css");
    useStyleSheet("style/everywidget.css");
    useStyleSheet("style/dragdrop.css");
    useStyleSheet("style/combostyle.css");
    useStyleSheet("style/pygments.css");
    useStyleSheet("style/filedrop.css");
    useStyleSheet("style/form.css");
    useStyleSheet("resources/jPlayer/skin/jplayer.blue.monday.css");
    useStyleSheet("style/widgetgallery.css");
    // CSS custom pour faire beau
    useStyleSheet("style/style.css");

    WCssDecorationStyle EssStyle;
    EssStyle.font().setSize(FontSize::Medium);
    EssStyle.setCursor(Cursor::PointingHand);
    EssStyle.setForegroundColor(WColor("gray"));
    styleSheet().addRule(".ess", EssStyle);

    EssStyle.font().setSize(FontSize::Smaller);
    styleSheet().addRule(".tree", EssStyle);
    EssStyle.font().setSize(FontSize::Medium);

    EssStyle.setForegroundColor(WColor("black"));
    EssStyle.setTextDecoration(TextDecoration::Underline);
    EssStyle.setCursor(Cursor::Arrow);
    EssStyle.font().setWeight(FontWeight::Bold);
    styleSheet().addRule(".currentEss", EssStyle);

    // init the OpenLayers javascript api
    require("jslib/v6.4.3-dist/ol.js");
    useStyleSheet("jslib/v6.4.3-dist/ol.css");
    require("jslib/proj4js-2.6.1/dist/proj4.js");
    require("jslib/proj4js-2.6.1/dist/proj4-src.js");
    enableUpdates();
}

std::unique_ptr<Wt::Auth::AuthWidget> cWebAptitude::loadAuthWidget(){
    // auth widget (login)
    if (globTest){printf("Auth widget...");}
    session_.login().changed().connect(this, &cWebAptitude::authEvent);
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget_ = std::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget=authWidget_.get();
    authWidget->model()->addPasswordAuth(&session_.passwordAuth());
    //authWidget_->model()->addOAuth(Session::oAuth()); // no need google and facebook
    authWidget->setRegistrationEnabled(true);
    authWidget->processEnvironment();
    authWidget->addStyleClass("Wt-auth-login-container");
    //authWidget->setInternalPathEncoding(true);
    //authWidget->setInternalBasePath("/auth");
    //authWidget->

    if (globTest){printf("done\n");}
    return authWidget_;
}

void cWebAptitude::authEvent() {
    std::cout << "autEvent..." << std::endl;
    if(!loaded_)return;
    if (session_.login().loggedIn()) {
        const Wt::Auth::User& u = session_.login().user();
        log("notice")
                << "User " << u.id()
                << " (" << u.identity(Wt::Auth::Identity::LoginName) << ")"
                << " logged in. (2)";
        if(loaded_){
            menuitem_login->setIcon("resources/user_icon_logout.png");
            mGroupL->updateGL();
            dialog_auth->hide();
        }
    } else{
        log("notice") << "User logged out.";
        if(loaded_){
            menuitem_login->setIcon("resources/user_icon_149851.png");
            mGroupL->updateGL();
        }
    }
    showDialogues(1);
}

bool cWebAptitude::isLoggedIn(){
    return session_.login().loggedIn();
}

void cWebAptitude::logout(){
    session_.login().logout();
}

Wt::Auth::User cWebAptitude::getUser(){
    return session_.login().user();
}

void cWebAptitude::addLog(std::string page, typeLog cat){
    // check pour ne pas ajouter dix fois sur la mm journée et pour le même utilisateur un log identique.
    if (!mAnal.logExist(this->environment(),page, cat)){
        mAnal.addLog(this->environment(), page,(int (cat)));
    }
}

void cWebAptitude::clientIDcookies(){
    if (!environment().getCookie(cookies_clientIDName)){
        int16_t bitNum = (int16_t)rand()%0x10000;
        Http::Cookie coClientID(cookies_clientIDName, std::to_string(bitNum), std::chrono::seconds(604800));
        setCookie(coClientID);
        //if (globTest) {std::cout << "cookies set " << bitNum << std::endl;}
        /*} else {
         if (globTest) {std::cout << "cookies get " << *environment().getCookie(cookies_clientIDName) << std::endl;}
    */}
}
