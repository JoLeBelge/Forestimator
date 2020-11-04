#include "grouplayers.h"

//const TypeClassifST cl[] = { FEE, CS };
//std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};

int maxSizePix4Export(30000);

groupLayers::groupLayers(cDicoApt * aDico, WOpenLayers *aMap, AuthApplication *app, stackInfoPtr * aStackInfoPtr):
    mDico(aDico)
  ,mTypeClassifST(FEE)
  ,mMap(aMap)
  ,mParent(aStackInfoPtr->mGroupLayerW)
  //,mPBar(NULL)
  ,m_app(app)
  ,mLegend(NULL)
  //,mSelect4Stat(NULL)
  ,mSelect4Download(NULL)
  ,mStackInfoPtr(aStackInfoPtr)
  ,mapExtent_(this,"1.0")
  ,sigMapCenter(this,"2.0")
  ,slot(this)
  ,slotMapCenter(this)
{
    //std::cout << "constructeur GL " << std::endl;

    mParent->setOverflow(Wt::Overflow::Visible);
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);

    slot.setJavaScript("function toto(e) {"
                       "var extent = map.getView().calculateExtent(map.getSize());"
                       "var bottomLeft = ol.extent.getBottomLeft(extent);"
                       "var topRight =ol.extent.getTopRight(extent);"
                       "if (bottomLeft != null) {"
                       + mapExtent_.createCall({"topRight[0]","topRight[1]","bottomLeft[0]","bottomLeft[1]"})
                       + "}}"
                       );

    this->getMapExtendSignal().connect(std::bind(&groupLayers::updateMapExtentAndCropIm,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    slotMapCenter.setJavaScript("function () {"
                              "var centre = map.getView().getCenter();"
                              "var z = map.getView().getZoom();"
                              + sigMapCenter.createCall({"centre[0]","centre[1]","z"}) + "}");
    this->sigMapCenter.connect(std::bind(&groupLayers::saveExtent,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));


    auto legendCombo = mParent->addWidget(cpp14::make_unique<WCheckBox>(tr("legendCheckbox")));
    legendCombo->changed().connect([this]{
        if(mLegendDiv->isVisible())
            mLegendDiv->hide();
        else
            mLegendDiv->show();
    });
    mLegendDiv = mParent->addWidget(cpp14::make_unique<WContainerWidget>());
    mLegendDiv->hide();
    mTitle = mLegendDiv->addWidget(cpp14::make_unique<WText>(WString::tr("legendMsg")));
    mLegendIndiv = mLegendDiv->addWidget(cpp14::make_unique<WTable>());
    mLegendIndiv->setHeaderCount(1);
    mLegendIndiv->setWidth(Wt::WLength("90%"));
    mLegendIndiv->toggleStyleClass("table-striped",true);

    /* Liste cartes 1	*/
    std::unique_ptr<Wt::WTree> tree = Wt::cpp14::make_unique<Wt::WTree>();
    tree->setSelectionMode(Wt::SelectionMode::Extended);
    tree->addStyleClass("tree_left");
    //auto folderIcon = Wt::cpp14::make_unique<Wt::WIconPair>("icons/yellow-folder-closed.png", "icons/yellow-folder-open.png", false);

    auto main_node = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAll")); // std::move(folderIcon) // pour mettre des icones ouvert/fermé !
    tree->setTreeRoot(std::move(main_node));
    tree->treeRoot()->label()->setTextFormat(Wt::TextFormat::Plain);
    tree->treeRoot()->setLoadPolicy(Wt::ContentLoading::NextLevel);
    auto node1 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheThem"));
    auto node1_ = tree->treeRoot()->addChildNode(std::move(node1));
    node1_->addStyleClass("tree_node");
    tree->treeRoot()->expand();

    auto node0 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCouchePeup"));
    auto node0_ = tree->treeRoot()->addChildNode(std::move(node0));
    node0_->addStyleClass("tree_node");

    // creation des layers pour les KK du CS

        for (auto & pair : *mDico->codeKK2Nom()){
            Wt::WTreeNode * n = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
            WText *label=n->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::KK);
            std::string aCode=pair.first;
            label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::KK);});
            aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
            mVLs.push_back(aL);
        }

    // ajout des cartes "FEE" ; NT NH Topo AE SS

    for (auto & pair : *mDico->RasterType()){
         if (mDico->rasterCat(pair.first)!="Peuplement"){
        Wt::WTreeNode * n = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
        WText *label=n->label();
        Layer  * aL= new Layer(this,pair.first,label,TypeLayer::Thematique);
        std::string aCode=pair.first;
        // un peu bidouille mais le typelayer de MNH est peuplement et il est redéfini dans le constructeur de layer
        TypeLayer type= aL->Type();
        label->clicked().connect([this,aCode,type]{clickOnName(aCode,type);});
        aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
        mVLs.push_back(aL);
         }
    }

    node1_->expand();

    for (auto & pair : *mDico->RasterType()){
        // couche catégorie peuplements
        if (mDico->rasterCat(pair.first)=="Peuplement"){
            Wt::WTreeNode * n = node0_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
            WText *label=n->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::Thematique);
            std::string aCode=pair.first;
            // un peu bidouille mais le typelayer de MNH et le masque forestier sont peuplement et il est redéfini dans le constructeur de layer
            TypeLayer type= aL->Type();
            label->clicked().connect([this,aCode,type]{clickOnName(aCode,type);});
            aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
            mVLs.push_back(aL);
        }
    }

    node0_->expand();

    auto node2 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptFEE"));
    auto node2_ = tree->treeRoot()->addChildNode(std::move(node2));
    node2_->addStyleClass("tree_node");
    node2_->expand();

    auto node3 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptCS"));
    auto node3_ = node3.get();
    node3_->addStyleClass("tree_node");
    node3_ = tree->treeRoot()->addChildNode(std::move(node3));
    // ici je  vais devoir rendre ce noeuds invisible si mode normal
    this->changeExpertMode().connect(node3_,&Wt::WTreeNode::setNodeVisible);

    // creation des layers pour les essences qui ont des aptitudes
    for (auto & pair : *mDico->code2Nom()){
        cEss ess(pair.first,mDico);
        //std::cout << "fee" << std::endl;
        if (ess.hasFEEApt()){

            Wt::WTreeNode * n = node2_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
            WText *label=n->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::FEE);
            std::string aCode=pair.first;
            label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::FEE);});
            aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
            mVLs.push_back(aL);
        }

        if (ess.hasCSApt()){
            Wt::WTreeNode * n = node3_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
            WText *label=n->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::CS);
            mVLs.push_back(aL);
            std::string aCode=pair.first;
            label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::CS);});
            aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
        }
    }

    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep1")));
    mParent->addWidget(std::move(tree));
    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep2")));

    mExtentDivGlob = mParent->addWidget(cpp14::make_unique<WContainerWidget>());
    WPushButton * button_e = mExtentDivGlob->addWidget(cpp14::make_unique<WPushButton>(tr("afficher_extent")));
    button_e->clicked().connect([=] {
        if(mExtentDiv->isVisible())
            mExtentDiv->hide();
        else
            mExtentDiv->show();
    });
    button_e->addStyleClass("btn btn-info");
    printf("mextentdiv\n");
    mExtentDiv = mExtentDivGlob->addWidget(cpp14::make_unique<WContainerWidget>());
    mExtentDiv->setMargin(15,Wt::Side::Left);
    mExtentDiv->setMargin(15,Wt::Side::Right);
    mExtentDiv->addStyleClass("div_extent");
    mExtentDiv->hide();

    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep3")));
    WPushButton * bExportTiff = mParent->addWidget(cpp14::make_unique<WPushButton>("Télécharger"));

    // pour l'instant tout passe par le slot JS qui renvoi un extent valide avant d'effectuer le crop et l'envoi de la carte à l'utilisateur
    // c'est pour éviter que l'extent ne soit pas "à jour" avant le crop
    bExportTiff->clicked().connect(this->slot);
    //bExportTiff->clicked().connect(this,&groupLayers::updateMapExtentAndCropIm);

    //mSelect4Stat= new selectLayers4Stat(this);
    mSelect4Download= new selectLayers4Download(this);

    /*   AUTRES ONLGETS de la stack   */
    // création de la légende (vide pour le moment)
    mLegend = new simplepoint(this, mStackInfoPtr->mSimplepointW);
    mStation = new ST(mDico);

    // updateGL pour cacher les couches expert
    updateGL();
    //std::cout << "done " << std::endl;
}

