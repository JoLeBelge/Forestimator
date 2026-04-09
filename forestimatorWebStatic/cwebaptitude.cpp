#include "cwebaptitude.h"

extern bool globTest;
std::string cookies_clientIDName = "clientID";

/*
 * Redirections en fonction du internal path
 * (ne pas oublier de compléter dans le main.cpp si internalpath changés - S.Q.)
 */
void cWebAptitude::handlePathChange()
{
    bool baddLog(1);
    std::string docInternalP("/documentation");
    std::size_t found = internalPath().find(docInternalP);
    if (internalPath() == "/documentation" | found != std::string::npos)
    {
        top_stack->setCurrentIndex(0);
        menuitem_documentation->select();

        // une description différentes pour chaques page de documentation. Il faut préhalablement retirer la description, sinon ça n'aura pas d'effet
        // fonctionne pas si session avec javascript, mais pour les robots ça va quand-même. Sur mon navigateur, le meta descr c'est celui de la première page consultée de la session
        // check si j'ai un header description pour cette sous-section, sinon celui général à la page documentation
        if (globTest)
        {
            std::cout << "handlepathChange documentation : " << internalPath() << std::endl;
        }
        if (found != std::string::npos && found + docInternalP.length() + 1 < internalPath().size())
        { // test sur la longueur car il faut gérer le cas ou on a juste un backslach : /documentation/
            std::string sectionDoc = internalPath().erase(0, found + docInternalP.length() + 1);
            // retirer l'éventuel baslach en fin de url
            if (sectionDoc.back() == '/')
            {
                // sectionDoc.pop_back();
                sectionDoc.erase(sectionDoc.size() - 1);
            }

            // sectionDoc.erase(std::remove(sectionDoc.begin(), sectionDoc.end(), '/'), sectionDoc.end());
            if (globTest)
            {
                std::cout << "section doc : " << sectionDoc << std::endl;
            }
            changeHeader(sectionDoc);
        }
        else
        {
            changeHeader("documentation");
        }
        showDialogues(0);
    }
    else if (internalPath() == "/cartographie" || internalPath() == "/" || internalPath() == "")
    {
        top_stack->setCurrentIndex(1);
        menuitem_app->select();
        removeMetaHeader(MetaHeaderType::Meta, "description");
        addMetaHeader(MetaHeaderType::Meta, "description", Wt::WString::tr("meta.desc.carto"), "fr");
        showDialogues(1);
    }
    else if (internalPath() == "/resultat")
    {
        top_stack->setCurrentIndex(2);
        showDialogues(0);
    }
    else
    {
        std::cout << "m_app->internalPath() " << internalPath() << std::endl;
        std::cout << "internal path pas geré dans le handler " << internalPath() << std::endl;
        baddLog = 0;
    }

    if (baddLog)
    {
        addLog(internalPath());
    }

    // TODO css min-size [menu_analyse] display:none if width<900
    // TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;
}

void cWebAptitude::changeHeader(std::string aSection)
{
    std::string message(WString::tr("meta.desc." + aSection).toUTF8());
    if (message.substr(0, 2) != "??")
    {
        removeMetaHeader(MetaHeaderType::Meta, "description");
        addMetaHeader(MetaHeaderType::Meta, "description", message, "fr");
    }
    message = WString::tr("meta.titre." + aSection).toUTF8();
    if (message.substr(0, 2) != "??")
    {
        removeMetaHeader(MetaHeaderType::Meta, "titre");
        addMetaHeader(MetaHeaderType::Meta, "titre", message, "fr");
    }
}

