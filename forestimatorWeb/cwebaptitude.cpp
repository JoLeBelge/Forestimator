#include "cwebaptitude.h"
#include "grouplayers.h"
#include "presentationpage.h"
#include "panier.h"
#include "wopenlayers.h"
#include "statwindow.h"

extern bool globTest;
std::string cookies_clientIDName="clientID";

void cWebAptitude::handlePathChange()
{
    bool baddLog(1);
    std::string docInternalP("/documentation");
    std::size_t found = internalPath().find(docInternalP);
    if (internalPath() == "/documentation" || found!=std::string::npos){
        top_stack->setCurrentIndex(0);
        menuitem_documentation->select();

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
            mShow=1;
            this->myshow();
            mMenu->decorationStyle().setBackgroundColor(col_sel);
            this->raiseToFront();
        }
    });
}


cWebAptitude::cWebAptitude(const Wt::WEnvironment& env, cDicoApt *dico)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db"),mDico(dico),mAnal(dico->File("docroot")+"analytics.db"),activeLayerCode("IGN")
{

    messageResourceBundle().use(docRoot() + "/forestimator");
    messageResourceBundle().use(docRoot() + "/forestimator-documentation");
    messageResourceBundle().use(docRoot() + "/forestimator-CS");

    setTitle("Forestimator");
    loadStyles();
    root()->addStyleClass("layout_main");

    auto layout = root()->setLayout(std::make_unique<Wt::WVBoxLayout>());

    dialog_auth = layout->addChild(std::make_unique<Wt::WDialog>("Connexion"));
    dialog_auth->setResizable(true);
    dialog_auth->setModal(true);
    dialog_auth->setMaximumSize(700,600);
    dialog_auth->setWidth(600);
    dialog_auth->setHeight(500);
    dialog_auth->contents()->setOverflow(Overflow::Scroll);
    dialog_auth->setClosable(true);
    dialog_auth->contents()->addWidget(std::move(loadAuthWidget()));

    Wt::WContainerWidget* contPrincipal = layout->addWidget(std::make_unique<Wt::WContainerWidget>());
    contPrincipal->setContentAlignment(Wt::AlignmentFlag::Top);// j'ai mis ça sur deux ou trois conteneur car sinon j'ai un bug avec la taille de la page "catalogue station" qui augmente sa taille à l'infini (voir post sur e layout manager for a container which does not have a height that is constrained somehow, you need to specify AlignTop in the alignment flags of WContainerWidget::setLayout().
    GDALAllRegister();

    setLoadingIndicator(std::make_unique<Wt::WOverlayLoadingIndicator>());
    loadingIndicator()->setMessage(Wt::WString::tr("defaultLoadingI"));

    for (const auto & kv : mDico->colors){
        color * col=kv.second.get();
        WCssDecorationStyle styleBgrd;
        styleBgrd.setBackgroundColor(WColor(col->mR,col->mG,col->mB));
        if (col->dark()){styleBgrd.setForegroundColor(WColor("white"));}
        styleSheet().addRule(col->getStyleName(), styleBgrd);
    }

    contPrincipal->addStyleClass("cWebAptitude");

    /*	NAVIGATION BAR	*/
    navigation = contPrincipal->addWidget(std::make_unique<WNavigationBar>());
    navigation->setResponsive(true);
    navigation->addStyleClass("carto_menu");
    navigation->setTitle("<strong>Forestimator</strong>");

    std::unique_ptr<WMenu> menu_ = std::make_unique<WMenu>();
    WMenu * navbar_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);

    // menu app
    menuitem_app = navbar_menu->addItem("resources/layers_filled_icon_149920.png","");
    menuitem_app->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    menuitem_app->setToolTip(Wt::WString::tr("menu.button.tooltip.carto"));
    // menu doc
    menuitem_documentation = navbar_menu->addItem("resources/problem_analysis_icon_149897.png","");
    //menuitem_documentation->setLink(WLink(LinkType::Url, "https://forestimator.gembloux.ulg.ac.be/documentation"));
    WLink l= WLink(LinkType::Url, "https://forestimator.gembloux.ulg.ac.be/documentation");
    //l.setTarget(LinkTarget::NewWindow);
    menuitem_documentation->setLink(l);
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
    top_stack  = contPrincipal->addWidget(std::make_unique<Wt::WStackedWidget>());
    top_stack->setOverflow(Overflow::Auto);
    top_stack->addStyleClass("stackfit");
    // load DOC page
    top_stack->addWidget(std::make_unique<presentationPage>(mDico,this));
    // load MAP page

    Wt::WTemplate * tpl_content_app = top_stack->addWidget(std::make_unique<Wt::WTemplate>(WString::tr("template.carto_div")));

    mMap = tpl_content_app->bindWidget("WOpenLayers", std::make_unique<WOpenLayers>(mDico));
    WMenu * menu = tpl_content_app->bindWidget("menu", std::make_unique<WMenu>());

    menu->setStyleClass("nav-stacked");
    menu->addStyleClass("nav-apt");
    /*
    menuitem_panier = menu->addItem("resources/right_angle_circle_icon_149877.png","");
    menuitem_panier->clicked().connect([=] {
        std::cout << content_couches->width().value() << std::endl;
        if(content_couches->width().value()>60 || content_couches->width().value()==-1){
            //content_couches->setWidth(60);
            menuitem_panier->setIcon("resources/right_angle_circle_icon_149877d.png");
            mPanier->hide();
        }else{
            //content_couches->setWidth(400);
            menuitem_panier->setIcon("resources/right_angle_circle_icon_149877.png");
            mPanier->show();
        }
    });
    menuitem_panier->setToolTip(WString::tr("menu.button.tooltip.panier_collapse"));
    */
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
    dialog_info = tpl_content_app->addChild(std::make_unique<dialog>("Info ponctuelle",menuitem_simplepoint,&environment()));

    mAnaPoint = dialog_info->contents()->addWidget(std::make_unique<simplepoint>(this));

    // analyse
    dialog_anal = tpl_content_app->addChild(std::make_unique<dialog>("Analyse surfacique",menuitem_analyse,&environment()));
    // catalogue
    dialog_catalog = tpl_content_app->addChild(std::make_unique<dialog>("Catalogue des couches",menuitem_catalog,&environment()));

    dialog_catalog->contents()->setStyleClass("content_GL");
    //auto content_catalog = dialog_catalog->contents()->addWidget(std::make_unique<WContainerWidget>());
    //content_catalog->addStyleClass("content_catalog");

    // cadastre
    dialog_cadastre = tpl_content_app->addChild(std::make_unique<dialog>("Recherche cadastrale",menuitem_cadastre,&environment()));
    // legende
    dialog_legend = tpl_content_app->addChild(std::make_unique<dialog>("Légende",menuitem_legend,&environment()));
    mLegendW = dialog_legend->contents()->addWidget(std::make_unique<WContainerWidget>());
    mLegendW->addStyleClass("content_legend");

    widgetCadastre * content_cadastre;
    content_cadastre = dialog_cadastre->contents()->addWidget(std::make_unique<widgetCadastre>(mDico->mCadastre.get(),this));
    content_cadastre->addStyleClass("content_cadastre");

    /* CHARGE ONGLET COUCHES & SIMPLEPOINT */
    if (globTest){ printf("create GL\n");}
    mGroupL = dialog_catalog->contents()->addWidget(std::make_unique<groupLayers>(this));
    if (globTest){printf("done\n");}

    mPanier = tpl_content_app->bindWidget("panier", std::make_unique<panier>(this));

    statWindow * page_camembert = top_stack->addWidget(std::make_unique<statWindow>(this));

    /* CHARGE ONGLET ANALYSES */
    //printf("create PA\n");
    mPA = dialog_anal->contents()->addWidget(std::make_unique<parcellaire>(mGroupL,this,page_camembert));
    mPA->addStyleClass("content_analyse");

    mGroupL->updateGL();
    mGroupL->clickOnName("IGN");

    /*	ACTIONS	: on connect les events aux méthodes	*/
    mMap->xy().connect(std::bind(&simplepoint::extractInfo,mAnaPoint, std::placeholders::_1,std::placeholders::_2));
    mMap->xySelect().connect(std::bind(&parcellaire::selectPolygon,mPA, std::placeholders::_1,std::placeholders::_2));
    //cadastre
    content_cadastre->sendPolygone().connect(std::bind(&parcellaire::polygoneCadastre,mPA,std::placeholders::_1,std::placeholders::_2));
    internalPathChanged().connect(this, &cWebAptitude::handlePathChange);

    handlePathChange();

    loaded_=true;
    top_stack->setCurrentIndex(1);

    clientIDcookies();

    // avant c'était dans wopenwidget
    std::ifstream t(mDico->File("initOL"));
    std::stringstream ss;
    ss << t.rdbuf();
    t.close();
    doJavaScript(ss.str(),false);
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


    require("jslib/v10.8.0-package/dist/ol.js");
    useStyleSheet("jslib/v10.8.0-package/ol.css");
    require("jslib/proj4js-2.20.3-dist/proj4.js");
    require("jslib/proj4js-2.20.3-dist/proj4-src.js");
    enableUpdates();
}

std::unique_ptr<Wt::Auth::AuthWidget> cWebAptitude::loadAuthWidget(){
    session_.login().changed().connect(this, &cWebAptitude::authEvent);
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget_ = std::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget=authWidget_.get();
    authWidget->model()->addPasswordAuth(&session_.passwordAuth());
    //authWidget_->model()->addOAuth(Session::oAuth()); // no need google and facebook
    authWidget->setRegistrationEnabled(true);
    authWidget->processEnvironment();
    authWidget->addStyleClass("Wt-auth-login-container");
    return authWidget_;
}

void cWebAptitude::authEvent() {
    if (globTest){std::cout << "autEvent..." << std::endl;}
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
    }
}