/*
groupLayers::~groupLayers(){
    std::cout << "destructeur de group layer " << std::endl;
    delete mLegend;
    delete mStation;
    delete mSelect4Stat;
    delete mSelect4Download;
    printf("null mMap GL\n");
    mMap=NULL;
    m_app=NULL;
    mDico=NULL;
    mLegend=NULL;
    printf("nulled mMap GL\n");
    mVLs.clear();
}
*/

void groupLayers::update(std::string aCode, TypeLayer type){
    //std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    // désactiver toutes les couches actives et changer le rendu du label
    for (Layer * l : mVLs){
        l->setActive(aCode==l->getCode() && type==l->Type());
    }
    //std::cout << "update done " << std::endl;
}

void groupLayers::clickOnName(std::string aCode, TypeLayer type){

    //std::cout << " j'ai cliqué sur un label " << aCode <<  "\n\n"<< std::endl;
    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    update(aCode, type);

    // changer le mode CS vs FEE de grouplayer, utilse pour le tableau d'aptitude
    if (type == TypeLayer::CS){ mTypeClassifST=TypeClassifST::CS;}
    if (type == TypeLayer::FEE){ mTypeClassifST=TypeClassifST::FEE;}
    // ajouter la couche à la carte
    for (Layer * l : mVLs){
        if (l->IsActive()){ //&& type==l->Type()){
            l->displayLayer();
            updateLegende(l);
            break;
        }
    }
}

