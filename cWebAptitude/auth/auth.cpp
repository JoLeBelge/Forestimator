#pragma once
#include "auth.h"
#include "cwebaptitude.h"

using namespace std;
using namespace Wt;

AuthApplication::AuthApplication(const Wt::WEnvironment& env, cDicoApt *dico, Analytics *anal)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db"),mDico(dico),mAnal(anal)
{
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    messageResourceBundle().use(docRoot() + "/forestimator");

    setTitle("Forestimator");

    loadStyles();

    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    root()->setMargin(0);
    root()->setPadding(0);

    layout->addWidget(std::move(loadAuthWidget()));

    cWebApt = layout->addWidget(Wt::cpp14::make_unique<cWebAptitude>(this, authWidget_));

    // stats web
    if (session_.login().loggedIn()) {
        const Wt::Auth::User& u = session_.login().user();
        anal->addLog(env,atol(u.id().c_str()));
    }else
        anal->addLog(env);

    root()->addStyleClass("layout_main");
    loaded_=true;
}

void AuthApplication::loadStyles(){
    // thème bootstrap 3
    auto theme = std::make_shared<Wt::WBootstrapTheme>();
    theme->setVersion(Wt::BootstrapVersion::v3);
    //spécifier ça également dans wt_option.xml sinon ne fonctionne pas
    theme->setResponsive(true);
    setTheme(theme);

    // load the default bootstrap3 (sub-)theme (nécessaire en plus de theme->setVersion)
    useStyleSheet("style/bootstrap-theme.min.css");
    // tout les style de wt gallery
    useStyleSheet("style/everywidget.css");
    useStyleSheet("style/dragdrop.css");
    useStyleSheet("style/combostyle.css");
    useStyleSheet("style/pygments.css");
    useStyleSheet("style/filedrop.css");
    useStyleSheet("style/wt.css");
    useStyleSheet("style/form.css");
    useStyleSheet("resources/themes/polished/wt.css");

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

    WCssDecorationStyle BoldStyle;
    BoldStyle.font().setSize(FontSize::XSmall);
    BoldStyle.setForegroundColor(WColor("black"));
    BoldStyle.font().setWeight(FontWeight::Bold);
    styleSheet().addRule(".bold", BoldStyle);

    // init the OpenLayers javascript api
    require("jslib/v6.4.3-dist/ol.js");
    useStyleSheet("jslib/v6.4.3-dist/ol.css");
    require("jslib/proj4js-2.6.1/dist/proj4.js");
    require("jslib/proj4js-2.6.1/dist/proj4-src.js");

    // CSS custom pour faire beau
    useStyleSheet("style/style.css");

    enableUpdates();
}

std::unique_ptr<Wt::Auth::AuthWidget> AuthApplication::loadAuthWidget(){
    // auth widget (login)
    printf("Auth widget...");
    session_.login().changed().connect(this, &AuthApplication::authEvent);
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget = cpp14::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget_=authWidget.get();
    authWidget_->model()->addPasswordAuth(&session_.passwordAuth());
    //authWidget_->model()->addOAuth(Session::oAuth()); // no need google and facebook
    authWidget_->setRegistrationEnabled(true);
    authWidget_->processEnvironment();
    authWidget_->addStyleClass("Wt-auth-login-container");
    authWidget_->addStyleClass("nonvisible");
    printf("done\n");
    return authWidget;
}

void AuthApplication::authEvent() {
    std::cout << "autEvent..." << std::endl;
    if(!loaded_)return;
    if (session_.login().loggedIn()) {
        const Wt::Auth::User& u = session_.login().user();
        log("notice")
                << "User " << u.id()
                << " (" << u.identity(Wt::Auth::Identity::LoginName) << ")"
                << " logged in. (2)";
        if(loaded_){
            cWebApt->menuitem_login->setIcon("resources/user_icon_logout.png");
            std::string JScommand("$('.Wt-auth-login-container').removeClass('visible').addClass('nonvisible');");
            doJavaScript(JScommand);
            cWebApt->mGroupL->updateGL();
        }
    } else{
        log("notice") << "User logged out.";
        if(loaded_){
            cWebApt->menuitem_login->setIcon("resources/user_icon_149851.png");
            cWebApt->mGroupL->updateGL();
        }
    }
}

bool AuthApplication::isLoggedIn(){
    return session_.login().loggedIn();
}

void AuthApplication::logout(){
    session_.login().logout();
}

Wt::Auth::User AuthApplication::getUser(){
    return session_.login().user();
}

void AuthApplication::addLog(std::string page){
    if (session_.login().loggedIn()) {
        const Wt::Auth::User& u = session_.login().user();
        mAnal->addLog(this->environment(),atol(u.id().c_str()), page);
    }else
        mAnal->addLog(this->environment(), page);
}
