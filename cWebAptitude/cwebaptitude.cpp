#include "cwebaptitude.h"
const char *cl[] = { "FEE", "CS" };
//std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};
extern std::vector<std::string> classes;

cWebAptitude::cWebAptitude(Wt::WApplication* app)
    : WContainerWidget()
{
    m_app = app;
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
    titre->decorationStyle().font().setSize(FontSize::XLarge);
    titre->decorationStyle().setForegroundColor(WColor(192,192,192));
    // le set padding ne fonctionne que si je désactive le inline
    titre->setInline(0);
    titre->setPadding(25,Wt::Side::Bottom | Wt::Side::Top);

    auto pane = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * pane_ = pane.get();

    Wt::WLayout * toto;
    auto hLayout = pane_->setLayout(Wt::cpp14::make_unique<Wt::WHBoxLayout>());

    pane_->setHeight("65%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    // non pas d'overflow pour la carte, qui est dans pane_
    pane_->setOverflow(Wt::Overflow::Visible);

    auto infoW = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    Wt::WContainerWidget * infoW_ = infoW.get();
    infoW_->setOverflow(Wt::Overflow::Auto);
     infoW_->setWidth("30%");
     //infoW_->setHeight("50%");
    auto map = Wt::cpp14::make_unique<WOpenLayers>(this,mDico);
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
    auto groupL = Wt::cpp14::make_unique<groupLayers>(mDico,this,infoW_);
    mGroupL = groupL.get();
    // je ne parviens pas à faire le connect correctement, je dois me tromper quelque part.
    mGroupL->focusMap().connect(mMap,&WOpenLayers::giveFocus);

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
}