/**
 * @brief groupLayers::extractInfo Affiche les stat d'un point clicqué sur la map.
 * @param x
 * @param y
 */
void groupLayers::extractInfo(double x, double y){

    if(!isnan(x) && !isnan(y) && !(x==0 && y==0)){
        // allow only if la page est simplepoint ???
        //if (m_app->internalPath() != "/point") return;

        std::cout << "groupLayers ; extractInfo " << x << " , " << y << std::endl;
        mStation->vider();
        mStation->setX(x);
        mStation->setY(y);
        mLegend->vider();


        /*int cur_index = mStackInfoPtr->stack_info->currentIndex(); // pour savoir où on en est
        mStackInfoPtr->stack_info->setCurrentIndex(0);
        // bouton retour
        WPushButton * retour = mLegend->addWidget(Wt::cpp14::make_unique<WPushButton>("Retour"));
        retour->addStyleClass("btn btn-info");

        retour->clicked().connect([=]{
            mStackInfoPtr->stack_info->setCurrentIndex(cur_index);
        });*/

        // tableau des informations globales - durant ce round, l'objet ST est modifié
        mLegend->titreInfoRaster();
        // la création d'un objet ptPedo évite de devoir lire à plusieur reprise la cnsw
        ptPedo ptPed=ptPedo(mDico->mPedo,x,y);

        mLegend->add1InfoRaster(ptPed.displayInfo(PEDO::DRAINAGE));
        mLegend->add1InfoRaster(ptPed.displayInfo(PEDO::PROFONDEUR));
        mLegend->add1InfoRaster(ptPed.displayInfo(PEDO::TEXTURE));

        for (Layer * l : mVLs){
            if (((l->Type()==TypeLayer::KK )| (l->Type()==TypeLayer::Thematique )) | (( l->IsActive()) & (l->Type()!=TypeLayer::Externe))){

                std::vector<std::string> layerLabelAndValue=l->displayInfo(x,y);
                mLegend->add1InfoRaster(layerLabelAndValue);

                if (( l->IsActive())){
                    // affiche une popup pour indiquer la valeur pour cette couche
                    // attention, il faut escaper les caractères à problèmes du genre apostrophe
                    boost::replace_all(layerLabelAndValue.at(1),"'","\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                    boost::replace_all(layerLabelAndValue.at(0),"'","\\'");
                    mParent->doJavaScript("content.innerHTML = '<p>"+layerLabelAndValue.at(0)+":</p><code>"+ layerLabelAndValue.at(1)+ "</code>';"
                                          +"var coordinate = ["+std::to_string(x) + ","+ std::to_string(y) +"];"
                                          +"overlay.setPosition(coordinate);"
                                          +"overlay.setPosition(coordinate);"
                                          );
                }

            }
        }

        // tableau du détail du calcul de l'aptitude d'une essence pour FEE
        for (Layer * l : mVLs){
            // on a bien une essence active et on est en mode FEE
            if ( l->IsActive() && l->Type()==TypeLayer::FEE && mTypeClassifST==FEE){
                // on note la chose dans l'objet ecogramme, car la classe simplepoint va devoir savoir si il y a un ecogramme à export en jpg ou non
                mStation->setHasFEEApt(1);
                mLegend->detailCalculAptFEE(mStation);

            }
        }
        // tableau des aptitudes pour toutes les essences
        mLegend->afficheAptAllEss();

        mMap->updateView();
    }else {
        std::cout << "x et y ne sont pas des nombres , pas bien " << std::endl;
    }
}

void groupLayers::computeStatGlob(OGRGeometry *poGeomGlobale){
    std::cout << " groupLayers::computeStatGlob " << std::endl;
    // clear d'un vecteur de pointeur, c'est mal. d'ailleurs ça bug si on calcule plusieur fois des stat dans la mm session, à regler donc
    for (auto p : mVLStat)
    {
        delete p;
    }
    mVLStat.clear();
    std::cout << " mVLStat.cleared " << std::endl;

    // pour les statistiques globales, on prend toutes les couches selectionnées par select4Download

    for (auto & l: getSelectedLayer4Download() ){


        // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        std::map<std::string,int> stat = l->computeStatOnPolyg(poGeomGlobale);

        mVLStat.push_back(new layerStatChart(l,stat,poGeomGlobale));

        //mPBar->setValue(mPBar->value() + 1);
        //m_app->processEvents();
    }
    //mPBar->setValue(mPBar->maximum());

    //return aRes;
}

void groupLayers::computeStatOnPolyg(OGRLayer * lay,bool mergeOT){

    for (auto & l : getSelectedLayer4Stat() ){

        // défini le nouveau champ à ajouter à la table d'attribut - vérifie qu'il n'existe pas préhalablement
        if (lay->FindFieldIndex(l->getFieldName().c_str(),0)==-1){

            OGRFieldDefn * oFLD(NULL);
            if (l->getFieldType()=="int"){
                oFLD= new OGRFieldDefn(l->getFieldName().c_str(),  OFTInteger);
            }
            if (l->getFieldType()=="str"){
                oFLD= new OGRFieldDefn(l->getFieldName().c_str(),  OFTString);
            }

            oFLD->SetJustify(OGRJustification::OJLeft);

            lay->CreateField(oFLD);
        }

        OGRFeature *poFeature;
        lay->ResetReading();
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
            OGRGeometry * poGeom = poFeature->GetGeometryRef();
            poGeom->closeRings();
            poGeom->flattenTo2D();
            //std::map<std::string,int> stat = l->computeStatOnPolyg(poGeom,aMode);
            layerStat ls(l,l->computeStatOnPolyg(poGeom));
            // on met un résumé des stat dans le champ nouvellement créé
            if (l->getFieldType()=="int"){
                poFeature->SetField(l->getFieldName().c_str(), ls.getFieldVal(mergeOT));
            }
            if (l->getFieldType()=="str"){

                //std::cout << "set field "<< l->getFieldName() << " to " <<    ls.getFieldValStr() << std::endl;
                poFeature->SetField(l->getFieldName().c_str(), ls.getFieldValStr().c_str());
            }

            //poFeature->SetField(); This method has only an effect on the in-memory feature object. If this object comes from a layer and the modifications must be serialized back to the datasource, OGR_L_SetFeature()
            lay->SetFeature(poFeature);
        }
    }
}

