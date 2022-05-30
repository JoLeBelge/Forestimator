#include "matapt.h"

matApt::matApt(std::shared_ptr<cdicoAptBase> aDicoApt):mDicoApt(aDicoApt),zbio_(1),nh_(10),nt_(10),bt_compare4Predicted(NULL)
{
    setOverflow(Wt::Overflow::Auto);
    setId("matAptCont");
    // un nouveau div enfant car le parent est dans le stack, avec display flex, ce qui fait foirer un scroll général sur tout le contenu.
    WContainerWidget * global = addWidget(cpp14::make_unique<WContainerWidget>());
    WVBoxLayout * layoutGlobalA = global->setLayout(cpp14::make_unique<WVBoxLayout>());
    WContainerWidget * cA = layoutGlobalA->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutGlobal = cA->setLayout(cpp14::make_unique<WHBoxLayout>());
    layoutGlobal->setContentsMargins(0,0,0,0);
    WContainerWidget * contG = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());
    // on commence avec tableau ecogramme:
    contG->setOverflow(Wt::Overflow::Auto);
    contG->setMaximumSize("400px","100%");
    mEco= contG->addNew<WTable>();
    mEco->setStyleClass("eco-table");
    // création des nom de colonne
    mEco->setHeaderCount(2);
    mEco->elementAt(0,2)->setColumnSpan(6);
    // titre colonne
    mEco->elementAt(0,2)->addWidget(cpp14::make_unique<WText>(tr("eco.NT.titre")));
    mEco->elementAt(1,2)->addWidget(cpp14::make_unique<WText>("-3"));
    mEco->elementAt(1,3)->addWidget(cpp14::make_unique<WText>("-2"));
    mEco->elementAt(1,4)->addWidget(cpp14::make_unique<WText>("-1"));
    mEco->elementAt(1,5)->addWidget(cpp14::make_unique<WText>("0"));
    mEco->elementAt(1,6)->addWidget(cpp14::make_unique<WText>("+1"));
    mEco->elementAt(1,7)->addWidget(cpp14::make_unique<WText>("+2"));
    mEco->elementAt(2,0)->setRowSpan(14);
    mEco->elementAt(2,0)->addWidget(cpp14::make_unique<WText>(tr("eco.NH.titre")));

    int addr(2),addc(2);
    // création de toutes les cellulles de l'écogramme
    for (auto kvNH : *mDicoApt->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
            int row=mDicoApt->posEcoNH(codeNH)+addr;
            // ajout titre
            mEco->elementAt(row,1)->addWidget(cpp14::make_unique<WText>(kvNH.second));
            for (auto kvNT : *mDicoApt->NT()){
                int codeNT=kvNT.first;
                std::tuple<int,int> ntnh(codeNT,codeNH);
                int col= mDicoApt->posEcoNT(codeNT)+addc;
                WContainerWidget * c = mEco->elementAt(row,col)->addNew<WContainerWidget>();
                WTableCell * tc = mEco->elementAt(row,col);
                tc->setToolTip("Niveau trophique " + kvNT.second + ", Niveau Hydrique " + kvNH.second);
                c->setStyleClass("circle_eco");
                c->setId("ntnh"+std::to_string(codeNT)+std::to_string(codeNH));
                mMapCircleEco.emplace(std::make_pair(ntnh,c));
                tc->mouseWentOver().connect([=] {
                    hoverBubble(c,1);
                });
                tc->mouseWentOut().connect([=] {
                    hoverBubble(c,0);
                });
                //tc->clicked().connect(std::bind(&matApt::clicEco,this,ntnh));
                tc->clicked().connect(std::bind(&matApt::filterMouseEvent,this,std::placeholders::_1,ntnh));
            }
        }
    }
    // partie droite
    WContainerWidget * contD = layoutGlobal->addWidget(cpp14::make_unique<WContainerWidget>());

    WVBoxLayout * layoutDroite = contD->setLayout(cpp14::make_unique<WVBoxLayout>());
    WContainerWidget * contZbio = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutzbio = contZbio->setLayout(cpp14::make_unique<WHBoxLayout>());
    WContainerWidget * contZbioGauche = layoutzbio->addWidget(cpp14::make_unique<WContainerWidget>());
    contZbioGauche->addWidget(std::make_unique<Wt::WText>(tr("zbio.titre")));
    //contZbio->setMinimumSize("50%","100px");
    //contZbio->setMaximumSize("50%","300px");
    //contZbio->addNew<Wt::WBreak>();
    zbioSelection_  =contZbioGauche->addWidget(std::make_unique<Wt::WComboBox>());
    for (auto kv : *mDicoApt->ZBIO()){
        zbioSelection_->addItem(kv.second);
    }
    zbioSelection_->changed().connect(std::bind(&matApt::changeZbio,this));
    zbioSelection_->setCurrentIndex(0);
    contZbioGauche->addWidget(std::make_unique<Wt::WBreak>());
    bt_compare4Predicted =contZbioGauche->addWidget(std::make_unique<Wt::WPushButton>());
    bt_compare4Predicted->setText(tr("matApt.compare4Predicted"));
    bt_compare4Predicted->hide();
    bt_compare4Predicted->clicked().connect(std::bind(&matApt::comparison4predicted,this));

    std::string  aShp=mDicoApt->File("ZBIOSIMP");
    graphZbio = layoutzbio->addWidget(std::make_unique<zbioPainted>(aShp,mDicoApt));

    WContainerWidget * contApt = layoutDroite->addWidget(cpp14::make_unique<WContainerWidget>());
    contApt->setOverflow(Wt::Overflow::Auto);
    mAptTable= contApt->addNew<WTable>();
    contApt->addNew<Wt::WText>(tr("matApt.asterisque.doubleApt"));
    // partie documentation
    WContainerWidget * cB = layoutGlobalA->addWidget(cpp14::make_unique<WContainerWidget>(),1);
    cB->addNew<Wt::WText>(tr("matApt.documentation"));
    cB->setOverflow(Wt::Overflow::Auto);
}