dialog::dialog(const WString &windowTitle, Wt::WMenuItem *aMenu, const WEnvironment *env) : WDialog(windowTitle), mMenu(aMenu), mShow(0), env_(env)
{

    setResizable(true);
    setModal(false);

    contents()->setOverflow(Overflow::Scroll);
    setClosable(true);

    // faire les liens entre le boutton menu et l'affichage de la fenetre
    finished().connect([=]
                       {mMenu->decorationStyle().setBackgroundColor(col_not_sel); mShow=0; });

    mMenu->clicked().connect([=]
                             {
        if(this->isVisible()){
            this->hide();
            mMenu->decorationStyle().setBackgroundColor(col_not_sel);
            mShow=0;
        }else{
            mShow=1;
            this->myshow();
            mMenu->decorationStyle().setBackgroundColor(col_sel);
            this->raiseToFront();
        } });
}

cWebAptitude::cWebAptitude(const Wt::WEnvironment &env, cDicoApt *dico)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db"), mDico(dico), mAnal(dico->File("docroot") + "analytics.db")
{

    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    messageResourceBundle().use(docRoot() + "/forestimator");
    messageResourceBundle().use(docRoot() + "/forestimator-documentation");
    messageResourceBundle().use(docRoot() + "/forestimator-CS");

    setTitle("Forestimator");
    loadStyles();

    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    root()->setMargin(0);
    root()->setPadding(0);

    dialog_auth = layout->addChild(Wt::cpp14::make_unique<Wt::WDialog>("Connexion"));
    dialog_auth->setResizable(true);
    dialog_auth->setModal(true);
    dialog_auth->setMaximumSize(700, 600);
    dialog_auth->setWidth(600);
    dialog_auth->setHeight(500);
    dialog_auth->contents()->setOverflow(Overflow::Scroll);
    dialog_auth->setClosable(true);
    dialog_auth->contents()->addWidget(std::move(loadAuthWidget()));

    Wt::WContainerWidget *contPrincipal = layout->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    contPrincipal->setContentAlignment(Wt::AlignmentFlag::Top); // j'ai mis ça sur deux ou trois conteneur car sinon j'ai un bug avec la taille de la page "catalogue station" qui augmente sa taille à l'infini (voir post sur e layout manager for a container which does not have a height that is constrained somehow, you need to specify AlignTop in the alignment flags of WContainerWidget::setLayout().
    GDALAllRegister();

    setLoadingIndicator(std::make_unique<Wt::WOverlayLoadingIndicator>());
    loadingIndicator()->setMessage(Wt::WString::tr("defaultLoadingI"));

    for (const auto &kv : mDico->colors)
    {
        color *col = kv.second.get();
        WCssDecorationStyle styleBgrd;
        styleBgrd.setBackgroundColor(WColor(col->mR, col->mG, col->mB));
        if (col->dark())
        {
            styleBgrd.setForegroundColor(WColor("white"));
        }
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
    WMenu *navbar_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);

    // menu app
    menuitem_app = navbar_menu->addItem("resources/layers_filled_icon_149920.png", "");
    menuitem_app->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    menuitem_app->setToolTip(Wt::WString::tr("menu.button.tooltip.carto"));
    // menu doc
    menuitem_documentation = navbar_menu->addItem("resources/problem_analysis_icon_149897.png", "");
    menuitem_documentation->setLink(WLink(LinkType::InternalPath, "/documentation"));
    menuitem_documentation->setToolTip(Wt::WString::tr("menu.button.tooltip.doc"));
    // menu login
    menuitem_login = navbar_menu->addItem(isLoggedIn() ? "resources/user_icon_logout.png" : "resources/user_icon_149851.png", "");
    menuitem_login->setToolTip(WString::tr("menu.button.tooltip.login"));
    menuitem_login->clicked().connect([=]
                                      {
        if(isLoggedIn()){
            menuitem_login->setIcon("resources/user_icon_149851.png");
            logout();
            dialog_auth->hide();
        }else{
            showDialogues(0); // les autres fenetres
            dialog_auth->show();
            dialog_auth->raiseToFront();
        } });

    // main stack : DOC and MAP and RESULTS divs
    top_stack = contPrincipal->addWidget(Wt::cpp14::make_unique<Wt::WStackedWidget>());
    top_stack->setMargin(0);
    // top_stack->setHeight("100%");
    top_stack->setOverflow(Overflow::Auto);
    top_stack->addStyleClass("stackfit");
    // load DOC page
    presentationP = top_stack->addNew<presentationPage>(mDico, this);
    // load MAP page
    top_stack->setCurrentIndex(0);
    // load RESULT page TODO

    /*** page principale application map ***/

    auto menu_gauche = layoutD->addWidget(std::make_unique<WContainerWidget>());
    WContainerWidget *content_panier = layoutD->addWidget(std::make_unique<WContainerWidget>());

    menu_gauche->setWidth(60);
    menu_gauche->addStyleClass("menu_gauche");

    content_panier->addNew<Wt::WText>(WString::tr("panier.header"));
    content_panier->setWidth("100%");
    // content_panier->setId("content_panier");
    content_panier->setOverflow(Overflow::Scroll);

    internalPathChanged().connect(this, &cWebAptitude::handlePathChange);
    // force first route
    // setInternalPath(ointernalPath);
    handlePathChange();

    root()->addStyleClass("layout_main");
    loaded_ = true;
}

