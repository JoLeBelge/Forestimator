#include "statwindow.h"
extern bool globTest;

statWindow::statWindow(groupLayers * aGL):mDico(aGL->Dico()), mApp(aGL->m_app),mGL(aGL)//, sigImgPDF(this,"pdf"), slotImgPDF(this)
{
    if (globTest){std::cout << "statWindow::statWindow" << std::endl;}
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
    retour->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    // bouton export PDF
    if (0){
    createPdfBut = contTitre_->addWidget(cpp14::make_unique<WPushButton>("Export PDF"));
    //createPdfBut->clicked().connect(this,&statWindow::export2pdf);
    }

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

    mAllStatIndivCont = addWidget(cpp14::make_unique<WContainerWidget>());
    mAllStatIndivCont->setId("AllStatIndividuelle");

    mIGN= mDico->getLayerBase("IGNgrfmn");
    mMNT= mDico->getLayerBase("MNT");
    mZBIO = mDico->getLayerBase("ZBIO");
    mPente= mDico->getLayerBase("slope");
}

void statWindow::genIndivCarteAndAptT(){
    for (layerStatChart * chart : mGL->ptrVLStat()) {
        if (chart->deserveChart()){
            if (chart->Lay()->getCatLayer()==TypeLayer::FEE | chart->Lay()->getCatLayer()==TypeLayer::CS){
                add1Aptitude(chart);
            } else {
                add1layerStat(chart);
            }
        }
    }

    for (std::unique_ptr<Wt::WContainerWidget> & chart : mGL->mVLStatCont) {
       // if (chart->deserveChart()){
            // je veux un comportement différent pour
                add1layerStat(std::move(chart));
       // }
    }

    /*if (mGL->mCompo){
        mAllStatIndivCont->addWidget(mGL->mCompo->getResult());
    }*/
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

    mAptTable->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(lstat->Lay()->getLegendLabel(false)));
    // ajout de la barre de statistique
    mAptTable->elementAt(row, 1)->addWidget(lstat->getBarStat());
    mAptTable->elementAt(row, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
}

void statWindow::add1layerStat(layerStatChart * layerStat){  
    // ici ça à l'air de se compliquer car je transforme un ptr layerStat en std::unique_ptr<Wt::WContainerWidget> et cela à l'air de rendre le raw pointer dandling, il sera par la suite deleted dans groupLayer et là segfault
    // donc il faut rester sur la solution qui fait que layerStat crée un chart pour l'affichage, puis crée un autre contenu pour le rendu
    mAllStatIndivCont->addWidget(layerStat->getChart());
    //mContStatIndiv->addWidget(std::unique_ptr<Wt::WContainerWidget>(layerStat));
    std::cout << "statWindow::add1layerStat done" << std::endl;
}
void statWindow::add1layerStat(std::unique_ptr<Wt::WContainerWidget> cont){
    mAllStatIndivCont->addWidget(std::move(cont));
}

void statWindow::vider()
{
    mCarteGenCont->clear();
    mAptTable->clear();
    mAllStatIndivCont->clear();
    mTitre->setText("toto");
}

void statWindow::generateGenCarte(OGRFeature * poFeature){
     WVBoxLayout * layoutV =mCarteGenCont->setLayout(cpp14::make_unique<WVBoxLayout>());
     layoutV->addWidget(cpp14::make_unique<WText>("<h4>Aperçu</h4>"));
     //aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
     WContainerWidget * aContCarte = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
     WHBoxLayout * layoutH = aContCarte->setLayout(cpp14::make_unique<WHBoxLayout>());
     // ajout de la carte pour cette couche
     staticMap sm(mIGN,poFeature->GetGeometryRef());
     Wt::WImage * im =layoutH->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLinkRel()),0);
     im->resize(350,"100%");
     // need to set it here after initialization of the map id !

     // description générale ; lecture des attribut du polygone?calcul de pente, zone bioclim, et élévation
     WContainerWidget * aContInfo = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());

      // je refais les calculs pour les couches qui m'intéressent

     basicStat statMNT= mMNT->computeBasicStatOnPolyg(poFeature->GetGeometryRef());
     basicStat statPente= mPente->computeBasicStatOnPolyg(poFeature->GetGeometryRef());

     aContInfo->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.zbio.t")));

     std::cout << "statWindow::generateGenCarte ---" << std::endl;
     aContInfo->addWidget(cpp14::make_unique<WText>(mZBIO->summaryStat(poFeature->GetGeometryRef())));

     aContInfo->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.relief.t")));

     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude maximum : "+ statMNT.getMax() + " m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude moyenne : "+ statMNT.getMean()+ " m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Altitude minimum : "+ statMNT.getMin()+ " m"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Pente moyenne : "+ statPente.getMean()+ " %"));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());

     // analyse pédo surfacique

     surfPedo statPedo(mDico->mPedo,poFeature->GetGeometryRef());
     aContInfo->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.pedo.t")));
     aContInfo->addWidget(cpp14::make_unique<WText>("Texture : "+ statPedo.getSummary(PEDO::TEXTURE)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Drainage : "+ statPedo.getSummary(PEDO::DRAINAGE)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
     aContInfo->addWidget(cpp14::make_unique<WText>("Profondeur : "+ statPedo.getSummary(PEDO::PROFONDEUR)));
     aContInfo->addWidget(cpp14::make_unique<WBreak>());
}

void statWindow::export2pdf(std::string titre){

    std::cout << "statWindow::export2pdf()" << std::endl;
    // création du pdf
    HPDF_Doc pdf = HPDF_New(error_handler, 0);

    HPDF_UseUTFEncodings(pdf);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);// sinon pdf fait 5 Mo pour rien du tout

    MyRenderer renderer(pdf, page,titre,mDico);

    // post sur l'export des widgets vers pdf - pas super clair mais informatif
    //https://redmine.webtoolkit.eu/boards/2/topics/14392?r=14462#message-14462
    Wt::WString tpl = tr("report.analyse.surf");
    std::string tp = tpl.toUTF8();

// cette boucle fait crasher wt avec un g is null de javascript, à la fin de la méthode. Je sais pas pourquoi.
  /* for (layerStatChart * stat :VStatIndiv()){
        std::ostringstream o;
        //if (VStatIndiv().size()>0){
        // ça ça fait crasher mais pas
        stat->getChart(1)->htmlText(o);
        // pas cette ligne
        //stat->getChart(1);
        //boost::replace_all(tp,"${InfoT}",o.str());
        o.str("");
        o.clear();
    }*/
    //boost::replace_all(tp,"${InfoT}",o.str());

    //boost::replace_all(tp,"${AptTable}",o.str()); // c'est buggé donc juste desactive pour instant...
    //boost::replace_all(tp,"${carte_static_1}",strImgPDF);

    renderer.render(tp);

    std::string name0 = std::tmpnam(nullptr);
    std::string name1 = name0.substr(5,name0.size()-5);
    std::string aOut = mDico->File("TMPDIR")+"/"+name1+".pdf";

    HPDF_SaveToFile(pdf,aOut.c_str());
    HPDF_Free(pdf);

     //std::unique_ptr<WFileResource> fileResource = std::make_unique<Wt::WFileResource>("plain/text",aOut);
     WFileResource *fileResource = new Wt::WFileResource("application/pdf",aOut);
    fileResource->suggestFileName("Forestimator-info-parcelaire.pdf");
    mApp->redirect(fileResource->url());


}


void statWindow::renderPdf(Wt::Render::WPdfRenderer * renderer){
    if (globTest){std::cout << "statWindow::render" << std::endl;}
}