std::map<std::string,int> groupLayers::apts(){
    std::map<std::string,int> aRes;
    switch (mTypeClassifST){
    case FEE:
        if (mStation->readyFEE()){
            for (Layer * l : mVLs){
                if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    cEss  * Ess= l->Ess();
                    int apt = Ess->getFinalApt(mStation->mNT,mStation->mNH, mStation->mZBIO, mStation->mTOPO);
                    aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    case CS:
        if (mStation->readyCS()){
            for (Layer * l : mVLs){
                if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    cEss  * Ess= l->Ess();
                    int apt = Ess->getApt(mStation->mZBIO, mStation->mSt);
                    if (apt!=0) aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    }
    return aRes;
}

void groupLayers::updateGL(bool expertMode){

    // boucle sur les layers et envoi du signal pour cacher ou rendre visible les checkbox
    for (Layer * l : mVLs){
       l->ExpertMode(expertMode);
    }
    // pour cacher les noeuds racines , celui "aptitude CS"
    expertMode_.emit(expertMode);

    if(m_app->isLoggedIn()){
    loadExtents(m_app->getUser().id());
    mExtentDivGlob->show();
    } else {
      mExtentDivGlob->hide();
    }

}

void groupLayers::updateLegende(const Layer * l){
    // vider la légende et afficher la légende personnelle de la couche active
    mLegendIndiv->clear();
    if (l->Type()!=TypeLayer::Externe){

        mTitle->setText(WString::tr("legendTitre"));
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
    }else {
        mTitle->setText(WString::tr("legendMsg"));
    }
}

Layer * groupLayers::getActiveLay(){
    Layer * aRes=NULL;
    for (Layer * l : mVLs){
        if (( l->IsActive())){aRes=l;break;}
    }
    // au lancement de l'appli, aucune couche n'est active
    if (aRes==NULL){
        for (Layer * l : mVLs){
            if (( l->getCode()=="IGN")){aRes=l;
                l->setActive();
                break;}
        }
    }
    return aRes;
}

void groupLayers::exportLayMapView(){

    std::cout << "exportLayMapView " << std::endl;

    Layer * l=getActiveLay();// attention, si on vient d'ouvrir le soft, aucune layer n'est actives!! gerer les ptr null
    if (l && l->Type()!=TypeLayer::Externe){
        m_app->loadingIndicator()->setMessage(tr("LoadingI3"));
        m_app->loadingIndicator()->show();
        // crop layer and download

        // crée l'archive
        std::string archiveFileName = mDico->File("TMPDIR")+"/"+l->getCode()+".zip";
        std::string aCroppedRFile = mDico->File("TMPDIR")+"/"+l->getCode()+"_crop.tif";
        std::string mClientName=l->getCode()+"_crop";
        rasterFiles r= l->getRasterfile();
        if ( cropIm(l->getPathTif(), aCroppedRFile, mMapExtent)){
            std::cout << "create archive pour raster croppé " << std::endl;
            ZipArchive* zf = new ZipArchive(archiveFileName);
            zf->open(ZipArchive::WRITE);
            // pour bien faire; choisir un nom qui soit unique, pour éviter conflict si plusieurs utilisateurs croppent la mm carte en mm temps
            zf->addFile(mClientName+".tif",aCroppedRFile);
            if (r.hasSymbology()){zf->addFile(mClientName+".qml",r.symbology());}
            m_app->processEvents();
            zf->close();
            delete zf;
            WFileResource *fileResource = new Wt::WFileResource("plain/text",archiveFileName);
            fileResource->suggestFileName(mClientName+".zip");
            m_app->redirect(fileResource->url());
        } else {
            Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                              "Erreur",
                                                              "<p> Cette couche ne peut être découpée sur cette emprise, essayer avec une zone plus petite.</p>",
                                                              Wt::Icon::Critical,
                                                              Wt::StandardButton::Ok));

            messageBox->setModal(false);
            messageBox->buttonClicked().connect([=] {
                this->removeChild(messageBox);
            });
            messageBox->show();
        }

        m_app->loadingIndicator()->hide();
        m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));

    }else {
        Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                          "Erreur",
                                                          "<p> Cette couche ne peut être téléchargé, veillez essayer avec une autres couche.</p>",
                                                          Wt::Icon::Critical,
                                                          Wt::StandardButton::Ok));

        messageBox->setModal(false);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    }
}


bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext){
    bool aRes(0);
    std::cout << " cropIm" << std::endl;
    GDALAllRegister();
    if (exists(inputRaster)){

        const char *inputPath=inputRaster.c_str();
        const char *cropPath=aOut.c_str();
        GDALDataset *pInputRaster, *pCroppedRaster;
        GDALDriver *pDriver;
        const char *pszFormat = "GTiff";
        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);
        double transform[6], tr1[6];
        pInputRaster->GetGeoTransform(transform);
        pInputRaster->GetGeoTransform(tr1);

        OGREnvelope extGlob=OGREnvelope();
        extGlob.MaxY=transform[3];
        extGlob.MinX=transform[0];
        extGlob.MinY=transform[3]+transform[5]*pInputRaster->GetRasterBand(1)->GetYSize();
        extGlob.MaxX=transform[0]+transform[1]*pInputRaster->GetRasterBand(1)->GetXSize();

        //std::cout << ext.MinX << " , " << ext.MaxX << " , " << ext.MinY << " , " << ext.MaxY << " avant intersect " << std::endl;
        // garder l'intersect des 2 extend
        ext.Intersect(extGlob);
        //std::cout << ext.MinX << " , " << ext.MaxX << " , " << ext.MinY << " , " << ext.MaxY << " après intersect " << std::endl;

        if (extGlob.MinX==ext.MinX && extGlob.MaxX==ext.MaxX && extGlob.MinY==ext.MinY && extGlob.MaxY==ext.MaxY){
            //std::cout << "je vais faire une copie de tout le raster plutôt que de le cropper " << std::endl;
            //pInputRaster->
            pDriver->CopyFiles(cropPath,inputPath);
            aRes=1;
        } else {
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

            //adjust top left coordinates
            transform[0] = ext.MinX;
            transform[3] = ext.MaxY;
            //determine dimensions of the new (cropped) raster in cells
            int xSize = round(width/transform[1]);
            int ySize = round(height/transform[1]);
            //std::cout << "xSize " << xSize << ", ySize " << ySize << std::endl;
            //create the new (cropped) dataset
            if (xSize>0 && ySize>0 && xSize < maxSizePix4Export && ySize<maxSizePix4Export){
                pCroppedRaster = pDriver->Create(cropPath, xSize, ySize, 1, GDT_Byte, NULL); //or something similar

                pCroppedRaster->SetProjection( pInputRaster->GetProjectionRef() );
                pCroppedRaster->SetGeoTransform( transform );

                int xOffset=round((transform[0]-tr1[0])/tr1[1]);
                int yOffset=round((transform[3]-tr1[3])/tr1[5]);
                float *scanline;
                scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
                // boucle sur chaque ligne
                for ( int row = 0; row < ySize; row++ )
                {
                    // lecture
                    pInputRaster->GetRasterBand(1)->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                    // écriture
                    pCroppedRaster->GetRasterBand(1)->RasterIO( GF_Write, 0, row, xSize,1, scanline, xSize, 1,GDT_Float32, 0, 0 );
                }
                CPLFree(scanline);

                aRes=1;
                if( pCroppedRaster != NULL ){GDALClose( (GDALDatasetH) pCroppedRaster );}
            } else {
                std::cout << " crop du raster a échoué: taille pas correcte " << std::endl;
            }

        }
        GDALClose(pInputRaster);
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
    return aRes;
}

