#include "mataptcs.h"
extern bool globTest;

matAptCS::matAptCS(cDicoApt *aDicoApt):mDicoApt(aDicoApt),zbio_(1),US_(1),mVar_("")
{
    //setOverflow(Wt::Overflow::Auto);
    setId("pageCS");

    //setMaximumSize("100%","5000px");
    addWidget(std::make_unique<Wt::WText>(tr("matAptCS.zbio")));

    zbioSelection_  =addWidget(std::make_unique<Wt::WComboBox>());
    for (const auto &kv : *mDicoApt->ZBIO()){
        if((kv.first == 1) | (kv.first == 2)  | (kv.first == 10) | (kv.first == 4)){//| (kv.first == 3)  | (kv.first == 5)
            zbioSelection_->addItem(kv.second);
        }
    }
    zbioSelection_->changed().connect(std::bind(&matAptCS::changeZbio,this));
    zbioSelection_->setCurrentIndex(0);
    prefixZbio_="A";

    addNew<Wt::WBreak>();
    addNew<Wt::WBreak>();

    Wt::WTemplate * tpl = addWidget(cpp14::make_unique<Wt::WTemplate>(tr("template.CS")));
    std::string  aShp=mDicoApt->File("ZBIOSIMP");
    graphZbio = tpl->bindWidget("graphZbio", std::make_unique<zbioPainted>(aShp,mDicoApt));
    contListeUS = tpl->bindWidget("listeUS", std::make_unique<WContainerWidget>());
    WContainerWidget * contlisteEss = tpl->bindWidget("listeEssence", std::make_unique<WContainerWidget>());
    contlisteEss->setStyleClass("row");
    contlisteEss->addNew<Wt::WText>(tr("listeEss.CS.titre"));
    // création de la liste de toutes les essences
    int r(1);
    int nb(mDicoApt->getAllEss().size());
    int ncells(nb/2);
    WContainerWidget * rowCont = contlisteEss->addNew<Wt::WContainerWidget>();
    rowCont->setStyleClass("col-6");
    for (auto & kv : mDicoApt->getAllEss()){
     cEss * ess=kv.second.get();
     if (ess->hasCSApt()){
         Wt::WText * t = rowCont->addNew<Wt::WText>(ess->Code()+ " - " +ess->Nom());
         t->setStyleClass("ess");
         rowCont->addNew<Wt::WBreak>();
         t->mouseWentOver().connect([=] {
             displayNiche(ess->Code());
             //graphZbio->displayAptMap(ess->Code());
             t->setStyleClass("currentEss");
         });
         t->mouseWentOut().connect([=] {
             resetNiche();
             //graphZbio->selectZbio(zbio_);
             t->setStyleClass("ess");
         });

         t->clicked().connect([=] {
             t->setStyleClass("ess");
             Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                               ess->Nom(),
                                                               "",
                                                               Wt::Icon::Information,
                                                               Wt::StandardButton::Ok));
             Wt::WLink l("https://www.fichierecologique.be/resources/fee/FEE-"+ess->Code()+".pdf");
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
             messageBox->show();
         });

        r++;
         if (r==ncells){
             rowCont = contlisteEss->addNew<Wt::WContainerWidget>();
             rowCont->setStyleClass("col-6");
             r=1;
         }
     }
    }

     /* 4 Description de unités stationnelles ---------------------------*/
    contFicheUS = addWidget(cpp14::make_unique<WContainerWidget>());
    contFicheUS->setId("ficheUS");

    updateListeUS();
    setHeight(Wt::WLength(3000));
}


void matAptCS::updateListeUS(){

    mMapButtonUS.clear();
    contListeUS->clear();
    contFicheUS->clear();
    contListeUS->addWidget(std::make_unique<Wt::WText>(tr("matAptCS.US").arg(mDicoApt->ZBIO(zbio_))));

    // pour avoir les couleurs des stations, je dois utiliser la layer
    std::shared_ptr<layerBase> CSlay=mDicoApt->getLayerBase(mDicoApt->ZBIO2CSlay(zbio_));

    for (auto & kv : mDicoApt->aVStation(mDicoApt->ZBIO2CSid(zbio_))){
        if (mDicoApt->isStationMaj(zbio_, std::get<0>(kv.first),std::get<1>(kv.first))){

        std::string stationName=mDicoApt->station(mDicoApt->ZBIO2CSid(zbio_),std::get<0>(kv.first),std::get<1>(kv.first));
        // un boutton avec un badge de la couleur de la station
        Wt::WPushButton* us =contListeUS->addWidget(std::make_unique<Wt::WPushButton>());
        us->addStyleClass("position-relative");
        us->setTextFormat(Wt::TextFormat::XHTML);
        std::shared_ptr<color> col=CSlay->getColor(std::get<0>(kv.first));

        us->setText(tr("matAptCS.nobadge").arg(prefixZbio_).arg(std::to_string(std::get<0>(kv.first))).arg(stationName).arg(col->getRGB()));
        us->addStyleClass("us-button");

        //if (std::get<1>(kv.first)==""){
        //    us->setText(tr("matAptCS.nobadge").arg(std::to_string(std::get<0>(kv.first))).arg(col->getRGB()));
        //}else{
        //    us->setText(tr("matAptCS.badge").arg(std::to_string(std::get<0>(kv.first))).arg(std::get<1>(kv.first)).arg(col->getRGB()));
        //}

        //us->setToolTip(mDicoApt->stationEtVar(mDicoApt->ZBIO2CSid(zbio_),std::get<0>(kv.first),std::get<1>(kv.first)));

        us->setToolTip(stationName);
        us->clicked().connect([=]{this->showFicheUS(std::get<0>(kv.first),std::get<1>(kv.first));});
        mMapButtonUS.emplace(std::make_pair(kv.first,us));
    }
    }
    if (mDicoApt->aVStation(mDicoApt->ZBIO2CSid(zbio_)).size()==0){
        contListeUS->addNew<Wt::WText>(tr("matAptCS.msg.noUS4Zbio").arg(mDicoApt->ZBIO(zbio_)));
    }

}

