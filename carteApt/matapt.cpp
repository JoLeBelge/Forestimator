#include "matapt.h"

matApt::matApt(std::shared_ptr<cdicoAptBase> aDicoApt):mDicoApt(aDicoApt),zbio_(1)
{
    WHBoxLayout * layoutGlobal = setLayout(cpp14::make_unique<WHBoxLayout>());
    layoutGlobal->setContentsMargins(0,0,0,0);
    WContainerWidget * contG = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    // on commence avec tableau ecogramme:
    mEco= contG->addNew<WTable>();
    mEco->setStyleClass("eco-table");
    // création de toutes les cellulles de l'écogramme
    for (auto kvNH : *mDicoApt->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
            for (auto kvNT : *mDicoApt->NT()){
                int codeNT=kvNT.first;
                std::tuple<int,int> ntnh(codeNT,codeNH);
                int col= mDicoApt->posEcoNT(codeNT);
                int row=mDicoApt->posEcoNH(codeNH);
                WContainerWidget * c = mEco->elementAt(row,col)->addNew<WContainerWidget>();
                WTableCell * tc = mEco->elementAt(row,col);
                tc->setToolTip("Niveau trophique " + kvNT.second + ", Niveau Hydrique " + kvNH.second);
                c->setStyleClass("circle_eco");
                c->setId("ntnh"+std::to_string(codeNT)+std::to_string(codeNH));
                mMapCircleEco.emplace(std::make_pair(ntnh,c));
                tc->mouseWentOver().connect([=] {
                    this->hoverEco(c,1);
                });
                tc->mouseWentOut().connect([=] {
                    this->hoverEco(c,0);
                });
                tc->clicked().connect(std::bind(&matApt::clicEco,this,ntnh));
            }
        }
    }
    // partie droite
    WContainerWidget * contD = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    WVBoxLayout * layoutDroite = contD->setLayout(cpp14::make_unique<WVBoxLayout>());
    WContainerWidget * contZbio = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    contZbio->addNew<Wt::WText>(tr("zbio.titre"));
    //contZbio->addNew<Wt::WBreak>();
    zbioSelection_  =contZbio->addNew<Wt::WComboBox>();
    for (auto kv : *mDicoApt->ZBIO()){
        zbioSelection_->addItem(kv.second);
    }
    zbioSelection_->changed().connect(std::bind(&matApt::changeZbio,this));
    zbioSelection_->setCurrentIndex(0);

    WContainerWidget * contApt = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    mAptTable= contApt->addNew<WTable>();

}

void matApt::hoverEco(WContainerWidget * c, bool hover){
    // if(hover){c->addStyleClass("circle_eco_large");} else {c->setStyleClass("circle_eco");}
    if(hover){c->addStyleClass("circle_eco_large");} else {c->removeStyleClass("circle_eco_large");}
}