void matApt::hoverBubble(WContainerWidget * c, bool hover){
    // if(hover){c->addStyleClass("circle_eco_large");} else {c->setStyleClass("circle_eco");}
    if(hover){c->addStyleClass("circle_eco_large");} else {c->removeStyleClass("circle_eco_large");}
}

void matApt::clicEco(std::tuple<int,int> ntnh){
    mVNtnh4Comparison.clear();
    nt_=std::get<0>(ntnh);
    nh_=std::get<1>(ntnh);
    selectLevel4comparison(ntnh);
}

/*
void matApt::displayMatApt(){   
    //std::cout << "clic Eco sur nt " << nt << ", nh " << nh << std::endl;
    initAptTable(tr("aptHT.titre").toUTF8()+" NT "+mDicoApt->NT(nt_) +", NH "+mDicoApt->NH(nh_));
    // boucle sur toutes les essences pour déterminer dans quelles cellules elles se situent
    std::tuple<int,int> ntnh(nt_,nh_);
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
            std::string styleNameCol("col-apt"+std::to_string(std::max(aptZbio,aptHT)));
            //std::cout << " aptZbio de " << aptZbio << ", aptHT de " << aptHT << ", cellulle " <<  aptZbio-1 << "," << aptHT-1 << ", nb Ess " << aV.size() << ", sytle " << styleName << std::endl;
            int r(0),col(0);
            for (int n(0);n<aV.size();n++){
                WContainerWidget * c = t1->elementAt(r,col)->addNew<WContainerWidget>();
                // si je veux une ancre pour y lier un lien, je peux mais je dois refaire le style de symbologie.
                //WAnchor * c = t1->elementAt(r,col)->addNew<WAnchor>();
                //c->setLink();
                std::string text(aV.at(n)->Code());
                // check si double apt
                if (mDicoApt->isDoubleApt(aV.at(n)->getApt(zbio_))){text+="*";}
                if (mDicoApt->isDoubleApt(aV.at(n)->getApt(nt_,nh_,zbio_,false))){text+="*";}
                c->addNew<Wt::WText>(text);

                c->addStyleClass("circle_eco");
                c->addStyleClass(styleNameCol);
                c->mouseWentOver().connect([=] {
                    hoverBubble(c,1);
                    displayNiche(aV.at(n)->Code());
                    graphZbio->displayAptMap(aV.at(n)->Code());
                });
                c->mouseWentOut().connect([=] {
                    hoverBubble(c,0);
                    resetEco();
                    graphZbio->selectZbio(zbio_);
                });
                c->setToolTip(aV.at(n)->Nom());
                //if(aV.at(n)->Code()=="EC"){ std::cout << " érable champêtre, aptitude zbio climatique " << mDicoApt->code2AptFull(aV.at(n)->getApt(zbio_)) << ", aptitude HT " << mDicoApt->code2AptFull(aV.at(n)->getApt(nt,nh,zbio_,false)) << std::endl;}
                //if(aV.at(n)->Code()=="EC"){ std::cout << " érable champêtre, aptitude zbio climatique " << aV.at(n)->getApt(zbio_) << ", aptitude HT " << aV.at(n)->getApt(nt,nh,zbio_,false) << std::endl;}

                c->clicked().connect([=] {
                    Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                                      aV.at(n)->Nom(),
                                                                      "",
                                                                      Wt::Icon::Information,
                                                                      Wt::StandardButton::Ok));
                    messageBox->contents()->addNew<Wt::WText>("Aptitude bioclimatique : " + mDicoApt->code2AptFull(aV.at(n)->getApt(zbio_)));
                    messageBox->contents()->addNew<Wt::WBreak>();
                    messageBox->contents()->addNew<Wt::WText>("Aptitude hydro-trophique : " + mDicoApt->code2AptFull(aV.at(n)->getApt(nt_,nh_,zbio_,false)));
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
}*/