void matAptCS::showFicheUS(int US, std::string aVar){

    US_=US;
    mVar_=aVar;
    mVEss.clear();
    contFicheUS->clear();

    Wt::WTemplate * tpl = contFicheUS->addWidget(cpp14::make_unique<Wt::WTemplate>(tr("template.CS.fiche")));

    Wt::WContainerWidget * cont = tpl->bindWidget("teaser", std::make_unique<Wt::WContainerWidget>());

    // consultation du pdf - si il existe.

    if (boost::filesystem::exists(mDicoApt->File("docroot")+"/pdf/US-"+prefixZbio_+std::to_string(US_)+".pdf")){
    WAnchor * a = cont->addNew<WAnchor>(WLink("pdf/US-A"+std::to_string(US_)+".pdf"));
    a->setImage(std::make_unique<Wt::WImage>(WLink("img/CS/US-A"+std::to_string(US_)+".jpeg"),"illustration de l'unité stationnelle"));
    }

    // 7 classe d'apt, moins 1 qui est l'exclusion
    std::vector<int> apts = {1,2,3,4,5,6};
    for (int apt : apts){
        std::vector<cEss*> aV;
        for (const auto &kv : mDicoApt->getAllEss()){
            cEss * ess = kv.second.get();
            if (ess->hasCSApt()){
                if (ess->getApt(zbio_,US_,mVar_)==apt){
                    aV.push_back(ess);
                }
            }
        }
        mVEss.push_back(aV);
    }
    // maintenant que les essences sont triées, update table apt
    //contFicheUS->addWidget(Wt::WText::tr("t"));

    cont = tpl->bindWidget("listeEssence", std::make_unique<Wt::WContainerWidget>());
    mAptTable = cont->addWidget(cpp14::make_unique<WTable>());
    int r(0);
    mAptTable->setStyleClass("table-recommand");
    for (std::vector<cEss*> aV : mVEss){
        for (int n(0);n<aV.size();n++){
            WContainerWidget * c = mAptTable->elementAt(r,0)->addNew<WContainerWidget>();
            std::string essCode(aV.at(n)->Code());
            c->addStyleClass("circle_eco");
            std::string styleNameCol("col-aptCS"+std::to_string(aV.at(n)->getApt(zbio_,US_,mVar_)));
            c->addStyleClass(styleNameCol);
            c->setToolTip(aV.at(n)->Nom());
            c->addNew<Wt::WText>(essCode);
            c->mouseWentOver().connect([=] {
                hoverBubble(c,1);
                displayNiche(aV.at(n)->Code());
            });
            c->mouseWentOut().connect([=] {
                hoverBubble(c,0);
               resetNiche();
            });
            c->clicked().connect([=] {
                Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                                  aV.at(n)->Nom(),
                                                                  "",
                                                                  Wt::Icon::Information,
                                                                  Wt::StandardButton::Ok));
                messageBox->contents()->addNew<Wt::WText>("Recommandation pour cette essence : " + mDicoApt->code2Recommandation(aV.at(n)->getApt(zbio_,US_,mVar_)));
                //messageBox->contents()->addNew<Wt::WBreak>();
                //messageBox->contents()->addNew<Wt::WText>("Sensibilité climatique : "+mDicoApt->CSClim(aV.at(n)->getCSClim(US_,mVar_)));
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
                messageBox->show();
            });
            WText * t = mAptTable->elementAt(r,1)->addNew<WText>(aV.at(n)->Nom());
            r++;
        }
    }

    /*
    idMessage="zbio"+std::to_string(mDicoApt->ZBIO2CSid(zbio_)) +".US"+std::to_string(US)+".part2";
    s=Wt::WString::tr(idMessage);
    if (s.toUTF8().substr(0,2)!="??"){
    contFicheUS->addNew<Wt::WText>(tr(idMessage));
    }

    // synthèse globale
    contFicheUS->addNew<Wt::WText>(tr("CS.titre.synthese"));
    contFicheUS->addNew<Wt::WBreak>();

    int val=mDicoApt->getKKCS(mDicoApt->ZBIO2CSid(zbio_),US_).VCP;
    Wt::WTemplate * tpl = contFicheUS->addNew<Wt::WTemplate>(tr("template.CS.progressBar").arg(mDicoApt->codeKK2Nom("VCP")));
    Wt::WContainerWidget * cont = tpl->bindWidget("progress", std::make_unique<Wt::WContainerWidget>());
    cont->setStyleClass("syntheseCS"+std::to_string(val));

    val=mDicoApt->getKKCS(mDicoApt->ZBIO2CSid(zbio_),US_).SES;
    tpl = contFicheUS->addNew<Wt::WTemplate>(tr("template.CS.progressBar").arg(mDicoApt->codeKK2Nom("SES")));
    cont = tpl->bindWidget("progress", std::make_unique<Wt::WContainerWidget>());
    cont->setStyleClass("syntheseCS"+std::to_string(val));

    val=mDicoApt->getKKCS(mDicoApt->ZBIO2CSid(zbio_),US_).SCC;
    tpl = contFicheUS->addNew<Wt::WTemplate>(tr("template.CS.progressBar").arg(mDicoApt->codeKK2Nom("SCC")));
    cont = tpl->bindWidget("progress", Wt::cpp14::make_unique<Wt::WContainerWidget>());

    cont->setStyleClass("syntheseCS"+std::to_string(val));

    val=mDicoApt->getKKCS(mDicoApt->ZBIO2CSid(zbio_),US_).RCS;
    tpl = contFicheUS->addNew<Wt::WTemplate>(tr("template.CS.progressBar").arg(mDicoApt->codeKK2Nom("RCS")));
    cont = tpl->bindWidget("progress", std::make_unique<Wt::WContainerWidget>());
    cont->setStyleClass("syntheseCS"+std::to_string(val));

    val=mDicoApt->getKKCS(mDicoApt->ZBIO2CSid(zbio_),US_).PB;
    tpl = contFicheUS->addNew<Wt::WTemplate>(tr("template.CS.progressBar").arg(mDicoApt->codeKK2Nom("PB")));
    cont = tpl->bindWidget("progress", std::make_unique<Wt::WContainerWidget>());
    cont->setStyleClass("syntheseCS"+std::to_string(val));

    idMessage="zbio"+std::to_string(mDicoApt->ZBIO2CSid(zbio_)) +".US"+std::to_string(US)+".part3";
    s=Wt::WString::tr(idMessage);
    if (s.toUTF8().substr(0,2)!="??"){
    contFicheUS->addNew<Wt::WText>(tr(idMessage));
    }
    */
    //doJavaScript("ficheUS.scrollIntoView({ behavior: 'smooth', block: 'start' })");
}