/*
 * Fonctions pour gérer les extents sauvés dans DB
 *
 */
int groupLayers::openConnection(){
    int rc;
    std::string db_path = m_app->docRoot() + "/extents.db";
    //std::cout << "..." << std::endl;
    rc = sqlite3_open_v2(db_path.c_str(), &db_,SQLITE_OPEN_READWRITE, NULL);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
        std::cout << " db_path " << db_path << std::endl;
    }
    return rc;
}

void groupLayers::closeConnection(){

    int rc = sqlite3_close_v2(db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(db_));
    }
}


bool groupLayers::getExpertModeForUser(std::string id){

    openConnection();
    printf("get Expert Mode For User...");
    bool aRes(0);
    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT ModeExpert FROM user_expert WHERE id_user="+id+";";
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
            aRes=sqlite3_column_int(stmt, 0);
        } else {std::cout << "je ne parviens pas à lire la table user_expert " << std::endl;  }
    }

    closeConnection();
    std::cout << "mode expert est à " << aRes << std::endl;
    return aRes;
}

void groupLayers::loadExtents(std::string id){
    openConnection();
    printf("loadextents...");
    mExtentDiv->clear();

    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT centre_x,centre_y,zoom,name,id FROM user_extent WHERE id_user="+id; // std::to_string(id);
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string cx=std::string( (char *)sqlite3_column_text(stmt, 0));
        std::string cy=std::string( (char *)sqlite3_column_text(stmt, 1));
        std::string z=std::string( (char *)sqlite3_column_text(stmt, 2));
        std::string n=std::string( (char *)sqlite3_column_text(stmt, 3));
        std::string id_extent=std::string( (char *)sqlite3_column_text(stmt, 4));
        /*std::cout << " value 1 : " << cx << std::endl;
        std::cout << " value 2 : " << cy << std::endl;
        std::cout << " value 3 : " << z << std::endl;*/
        WPushButton *bp = mExtentDiv->addNew<Wt::WPushButton>(n);
        bp->setInline(1);
        bp->clicked().connect([=]{
            mParent->doJavaScript("map.getView().setCenter(["+cx+","+cy+" ]);");
            mParent->doJavaScript("map.getView().setZoom("+z+");");
        });
        WAnchor *del = mExtentDiv->addNew<Wt::WAnchor>(WLink(""),"(x)");
        del->clicked().connect([=]{
            WDialog * dialogPtr =  mParent->addChild(Wt::cpp14::make_unique<Wt::WDialog>(tr("extent_del_comfirm")));
            WPushButton *ok = dialogPtr->footer()->addNew<Wt::WPushButton>("Supprimer");
            ok->setDefault(false);
            ok->clicked().connect([=]{
                printf("delete extent\n");
                deleteExtent(id_extent);
                dialogPtr->reject();
            });

            WPushButton * annuler = dialogPtr->footer()->addNew<Wt::WPushButton>("Annuler");
            annuler->setDefault(true);
            annuler->clicked().connect([=]{dialogPtr->reject();});

            dialogPtr->show();
        });
    }
    closeConnection();

    // boutons pour enregistrer l'extent courant
    mExtentDiv->addNew<Wt::WBreak>();
    tb_extent_name = mExtentDiv->addWidget(cpp14::make_unique<WLineEdit>());
    tb_extent_name->setInline(1);
    tb_extent_name->setPlaceholderText("Nom de l'emprise...");
    tb_extent_name->setWidth("200px");
    tb_extent_name->addStyleClass("extent_inline");
    WPushButton * button_s = mExtentDiv->addWidget(cpp14::make_unique<WPushButton>(tr("sauver_extent")));
    button_s->addStyleClass("btn btn-success");
    button_s->setInline(1);
    button_s->addStyleClass("extent_inline extent_margin");
    button_s->clicked().connect(this->slotMapCenter);

    std::cout << "done" << std::endl;
}

