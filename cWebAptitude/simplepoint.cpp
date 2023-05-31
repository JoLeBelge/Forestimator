#include "simplepoint.h"


simplepoint::simplepoint(groupLayers *aGL, WContainerWidget *parent):mGL(aGL)
  ,mParent(parent)
  ,mDico(aGL->Dico())
  ,createPdfBut(NULL)
{
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    mParent->setMargin(1,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);// si pas inline Et bizarrement si pas de setMargin autre que 0, pas de scrollbar pour l'overflow!

    mIntroTxt = mParent->addWidget(cpp14::make_unique<WText>(tr("sp_infoclic")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    createPdfBut = mParent->addWidget(Wt::cpp14::make_unique<WPushButton>(tr("ana.pt.export.pdf")));
    //createPdfBut->clicked().connect(this,&simplepoint::export2pdfTitreDialog);

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
    mInfoT->setId("infoT");
    mInfoT->setHeaderCount(1);
    mInfoT->setWidth(Wt::WLength("90%"));
    mInfoT->toggleStyleClass("table-striped",true);
}

void simplepoint::vider()
{
    mInfoT->clear();
    mDetAptFEE->clear();
    mAptAllEss->clear();
    mContEco->clear();  
    mIntroTxt->hide();
}

void simplepoint::titreInfoRaster(){
    mInfoT->elementAt(0, 0)->setColumnSpan(2);
    mInfoT->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    mInfoT->elementAt(0, 0)->setPadding(10);
    mInfoT->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(tr("titre-InfoTableAna-point")));
    // mInfoT->elementAt(1, 0)->addWidget(cpp14::make_unique<WText>("Couche"));
    //mInfoT->elementAt(1, 1)->addWidget(cpp14::make_unique<WText>("Valeur"));
}

void simplepoint::add1InfoRaster(std::vector<std::string> aV){

    if (aV.size()>1 && aV.at(1)!=""){
        int row=mInfoT->rowCount();
        mInfoT->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(WString::fromUTF8(aV.at(0))));
        auto t2 =mInfoT->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aV.at(1)));
        mInfoT->elementAt(row, 1)->setContentAlignment(AlignmentFlag::Right);

        if (aV.size()>2 && aV.at(2)=="bold"){
            mInfoT->elementAt(row, 0)->setStyleClass("bold");
            t2->setStyleClass("bold");
        }
    }
}

void simplepoint::detailCalculAptFEE(ST * aST){

    std::shared_ptr<cEss> Ess= aST->mActiveEss;
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
    int apt=Ess->getApt(aST->mNT,aST->mNH,aST->mZBIO,true);
    int aptComp=Ess->getApt(aST->mNT,aST->mNH,aST->mZBIO,true,aST->mTOPO);
    if ( aptComp!=apt) {
        // la compensation liée à la situation topographique impacte l'aptitude finale :
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Situation Topographique"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->TOPO()));
        row++;
        Wt::WText * r=mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("rique pour l'essence :"));
       mDetAptFEE->elementAt(row, 0)->setToolTip(tr("tooltip.compensationTopo"));;
        int risque=Ess->getRisque(aST->mZBIO,aST->mTOPO);
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->Risque(risque)));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude bioclimatique avec micro-climat:"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(Ess->corrigAptBioRisqueTopo(Ess->getApt(aST->mZBIO),aST->mTOPO,aST->mZBIO))));
        row++;
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude Finale :"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(aptComp)));
        mDetAptFEE->elementAt(row, 1)->setToolTip(tr("tooltip.compensationTopo"));
    } else {
        mDetAptFEE->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>("Aptitude la plus contraignante :"));
        mDetAptFEE->elementAt(row, 1)->addWidget(cpp14::make_unique<WText>(aST->mDico->code2AptFull(apt)));
        row++;
    }
    // un titre pour l'écogramme
    mContEco->addWidget(cpp14::make_unique<WText>(tr("titreEcogramme")));
    mEcoEss = mContEco->addWidget(Wt::cpp14::make_unique<EcogrammeEss>(Ess.get(),aST));
    mContEco->addWidget(cpp14::make_unique<WText>(tr("legendEcogramme")));
}