void matAptCS::changeZbio(){
    for (auto & kv : *mDicoApt->ZBIO()){
        if (kv.second==zbioSelection_->currentText()){zbio_=kv.first;}
    }
    if (zbio_==1 | zbio_==2 | zbio_==10){prefixZbio_="A";}
    if (zbio_==4){prefixZbio_="F";}

    graphZbio->selectZbio(zbio_);
    contFicheUS->clear();
    updateListeUS();
}

void matAptCS::hoverBubble(WContainerWidget * c, bool hover){
    if(hover){c->addStyleClass("circle_eco_large");} else {c->removeStyleClass("circle_eco_large");}
}

void matAptCS::displayNiche(std::string aEssCode){
    cEss * ess=mDicoApt->getEss(aEssCode).get();
    // boucle sur toute les stations
    for (auto kv : mMapButtonUS){
        int us=std::get<0>(kv.first);
        std::string var=std::get<1>(kv.first);
        // récupérer l'aptitude
        int apt=ess->getApt(zbio_,us,var);
        // choisir la couleur en fonction de l'aptitude  - attention, couleur Apt CS et non pas FEE
        std::string styleName("col-aptCS"+std::to_string(apt));
        kv.second->addStyleClass(styleName);
    }
}

void matAptCS::resetNiche(){
    for (auto kv : mMapButtonUS){
        kv.second->removeStyleClass("col-aptCS1");
        kv.second->removeStyleClass("col-aptCS2");
        kv.second->removeStyleClass("col-aptCS3");
        kv.second->removeStyleClass("col-aptCS4");
        kv.second->removeStyleClass("col-aptCS5");
        kv.second->removeStyleClass("col-aptCS6");
        kv.second->removeStyleClass("col-aptCS7");
    }
}


