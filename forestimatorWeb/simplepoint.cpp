#include "simplepoint.h"
#include <boost/filesystem.hpp>

simplepoint::simplepoint(cWebAptitude *app) : m_app(app), mDico(app->mDico), createPdfBut(NULL),mNT(666),mNH(666),mZBIO(666),mTOPO(666),mActiveEss(NULL),HaveEss(0),mSt(0),mEmpty(1)
{
    addStyleClass("content_info");

    mIntroTxt = addWidget(std::make_unique<WText>(tr("sp_infoclic")));
    addWidget(std::make_unique<Wt::WBreak>());
    createPdfBut = addWidget(std::make_unique<WPushButton>(tr("ana.pt.export.pdf")));
    auto pdf = std::make_shared<pointPdfResource>(this);
    auto pdfLink = Wt::WLink(pdf);
    pdfLink.setTarget(Wt::LinkTarget::NewWindow);
    createPdfBut->setLink(pdfLink);

    mAptAllEss = addWidget(std::make_unique<WTable>());
    mAptAllEss->setHeaderCount(1);
    mAptAllEss->setWidth(Wt::WLength("90%"));
    mAptAllEss->toggleStyleClass("table-striped", true);

    mDetAptFEE = addWidget(std::make_unique<WTable>());
    mDetAptFEE->setHeaderCount(1);
    mDetAptFEE->setWidth(Wt::WLength("90%"));
    mDetAptFEE->toggleStyleClass("table-striped", true);

    addWidget(std::make_unique<Wt::WBreak>());
    mContEco = addWidget(std::make_unique<Wt::WContainerWidget>());
    mContEco->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);

    mInfoT = addWidget(std::make_unique<WTable>());
    mInfoT->setId("infoT");
    mInfoT->setHeaderCount(1);
    mInfoT->setWidth(Wt::WLength("90%"));
    mInfoT->toggleStyleClass("table-striped", true);
}

void simplepoint::vider()
{
    mInfoT->clear();
    mDetAptFEE->clear();
    mAptAllEss->clear();
    mContEco->clear();
    mIntroTxt->hide();
    initStation();
}

void simplepoint::initStation(){
    mNT=666;
    mNH=666;
    mZBIO=666;
    mTOPO=666;
    HaveEss=0;
    mSt=666;
    hasFEEApt=0;
    mEmpty=1;
}

void simplepoint::add1InfoRaster(std::vector<std::string> aV)
{
    if (aV.size() > 1 && aV.at(1) != "")
    {
        int row = mInfoT->rowCount();
        mInfoT->elementAt(row, 0)->addWidget(std::make_unique<WText>(WString::fromUTF8(aV.at(0))));
        auto t2 = mInfoT->elementAt(row, 1)->addWidget(std::make_unique<WText>(aV.at(1)));
        mInfoT->elementAt(row, 1)->setContentAlignment(AlignmentFlag::Right);

        if (aV.size() > 2 && aV.at(2) == "bold")
        {
            mInfoT->elementAt(row, 0)->setStyleClass("bold");
            t2->setStyleClass("bold");
        }
    }
}