void cWebAptitude::loadStyles()
{
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
    require("jslib/v10.8.0-package/dist/ol.js");
    useStyleSheet("jslib/v10.8.0-package/ol.css");
    require("jslib/proj4js-2.20.3-dist/proj4.js");
    require("jslib/proj4js-2.20.3-dist/proj4-src.js");
    enableUpdates();
}

std::unique_ptr<Wt::Auth::AuthWidget> cWebAptitude::loadAuthWidget()
{
    session_.login().changed().connect(this, &cWebAptitude::authEvent);
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget_ = std::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget = authWidget_.get();
    authWidget->model()->addPasswordAuth(&session_.passwordAuth());
    // authWidget_->model()->addOAuth(Session::oAuth()); // no need google and facebook
    authWidget->setRegistrationEnabled(true);
    authWidget->processEnvironment();
    authWidget->addStyleClass("Wt-auth-login-container");
    return authWidget_;
}

void cWebAptitude::authEvent()
{
    if (globTest)
    {
        std::cout << "autEvent..." << std::endl;
    }
    if (!loaded_)
        return;
    if (session_.login().loggedIn())
    {
        const Wt::Auth::User &u = session_.login().user();
        log("notice")
            << "User " << u.id()
            << " (" << u.identity(Wt::Auth::Identity::LoginName) << ")"
            << " logged in. (2)";
        if (loaded_)
        {
            menuitem_login->setIcon("resources/user_icon_logout.png");
            mGroupL->updateGL();
            dialog_auth->hide();
        }
    }
    else
    {
        log("notice") << "User logged out.";
        if (loaded_)
        {
            menuitem_login->setIcon("resources/user_icon_149851.png");
            mGroupL->updateGL();
        }
    }
    showDialogues(1);
}

bool cWebAptitude::isLoggedIn()
{
    return session_.login().loggedIn();
}

void cWebAptitude::logout()
{
    session_.login().logout();
}

Wt::Auth::User cWebAptitude::getUser()
{
    return session_.login().user();
}

void cWebAptitude::addLog(std::string page, typeLog cat)
{
    // check pour ne pas ajouter dix fois sur la mm journée et pour le même utilisateur un log identique.
    if (!mAnal.logExist(this->environment(), page, cat))
    {
        mAnal.addLog(this->environment(), page, (int(cat)));
    }
}

void cWebAptitude::clientIDcookies()
{
    if (!environment().getCookie(cookies_clientIDName))
    {
        int16_t bitNum = (int16_t)rand() % 0x10000;
        Http::Cookie coClientID(cookies_clientIDName, std::to_string(bitNum), std::chrono::seconds(604800));
        setCookie(coClientID);
    }
}