void simplepoint::afficheAptAllEss(){
    if (globTest){std::cout << "simplepoint::afficheAptAllEss" << std::endl;}
    std::map<std::string, int> Apts = mGL->apts();
    if (Apts.size()>1){
         if (globTest){std::cout << "Apts.size() : " << Apts.size() << std::endl;}

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
        std::shared_ptr<color> col=std::make_shared<color>(0,0,0);
        if (O.size()>0){
            col = mGL->Dico()->Apt2col(1);
            for (auto & kv : O){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("O");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=1;
        }

        if (T.size()>0){
            col = mGL->Dico()->Apt2col(2);
            for (auto & kv : T){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second!=2) {
                    mAptAllEss->elementAt(row, column)->setToolTip( mAptAllEss->elementAt(row, column)->toolTip()+ " - " +WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor(120,120,120));
                }
                mAptAllEss->elementAt(row, column)->setStyleClass("T");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=1;
        }
        if (TE.size()>0){
            col = mGL->Dico()->Apt2col(3);
            for (auto & kv : TE){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("TE");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
                mAptAllEss->elementAt(row, column)->setToolTip(mGL->Dico()->accroEss2Nom(kv.first));
                // pour le moment, si double aptitude, celle-ci est visible dans le tooltip
                if (kv.second!=3) {
                    mAptAllEss->elementAt(row, column)->setToolTip( mAptAllEss->elementAt(row, column)->toolTip()+ " - " +WString::fromUTF8(mDico->code2AptFull(kv.second)));
                    mAptAllEss->elementAt(row, column)->decorationStyle().setForegroundColor(WColor(120,120,120));
                }
                mAptAllEss->elementAt(row, column)->setContentAlignment(AlignmentFlag::Center);
                row++;
            }
            column++;
            row=1;
        }
        if (E.size()>0){
            col = mGL->Dico()->Apt2col(4);
            for (auto & kv : E){
                mAptAllEss->elementAt(row, column)->addWidget(cpp14::make_unique<WText>(kv.first));
                mAptAllEss->elementAt(row, column)->setStyleClass("E");
                mAptAllEss->elementAt(row, column)->addStyleClass("bold");
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
    } else {
        std::cout << "simplePoint affiche tableau apt : pas d'essence pour le tableau" << std::endl;
    }
}


/*void simplepoint::export2pdfTitreDialog(){

    // check si l'utilisateur à bien double-cliqué sur une station
    if (mGL->mStation->isOK()){

        // TITRE
        Wt::WDialog * dialog = this->addChild(Wt::cpp14::make_unique<Wt::WDialog>("Titre du rapport pdf"));
        Wt::WLabel *label = dialog->contents()->addNew<Wt::WLabel>("Titre: ");
        Wt::WLineEdit *edit =dialog->contents()->addNew<Wt::WLineEdit>();
        edit->setText(tr("report.analyse.point.titre"));
        label->setBuddy(edit);
        dialog->contents()->addStyleClass("form-group");
        Wt::WPush *ok =
                dialog->footer()->addNew<Wt::WPushButton>("OK");
        ok->setDefault(true);
        ok->clicked().connect([=] {
            dialog->accept();
        });

        dialog->finished().connect([=] {
            export2pdf(edit->text().toUTF8());
            removeChild(dialog);
        });
        dialog->show();
    }else{
    // pas de station donc pas d'export
        WMessageBox * messageBox =addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(tr("ana.point.titre"),
                                                                                   tr("ana.point.error.exportpdf.noStation"),
                                                                                   Wt::Icon::Information,
                                                                                   Wt::StandardButton::Ok));

        messageBox->buttonClicked().connect([=] {
            removeChild(messageBox);
        });
        messageBox->show();

    }

}
*/

void pointPdfResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    if (globTest) {std::cout << "\n pointPdfResource handle request \n " << std::endl;}

    HPDF_Doc pdf = HPDF_New(error_handler, 0);

    HPDF_UseUTFEncodings(pdf);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);// sinon pdf fait 5 Mo pour rien du tout

    std::string titre("Analyse Ponctuelle");
    MyRenderer renderer(pdf, page,titre,mSP->mDico);

    Wt::WString tpl = Wt::WText::tr("report.analyse.point");
    std::string tp = tpl.toUTF8();
    std::ostringstream o;
    //renderer.addFontCollection("/usr/share/fonts/truetype",true); plus nécessaire si compilé avec pango

    boost::replace_all(tp,"TITRE_REPPORT",titre);

    // RENDU TABLE d'APTITUDE
    mSP->mAptAllEss->htmlText(o);
    boost::replace_all(tp,"${AptTable}",o.str());

    // RENDU TABLES AVEC DESCRIPTION DE LA STATION
    // vider le oss
    o.str("");
    o.clear();
    mSP->mInfoT->htmlText(o);
    boost::replace_all(tp,"${InfoT}",o.str());

    // RENDU LISTE D'ACCRONYME des ESSENCES

    Wt::WTable * legendAcroEs= new Wt::WTable();
    legendAcroEs->setId("legendAcroEs"); // dans le template html j'ai un style défini pour l'objet ayant cet id
    int row(0);
    int col(0);
    for (auto kv : *mSP->mGL->Dico()->codeEs2Nom() ){
        legendAcroEs->elementAt(row, col)->addWidget(cpp14::make_unique<WText>(kv.first));
        legendAcroEs->elementAt(row, col+1)->addWidget(cpp14::make_unique<WText>(kv.second));
        row++;
        if (row>20){row=0;col=col+2;}
    }
    o.str("");
    o.clear();
    legendAcroEs->htmlText(o);
    boost::replace_all(tp,"${legendAcro}",o.str());
    o.str("");
    o.clear();

    // RENDU CARTE ACTIVE AVEC POSITION de la STATION

    OGRPoint pt = mSP->mGL->mStation->getPoint();

    // je peux pas utiliser le membre mapextent de GL car celui-ci ne se met à jours que lorsqu'on télécharge une carte sur l'emprise courante...
    //staticMap sm(mGL->getActiveLay(),&pt,mGL->getMapExtent());
    // depuis que l'IGN est passé à la version 1.3 du serveur WMS, je ne parviens plus à télécharger les cartes wms en jpg.
    // donc il faut changer cette ligne. utiliser l'IGN du serveur grfmn?
    staticMap sm(mSP->mGL->getLay("IGNgrfmn"),&pt);
    // ajout du logo IGN. ajout des crédits ; toujours les mêmes, en dur.
    sm.addImg(mSP->mDico->File("logoIGN"));
    boost::replace_all(tp,"PATH_CARTE",sm.getFileName());
    boost::replace_all(tp,"TITRE_CARTE",mSP->mGL->getLay("IGN")->getLegendLabel(0));
    boost::replace_all(tp,"POSITION_PTX",roundDouble(mSP->mGL->mStation->getX(),1));
    boost::replace_all(tp,"POSITION_PTY",roundDouble(mSP->mGL->mStation->getY(),1));

    if (mSP->mGL->mStation->ecogramme()){
        // RENDU ECOGRAMME
        // export de l'image de l'écogramme - bug constaté ; https://redmine.webtoolkit.eu/issues/7769
        // pour ma part j'ai compilé Graphicmagick puis cela fonctionnais
        int aEcoWidth(750);
        int aHeigth(aEcoWidth*15.0/7.0);

        Wt::WRasterImage pngImage("png", WLength(aEcoWidth), WLength(aHeigth));
        std::string name01 = std::tmpnam(nullptr);
        std::string name11 = name01.substr(5,name01.size()-5);
        std::string aEcoPng = mSP->mDico->File("TMPDIR")+"/"+name11+".png";
        std::ofstream f(aEcoPng, std::ios::out | std::ios::binary);
        WPainter painter(&pngImage);

        std::string ecoBgPath=mSP->mDico->File("docroot")+"img/ecogrammeAxes.png";
        if (exists(ecoBgPath)){
        Wt::WPainter::Image ecoBg(ecoBgPath,ecoBgPath);
        Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,aEcoWidth, aHeigth);
        painter.drawImage(destinationRect,ecoBg);
        }
        // pour determiner ces positions, j'ai fixé aEcoWidth à 750, j'ai fait tourné le code, puis j'ai ouvert l'image et mesuré les positions
        mSP->mEcoEss->draw(&painter,292,332,700,1240,1330);
        pngImage.done();
        pngImage.write(f);
        f.close();
        // ajout dans le pdf
        std::string baliseEco = Wt::WText::tr("report.eco").toUTF8();
        boost::replace_all(baliseEco,"PATH_ECO",aEcoPng);
        boost::replace_all(baliseEco,"TITRE_ECO","Ecogramme - "+mSP->mGL->mStation->mActiveEss->Nom());
        mSP->mDetAptFEE->htmlText(o);
        boost::replace_all(baliseEco,"detAptFEE",o.str());
        o.str("");
        o.clear();

        boost::replace_all(tp,"REPORT_ECO",baliseEco);

    } else {
        boost::replace_all(tp,"REPORT_ECO","");
    }
    renderer.render(tp);
    response.setMimeType("application/pdf");
    HPDF_SaveToStream (pdf);
    unsigned int size = HPDF_GetStreamSize (pdf);
    HPDF_BYTE * buf = new HPDF_BYTE [size];
    HPDF_ReadFromStream (pdf, buf, & size);
    HPDF_Free (pdf);
    response.out (). write ((char *) buf, size);
    delete [] buf;
}