void matApt::clicEco(std::tuple<int,int> ntnh){

    int nt=std::get<0>(ntnh);
    int nh=std::get<1>(ntnh);

    //std::cout << "clic Eco sur nt " << nt << ", nh " << nh << std::endl;

    mAptTable->clear();
    mAptTable->setStyleClass("table-AptGlob");

    mAptTable->setHeaderCount(2);
    mAptTable->elementAt(0,2)->setColumnSpan(3);
    // titre colonne
    mAptTable->elementAt(0,2)->addWidget(cpp14::make_unique<WText>(tr("aptHT.titre")+" NT "+mDicoApt->NT(nt) +", NH "+mDicoApt->NH(nh)));
    mAptTable->elementAt(1,2)->addWidget(cpp14::make_unique<WText>(tr("apt.t.O")));
    mAptTable->elementAt(1,3)->addWidget(cpp14::make_unique<WText>(tr("apt.t.T")));
    mAptTable->elementAt(1,4)->addWidget(cpp14::make_unique<WText>(tr("apt.t.TE")));
    mAptTable->elementAt(2,0)->setRowSpan(3);
    mAptTable->elementAt(2,0)->addWidget(cpp14::make_unique<WText>(tr("aptZ.titre")));
    //WText * titre =mAptTable->elementAt(2,0)->addWidget(cpp14::make_unique<WText>(tr("aptZ.titre")+" "+WString(mDicoApt->ZBIO(zbio_))));
    //titre->setStyleClass("vertical-text");
    mAptTable->elementAt(2,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.O")));
    mAptTable->elementAt(3,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.T")));
    mAptTable->elementAt(4,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.TE")));

    // boucle sur toutes les essences pour déterminer dans quelles cellules elles se situent
    trierEss(ntnh,zbio_);
    // on commence avec un tableau 3x3
    for (int aptZbio : {1,2,3}){
        for (int aptHT : {1,2,3}){
            std::vector<std::shared_ptr<cEss>> aV=mVEss.at(aptHT+(3*(aptZbio-1))-1);
            int ncells=std::ceil(std::sqrt(aV.size()));
            int rGlob(aptZbio+1),cGlob(aptHT+1);
             Wt::WTable * t1 = mAptTable->elementAt(rGlob,cGlob)->addNew<Wt::WTable>();
             t1->setStyleClass("table-apt");
            std::string styleName("table-apt"+std::to_string(std::max(aptZbio,aptHT)));
            mAptTable->elementAt(rGlob,cGlob)->addStyleClass(styleName);
            //std::cout << " aptZbio de " << aptZbio << ", aptHT de " << aptHT << ", cellulle " <<  aptZbio-1 << "," << aptHT-1 << ", nb Ess " << aV.size() << ", sytle " << styleName << std::endl;
            int r(0),col(0);
            for (int n(0);n<aV.size();n++){
                WContainerWidget * c = t1->elementAt(r,col)->addNew<WContainerWidget>();
                // si je veux une ancre pour y lier un lien, je peux mais je dois refaire le style de symbologie.
                //WAnchor * c = t1->elementAt(r,col)->addNew<WAnchor>();
                                //c->setLink();
                std::string text(aV.at(n)->Code());
                // check si double apt
                if (mDicoApt->isDoubleApt(aV.at(n)->getApt(aptZbio))){text+="*";}
                if (mDicoApt->isDoubleApt(aV.at(n)->getApt(nt,nh,aptZbio,false))){text+="*";}
                c->addNew<Wt::WText>(text);

                c->addStyleClass("circle_eco");
                c->mouseWentOver().connect([=] {
                    this->hoverEco(c,1);
                });
                c->mouseWentOut().connect([=] {
                    this->hoverEco(c,0);
                });
                c->setToolTip(aV.at(n)->Nom());
                c->clicked().connect([=] {
                    Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                                      aV.at(n)->Nom(),
                                                                      "",
                                                                      Wt::Icon::Information,
                                                                      Wt::StandardButton::Ok));
                    messageBox->contents()->addNew<Wt::WText>("Aptitude bioclimatique : " + mDicoApt->code2AptFull(aV.at(n)->getApt(aptZbio)));
                    messageBox->contents()->addNew<Wt::WBreak>();
                    messageBox->contents()->addNew<Wt::WText>("Aptitude hydro-trophique : " + mDicoApt->code2AptFull(aV.at(n)->getApt(nt,nh,aptZbio,false)));
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
                col++;
                if (col+1>ncells){col=0;r++;}
            }
        }
    }
    // changer symbologie des circles
    for (auto kv : mMapCircleEco){
        if (kv.first==ntnh){
            kv.second->addStyleClass("circle_eco_selected");
            kv.second->removeStyleClass("circle_eco_large");
        } else {
            kv.second->removeStyleClass("circle_eco_selected");
            kv.second->removeStyleClass("circle_eco_large");
        }
    }

}

void matApt::trierEss(std::tuple<int,int> ntnh, int zbio){
    mVEss.clear();
    int nt=std::get<0>(ntnh);
    int nh=std::get<1>(ntnh);
    for (int aptZbio : {1,2,3}){
        for (int aptHT : {1,2,3}){
            std::vector<std::shared_ptr<cEss>> aV;
            for (auto kv : mDicoApt->getAllEss()){
                std::shared_ptr<cEss> ess = kv.second;
                if (ess->hasFEEApt()){
                    if(mDicoApt->AptNonContraignante(ess->getApt(zbio))==aptZbio){
                        if (mDicoApt->AptNonContraignante(ess->getApt(nt,nh,zbio,false))==aptHT){
                            aV.push_back(ess);
                        }
                    }
                }
            }
            mVEss.push_back(aV);
        }
    }
}

void matApt::changeZbio(){
    for (auto & kv : *mDicoApt->ZBIO()){
       if (kv.second==zbioSelection_->currentText()){zbio_=kv.first;}
   }
}

void matApt::receivePrediction(int aCode,std::vector<double> aVPredNT,std::vector<double> aVPredNH){
    int max_index_nh=0;
    int max_index_nt=0;
    if (aCode==1){
        // retirer le maximum nt et nh
        double max_nt=0;
        double max_nh=0;
        int i=7;// les codes NT démarrent à 7

     for (double pct : aVPredNT){
         if (pct>0){
             if (pct>max_nt){
                 max_nt=pct;
                 max_index_nt=i;
             }
         }
          i++;
      }
     i=1;// les codes nh démarrent à 1
     for (double pct : aVPredNH){
         if (pct>0){
             if (pct>max_nh){
                 max_nh=pct;
                 max_index_nh=i;
             }
         }
         i++;
     }
   }
   // identifier la liste de code ntnh qui correspondent à ces groupes (groupe hydrique)
    if (globTest2){std::cout << " max nh index " << max_index_nh << " , max nt index " << max_index_nt << std::endl;}
    std::vector<std::tuple<int,int>> aVNTNH;
    if (max_index_nh>0){
        int nhStart= mDicoApt->groupeNH2NHStart(max_index_nh);
        for (int j(0);j<mDicoApt->groupeNH2Nb(max_index_nh);j++){
           int nh=nhStart-j;
           //std::cout << " ajout nt " << max_index_nt << " , nh " << nh << ", nh start " << nhStart << std::endl;
           std::tuple<int,int> ntnh(max_index_nt,nh);
           aVNTNH.push_back(ntnh);
        }
    }
   // reset des syles de couleurs jaunes pours les disques
    for (auto kv : mMapCircleEco){
        if (std::find(aVNTNH.begin(), aVNTNH.end(), kv.first) != aVNTNH.end()){
            kv.second->addStyleClass("circle_eco_phyto");
        } else {
            kv.second->removeStyleClass("circle_eco_phyto");
        }
    }
}
