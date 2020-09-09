#include "statwindow.h"

statWindow::statWindow(cDicoApt *aDico):mDico(aDico)
{
    setId("statWindow");
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    setMargin(1,Wt::Side::Bottom | Wt::Side::Top);
    setOverflow(Overflow::Auto);
    addStyleClass("statWindow");


    WContainerWidget * contTitre_ =  addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    mTitre = contTitre_->addWidget(cpp14::make_unique<WText>());
    contTitre_->addWidget(cpp14::make_unique<WText>(tr("infoDansVisuStat")));
    // bouton retour
    auto * tpl = contTitre_->addWidget(cpp14::make_unique<Wt::WTemplate>(tr("bouton_retour_parcelaire")));
    WPushButton * retour = tpl->bindWidget("retour", Wt::cpp14::make_unique<WPushButton>("Retour"));
    retour->setLink(WLink(LinkType::InternalPath, "/analyse"));

    mCarteGenCont = addWidget(cpp14::make_unique<WContainerWidget>());
    mCarteGenCont->setId("carteGenStat");

    mAptTable = addWidget(cpp14::make_unique<WTable>());
    mAptTable->setHeaderCount(1);
    mAptTable->columnAt(0)->setWidth("60%");

    mAptTable->setWidth(Wt::WLength("90%"));
    mAptTable->toggleStyleClass("table-striped",true);

    // création de la carte IGN, nécessaire pour pouvoir l'afficher dans la carte générale
    auto labelBidon_ = cpp14::make_unique<WText>();
    WText * labelBidon = labelBidon_.get();

    mIGN = new Layer("IGN",mDico,TypeLayer::Externe);
}

void statWindow::add1Aptitude(layerStatChart * lstat){
    int row=mAptTable->rowCount();
    if (row==0){
        mAptTable->elementAt(0, 0)->setColumnSpan(2);
        mAptTable->elementAt(0, 0)->addWidget(cpp14::make_unique<WText>(tr("StatWTitreTabApt")));
        mAptTable->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptTable->elementAt(0, 0)->setPadding(10);
        mAptTable->elementAt(1, 0)->addWidget(cpp14::make_unique<WText>("Essence"));
        mAptTable->elementAt(1, 1)->addWidget(cpp14::make_unique<WText>("Aptitude"));
        mAptTable->elementAt(0, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        row=2;
    }

    mAptTable->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(lstat->Lay()->getLegendLabel()));
    // ajout de la barre de statistique
    mAptTable->elementAt(row, 1)->addWidget(std::unique_ptr<Wt::WContainerWidget>(lstat->getBarStat()));
    mAptTable->elementAt(row, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
}
void statWindow::add1layerStat(Wt::WContainerWidget * layerStat){
    mVContStatIndiv.push_back(layerStat);
    addWidget(std::unique_ptr<Wt::WContainerWidget>(layerStat));
    //mContStatIndiv->addWidget(std::unique_ptr<Wt::WContainerWidget>(layerStat));
}


void statWindow::vider()
{
    mCarteGenCont->clear();
    mAptTable->clear();
    for (WContainerWidget * p :  mVContStatIndiv){
        removeWidget(p);
    }
    mVContStatIndiv.clear();
    mTitre->setText("toto");
}

void statWindow::generateGenCarte(OGRFeature * poFeature){

     mCarteGenCont->addWidget(cpp14::make_unique<olOneLay>(mIGN,poFeature->GetGeometryRef()));
}