void matApt::displayNiche(std::string aEssCode){
    std::shared_ptr<cEss> ess=mDicoApt->getEss(aEssCode);
    // boucle sur tout les ntnh de l'écogramme
    for (auto kv : mMapCircleEco){
        int nt=std::get<0>(kv.first);
        int nh=std::get<1>(kv.first);
        // récupérer l'aptitude
        int apt=ess->getApt(nt,nh,zbio_,false);
        // choisir la couleur en fonction de l'aptitude
        std::string styleName("col-apt"+std::to_string(mDicoApt->AptNonContraignante(apt)));
        kv.second->addStyleClass(styleName);
    }
}

void matApt::resetEco(){
    for (auto kv : mMapCircleEco){
        kv.second->removeStyleClass("col-apt1");
        kv.second->removeStyleClass("col-apt2");
        kv.second->removeStyleClass("col-apt3");
        kv.second->removeStyleClass("col-apt4");
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
                            //if(ess->Code()=="SO"){ std::cout << " SO, aptitude zbio climatique " << ess->getApt(zbio) << ", aptitude HT " << ess->getApt(nt,nh,zbio,false) << std::endl;}
                            aV.push_back(ess);
                        }
                    }
                }
            }
            mVEss.push_back(aV);
        }
    }
}

void matApt::trierEss(std::tuple<int,int> ntnh, int zbio, std::vector<std::vector<std::shared_ptr<cEss>>> * aVEss){
    aVEss->clear();
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
                            //if(ess->Code()=="SO"){ std::cout << " SO, aptitude zbio climatique " << ess->getApt(zbio) << ", aptitude HT " << ess->getApt(nt,nh,zbio,false) << std::endl;}
                            aV.push_back(ess);
                        }
                    }
                }
            }
            aVEss->push_back(aV);
        }
    }
}


void matApt::changeZbio(){
    for (auto & kv : *mDicoApt->ZBIO()){
        if (kv.second==zbioSelection_->currentText()){zbio_=kv.first;}
    }
    graphZbio->selectZbio(zbio_);
    //displayMatApt();
    compareMatApt();
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
    //if (globTest2){std::cout << " max nh index " << max_index_nh << " , max nt index " << max_index_nt << std::endl;}
    //std::vector<std::tuple<int,int>> aVNTNH;
    mVPredictedNtnh.clear();
    if (max_index_nh>0){
        int nhStart= mDicoApt->groupeNH2NHStart(max_index_nh);
        for (int j(0);j<mDicoApt->groupeNH2Nb(max_index_nh);j++){
            int nh=nhStart-j;
            //std::cout << " ajout nt " << max_index_nt << " , nh " << nh << ", nh start " << nhStart << std::endl;
            std::tuple<int,int> ntnh(max_index_nt,nh);
            mVPredictedNtnh.push_back(ntnh);
        }
    }
    // reset des syles de couleurs jaunes pours les disques
    for (auto kv : mMapCircleEco){
        if (std::find(mVPredictedNtnh.begin(), mVPredictedNtnh.end(), kv.first) != mVPredictedNtnh.end()){
            kv.second->addStyleClass("circle_eco_phyto");
        } else {
            kv.second->removeStyleClass("circle_eco_phyto");
        }
    }
    bt_compare4Predicted->show();
}

