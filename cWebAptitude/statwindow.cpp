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

    auto * tpl2 = contTitre_->addWidget(cpp14::make_unique<Wt::WTemplate>(tr("bouton_retour_parcelaire")));
    createPdfBut = tpl2->bindWidget("retour", Wt::cpp14::make_unique<WPushButton>("Pdf"));

    auto pdf = std::make_shared<ReportResource>(this);
    createPdfBut->setLink(WLink(pdf));
    //createPdfBut->clicked().connect(this,&statWindow::export2pdf);

    mCarteGenCont = addWidget(cpp14::make_unique<WContainerWidget>());
    mCarteGenCont->setId("carteGenStat");

    mAptTable = addWidget(cpp14::make_unique<WTable>());
    mAptTable->setHeaderCount(1);
    mAptTable->columnAt(0)->setWidth("60%");

    mAptTable->setWidth(Wt::WLength("90%"));
    mAptTable->toggleStyleClass("table-striped",true);

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

     mCarteGenCont->addWidget(cpp14::make_unique<olOneLay>(mIGN,poFeature->GetGeometryRef()));
}

void statWindow::export2pdf(){
     std::cout << "statWindow::export2pdf()" << std::endl;
    //std::ostream o
    //std::ostringstream o;
    //this->htmlText(o);
    //std::stringstream ss;
    //ss << o.rdbuf();
    //std::string myString = o.str();

    //std::cout << myString << std::endl;
    auto pdf = std::make_shared<ReportResource>(this);
    //pdf->ren

    //m_app->redirect(pdf->url());
}
