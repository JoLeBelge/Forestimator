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
    mTitre->setId("statWindowTitre");
    contTitre_->addWidget(cpp14::make_unique<WText>(tr("infoDansVisuStat")));
    // bouton retour
    auto * tpl = contTitre_->addWidget(cpp14::make_unique<Wt::WTemplate>(tr("bouton_retour_parcelaire")));
    WPushButton * retour = tpl->bindWidget("retour", Wt::cpp14::make_unique<WPushButton>("Retour"));
    retour->setLink(WLink(LinkType::InternalPath, "/analyse"));

    /*
     *ON met cette partie en standby
     * auto * tpl2 = contTitre_->addWidget(cpp14::make_unique<Wt::WTemplate>(tr("bouton_retour_parcelaire")));
    createPdfBut = tpl2->bindWidget("retour", Wt::cpp14::make_unique<WPushButton>("Pdf"));

    auto pdf = std::make_shared<ReportResource>(this);
    createPdfBut->setLink(WLink(pdf));
    */
    //createPdfBut->clicked().connect(this,&statWindow::export2pdf);

    mCarteGenCont = addWidget(cpp14::make_unique<WContainerWidget>());
    mCarteGenCont->setId("carteGenStat");
    mCarteGenCont->setInline(0);
    mCarteGenCont->setOverflow(Wt::Overflow::Auto);


    mAptTable = addWidget(cpp14::make_unique<WTable>());
    mAptTable->setId("AptitudeTable");
    mAptTable->setHeaderCount(1);
    mAptTable->columnAt(0)->setWidth("60%");

    mAptTable->setWidth(Wt::WLength("90%"));
    mAptTable->toggleStyleClass("table-striped",true);

    mIGN = new Layer("IGN",mDico,TypeLayer::Thematique);
    mMNT = new Layer("MNT",mDico,TypeLayer::Thematique);
    mZBIO = new Layer("ZBIO",mDico,TypeLayer::Thematique);
    mPente = new Layer("slope",mDico,TypeLayer::Thematique);
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
        mAptTable->elementAt(1, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptTable->elementAt(1, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
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

     WVBoxLayout * layoutV =mCarteGenCont->setLayout(cpp14::make_unique<WVBoxLayout>());
     layoutV->addWidget(cpp14::make_unique<WText>("<h4>Apperçu</h4>"));
     //aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
     WContainerWidget * aContCarte = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
     WHBoxLayout * layoutH = aContCarte->setLayout(cpp14::make_unique<WHBoxLayout>());
     // ajout de la carte pour cette couche
     layoutH->addWidget(cpp14::make_unique<olOneLay>(mIGN,poFeature->GetGeometryRef()),0);
     // description générale ; lecture des attribut du polygone?calcul de pente, zone bioclim, et élévation
     WContainerWidget * aContInfo = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
     // je refais les calculs pour les couches qui m'intéressent
     basicStat statMNT= mMNT->computeBasicStatOnPolyg(poFeature->GetGeometryRef());
     basicStat statPente= mPente->computeBasicStatOnPolyg(poFeature->GetGeometryRef());

     aContInfo->addWidget(cpp14::make_unique<WText>("<h5>Zone bioclimatique</h5>"));

     aContInfo->addWidget(cpp14::make_unique<WText>(mZBIO->summaryStat(poFeature->GetGeometryRef())));

     aContInfo->addWidget(cpp14::make_unique<WText>("<h5>Relief</h5>"));
     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude maximum : "+ roundDouble(statMNT.max) + "m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude moyenne : "+ roundDouble(statMNT.mean)+ "m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude maximum : "+ roundDouble(statMNT.min)+ "m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Pente moyenne : "+ roundDouble(statPente.mean)+ "%"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());


     // analyse pédo surfacique
     surfPedo statPedo(mDico->mPedo,poFeature->GetGeometryRef());
     aContInfo->addWidget(cpp14::make_unique<WText>("<h5>Pédologie</h5>"));
     aContInfo->addWidget(cpp14::make_unique<WText>("Texture : "+ statPedo.getSummary(PEDO::TEXTURE)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Drainage : "+ statPedo.getSummary(PEDO::DRAINAGE)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Profondeur : "+ statPedo.getSummary(PEDO::PROFONDEUR)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());


}

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {aRes=std::to_string(d+0.5).substr(0, std::to_string(d).find("."));}
   return aRes;
}


void statWindow::export2pdf(){
     std::cout << "statWindow::export2pdf()" << std::endl;

}