void matApt::initAptTable(std::string aNTNHTitle){
    mAptTable->clear();
    mAptTable->setStyleClass("table-AptGlob");
    mAptTable->setHeaderCount(2);
    mAptTable->elementAt(0,2)->setColumnSpan(3);
    // titre colonne
    mAptTable->elementAt(0,2)->addWidget(cpp14::make_unique<WText>( aNTNHTitle));
    mAptTable->elementAt(1,2)->addWidget(cpp14::make_unique<WText>(tr("apt.t.O")));
    mAptTable->elementAt(1,3)->addWidget(cpp14::make_unique<WText>(tr("apt.t.T")));
    mAptTable->elementAt(1,4)->addWidget(cpp14::make_unique<WText>(tr("apt.t.TE")));
    mAptTable->elementAt(2,0)->setRowSpan(3);
    //mAptTable->elementAt(2,0)->addWidget(cpp14::make_unique<WText>(tr("aptZ.titre")));

    mAptTable->elementAt(2,0)->addWidget(cpp14::make_unique<WText>("<strong class='vertical-text'><span>Aptitude climatique : "+WString(mDicoApt->ZBIO(zbio_))+"</span></strong>"));
    mAptTable->elementAt(2,0)->addStyleClass("rel-pos");
    mAptTable->elementAt(2,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.O")));
    mAptTable->elementAt(3,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.T")));
    mAptTable->elementAt(4,1)->addWidget(cpp14::make_unique<WText>(tr("apt.t.TE")));
}

void matApt::compareMatApt(){

    for (auto kv : mMapCircleEco){
        if (std::find(mVPredictedNtnh.begin(), mVPredictedNtnh.end(), kv.first) != mVPredictedNtnh.end()){
            kv.second->addStyleClass("circle_eco_phyto");
        } else {
            kv.second->removeStyleClass("circle_eco_phyto");
        }

        if (std::find(mVNtnh4Comparison.begin(), mVNtnh4Comparison.end(), kv.first) != mVNtnh4Comparison.end()){
            kv.second->addStyleClass("circle_eco_selected");
            kv.second->removeStyleClass("circle_eco_large");
            kv.second->removeStyleClass("circle_eco_phyto");
            //std::cout << " style rouge pour " << std::get<0>(kv.first) << ", " << std::get<1>(kv.first) << std::endl;
        } else {
            kv.second->removeStyleClass("circle_eco_selected");
            kv.second->removeStyleClass("circle_eco_large");
        }

    }

    if (mVNtnh4Comparison.size()>0){
        std::vector<std::vector<std::vector<std::shared_ptr<cEss>>>> allVEss; // mettre tout dans ce vecteur

        std::string aTitre;
        bool first(1); int n(1);
        for (std::tuple<int,int> ntnh : mVNtnh4Comparison){
            std::vector<std::vector<std::shared_ptr<cEss>>> aVEss;
            trierEss(ntnh,zbio_,& aVEss);
            allVEss.push_back(aVEss);
            // titre pour le tableau
            int nt=std::get<0>(ntnh);
            int nh=std::get<1>(ntnh);
            if (first){
                if (mVNtnh4Comparison.size()==1){aTitre=tr("aptHT.titre").toUTF8()+" NT "+mDicoApt->NT(nt) +", NH "+mDicoApt->NH(nh);
                }else{aTitre=tr("aptHT.titre").toUTF8()+" commune à NT "+mDicoApt->NT(nt) +", NH "+mDicoApt->NH(nh);}
                first=0;
            } else {
                if (n<3){
                    aTitre+=" et NT " +mDicoApt->NT(nt)+", NH "+mDicoApt->NH(nh);
                } else if(n==3){
                    aTitre+=" et "+ std::to_string(mVNtnh4Comparison.size()-2) + " autre(s) niveau(x)";
                }
            }
            n++;
        }

        initAptTable(aTitre);

        // On sépare les espèces en commun (mm aptitude pour tout les ntnh) des espèces pour lesquelles on a des aptitudes différentes (l'aptitude la plus contraignante est retenue)
        std::vector<std::vector<std::shared_ptr<cEss>>> aVVEssCommun;
        std::vector<std::shared_ptr<cEss>> aVEssDiff; // d'abor on identifie toutes les essences qui ont des apt différentes

        for (int aptZbio : {1,2,3}){
            for (int aptHT : {1,2,3}){
                std::vector<std::shared_ptr<cEss>> aV1=allVEss.at(0).at(aptHT+(3*(aptZbio-1))-1);
                std::vector<std::shared_ptr<cEss>> aVEssCom;
                for (int c(0);c<allVEss.size();c++){
                    std::vector<std::shared_ptr<cEss>> aV2=allVEss.at(c).at(aptHT+(3*(aptZbio-1))-1);
                    getVEssCommun(aV1, aV2,aVEssCom, aVEssDiff);
                }
                aVVEssCommun.push_back(aVEssCom);
            }
        }
        // on retrie les essences "Différentes" en fonction de leur aptitude la plus contraignante
        std::vector<std::vector<std::shared_ptr<cEss>>> aVVEssDiff;
        for (int aptZbio : {1,2,3}){
            for (int aptHT : {1,2,3}){
                std::vector<std::shared_ptr<cEss>> aV;
                for (std::shared_ptr<cEss> ess : aVEssDiff){
                    if(mDicoApt->AptNonContraignante(ess->getApt(zbio_))==aptZbio){
                        // retourner l'apt max de plusieurs ntnh
                        int aptMax=ess->getMaxAptHT(mVNtnh4Comparison,zbio_);
                        if (aptMax==aptHT){
                            //std::cout << " essence " << ess->Code() << " a comme aptitude max " << aptMax << " , soit " << mDicoApt->code2Apt(aptMax) << ", ajout dans le vecteur d'essence pour le niveau d'aptitude " << aptHT << std::endl;
                            aV.push_back(ess);
                        }
                    }
                }
                aVVEssDiff.push_back(aV);
            }
        }

        for (int aptZbio : {1,2,3}){
            for (int aptHT : {1,2,3}){
                std::vector<std::shared_ptr<cEss>> aV1=aVVEssCommun.at(aptHT+(3*(aptZbio-1))-1);
                std::vector<std::shared_ptr<cEss>> aV2=aVVEssDiff.at(aptHT+(3*(aptZbio-1))-1);
                int nb=aV1.size()+aV2.size();
                int ncells=std::ceil(std::sqrt(nb));
                int rGlob(aptZbio+1),cGlob(aptHT+1);
                Wt::WTable * t1 = mAptTable->elementAt(rGlob,cGlob)->addNew<Wt::WTable>();
                t1->setStyleClass("table-apt");
                std::string styleName("table-apt"+std::to_string(std::max(aptZbio,aptHT)));
                mAptTable->elementAt(rGlob,cGlob)->addStyleClass(styleName);
                std::string styleNameCol("col-apt"+std::to_string(std::max(aptZbio,aptHT)));

                //std::cout << " aptZbio de " << aptZbio << ", aptHT de " << aptHT << ", cellulle " <<  aptZbio-1 << "," << aptHT-1 << ", nb Ess " << aV.size() << ", sytle " << styleName << std::endl;
                int r(0),col(0);

                // les essences qui partagent la mm aptitude
                for (int n(0);n<aV1.size();n++){
                    WContainerWidget * c = t1->elementAt(r,col)->addNew<WContainerWidget>();
                    std::string essCode(aV1.at(n)->Code());
                    c->addStyleClass("circle_eco");
                    c->addStyleClass(styleNameCol);
                    // check si double apt
                    if (mDicoApt->isDoubleApt(aV1.at(n)->getApt(zbio_))){essCode+="*";}
                    int nt=std::get<0>(mVNtnh4Comparison.at(0));
                    int nh=std::get<1>(mVNtnh4Comparison.at(0));
                    if (mDicoApt->isDoubleApt(aV1.at(n)->getApt(nt,nh,zbio_,false))){essCode+="*";}
                    c->addNew<Wt::WText>(essCode);
                    c->mouseWentOver().connect([=] {
                        hoverBubble(c,1);
                        displayNiche(aV1.at(n)->Code());
                        graphZbio->displayAptMap(aV1.at(n)->Code());
                    });
                    c->mouseWentOut().connect([=] {
                        hoverBubble(c,0);
                        resetEco();
                        graphZbio->selectZbio(zbio_);
                    });
                    c->setToolTip(aV1.at(n)->Nom());
                    c->clicked().connect([=] {
                        Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                                          aV1.at(n)->Nom(),
                                                                          "",
                                                                          Wt::Icon::Information,
                                                                          Wt::StandardButton::Ok));
                        messageBox->contents()->addNew<Wt::WText>("Aptitude bioclimatique : " + mDicoApt->code2AptFull(aV1.at(n)->getApt(zbio_)));
                        messageBox->contents()->addNew<Wt::WBreak>();
                        messageBox->contents()->addNew<Wt::WText>("Aptitude hydro-trophique : " + mDicoApt->code2AptFull(aV1.at(n)->getApt(nt,nh,zbio_,false)));
                        messageBox->contents()->addNew<Wt::WBreak>();
                        Wt::WLink l("https://www.fichierecologique.be/resources/fee/FEE-"+aV1.at(n)->Code()+".pdf");
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
                for (int n(0);n<aV2.size();n++){
                    WContainerWidget * c = t1->elementAt(r,col)->addNew<WContainerWidget>();
                    std::string essCode(aV2.at(n)->Code());
                    c->addStyleClass("circle_eco");
                    // check si double apt
                    if (mDicoApt->isDoubleApt(aV2.at(n)->getApt(zbio_))){essCode+="*";}
                    //  if (mDicoApt->isDoubleApt(aV2.at(n)->getApt(nt,nh,zbio_,false))){essCode+="*";}
                    c->addNew<Wt::WText>(essCode);
                    c->mouseWentOver().connect([=] {
                        hoverBubble(c,1);
                        displayNiche(aV2.at(n)->Code());
                        graphZbio->displayAptMap(aV2.at(n)->Code());
                    });
                    c->mouseWentOut().connect([=] {
                        hoverBubble(c,0);
                        resetEco();
                        graphZbio->selectZbio(zbio_);
                    });
                    c->setToolTip(aV2.at(n)->Nom());
                    c->clicked().connect([=] {
                        Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                                          aV2.at(n)->Nom(),
                                                                          "",
                                                                          Wt::Icon::Information,
                                                                          Wt::StandardButton::Ok));
                        messageBox->contents()->addNew<Wt::WText>("Aptitude bioclimatique : " + mDicoApt->code2AptFull(aV2.at(n)->getApt(zbio_)));
                        messageBox->contents()->addNew<Wt::WBreak>();
                        Wt::WLink l("https://www.fichierecologique.be/resources/fee/FEE-"+aV2.at(n)->Code()+".pdf");
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

                    // mise en évidence par couleur grise
                    c->addStyleClass("col-grey"); // n'est pas appliqué car style table-apt2 prend le dessus sur la couleur. c'est un style hérité du parent, donc je dois régler l'hiérarchie entre les deux..
                    // change table-apt3 div en .table-apt3 > div{ et ça fonctionne
                    col++;
                    if (col+1>ncells){col=0;r++;}
                }

            }
        }
    } else {
       resetEco();
    }
}

// s'utilise soit en one shot, soit de manière itérative plusieur fois d'affilé.
void matApt::getVEssCommun(std::vector<std::shared_ptr<cEss>> aV1,std::vector<std::shared_ptr<cEss>> aV2, std::vector<std::shared_ptr<cEss>> & aVCom, std::vector<std::shared_ptr<cEss>> & aVDiff){
    // d'abor avoir toute les essences pour boucler dessus, car elle ne sont pas d'office toute dans les vecteurs qui peuvent avoir des longeurs différentes.
    for (std::shared_ptr<cEss> ess : getAllEssFrom2V(aV1,aV2)){
        if (commonEss(ess->Code(),aV1) & commonEss(ess->Code(),aV2)){
            if (!commonEss(ess->Code(),aVCom) & !commonEss(ess->Code(),aVDiff)){
                // on rajoute
                aVCom.push_back(ess);
            }
        } else {
            if (!commonEss(ess->Code(),aVDiff)){
                aVDiff.push_back(ess);
            }
            if (commonEss(ess->Code(),aVCom)){
                // on retire l'essence du vecteur essence commune
                int c(0);
                for (std::shared_ptr<cEss> ess2 : aVCom){
                    if (ess2->Code()==ess->Code()) { aVCom.erase(aVCom.begin()+c); break;}
                    c++;
                }
            }
        }
    }
}

std::vector<std::shared_ptr<cEss>> matApt::getAllEssFrom2V(std::vector<std::shared_ptr<cEss>> aV1, std::vector<std::shared_ptr<cEss>> aV2){
    std::vector<std::shared_ptr<cEss>> aRes=aV1;
    for (std::shared_ptr<cEss> ess1 : aV2){
        if (!commonEss(ess1->Code(),aV1)){
            aRes.push_back(ess1);
        }
    }
    return aRes;
}


bool commonEss(std::string aCode, std::vector<std::shared_ptr<cEss>> & aV2){
    bool aRes(0);
    for (std::shared_ptr<cEss> ess : aV2){
        if (ess->Code()==aCode){aRes=1;break;}
    }
    return aRes;
}

zbioPainted::zbioPainted(std::string  aShp, std::shared_ptr<cdicoAptBase> aDico)
    : WPaintedWidget(),zbio_(1),shpPath(aShp),mSx(400),mSy(200),displayApt_(0),mDico(aDico),mlay(NULL)
{
    GDALAllRegister();
    resize(mSx,mSy);   // Provide a default size.
    const char *inputPath= shpPath.c_str();
    if (boost::filesystem::exists(inputPath)){
        mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " shp zbio : pas réussi à l'ouvrir." );
        } else{
            // layer
            std::cout << "zbioPainted création " << std::endl;
            mlay = mDS->GetLayer(0);
            ext= new OGREnvelope;
            mlay->GetExtent(ext);
            // taille de l'emprise de l'image  - mettre tout ça une fois dans le constructeur.
            mWx=ext->MaxX-ext->MinX;
            mWy=ext->MaxY-ext->MinY;
        }
    } else {std::cout << inputPath << " : n'existe pas (zbioPainted::zbioPainted)" << std::endl;}
}

void zbioPainted::paintEvent(Wt::WPaintDevice *paintDevice){
    //std::cout << " zbioPainted::paintEvent" << std::endl;
    Wt::WPainter painter(paintDevice);
    if(mlay!=NULL){
        OGRFeature *poFeature;
        mlay->ResetReading();
        while( (poFeature = mlay->GetNextFeature()) != NULL )
        {
            int currentZbio=poFeature->GetFieldAsInteger("Zbio");
            if (displayApt_){
                // choix de la couleur en fonction de l'aptitude de l'essence
                Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Black));
                pen0.setWidth(1);
                painter.setPen(pen0);
                int apt =mDico->getEss(essCoce_)->getApt(currentZbio);
                color col= mDico->Apt2col(apt);
                painter.setBrush(Wt::WBrush(Wt::WColor(col.mR,col.mG,col.mB)));
            } else {
                if (currentZbio==zbio_){
                    Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Yellow));
                    pen0.setWidth(3);
                    painter.setPen(pen0);
                    painter.setBrush(Wt::WBrush(Wt::WColor(Wt::StandardColor::Yellow)));
                } else {
                    Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Black));
                    pen0.setWidth(1);
                    painter.setPen(pen0);
                    painter.setBrush(Wt::WBrush(Wt::WColor(Wt::StandardColor::Green)));
                }
            }
            // on parcours la géométrie
            switch (poFeature->GetGeometryRef()->getGeometryType()){
            case (wkbPolygon):
            {
                OGRPolygon * pol =poFeature->GetGeometryRef()->toPolygon();
                drawPol(pol,&painter);
                break;
            }
            case wkbMultiPolygon:
            {
                OGRMultiPolygon * mulP = poFeature->GetGeometryRef()->toMultiPolygon();
                int n(mulP->getNumGeometries());
                for (int i(0);i<n;i++){
                    OGRGeometry * subGeom=mulP->getGeometryRef(i);
                    if (subGeom->getGeometryType()==wkbPolygon){
                        OGRPolygon * pol =subGeom->toPolygon();
                        drawPol(pol, &painter);
                    }
                }
                break;
            }
            }
        }
    }

}

