#include "statwindow.h"
extern bool globTest;

statWindow::statWindow(groupLayers * aGL):mDico(aGL->Dico()), m_app(aGL->m_app),mGL(aGL)//, sigImgPDF(this,"pdf"), slotImgPDF(this)
{
    if (globTest){std::cout << "statWindow::statWindow" << std::endl;}
    setId("statWindow");
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    setMargin(1,Wt::Side::Bottom | Wt::Side::Top);
    setOverflow(Overflow::Auto);
    addStyleClass("statWindow");

    WContainerWidget * contTitre_ =  addWidget(std::make_unique<Wt::WContainerWidget>());
    mTitre = contTitre_->addWidget(std::make_unique<WText>());
    mTitre->setId("statWindowTitre");
    contTitre_->addWidget(std::make_unique<WText>(tr("infoDansVisuStat")));
    // bouton retour
    auto * tpl = contTitre_->addWidget(std::make_unique<Wt::WTemplate>(tr("bouton_retour_parcelaire")));
    WPushButton * retour = tpl->bindWidget("retour", std::make_unique<WPushButton>("Retour"));
    retour->setLink(WLink(LinkType::InternalPath, "/cartographie"));
    // bouton export PDF
    createPdfBut = contTitre_->addWidget(std::make_unique<WPushButton>(tr("ana.pt.export.pdf")));

    mCarteGenCont = addWidget(std::make_unique<WContainerWidget>());
    mCarteGenCont->setId("carteGenStat");
    mCarteGenCont->setInline(0);
    mCarteGenCont->setOverflow(Wt::Overflow::Auto);

    mAptTable = addWidget(std::make_unique<WTable>());
    mAptTable->setId("AptitudeTable");
    mAptTable->setHeaderCount(1);
    mAptTable->columnAt(0)->setWidth("60%");

    mAptTable->setWidth(Wt::WLength("90%"));
    mAptTable->toggleStyleClass("table-striped",true);

    mAllStatIndivCont = addWidget(std::make_unique<WContainerWidget>());
    mAllStatIndivCont->setId("AllStatIndividuelle");

    mIGN= mDico->getLayerBase("IGNgrfmn");
    mMNT= mDico->getLayerBase("MNT");
    mZBIO = mDico->getLayerBase("ZBIO");
    mPente= mDico->getLayerBase("slope");
}

