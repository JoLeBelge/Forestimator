#include "legend.h"


legend::legend(groupLayers *aGL, WContainerWidget *parent):mGL(aGL),mParent(parent),mDico(aGL->Dico())
{
    createUI();
}
/*
legend::legend(WContainerWidget *parent):mParent(parent)
{
    createUI();
}*/

void legend::createUI()
{
    //mTitle = mParent->addWidget(cpp14::make_unique<WText>(WString::tr("legendMsg")));

    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    mParent->setMargin(1,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);// si pas inline Et bizarrement si pas de setMargin autre que 0, pas de scrollbar pour l'overflow!

    // bouton retour
    WPushButton * retour = mParent->addWidget(Wt::cpp14::make_unique<WPushButton>("Retour"));
    retour->addStyleClass("btn btn-info");
    //retour->setLink(WLink(LinkType::InternalPath, "/analyse"));
    retour->clicked().connect([=]{mGL->mStackInfoPtr->stack_info->setCurrentIndex(2);;});


    mAptAllEss = mParent->addWidget(cpp14::make_unique<WTable>());
    mAptAllEss->setHeaderCount(1);
    mAptAllEss->setWidth(Wt::WLength("90%"));
    mAptAllEss->toggleStyleClass("table-striped",true);

    mDetAptFEE = mParent->addWidget(cpp14::make_unique<WTable>());
    mDetAptFEE->setHeaderCount(1);
    mDetAptFEE->setWidth(Wt::WLength("90%"));
    mDetAptFEE->toggleStyleClass("table-striped",true);

    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mContEco = mParent->addWidget(cpp14::make_unique<Wt::WContainerWidget>());
    mContEco->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);

    mInfoT = mParent->addWidget(cpp14::make_unique<WTable>());
    mInfoT->setHeaderCount(2);
    mInfoT->setWidth(Wt::WLength("90%"));
    mInfoT->toggleStyleClass("table-striped",true);

}

void legend::vider()
{
	//mTitle->setStyleClass("nonvisible");
    //mTitle->setText(WString::tr("legendTitre"));
    mInfoT->clear();
    mDetAptFEE->clear();
    mAptAllEss->clear();

    mContEco->clear();
}

void legend::titreInfoRaster(){
    mInfoT->elementAt(0, 0)->setColumnSpan(2);
    mInfoT->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    mInfoT->elementAt(0, 0)->setPadding(10);
    //WText *titre = mInfoT->elementAt(0,0)->addWidget(cpp14::make_unique<WText>("<h4>Description de la station forestière </h4>"));
    mInfoT->elementAt(0,0)->addWidget(cpp14::make_unique<WText>("<h4>Description de la station forestière </h4>"));
    mInfoT->elementAt(1, 0)->addWidget(cpp14::make_unique<WText>("Raster"));
    mInfoT->elementAt(1, 1)->addWidget(cpp14::make_unique<WText>("Valeur"));
}

void legend::add1InfoRaster(std::vector<std::string> aV){

    if (aV.size()>1 && aV.at(1)!=""){
        int row=mInfoT->rowCount();
        //auto t1 = mInfoT->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(aV.at(0)));
        mInfoT->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(aV.at(0)));
        auto t2 =mInfoT->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aV.at(1)));

        if (aV.size()>2 && aV.at(2)=="bold"){
            mInfoT->elementAt(row, 0)->setStyleClass("bold");
            t2->setStyleClass("bold");
        }
    }
}

void legend::detailCalculAptFEE(ST * aST){

    cEss  * Ess= aST->mActiveEss;
    //std::cout << " je vais afficher le détail du calcul de l'aptitude FEE pour " <<Ess->Nom() <<std::endl;
    int row(0);
    mDetAptFEE->elementAt(row, 0)->setColumnSpan(2);
    mDetAptFEE->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    mDetAptFEE->elementAt(row, 0)->setPadding(10);
    //WText *titre = mDetAptFEE->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Détail de la détermination de l'aptitude FEE pour "+Ess->Nom()+"</h4>"));
    mDetAptFEE->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Détail de la détermination de l'aptitude FEE pour "+Ess->Nom()+"</h4>"));
    row++;
    mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude bioclimatique"));
    mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(Ess->getApt(aST->mZBIO))));
    row++;
    mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude hydro-trophique"));
    mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(Ess->getApt(aST->mNT,aST->mNH,aST->mZBIO,false))));
    row++;
    // test si apt bioclim et apt hydrotroph sont les même?
    mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude la plus contraignante :"));
    int apt=Ess->getApt(aST->mNT,aST->mNH,aST->mZBIO,true);
    mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(apt)));
    row++;
    if ( Ess->hasRisqueComp(aST->mZBIO,aST->mTOPO)) {
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Situation Topographique"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->TOPO()));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("rique pour l'essence :"));
        int risque=Ess->getRisque(aST->mZBIO,aST->mTOPO);
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->Risque(risque)));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude Finale :"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(Ess->corrigAptRisqueTopo(apt,aST->mTOPO,aST->mZBIO))));
    }
    // un titre pour l'écogramme
    mContEco->addWidget(cpp14::make_unique<WText>(tr("titreEcogramme")));
    mEcoEss = mContEco->addWidget(Wt::cpp14::make_unique<EcogrammeEss>(Ess,aST));
    mContEco->addWidget(cpp14::make_unique<WText>(tr("legendEcogramme")));
}