void zbioPainted::drawPol(OGRPolygon * pol, WPainter *painter){
    //std::cout << " zbioPainted::drawPol" << std::endl;
    if (pol){
        OGRPoint ptTemp1, ptTemp2;
        std::vector<Wt::WLineF> aVLines;
        int NumberOfVertices = pol->getExteriorRing()->getNumPoints();
        //std::cout << "number of verticiles " << std::endl;

        Wt::WPainterPath pp = Wt::WPainterPath();
        for ( int k = 0; k < NumberOfVertices-1; k++ )
        {

            pol->getExteriorRing()->getPoint(k,&ptTemp1);
            pol->getExteriorRing()->getPoint(k+1,&ptTemp2);
            aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
            if (k==0){pp.moveTo(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()));} else {pp.lineTo(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()));}
        }
        pp.closeSubPath();
        painter->drawPath(pp);

        // ajout segment final
        pol->getExteriorRing()->getPoint(0,&ptTemp1);
        pol->getExteriorRing()->getPoint(NumberOfVertices-1,&ptTemp2);
        aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));

        for (int c(0);c<pol->getNumInteriorRings();c++){
            int NumberOfVertices =pol->getInteriorRing(c)->getNumPoints();;
            for ( int k = 0; k < NumberOfVertices-1; k++ )
            {
                pol->getInteriorRing(c)->getPoint(k,&ptTemp1);
                pol->getInteriorRing(c)->getPoint(k+1,&ptTemp2);
                aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
            }
            // ajout segment final
            pol->getInteriorRing(c)->getPoint(0,&ptTemp1);
            pol->getInteriorRing(c)->getPoint(NumberOfVertices-1,&ptTemp2);
            aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
        }

        painter->drawLines(aVLines);
        //std::unique_ptr<Wt::WRectArea> rectA = Wt::cpp14::make_unique<Wt::WRectArea>(posX_.at(codeNT), posY_.at(codeNH), pixPerLevel,pixPerLevel);
        //rectA->setToolTip(label);
        //addArea(std::move(rectA));
    } else { std::cout << "polygone is null" << std::endl;}
}

