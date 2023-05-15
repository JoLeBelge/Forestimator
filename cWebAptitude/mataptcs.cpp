#include "mataptcs.h"

matAptCS::matAptCS(cDicoApt *aDicoApt):mDicoApt(aDicoApt),zbio_(1),US_(1)
{
    setOverflow(Wt::Overflow::Auto);
    setId("matAptCont");
    // un nouveau div enfant car le parent est dans le stack, avec display flex, ce qui fait foirer un scroll général sur tout le contenu.
    WVBoxLayout * layoutGlobal = setLayout(cpp14::make_unique<WVBoxLayout>());
    WContainerWidget * contG = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    // liste des unités stationnelles
    contG->setOverflow(Wt::Overflow::Auto);
    contG->setMaximumSize("400px","100%");

    for (auto & kv : mDicoApt->aVStation(zbio_)){
        // un boutton avec un badge de la couleur de la station
        Wt::WContainerWidget* us =contG->addWidget(cpp14::make_unique<Wt::WContainerWidget>());
        us->setStyleClass("btn btn-primary");


        Wt::WText * cont =us->addNew<Wt::WText>("US " + std::to_string(kv.first));
        Wt::WText * badge =us->addNew<Wt::WText>("tt");
        badge->setStyleClass("position-absolute top-0 start-100 translate-middle badge rounded-pill bg-danger");
        //us->clicked().connect(std::bind(&matAptCS::updateApt,this,std::to_string(kv.first)));

        us->clicked().connect([=]{this->updateApt(kv.first);});
    }

    // partie droite= zbio
    WContainerWidget * contD = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    WVBoxLayout * layoutDroite = contD->setLayout(cpp14::make_unique<WVBoxLayout>());
    WContainerWidget * contZbio = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutzbio = contZbio->setLayout(cpp14::make_unique<WHBoxLayout>());
    WContainerWidget * contZbioGauche = layoutzbio->addWidget(cpp14::make_unique<WContainerWidget>());
    //contZbioGauche->addWidget(std::make_unique<Wt::WText>(tr("zbio.titre")));
    //contZbio->setMinimumSize("50%","100px");
    //contZbio->setMaximumSize("50%","300px");
    //contZbio->addNew<Wt::WBreak>();
    zbioSelection_  =contZbioGauche->addWidget(std::make_unique<Wt::WComboBox>());
    for (auto kv : *mDicoApt->ZBIO()){
        zbioSelection_->addItem(kv.second);
    }
    zbioSelection_->changed().connect(std::bind(&matAptCS::changeZbio,this));
    zbioSelection_->setCurrentIndex(0);

    std::string  aShp=mDicoApt->File("ZBIOSIMP");
    graphZbio = layoutzbio->addWidget(std::make_unique<zbioPainted>(aShp,mDicoApt));

    WContainerWidget * contApt = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    contApt->setOverflow(Wt::Overflow::Auto);
    mAptTable= contApt->addNew<WTable>();
    //contApt->addNew<Wt::WText>(tr("matApt.asterisque.doubleApt"));
    // partie documentation
    WContainerWidget * cB = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>(),1);
    //cB->addNew<Wt::WText>(tr("matApt.documentation"));
    cB->setOverflow(Wt::Overflow::Auto);


     mAptTable = layoutGlobal->addWidget(cpp14::make_unique<WTable>());


}

void matAptCS::updateApt(int US){

    std::cout << " matAptCS::updateApt " <<std::endl;
    US_=US;
    mVEss.clear();
    for (int apt : {1,2,3}){
        std::vector<std::shared_ptr<cEss>> aV;
        for (auto kv : mDicoApt->getAllEss()){
            std::shared_ptr<cEss> ess = kv.second;
            if (ess->hasCSApt()){
                    if (mDicoApt->AptNonContraignante(ess->getApt(zbio_,US_))==apt){
                        aV.push_back(ess);
                    }
                }
            }
        std::cout << "aptitude " << std::to_string(apt) << " contient " << aV.size() << " essences" <<std::endl;
            mVEss.push_back(aV);
    }

    // maintenant que les essences sont triées, update table apt
    mAptTable->clear();
    int rGlob(1),cGlob(0);
    for (int apt : {1,2,3}){
        cGlob=apt;
        Wt::WTable * t1 = mAptTable->elementAt(rGlob,cGlob)->addNew<Wt::WTable>();
        t1->setStyleClass("table-apt");
        std::string styleName("table-apt"+std::to_string(apt));
        mAptTable->elementAt(rGlob,cGlob)->addStyleClass(styleName);
        std::string styleNameCol("col-apt"+std::to_string(apt));
        int r(0),col(0);
        std::vector<std::shared_ptr<cEss>> aV=mVEss.at(apt-1);
        int ncells=std::ceil(std::sqrt(aV.size()));
        for (int n(0);n<aV.size();n++){
            WContainerWidget * c = t1->elementAt(r,col)->addNew<WContainerWidget>();
            std::string essCode(aV.at(n)->Code());
            c->addStyleClass("circle_eco");
            c->addStyleClass(styleNameCol);
            // check si double apt
            if (mDicoApt->isDoubleApt(aV.at(n)->getApt(zbio_, US_))){essCode+="*";}
            c->addNew<Wt::WText>(essCode);
            c->mouseWentOver().connect([=] {
                hoverBubble(c,1);
                //displayNiche(aV1.at(n)->Code());
                graphZbio->displayAptMap(aV.at(n)->Code());
            });
            c->mouseWentOut().connect([=] {
                hoverBubble(c,0);
                //resetEco();
                graphZbio->selectZbio(zbio_);
            });
            c->setToolTip(aV.at(n)->Nom());
            c->clicked().connect([=] {
                Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                                  aV.at(n)->Nom(),
                                                                  "",
                                                                  Wt::Icon::Information,
                                                                  Wt::StandardButton::Ok));
                messageBox->contents()->addNew<Wt::WText>("Aptitude : " + mDicoApt->code2AptFull(aV.at(n)->getApt(zbio_, US_)));
                messageBox->contents()->addNew<Wt::WBreak>();

                Wt::WLink l("https://www.fichierecologique.be/resources/fee/FEE-"+aV.at(n)->Code()+".pdf");
                l.setTarget(Wt::LinkTarget::NewWindow);
                Wt::WString s("Fiche-essence disponible ici");
                Wt::WAnchor * a =messageBox->contents()->addNew<Wt::WAnchor>(l,s);
                a->clicked().connect([=] {
                    this->removeChild(messageBox);
                });
                messageBox->setModal(true);
                messageBox->buttonClicked().connect([=] {
                    this->removeChild(messageBox);
                });
                messageBox->setMinimumSize("40%","30%");
                messageBox->show();
            });
            col++;
            if (col+1>ncells){col=0;r++;}
        }
    }

}

void matAptCS::changeZbio(){
    for (auto & kv : *mDicoApt->ZBIO()){
        if (kv.second==zbioSelection_->currentText()){zbio_=kv.first;}
    }
    graphZbio->selectZbio(zbio_);
    mAptTable->clear();
}

void matAptCS::hoverBubble(WContainerWidget * c, bool hover){
    // if(hover){c->addStyleClass("circle_eco_large");} else {c->setStyleClass("circle_eco");}
    if(hover){c->addStyleClass("circle_eco_large");} else {c->removeStyleClass("circle_eco_large");}
}