/*void legend::afficheLegendeIndiv(const Layer * l){

    if (l->Type()!=TypeLayer::Externe){
        // vider la légende et afficher la légende personnelle de la couche active
        //std::cout << " je vais afficher la légende personnalisée pour " <<l->getLegendLabel() <<std::endl;
        vider();
        int row(0);
        mLegendIndiv->elementAt(row, 0)->setColumnSpan(2);
        mLegendIndiv->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mLegendIndiv->elementAt(row, 0)->setPadding(10);
        //WText *titre = mLegendIndiv->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>"+l->getLegendLabel()+"</h4>"));
        mLegendIndiv->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>"+l->getLegendLabel()+"</h4>"));
        row++;
        for (auto kv : *l->mDicoVal){
            if (l->hasColor(kv.first)){
                color col = l->getColor(kv.first);
                mLegendIndiv->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(kv.second));
                mLegendIndiv->elementAt(row, 1)->setWidth("40%");
                mLegendIndiv->elementAt(row, 1)->decorationStyle().setBackgroundColor(WColor(col.mR,col.mG,col.mB));
                row++;
            }
        }
    }
}*/

void legend::afficheAptAllEss(){

    std::map<std::string, int> Apts = mGL->apts();
    if (Apts.size()>1){

        // on splitte le vecteur aptitudes en 4 vecteurs, qui seront dans des colonnes différentes
        std::map<std::string, int> O;
        std::map<std::string, int> T;
        std::map<std::string, int> TE;
        std::map<std::string, int> E;
        for (auto & kv : Apts){
            switch (mGL->Dico()->AptContraignante(kv.second)){
            case 1:
                O.emplace(kv);
                break;
            case 2:
                T.emplace(kv);
                break;
            case 3:
                TE.emplace(kv);
                break;
            case 4:
                E.emplace(kv);
                break;
            }
        }

        int nbCol(4);
        int row(0),column(0);
        mAptAllEss->elementAt(row, 0)->setColumnSpan(nbCol);
        mAptAllEss->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptAllEss->elementAt(row, 0)->setPadding(10);
        //WText *titre = mAptAllEss->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Aptitude "+ mGL->TypeClasStr()+"</h4>"));
        mAptAllEss->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Aptitude "+ mGL->TypeClasStr()+"</h4>"));
        row++;
        color col(0,0,0);
        if (O.size()>1){
            col = mGL->Dico()->Apt2col(1);
            for (auto & kv : O){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("O");
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=0;
        }

        if (T.size()>1){
            col = mGL->Dico()->Apt2col(2);
            for (auto & kv : T){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                //mAptAllEss->elementAt(row, column)->decorationStyle().setBackgroundColor(WColor(col.mR,col.mG,col.mB));
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second!=2) {
                    mAptAllEss->elementAt(row, column)->setToolTip( mAptAllEss->elementAt(row, column)->toolTip()+ " - " +WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor("gray"));
                }
                mAptAllEss->elementAt(row, column)->setStyleClass("T");
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=0;
        }
        if (TE.size()>1){
            col = mGL->Dico()->Apt2col(3);
            for (auto & kv : TE){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("TE");
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second!=3) {
                    mAptAllEss->elementAt(row, column)->setToolTip( mAptAllEss->elementAt(row, column)->toolTip()+ " - " +WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor("gray"));
                }
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=0;
        }
        if (E.size()>1){
            col = mGL->Dico()->Apt2col(4);
            for (auto & kv : E){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("E");
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second!=4) {
                    mAptAllEss->elementAt(row, column)->setToolTip( mAptAllEss->elementAt(row, column)->toolTip()+ " - " +WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor("gray"));
                }
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);

                //WImage * i1 = new WImage("data/img/E.png",mAptAllEss->elementAt(row, column));
                //Wt::WImage *i1 = mAptAllEss->elementAt(row, column)->addNew<Wt::WImage>(Wt::WLink("data/img/E.png"));
                //i1->resize("100%","100%");
                row++;
            }
            column++;
            row=0;
        }
    }
}