double zbioPainted::xGeo2Im(double x){
    return mSx*(x-ext->MinX)/mWx;
}
double zbioPainted::yGeo2Im(double y){
    return mSy-(mSy*(y-ext->MinY)/mWy);
}

void matApt::selectLevel4comparison(std::tuple<int,int> ntnh){   
    if(mVNtnh4Comparison.size()==0){
        std::tuple<int,int> ntnhBase(nt_,nh_);
        if (ntnhBase!=ntnh){
        mVNtnh4Comparison.push_back(ntnhBase);
        }
    }
    // on retire le niveau si il est déjà présent
    int c(0); bool remove(0);
    for ( std::tuple<int,int> ntnh2 : mVNtnh4Comparison){
        if (ntnh2==ntnh) { mVNtnh4Comparison.erase(mVNtnh4Comparison.begin()+c); remove=1; break;}
        c++;
    }
    if (!remove | mVNtnh4Comparison.size()==0){
        mVNtnh4Comparison.push_back(ntnh);
    }
    compareMatApt();
}

void matApt::comparison4predicted(){
    std::vector<std::tuple<int,int>> Vntnh;
    if (mVPredictedNtnh.size()>1){
        mVNtnh4Comparison.clear();
        for (auto ntnh : mVPredictedNtnh){
            mVNtnh4Comparison.push_back(ntnh);
        }
        //mVNtnh4Comparison.push_back(mVPredictedNtnh.at(0));
        //mVNtnh4Comparison.push_back(mVPredictedNtnh.at(mVPredictedNtnh.size()-1));
        compareMatApt();
    } else {
        Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                          tr("matApt.compare4Predicted"),
                                                          tr("matApt.compare4Predicted.noPrediction"),
                                                          Wt::Icon::Critical,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->setMinimumSize("40%","30%");
        messageBox->show();
    }

}
