#include "cwebaptitude.h"
const char *cl[] = { "FEE", "CS" };
//std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};
extern std::vector<std::string> classes;

cWebAptitude::cWebAptitude(Wt::WApplication* app)
    : WContainerWidget()
{
    m_app = app;

    //app->removeMetaHeader(MetaHeaderType::Meta, "viewport");
    //app->addMetaHeader("viewport", "width=device-width, initial-scale=1");
    // fonctionne que avec progressive bootstrap
    //m_app->addMetaHeader(Wt::MetaHeaderType::HttpHeader, "viewport", "width=device-width, initial-scale=1");

    // std::cout << " wenvi ajax 2 " <<m_app->environment().ajax() << std::endl;
    // this->enableAjax();
    std::string aBD=loadBDpath();
    mDico=new cDicoApt(aBD);
    setOverflow(Wt::Overflow::Auto);
    setPadding(20);
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    this->addStyleClass("table form-inline");
    this->setStyleClass("table form-inline");
    auto titreCont = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * titreCont_ = titreCont.get();
       WText * titre = titreCont_->addWidget(cpp14::make_unique<WText>("Stations forestières et Aptitude des Essences"));
    titreCont_->setContentAlignment(AlignmentFlag::Center| AlignmentFlag::Middle);
    titre->decorationStyle().font().setSize(FontSize::Medium);
    titre->decorationStyle().setForegroundColor(WColor(192,192,192));
    // le set padding ne fonctionne que si je désactive le inline
    titre->setInline(0);
    titre->setPadding(0,Wt::Side::Bottom | Wt::Side::Top);

    auto pane = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * pane_ = pane.get();

    auto hLayout = pane_->setLayout(Wt::cpp14::make_unique<Wt::WHBoxLayout>());

    pane_->setHeight("60%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    // non pas d'overflow pour la carte, qui est dans pane_
    pane_->setOverflow(Wt::Overflow::Visible);

    auto infoW = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    Wt::WContainerWidget * infoW_ = infoW.get();
    infoW_->setOverflow(Wt::Overflow::Auto);
    infoW_->setWidth("30%");

    // creation d'un menum popup

    // Create a stack where the contents will be located.
    auto contents = Wt::cpp14::make_unique<Wt::WStackedWidget>();

    Wt::WMenu *menu = infoW_->addNew<Wt::WMenu>(contents.get());
    menu->setStyleClass("nav nav-pills nav-stacked");
    menu->setWidth("100%");

    // Add menu items using the default lazy loading policy.

    auto legendCont = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    auto PACont = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    //WMenuItem * Wt::WMenu::addItem ( const WString & text, T * target, void(V::*)() method )
    //WMenuItem * legendMenuIt = menu->addItem("Légende", Wt::cpp14::make_unique<Wt::WContainerWidget>());



    //menu->addItem("Téléchargement", Wt::cpp14::make_unique<Wt::WTextArea>("Téléchargement : to come soon"));
    infoW_->addWidget(std::move(contents));

    auto map = Wt::cpp14::make_unique<WOpenLayers>(mDico);
    mMap= map.get();
    mMap->setWidth("70%");
    //mMap->setHeight("50%"); // ca ca met la taille de la carte à 50 du conteneur dans le layout, donc c'est pas bon

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

    // creation de l'objet grouplayer
    //mGroupL =addWidget(cpp14::make_unique<groupLayers>(mDico,this));


    //auto groupL = Wt::cpp14::make_unique<groupLayers>(mDico,this,infoW_,mMap);
    auto groupL = Wt::cpp14::make_unique<groupLayers>(mDico,this,legendCont.get(),mMap,m_app);
    //WMenuItem * legendMenuIt = menu->addItem("Légende", std::move(legendCont));
    menu->addItem("Légende", std::move(legendCont));
    mGroupL = groupL.get();
    std::cout << "construction unique ptr parcellaire " << std::endl;
    // unique _ptr est détruit à la fin de la création de l'objet, doit etre déplacé avec move pour donner sa propriété à un autre objet pour ne pas être détruit
    //auto uPtrPA = Wt::cpp14::make_unique<parcellaire>(PACont.get(),mGroupL,m_app);
    //std::cout << "get pointeur mPA";
    //mPA = uPtrPA.get();
    mPA = new parcellaire(PACont.get(),mGroupL,m_app);
    // je ne parviens pas à faire le connect correctement, je dois me tromper quelque part.
    mGroupL->focusMap().connect(mMap,&WOpenLayers::giveFocus);

    //WMenuItem * PAmenuIt = menu->addItem("Plan d'amménagement", std::move(PACont));
    menu->addItem("Plan d'amménagement", std::move(PACont));

    // maintenant que tout les objets sont crées, je ferme la connection avec la BD sqlite3, plus propre
    mDico->closeConnection();

    mMap->clicked().connect(mMap->slot);
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));
    // je divise la fenetre en 2 dans la hauteur pour mettre la carte à droite et à gauche une fenetre avec les infos des couches
    auto layout = this->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    // hlayout c'est lié à pane
    hLayout->addWidget(std::move(map), 0);
    //hLayout-> wiget, int stretch, WFlagAlignement
    hLayout->addWidget(std::move(infoW), 1);
    layout->addWidget(std::move(titreCont), 0);
    layout->addWidget(std::move(pane), 0);
    layout->addWidget(std::move(groupL), 1); // si 1, laisse la place aux deux autres partie du layout car stretch, mais pas beau.
    std::cout << "toto " << std::endl;
}