void groupLayers::saveExtent(double c_x, double c_y, double zoom){
    openConnection();

    std::string id = m_app->getUser().id();
    std::string n = tb_extent_name->text().toUTF8();
    boost::replace_all(n,"'","");
    std::string cx=std::to_string((int)c_x);
    std::string cy=std::to_string((int)c_y);
    std::string z=std::to_string((int)zoom);
    std::string sql = "INSERT INTO user_extent (id_user,centre_x,centre_y,zoom,name) VALUES ("+id+","+cx+","+cy+","+z+",'"+n+"')";
    std::cout << sql << std::endl;
    sqlite3_exec(db_, sql.c_str(),NULL,NULL,NULL);

    closeConnection();
    loadExtents(id);
}

void groupLayers::deleteExtent(std::string id){
    openConnection();

    std::string sql = "DELETE FROM user_extent WHERE id="+id;
    std::cout << sql << std::endl;
    sqlite3_exec(db_, sql.c_str(),NULL,NULL,NULL);

    closeConnection();
    loadExtents(id);
}

std::vector<rasterFiles> groupLayers::getSelect4Download(){return mSelect4Download->getSelectedRaster();}
std::vector<rasterFiles> groupLayers::getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}

int groupLayers::getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
int groupLayers::getNumSelect4Download(){return mSelect4Download->numSelectedLayer();}
std::vector<Layer *>groupLayers::getSelectedLayer4Stat(){return mSelect4Stat->getSelectedLayer();}
std::vector<Layer *> groupLayers::getSelectedLayer4Download(){return mSelect4Download->getSelectedLayer();}
//std::vector<Layer *> groupLayers::getAllLayer(){return mSelect4Download->getAllLayer();}