void simplepoint::detailCalculAptFEE()
{

    std::shared_ptr<cEss> Ess = mActiveEss;
    // std::cout << " je vais afficher le détail du calcul de l'aptitude FEE pour " <<Ess->Nom() <<std::endl;
    int row(0);
    mDetAptFEE->elementAt(row, 0)->setColumnSpan(2);
    mDetAptFEE->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    mDetAptFEE->elementAt(row, 0)->setPadding(10);
    // WText *titre = mDetAptFEE->elementAt(row,0)->addWidget(std::make_unique<WText>("<h4>Détail de la détermination de l'aptitude FEE pour "+Ess->Nom()+"</h4>"));
    mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("<h4>Détail de la détermination de l'aptitude FEE pour " + Ess->Nom() + "</h4>"));
    row++;
    mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Aptitude bioclimatique"));
    mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->code2AptFull(Ess->getApt(mZBIO))));
    row++;
    mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Aptitude hydro-trophique"));
    mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->code2AptFull(Ess->getApt(mNT, mNH, mZBIO, false))));
    row++;
    int apt = Ess->getApt(mNT, mNH, mZBIO, true);
    int aptComp = Ess->getApt(mNT, mNH, mZBIO, true, mTOPO);
    if (aptComp != apt)
    {
        // la compensation liée à la situation topographique impacte l'aptitude finale :
        mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Situation Topographique"));
        mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(TOPO()));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("rique pour l'essence :"));
        mDetAptFEE->elementAt(row, 0)->setToolTip(tr("tooltip.compensationTopo"));
        ;
        int risque = Ess->getRisque(mZBIO, mTOPO);
        mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->Risque(risque)));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Aptitude bioclimatique avec micro-climat:"));
        mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->code2AptFull(Ess->corrigAptBioRisqueTopo(Ess->getApt(mZBIO), mTOPO, mZBIO))));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Aptitude Finale :"));
        mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->code2AptFull(aptComp)));
        mDetAptFEE->elementAt(row, 1)->setToolTip(tr("tooltip.compensationTopo"));
    }
    else
    {
        mDetAptFEE->elementAt(row, 0)->addWidget(std::make_unique<WText>("Aptitude la plus contraignante :"));
        mDetAptFEE->elementAt(row, 1)->addWidget(std::make_unique<WText>(mDico->code2AptFull(apt)));
        row++;
    }
    // un titre pour l'écogramme
    mContEco->addWidget(std::make_unique<WText>(tr("titreEcogramme")));
    mEcoEss = mContEco->addWidget(std::make_unique<EcogrammeEss>(Ess.get(), this));
    mContEco->addWidget(std::make_unique<WText>(tr("legendEcogramme")));
}

