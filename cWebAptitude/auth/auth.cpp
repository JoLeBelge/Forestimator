#pragma once
#include "auth.h"
#include "cwebaptitude.h"

using namespace std;
using namespace Wt;
extern bool globTest;

AuthApplication::AuthApplication(const Wt::WEnvironment& env, cDicoApt *dico)
    : Wt::WApplication(env),
      session_(docRoot() + "/auth.db"),mDico(dico),mAnal(dico->File("docroot")+"analytics.db")
{
    // charge le xml avec tout le texte qui sera chargé via la fonction tr()
    messageResourceBundle().use(docRoot() + "/forestimator");
    messageResourceBundle().use(docRoot() + "/forestimator-documentation");


    // export de tout les messages html vers un fichier csv qui sera traduit en text avec ./html2text -from_encoding UTF8 -nobs -o /home/jo/app/Forestimator/data/tmp/Forestimator.txt /home/jo/app/Forestimator/data/tmp/texteForestimator.csv pour correction orthographique
    bool forestimator(0);
    if (globTest){
          std::cout << " tata!!!!\n\n\n" << std::endl;
        std::ifstream theFile;
        std::string aFile(mDico->File("TMPDIR")+"texteForestimator.csv");
        if (forestimator){

            theFile.open(docRoot() + "/forestimator.xml");} else {
            std::cout << " toto !!!!\n\n\n" << std::endl;
            aFile=mDico->File("TMPDIR")+"textePhytospy.csv";
            theFile.open("/home/jo/app/phytospy/data/phytoTool.xml");
            messageResourceBundle().use("/home/jo/app/phytospy/data/phytoTool");
        }

        std::ofstream aOut;
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
                if (aId.find("Wt.Auth")==std::string::npos){
                    aOut << WText::tr(aId).toUTF8() ;
                    aOut <<"\n\n<br> <br/>" ;
                }
            } else {
                std::cout << "incorrect node " << std::endl;
            }
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

    cWebApt = layout->addWidget(Wt::cpp14::make_unique<cWebAptitude>(this, authWidget_));

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
    if (globTest){printf("Auth widget...");}
    session_.login().changed().connect(this, &AuthApplication::authEvent);
    std::unique_ptr<Wt::Auth::AuthWidget> authWidget = cpp14::make_unique<Wt::Auth::AuthWidget>(Session::auth(), session_.users(), session_.login());
    authWidget_=authWidget.get();
    authWidget_->model()->addPasswordAuth(&session_.passwordAuth());
    //authWidget_->model()->addOAuth(Session::oAuth()); // no need google and facebook
    authWidget_->setRegistrationEnabled(true);
    authWidget_->processEnvironment();
    authWidget_->addStyleClass("Wt-auth-login-container");
    if (globTest){printf("done\n");}
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
            cWebApt->mGroupL->updateGL();
            dialog_auth->hide();
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

void AuthApplication::addLog(std::string page, typeLog cat){

    // check pour ne pas ajouter dix fois sur la mm journée et pour le même utilisateur un log identique.
    if (!mAnal.logExist(this->environment(),page, cat)){
        if (session_.login().loggedIn()) {
            const Wt::Auth::User& u = session_.login().user();
            mAnal.addLog(this->environment(),atol(u.id().c_str()), page,(int (cat)));
        }else{
            mAnal.addLog(this->environment(), page,(int (cat)));
        }
    }
}