void statWindow::genIndivCarteAndAptT(){
    for (std::shared_ptr<layerStatChart> chart : mGL->ptrVLStat()) {
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

void statWindow::add1Aptitude(std::shared_ptr<layerStatChart> lstat){
    int row=mAptTable->rowCount();
    if (row==0){
        mAptTable->elementAt(0, 0)->setColumnSpan(2);
        mAptTable->elementAt(0, 0)->addWidget(std::make_unique<WText>(tr("StatWTitreTabApt")));
        mAptTable->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptTable->elementAt(0, 0)->setPadding(10);
        mAptTable->elementAt(1, 0)->addWidget(std::make_unique<WText>("Essence"));
        mAptTable->elementAt(1, 1)->addWidget(std::make_unique<WText>("Aptitude"));
        mAptTable->elementAt(1, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptTable->elementAt(1, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        row=2;
    }

    mAptTable->elementAt(row, 0)->addWidget(std::make_unique<WText>(lstat->Lay()->getLegendLabel(false)));
    // ajout de la barre de statistique
    mAptTable->elementAt(row, 1)->addWidget(lstat->getBarStat());
    mAptTable->elementAt(row, 1)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
}

void statWindow::add1layerStat(std::shared_ptr<layerStatChart> layerStat){
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

    std::cout << "statWindow::generateGenCarte ---" << std::endl;

    WVBoxLayout * layoutV =mCarteGenCont->setLayout(std::make_unique<WVBoxLayout>());
    layoutV->addWidget(std::make_unique<WText>("<h4>Aperçu</h4>"));
    //aRes->addWidget(std::make_unique<Wt::WBreak>());
    WContainerWidget * aContCarte = layoutV->addWidget(std::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = aContCarte->setLayout(std::make_unique<WHBoxLayout>());
    // ajout de la carte pour cette couche
    staticMap sm(mIGN,poFeature->GetGeometryRef());
    Wt::WImage * im =layoutH->addWidget(std::make_unique<Wt::WImage>(sm.getWLinkRel()),0);
    im->resize(350,"100%");
    // need to set it here after initialization of the map id !

    // description générale ; lecture des attribut du polygone?calcul de pente, zone bioclim, et élévation
    WContainerWidget * aContInfo = layoutH->addWidget(std::make_unique<WContainerWidget>());

    // je refais les calculs pour les couches qui m'intéressent

    basicStat statMNT= mMNT->computeBasicStatOnPolyg(poFeature->GetGeometryRef());
    basicStat statPente= mPente->computeBasicStatOnPolyg(poFeature->GetGeometryRef());

    // info de surface, en ha et en m2
    OGRMultiPolygon * pol =poFeature->GetGeometryRef()->toMultiPolygon();
    double surf_m=pol->get_Area();
    aContInfo->addWidget(std::make_unique<WText>(Wt::WString::tr("report.analyse.surf.area").arg(roundDouble(surf_m/10000.0)).arg(roundDouble(surf_m,0))));

    aContInfo->addWidget(std::make_unique<WText>(Wt::WString::tr("report.analyse.surf.zbio.t")));

    aContInfo->addWidget(std::make_unique<WText>(mZBIO->summaryStat(poFeature->GetGeometryRef())));

    aContInfo->addWidget(std::make_unique<WText>(Wt::WString::tr("report.analyse.surf.relief.t")));

    aContInfo->addWidget(std::make_unique<WText>("Altitude maximum : "+ statMNT.getMax() + " m"));
    aContInfo->addWidget(std::make_unique<WBreak>());
    aContInfo->addWidget(std::make_unique<WText>("Altitude moyenne : "+ statMNT.getMean()+ " m"));
    aContInfo->addWidget(std::make_unique<WBreak>());
    aContInfo->addWidget(std::make_unique<WText>("Altitude minimum : "+ statMNT.getMin()+ " m"));
    aContInfo->addWidget(std::make_unique<WBreak>());
    aContInfo->addWidget(std::make_unique<WText>("Pente moyenne : "+ statPente.getMean()+ " %"));
    aContInfo->addWidget(std::make_unique<WBreak>());

    // analyse pédo surfacique

    surfPedo statPedo(mDico->mPedo,poFeature->GetGeometryRef());
    aContInfo->addWidget(std::make_unique<WText>(Wt::WString::tr("report.analyse.surf.pedo.t")));
    aContInfo->addWidget(std::make_unique<WText>("Texture : "+ statPedo.getSummary(PEDO::TEXTURE)));
    aContInfo->addWidget(std::make_unique<WBreak>());
    aContInfo->addWidget(std::make_unique<WText>("Drainage : "+ statPedo.getSummary(PEDO::DRAINAGE)));
    aContInfo->addWidget(std::make_unique<WBreak>());
    aContInfo->addWidget(std::make_unique<WText>("Profondeur : "+ statPedo.getSummary(PEDO::PROFONDEUR)));
    aContInfo->addWidget(std::make_unique<WBreak>());
}


void surfPdfResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    if (globTest) {std::cout << "\n surfPdfResource handle request \n " << std::endl;}

    HPDF_Doc pdf = HPDF_New(error_handler, 0);

    HPDF_UseUTFEncodings(pdf);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);// sinon pdf fait 5 Mo pour rien du tout

    std::string titre("Analyse Surfacique");
    MyRenderer renderer(pdf, page,titre,mSW->mDico);

    Wt::WString tpl = Wt::WText::tr("report.analyse.surf");
    std::string tp = tpl.toUTF8();
    std::ostringstream o;
    //std::string htmlText;
    boost::replace_all(tp,"TITRE_REPPORT",titre);

    mSW->mTitre->htmlText(o);
    mSW->mCarteGenCont->htmlText(o);
    boost::replace_all(tp,"mCarteGenCont",o.str());

    o.str("");// en faut au lieu de vider à chaque fois le streamstream, je peux ajouter tout mon htmltext dedans
    mSW->mAllStatIndivCont->htmlText(o);
    boost::replace_all(tp,"mAllStatIndivCont",o.str());

    o.str("");
    // le plus délicat, c'est ce portage des WpaintedWidget batonnet qui sont dans la table d'aptitude vers le pdf
    //Je pourrais très simplement faire un "StatIndivCont" qui est générique à toute les cartes-> prend une autre forme et plus de place dans le pdf mais ça reste complêt et très bien.
    for (std::shared_ptr<layerStatChart> chart : mSW->mGL->ptrVLStat()) {
        if (chart->deserveChart()){
            if (chart->Lay()->getCatLayer()==TypeLayer::FEE | chart->Lay()->getCatLayer()==TypeLayer::CS){
                // problème : le layerStatChart d'une aptitude dois pouvoir utiliser la méthode getChart au lieu de getBarStat
               std::unique_ptr<WContainerWidget> toto= chart->getChart(1);
               toto->htmlText(o);
               mSW->mAllStatIndivCont->addWidget(std::move(toto));// rigolo, ça bug pas quand je déplace l'objet ici, mais si je ne fait rien avec, ça bug. Ces smartptr sont peut-être pas assez malin pour cette situation présente.
            }
        }
    }
    boost::replace_all(tp,"mApt",o.str());
    // le htmlText renseigne le chemin d'accès à des images en ommettant le docroot, alors que le pdfrenderer lui à besoin du chemin d'accès complêt.
    boost::replace_all(tp,"src=\"","src=\""+mSW->mDico->File("docroot"));

    o.str("");
    o.clear();

    renderer.render(tp);
    response.setMimeType("application/pdf");
    HPDF_SaveToStream (pdf);
    unsigned int size = HPDF_GetStreamSize (pdf);
    HPDF_BYTE * buf = new HPDF_BYTE [size];
    HPDF_ReadFromStream (pdf, buf, & size);
    HPDF_Free (pdf);

    response.out (). write ((char *) buf, size);
    delete [] buf;
    mSW->mGL->m_app->addLog("pdf ana surf", typeLog::danas);
}