void simplepoint::afficheAptAllEss()
{
    std::map<std::string, int> Apts = apts(TypeClassifST::FEE);
    if (Apts.size() > 1)
    {
        if (globTest)
        {
            std::cout << "Apts.size() : " << Apts.size() << std::endl;
        }

        // on splitte le vecteur aptitudes en 4 vecteurs, qui seront dans des colonnes différentes
        std::map<std::string, int> O;
        std::map<std::string, int> T;
        std::map<std::string, int> TE;
        std::map<std::string, int> E;
        for (auto &kv : Apts)
        {
            switch (mDico->AptContraignante(kv.second))
            {
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

            default:
                if (globTest){std::cout << "aptitude code non pris en compte" << std::endl;}
                break;
            }
        }

        int nbCol(4);
        int row(0), column(0);
        mAptAllEss->elementAt(row, 0)->setColumnSpan(nbCol);
        mAptAllEss->elementAt(row, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
        mAptAllEss->elementAt(row, 0)->setPadding(10);
        mAptAllEss->elementAt(row, 0)->addWidget(std::make_unique<WText>("<h4>Aptitude FEE </h4>"));
        row++;
        std::shared_ptr<color> col = std::make_shared<color>(0, 0, 0);
        if (O.size() > 0)
        {
            col = mDico->Apt2col(1);
            for (auto &kv : O)
            {
                mAptAllEss->elementAt(row, column)->addWidget(std::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("O");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setToolTip(mDico->accroEss2Nom(kv.first));
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row = 1;
        }

        if (T.size() > 0)
        {
            col = mDico->Apt2col(2);
            for (auto &kv : T)
            {
                mAptAllEss->elementAt(row, column)->addWidget(std::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setToolTip(mDico->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second != 2)
                {
                    mAptAllEss->elementAt(row, column)->setToolTip(mAptAllEss->elementAt(row, column)->toolTip() + " - " + WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor(120, 120, 120));
                }
                mAptAllEss->elementAt(row, column)->setStyleClass("T");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row = 1;
        }
        if (TE.size() > 0)
        {
            col = mDico->Apt2col(3);
            for (auto &kv : TE)
            {
                mAptAllEss->elementAt(row, column)->addWidget(std::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("TE");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setToolTip(mDico->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second != 3)
                {
                    mAptAllEss->elementAt(row, column)->setToolTip(mAptAllEss->elementAt(row, column)->toolTip() + " - " + WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor(120, 120, 120));
                }
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row = 1;
        }
        if (E.size() > 0)
        {
            col = mDico->Apt2col(4);
            for (auto &kv : E)
            {
                mAptAllEss->elementAt(row, column)->addWidget(std::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("E");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setToolTip(mDico->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second != 4)
                {
                    mAptAllEss->elementAt(row, column)->setToolTip(mAptAllEss->elementAt(row, column)->toolTip() + " - " + WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor("gray"));
                }
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);

                row++;
            }
            column++;
            row = 0;
        }
    }
    else
    {
        std::cout << "simplePoint affiche tableau apt : pas d'essence pour le tableau" << std::endl;
    }
}

void pointPdfResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    if (globTest)
    {
        std::cout << "\n pointPdfResource handle request \n " << std::endl;
    }

    HPDF_Doc pdf = HPDF_New(error_handler, 0);

    HPDF_UseUTFEncodings(pdf);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL); // sinon pdf fait 5 Mo pour rien du tout

    std::string titre("Analyse Ponctuelle");
    MyRenderer renderer(pdf, page, titre, mSP->mDico);

    Wt::WString tpl = Wt::WText::tr("report.analyse.point");
    std::string tp = tpl.toUTF8();
    std::ostringstream o;
    boost::replace_all(tp, "TITRE_REPPORT", titre);

    // RENDU TABLE d'APTITUDE
    mSP->mAptAllEss->htmlText(o);
    boost::replace_all(tp, "${AptTable}", o.str());

    // RENDU TABLES AVEC DESCRIPTION DE LA STATION
    // vider le oss
    o.str("");
    o.clear();
    mSP->mInfoT->htmlText(o);
    boost::replace_all(tp, "${InfoT}", o.str());

    // RENDU LISTE D'ACCRONYME des ESSENCES

    Wt::WTable *legendAcroEs = new Wt::WTable();
    legendAcroEs->setId("legendAcroEs"); // dans le template html j'ai un style défini pour l'objet ayant cet id
    int row(0);
    int col(0);
    for (auto kv : *mSP->mDico->codeEs2Nom())
    {
        legendAcroEs->elementAt(row, col)->addWidget(std::make_unique<WText>(kv.first));
        legendAcroEs->elementAt(row, col + 1)->addWidget(std::make_unique<WText>(kv.second));
        row++;
        if (row > 20)
        {
            row = 0;
            col = col + 2;
        }
    }
    o.str("");
    o.clear();
    legendAcroEs->htmlText(o);
    boost::replace_all(tp, "${legendAcro}", o.str());
    o.str("");
    o.clear();

    // RENDU CARTE ACTIVE AVEC POSITION de la STATION

    OGRPoint pt = mSP->getPoint();

    // je peux pas utiliser le membre mapextent de GL car celui-ci ne se met à jours que lorsqu'on télécharge une carte sur l'emprise courante...
    // depuis que l'IGN est passé à la version 1.3 du serveur WMS, je ne parviens plus à télécharger les cartes wms en jpg.
    // donc il faut changer cette ligne. utiliser l'IGN du serveur grfmn?
    staticMap sm = staticMap(mSP->mDico->getLayerBase("IGNgrfmn"), &pt);
    // ajout du logo IGN. ajout des crédits ; toujours les mêmes, en dur.
    sm.addImg(mSP->mDico->File("logoIGN"));
    boost::replace_all(tp, "PATH_CARTE", sm.getFileName());
    boost::replace_all(tp, "TITRE_CARTE", mSP->mDico->getLayerBase("IGN")->getLegendLabel(0));
    boost::replace_all(tp, "POSITION_PTX", roundDouble(mSP->getX(), 1));
    boost::replace_all(tp, "POSITION_PTY", roundDouble(mSP->getY(), 1));

    if (mSP->ecogramme())
    {
        // RENDU ECOGRAMME
        // export de l'image de l'écogramme - bug constaté ; https://redmine.webtoolkit.eu/issues/7769
        // pour ma part j'ai compilé Graphicmagick puis cela fonctionnais
        int aEcoWidth(750);
        int aHeigth(aEcoWidth * 15.0 / 7.0);

        Wt::WRasterImage pngImage("png", WLength(aEcoWidth), WLength(aHeigth));
        boost::filesystem::path tmpPath = boost::filesystem::path(mSP->mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.png");
        std::string aEcoPng = tmpPath.string();
        std::ofstream f(aEcoPng, std::ios::out | std::ios::binary);
        WPainter painter(&pngImage);

        std::string ecoBgPath = mSP->mDico->File("docroot") + "img/ecogrammeAxes.png";
        if (exists(ecoBgPath))
        {
            Wt::WPainter::Image ecoBg(ecoBgPath, ecoBgPath);
            Wt::WRectF destinationRect = Wt::WRectF(0.0, 0.0, aEcoWidth, aHeigth);
            painter.drawImage(destinationRect, ecoBg);
        }
        // pour determiner ces positions, j'ai fixé aEcoWidth à 750, j'ai fait tourné le code, puis j'ai ouvert l'image et mesuré les positions
        mSP->mEcoEss->draw(&painter, 292, 332, 700, 1240, 1330);
        pngImage.done();
        pngImage.write(f);
        f.close();
        // ajout dans le pdf
        std::string baliseEco = Wt::WText::tr("report.eco").toUTF8();
        boost::replace_all(baliseEco, "PATH_ECO", aEcoPng);
        boost::replace_all(baliseEco, "TITRE_ECO", "Ecogramme - " + mSP->mActiveEss->Nom());
        mSP->mDetAptFEE->htmlText(o);
        boost::replace_all(baliseEco, "detAptFEE", o.str());
        o.str("");
        o.clear();

        boost::replace_all(tp, "REPORT_ECO", baliseEco);
    }
    else
    {
        boost::replace_all(tp, "REPORT_ECO", "");
    }
    renderer.render(tp);
    response.setMimeType("application/pdf");
    HPDF_SaveToStream(pdf);
    unsigned int size = HPDF_GetStreamSize(pdf);
    HPDF_BYTE *buf = new HPDF_BYTE[size];
    HPDF_ReadFromStream(pdf, buf, &size);

    HPDF_FreeDocAll(pdf);
    // Bug memoryleak HPDF_Free (pdf);
    response.out().write((char *)buf, size);
    delete[] buf;
}

void simplepoint::extractInfo(double x, double y)
{
    vider();
    if (globTest){std::cout << "simplepoint::extractInfo " << x << " , " << y << std::endl;}
    _x=x;
    _y=y;
    initStation();

    // tableau des informations globales
    mInfoT->elementAt(0, 0)->setColumnSpan(2);
    mInfoT->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    mInfoT->elementAt(0, 0)->setPadding(10);
    mInfoT->elementAt(0, 0)->addWidget(std::make_unique<WText>(tr("titre-InfoTableAna-point")));

    ptPedo ptPed = ptPedo(mDico->mPedo, x, y);
    add1InfoRaster(ptPed.displayInfo(PEDO::DRAINAGE));
    add1InfoRaster(ptPed.displayInfo(PEDO::PROFONDEUR));
    add1InfoRaster(ptPed.displayInfo(PEDO::TEXTURE));
    add1InfoRaster(ptPed.displayInfo(PEDO::CHARGE));

    for (auto &pair  : mDico->VlayerBase()) {
        std::shared_ptr<layerBase> l = pair.second;
        bool activeL=(l->Code()==m_app->getActiveLay());

        if ((l->getCatLayer() == TypeLayer::Station) | (activeL && (l->getCatLayer() != TypeLayer::Externe)))
        {
            if (!l->Expert())
            {
                std::vector<std::string> layerLabelAndValue = displayInfo(l);
                if (l->l4StatP())
                {
                    add1InfoRaster(layerLabelAndValue);
                }
                if ((activeL && l->Code() != "CS_A"))
                {
                    boost::replace_all(layerLabelAndValue.at(1), "'", "\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                    boost::replace_all(layerLabelAndValue.at(0), "'", "\\'");
                    doJavaScript("if (content) {content.innerHTML = '<p>" + layerLabelAndValue.at(0) + ":</p><code>" + layerLabelAndValue.at(1) + "</code>';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);}");
                }
                else if (activeL && l->Code() == "CS_A")
                {
                    int aVal = l->getValue(x, y);
                    boost::replace_all(layerLabelAndValue.at(1), "'", "\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                    boost::replace_all(layerLabelAndValue.at(0), "'", "\\'");

                    if (aVal != 0)
                    {
                        std::string js = "if (content) {content.innerHTML = '<p>" + layerLabelAndValue.at(0) + ":</p><code>" + layerLabelAndValue.at(1) + "</code> <br></br> <a href=\"https://forestimator.gembloux.ulg.ac.be/telechargement/US-A" + std::to_string(aVal) + ".pdf\" target=\"_blank\" rel=\"noopener\">Consulter la description de la station forestière</a>';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);}";
                        doJavaScript(js);
                    }
                }
            }
        }

        // si la couche active est la CNSW, on affiche les info pédo dans la fenetre "overlay".
        if ((activeL && l->Code() == "CNSWrast"))
        {
            doJavaScript("if (content) {content.innerHTML = '" + ptPed.displayAllInfoInOverlay() + "';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);}");
        }
        if ((activeL && l->Code() == "Cadastre"))
        {

            ptCadastre *ptCad = new ptCadastre(mDico->mCadastre, x, y);
            ptCad->sendPolygone().connect(std::bind(&parcellaire::polygoneCadastre, m_app->mPA, std::placeholders::_1, std::placeholders::_2));
            doJavaScript("if (content) {content.innerHTML = '" + ptCad->displayAllInfoInOverlay() + "';" + "var coordinate = [" + std::to_string(x) + "," + std::to_string(y) + "];" + "overlay.setPosition(coordinate);}");
            WDialog *dialogPtr = addChild(std::make_unique<Wt::WDialog>("Charger la parcelle cadastrale"));
            dialogPtr->contents()->addNew<Wt::WText>(tr("msg.charger.poly.capa"));
            WPushButton *ok = dialogPtr->footer()->addNew<Wt::WPushButton>("Oui");
            ok->setDefault(false);
            ok->clicked().connect([=]
            {
                ptCad->usePolyg4Stat();
                delete(ptCad);
                dialogPtr->reject(); });
            WPushButton *annuler = dialogPtr->footer()->addNew<Wt::WPushButton>("Non");
            annuler->setDefault(true);
            annuler->clicked().connect([=]
            {dialogPtr->reject();
                delete(ptCad); });
            dialogPtr->show();
        }


        if (activeL && l->getCatLayer() == TypeLayer::FEE)
        {
            hasFEEApt=true;
            detailCalculAptFEE();
        }
    }

    afficheAptAllEss();
}


std::map<std::string, int> simplepoint::apts(TypeClassifST aType)
{
    std::map<std::string, int> aRes;
    switch(aType)
    {
    case FEE:
    {
        if (readyFEE())
        {
            for (auto &pair  : mDico->VlayerBase()) {
                std::shared_ptr<layerBase> l = pair.second;
                if (l->getCatLayer() == TypeLayer::FEE)
                {
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    // std::shared_ptr<cEss> Ess= l->Ess();
                    if (mDico->hasEss(l->EssCode()))
                    {
                        std::shared_ptr<cEss> Ess = mDico->getEss(l->EssCode());
                        int apt = Ess->getFinalApt(mNT, mNH, mZBIO, mTOPO);
                        aRes.emplace(std::make_pair(Ess->Code(), apt));
                    }
                }
            }
        }
        break;
    }
    case CS:
    {
        if (globTest)
        {
            std::cout << " GL get apt pour mode CS " << std::endl;
        }
        if (readyCS())
        {
            if (globTest)
            {
                std::cout << "station a bien une station du catalogue " << std::endl;
            }
            for (auto kv : mDico->getAllEss())
            {
                std::shared_ptr<cEss> Ess = kv.second;
                int apt = Ess->getApt(mZBIO, mSt);
                if (apt != 0)
                    aRes.emplace(std::make_pair(Ess->Code(), apt));
            }
        }
        break;
    }
    }
    return aRes;
}


std::vector<std::string> simplepoint::displayInfo(std::shared_ptr<layerBase> al){
    std::vector<std::string> aRes;
    aRes.push_back(al->getLegendLabel(false));
    std::string val("");
    bool activeL=al->Code()==m_app->getActiveLay();
    // on va affichier uniquement les informations de la couches d'apt qui est sélectionnée, et de toutes les couches thématiques (FEE et CS)
    if ((al->getCatLayer()==TypeLayer::KK )| (al->getCatLayer()==TypeLayer::Station) |( activeL)){
        // 1 extraction de la valeur
        int aVal=al->getValue(_x,_y);
        if (al->Code()=="NT"){ mNT=aVal;}
        if (al->Code()=="NH"){ mNH=aVal;}
        if (al->Code()=="ZBIO"){ mZBIO=aVal;}
        if (al->Code()=="Topo"){ mTOPO=aVal;}

        // station du CS
        if (al->Code().substr(0,2)=="CS" && aVal!=0){
            mSt=aVal;}
        val=al->getValLabel(aVal);
        if (al->getTypeVar()==TypeVar::Continu){ val=roundDouble(al->Gain()*(double) aVal,2);}
    }

    if ((al->getCatLayer()==TypeLayer::FEE || al->getCatLayer()==TypeLayer::CS) && (activeL)){
        mActiveEss=mDico->getEss(al->EssCode());
        HaveEss=1;
    }

    aRes.push_back(val);
    if (activeL) {aRes.push_back("bold");}
    return aRes;
}
