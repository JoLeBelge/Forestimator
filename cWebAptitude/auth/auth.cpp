#pragma once
#include "auth.h"
#include "cwebaptitude.h"

    using namespace std;
    using namespace Wt;

  AuthApplication::AuthApplication(const Wt::WEnvironment& env)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db")
  {
    setTitle("Forestimator");
    // thème bootstrap 3
    auto theme = std::make_shared<Wt::WBootstrapTheme>();
    theme->setVersion(Wt::BootstrapVersion::v3);
    //spécifier ça également dans wt_option.xml sinon ne fonctionne pas
    theme->setResponsive(true);
    setTheme(theme);

    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    messageResourceBundle().use(docRoot() + "/forestimator");

    session_.login().changed().connect(this, &AuthApplication::authEvent);
    // auth widget (login)
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget = cpp14::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget_=authWidget.get();
    authWidget_->model()->addPasswordAuth(&session_.passwordAuth());
    authWidget_->model()->addOAuth(Session::oAuth());
    authWidget_->setRegistrationEnabled(true);
    authWidget_->processEnvironment();
    authWidget_->addStyleClass("Wt-auth-login-container");

    // load the default bootstrap3 (sub-)theme (nécessaire en plus de theme->setVersion)
    useStyleSheet("style/bootstrap-theme.min.css");
    // tout les style de wt gallery
    useStyleSheet("style/everywidget.css");
    useStyleSheet("style/dragdrop.css");
    useStyleSheet("style/combostyle.css");
    useStyleSheet("style/pygments.css");
    //useStyleSheet("style/layout.css");
    useStyleSheet("style/filedrop.css");
    //useStyleSheet("style/home.css");
    useStyleSheet("style/wt.css");
    useStyleSheet("style/form.css");
    useStyleSheet("resources/themes/polished/wt.css");
    //useStyleSheet("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/css/ol.css");

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
    // avec ol 6, la carte met du temps à s'afficher. je reste à ol 4 pour le moment.
    //std::string ol("https://cdn.rawgit.com/openlayers/openlayers.github.io/master/en/v6.2.1/build/ol.js");
    // finalement le 6 fonctionee parfaitement, alors pourquoi rester au 4
    std::string ol("jslib/v6.3.1-dist/ol.js");
    //std::string ol("jslib/ol4.6.4-debug.js");
    require(ol);
    useStyleSheet("jslib/v6.3.1-dist/ol.css");
    require("jslib/proj4js-2.6.1/dist/proj4.js");
    require("jslib/proj4js-2.6.1/dist/proj4-src.js");

    // ajout d'un control print pour sauver en jpg les cartes ol - non plutôt voir les exemples récents de ol pour l'export
    //require("jslib/ol-ext.min.js");
    //require("jslib/FileSaver.js/dist/FileSaver.js");
    //useStyleSheet("jslib/ol-ext.min.css");

    // CSS custom pour faire beau
    useStyleSheet("style/style.css");

    enableUpdates();

    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    layout->addWidget(std::move(authWidget));
    //authWidget_->hide();

    std::unique_ptr<cWebAptitude> WebApt = Wt::cpp14::make_unique<cWebAptitude>(this, authWidget_);
    cwebapt = layout->addWidget(std::move(WebApt), 0);
    //layout->setContentsMargins(0,0,0,0);
    //root()->setPadding(0);
    root()->addStyleClass("layout_main");
    loaded_=true;
  }

  void AuthApplication::authEvent() {
      if (session_.login().loggedIn()) {
          const Wt::Auth::User& u = session_.login().user();

          bool modeExpert= cwebapt->mGroupL->getExpertModeForUser(u.id());

          log("notice")
                  << "User " << u.id()
                  << " (" << u.identity(Wt::Auth::Identity::LoginName) << ")"
                  << " logged in. (2)";
          if(loaded_){
              cwebapt->b_login->setText("Se déconnecter");
              std::string JScommand("$('.Wt-auth-login-container').removeClass('visible').addClass('nonvisible');");
              doJavaScript(JScommand);
              cwebapt->mGroupL->updateGL(modeExpert);
          }
      } else{
          log("notice") << "User logged out.";
          if(loaded_){
              cwebapt->b_login->setText("Se connecter");
              cwebapt->mGroupL->updateGL();
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
