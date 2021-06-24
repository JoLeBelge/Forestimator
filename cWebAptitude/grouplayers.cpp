#include "grouplayers.h"

int maxSizePix4Export(30000);
extern bool globTest;

groupLayers::groupLayers(AuthApplication *app, cWebAptitude * cWebApt):
   mDico(app->mDico)
  ,m_app(app)
  ,mcWebAptitude(cWebApt)
  ,mMap(cWebApt->mMap)
  ,mTypeClassifST(FEE)
  ,mParent(cWebApt->mGroupLayerW)
  ,mLegendDiv(cWebApt->mLegendW)
  ,mAnaPoint(NULL)
  ,mSelectLayers(NULL)
  ,sigMapExport(this,"1.0")
  ,sigMapCenter(this,"2.0")
  ,slotMapCenter(this)
{
    mParent->setOverflow(Wt::Overflow::Visible);
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);

    slotMapExport.setJavaScript(
       "function () {"
           "var extent = map.getView().calculateExtent(map.getSize());"
           "var bottomLeft = ol.extent.getBottomLeft(extent);"
           "var topRight =ol.extent.getTopRight(extent);"
           "if (bottomLeft != null) {"
           + sigMapExport.createCall({"topRight[0]","topRight[1]","bottomLeft[0]","bottomLeft[1]"})
       + "}}"
    );

    this->sigMapExport.connect(std::bind(&groupLayers::updateMapExtentAndCropIm,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    slotMapCenter.setJavaScript(
        "function () {"
            "var centre = map.getView().getCenter();"
            "var z = map.getView().getZoom();"
            + sigMapCenter.createCall({"centre[0]","centre[1]","z"})
        + "}"
    );
    this->sigMapCenter.connect(std::bind(&groupLayers::saveExtent,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

    mTitle = mLegendDiv->addWidget(cpp14::make_unique<WText>(WString::tr("legendMsg")));

    /* Liste cartes 1	*/
    std::unique_ptr<Wt::WTree> tree = Wt::cpp14::make_unique<Wt::WTree>();
    tree->setSelectionMode(Wt::SelectionMode::Extended);
    tree->addStyleClass("tree_left");
    //auto folderIcon = Wt::cpp14::make_unique<Wt::WIconPair>("icons/yellow-folder-closed.png", "icons/yellow-folder-open.png", false);

    auto main_node = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAll")); // std::move(folderIcon) // pour mettre des icones ouvert/fermé !
    tree->setTreeRoot(std::move(main_node));
    tree->treeRoot()->label()->setTextFormat(Wt::TextFormat::Plain);
    tree->treeRoot()->setLoadPolicy(Wt::ContentLoading::NextLevel);
    tree->treeRoot()->expand();
    auto node1 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheThem"));
    auto node1_ = tree->treeRoot()->addChildNode(std::move(node1));
    node1_->addStyleClass("tree_node");

    auto node4 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheKKCS"));
    auto node4_ = tree->treeRoot()->addChildNode(std::move(node4));
    node4_->addStyleClass("tree_node");
    this->changeExpertMode().connect(node4_,&Wt::WTreeNode::setNodeVisible);

    auto node0 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCouchePeup"));
    auto node0_ = tree->treeRoot()->addChildNode(std::move(node0));
    node0_->addStyleClass("tree_node");

    auto node2 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptFEE"));
    auto node2_ = tree->treeRoot()->addChildNode(std::move(node2));
    node2_->addStyleClass("tree_node");

    auto node3 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptCS"));
    auto node3_ = tree->treeRoot()->addChildNode(std::move(node3));
    node3_->addStyleClass("tree_node");

    // ici je  vais devoir rendre ce noeuds invisible si mode normal
    this->changeExpertMode().connect(node3_,&Wt::WTreeNode::setNodeVisible);

    /*node0_->expand();
    node1_->expand();
    node2_->expand();
    node3_->expand();
    node4_->expand();*/

    // ajout des cartes
    for (auto & pair :mDico->VlayerBase()){
        std::shared_ptr<layerBase> aLB=pair.second;

        if (mDico->lay4Visu(pair.first)){
            // 1) création du mWtText pour cette couche
            WText * wtext=NULL;
            Wt::WTreeNode * n=NULL;
            switch (aLB->getCatLayer()) {
            case (TypeLayer::Station):{
                n = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
                // carte thématique des catalogues de stations
            case (TypeLayer::KK):{
                n = node4_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
            case (TypeLayer::Externe):{
                n = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
            case TypeLayer::Peuplement:{
                n = node0_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
            case TypeLayer::FEE:{
                n = node2_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
            case TypeLayer::CS:{
                n = node3_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();
                break;}
            default:
                break;
            }
            // 2) création de la couche
            std::shared_ptr<Layer> aL=std::make_shared<Layer>(this,aLB,wtext);
            // 3) ajout des interactions
            TypeLayer type= aL->getCatLayer();
            std::string aCode=aL->Code();
            //wtext->clicked().connect([this,aCode,type]{clickOnName(aCode,type);});
            wtext->doubleClicked().connect([this,aCode,type]{clickOnName(aCode,type);});
            aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
            mVLs.push_back(aL);

        }else {
            mVLs.push_back(std::make_shared<Layer>(this,aLB));
        }
    }

    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep1")));
    mParent->addWidget(std::move(tree));
    /*Wt::WText * t =mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep2")));
    t->setToolTip(tr("coucheStep2.infoBulle"));*/





    //mSelect4Stat= new selectLayers4Stat(this);
    mSelectLayers= new selectLayers(this);
    mStation = new ST(mDico);

    /*   AUTRES DIV   */
    mAnaPoint = new simplepoint(this, mcWebAptitude->mSimplepointW);

    // updateGL pour cacher les couches expert
    //updateGL(); // -> bougé dans classe parent cwebapt car segfault not init refs !
    std::cout << "done " << std::endl;
}

groupLayers::~groupLayers(){
    std::cout << "destructeur de group layer " << std::endl;
    delete mAnaPoint;
    delete mStation;
    //delete mSelect4Stat;
    delete mSelectLayers;
    mMap=NULL;
    m_app=NULL;
    mDico=NULL;
    mAnaPoint=NULL;
    mVLs.clear();
}


void groupLayers::update(std::string aCode, TypeLayer type){
    //std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    // désactiver toutes les couches actives et changer le rendu du label
    for (std::shared_ptr<Layer> l : mVLs){
        l->setActive(aCode==l->Code() && type==l->getCatLayer());
    }
    //std::cout << "update done " << std::endl;
}

void groupLayers::clickOnName(std::string aCode, TypeLayer type){

    //std::cout << " j'ai cliqué sur un label " << aCode <<  "\n\n"<< std::endl;
    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    update(aCode, type);

    std::shared_ptr<Layer> layer;
    for (std::shared_ptr<Layer> l : mVLs){
        if(aCode==l->Code() && type==l->getCatLayer()){
            layer=l;
            break;
        }
    }

    mcWebAptitude->mPanier->addMap(aCode, type, layer);

    // cacher la fenetre popup
    mParent->doJavaScript("overlay.setVisible(0);");

    // changer le mode CS vs FEE de grouplayer, utilise pour le tableau d'aptitude
    // attention de ne pas prendre la couche "CS_FEE" dans le tas..
    if (type == TypeLayer::CS | type == TypeLayer::KK | (type == TypeLayer::Station & aCode.substr(0,2)=="CS")){ mTypeClassifST=TypeClassifST::CS;} else
    { mTypeClassifST=TypeClassifST::FEE;}

    // ajouter la couche à la carte
    for (std::shared_ptr<Layer> l : mVLs){
        if (l->IsActive()){ //&& type==l->Type()){
            l->displayLayer();
            updateLegendeDiv(mcWebAptitude->mPanier->mVLs);
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
        std::cout << "groupLayers ; extractInfo " << x << " , " << y << std::endl;
        mStation->vider();
        mStation->setOK();
        mStation->setX(x);
        mStation->setY(y);
        mAnaPoint->vider();

        // tableau des informations globales - durant ce round, l'objet ST est modifié
        mAnaPoint->titreInfoRaster();

        ptPedo ptPed=ptPedo(mDico->mPedo,x,y);
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::DRAINAGE));
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::PROFONDEUR));
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::TEXTURE));

        for (std::shared_ptr<Layer> l : mVLs){
            if (((l->getCatLayer()==TypeLayer::KK )| (l->getCatLayer()==TypeLayer::Station )) | (( l->IsActive()) & (l->getCatLayer()!=TypeLayer::Externe))){
                if (l->isVisible()){
                    std::vector<std::string> layerLabelAndValue=l->displayInfo(x,y);
                    if (l->l4Stat()){
                        mAnaPoint->add1InfoRaster(layerLabelAndValue);
                    }
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
        }
        // tableau du détail du calcul de l'aptitude d'une essence pour FEE
        for (std::shared_ptr<Layer> l : mVLs){
            // on a bien une essence active et on est en mode FEE
            if ( l->IsActive() && l->getCatLayer()==TypeLayer::FEE && mTypeClassifST==FEE){
                // on note la chose dans l'objet ecogramme, car la classe simplepoint va devoir savoir si il y a un ecogramme à export en jpg ou non
                mStation->setHasFEEApt(1);
                mAnaPoint->detailCalculAptFEE(mStation);

            }
        }

        // tableau des aptitudes pour toutes les essences
        mAnaPoint->afficheAptAllEss();


        mMap->updateView();
    }else {
        std::cout << "x et y ne sont pas des nombres , pas bien " << std::endl;
    }

}

void groupLayers::computeStatGlob(OGRGeometry *poGeomGlobale){
    std::cout << " groupLayers::computeStatGlob " << std::endl;
    /*char * test;
    poGeomGlobale->exportToWkt(&test);
    std::cout << " geometrie WKT :" << test << std::endl;
    */

    clearStat();

    // pour les statistiques globales, on prend toutes les couches selectionnées par select4Download
    for (auto & l: getSelectedLayer4Download() ){

        if (l->Code()=="MNH2019"){
            // calcul de Hdom
            mVLStatCont.push_back(new statHdom(l,poGeomGlobale));

        } else if(l->Code()=="COMPO"){
            // calcul des probabilités de présence pour les 9 sp.
            mCompo = std::make_unique<statCompo>(mDico,poGeomGlobale);
        } else {

            if (l->l4Stat()){
                // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
                std::map<std::string,int> stat = l->computeStat1(poGeomGlobale);
                mVLStat.push_back(new layerStatChart(l,stat,poGeomGlobale));
            }
        }

        //mPBar->setValue(mPBar->value() + 1);
        //m_app->processEvents();
    }
    //mPBar->setValue(mPBar->maximum());
    //return aRes;
}


std::map<std::string,int> groupLayers::apts(){
    std::map<std::string,int> aRes;
    switch (mTypeClassifST){
    case FEE:
        if (mStation->readyFEE()){
            for (std::shared_ptr<Layer> l : mVLs){
                if (l->getCatLayer()==TypeLayer::FEE ){//|| l->Type()==TypeLayer::CS){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    std::shared_ptr<cEss> Ess= l->Ess();
                    int apt = Ess->getFinalApt(mStation->mNT,mStation->mNH, mStation->mZBIO, mStation->mTOPO);
                    aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    case CS:
        if (mStation->readyCS()){
            for (std::shared_ptr<Layer> l : mVLs){
                if ( l->getCatLayer()==TypeLayer::CS){//l->Type()==TypeLayer::FEE ||
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    std::shared_ptr<cEss> Ess= l->Ess();
                    int apt = Ess->getApt(mStation->mZBIO, mStation->mSt);
                    if (apt!=0) aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    }
    return aRes;
}

void groupLayers::updateGL(){
    bool expertMode=0;

    if(m_app->isLoggedIn()){
        loadExtents(m_app->getUser().id());
        mExtentDivGlob->show();
        expertMode=getExpertModeForUser(m_app->getUser().id());
    } else {
        mExtentDivGlob->hide();
    }

    // boucle sur les layers et envoi du signal pour cacher ou rendre visible les checkbox
    for (std::shared_ptr<Layer> l : mVLs){
        l->ExpertMode(expertMode);
    }
    // pour cacher les noeuds racines , celui "aptitude CS"
    expertMode_.emit(expertMode);


}

void groupLayers::updateLegendeDiv(std::vector<std::shared_ptr<Layer>> layers){
    mLegendDiv->clear();

    if(layers.size()==0)
        mTitle = mLegendDiv->addWidget(cpp14::make_unique<WText>(WString::tr("legendMsg")));
    else{
        mTitle = mLegendDiv->addWidget(cpp14::make_unique<WText>(WString::tr("legendTitre")));

        for (auto layer : layers){
            this->updateLegende(layer);
        }
    }

}

void groupLayers::updateLegende(const std::shared_ptr<Layer> l){
    if (l->getCatLayer()!=TypeLayer::Externe){
        Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromTop, Wt::TimingFunction::EaseOut, 100);

        auto panel = Wt::cpp14::make_unique<Wt::WPanel>();
        panel->setTitle("<h3>"+l->getLegendLabel()+"</h3>");
        panel->addStyleClass("centered-example");
        panel->setCollapsible(true);
        panel->setAnimation(animation);
        //panel->setCollapsed(false);

        auto tab = Wt::cpp14::make_unique<WTable>();
        tab->setHeaderCount(1);
        tab->setWidth(Wt::WLength("90%"));
        tab->toggleStyleClass("table-striped",true);
        tab->setMaximumSize(1000,1000);

        //mTitle->setText(WString::tr("legendTitre"));
        int row(0);
        for (auto kv : l->getDicoVal()){
            if (l->hasColor(kv.first)){
                color col = l->getColor(kv.first);
                tab->elementAt(row, 0)->addWidget(cpp14::make_unique<WText>(kv.second));
                tab->elementAt(row, 1)->setWidth("40%");
                tab->elementAt(row, 1)->decorationStyle().setBackgroundColor(WColor(col.mR,col.mG,col.mB));
                row++;
            }
        }
        panel->setCentralWidget(std::move(tab));
        mLegendDiv->addWidget(std::move(panel));
    }
}

std::shared_ptr<Layer> groupLayers::getActiveLay(){
    std::shared_ptr<Layer> aRes=NULL;
    for (std::shared_ptr<Layer> l : mVLs){
        if (( l->IsActive())){aRes=l;break;}
    }
    // au lancement de l'appli, aucune couche n'est active
    if (aRes==NULL){
        for (std::shared_ptr<Layer>l : mVLs){
            if (( l->Code()=="IGN")){aRes=l;
                l->setActive();
                break;}
        }
    }
    return aRes;
}

std::shared_ptr<Layer> groupLayers::getLay(std::string aCode){
    std::shared_ptr<Layer> aRes=NULL;
    for (std::shared_ptr<Layer> l : mVLs){
        if (( l->Code()==aCode)){aRes=l;break;}
    }
    return aRes;
}

void groupLayers::exportLayMapView(){

    std::cout << "exportLayMapView " << std::endl;

    std::shared_ptr<Layer> l=getActiveLay();// attention, si on vient d'ouvrir le soft, aucune layer n'est actives!! gerer les ptr null
    if (l && l->getCatLayer()!=TypeLayer::Externe){
        m_app->loadingIndicator()->setMessage(tr("LoadingI3"));
        m_app->loadingIndicator()->show();
        // crop layer and download

        // crée l'archive
        std::string archiveFileName = mDico->File("TMPDIR")+"/"+l->Code()+".zip";
        std::string aCroppedRFile = mDico->File("TMPDIR")+"/"+l->Code()+"_crop.tif";
        std::string mClientName=l->Code()+"_crop";
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
            // le fileressources sera détruit au moment de la destruction GroupL
            //std::unique_ptr<WFileResource> fileResource = std::make_unique<Wt::WFileResource>("plain/text",archiveFileName);
            WFileResource * fileResource = new Wt::WFileResource("plain/text",archiveFileName);
            fileResource->suggestFileName(mClientName+".zip");
            m_app->redirect(fileResource->url());
        } else {
            Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                              "Erreur",
                                                              "<p> Cette couche ne peut être découpée sur cette emprise, essayer avec une zone plus petite.</p>",
                                                              Wt::Icon::Critical,
                                                              Wt::StandardButton::Ok));
            messageBox->setModal(true);
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

        messageBox->setModal(true);
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
        // garder l'intersect des 2 extend
        ext.Intersect(extGlob);
        //std::cout << ext.MinX << " , " << ext.MaxX << " , " << ext.MinY << " , " << ext.MaxY << " après intersect " << std::endl;

        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

        //adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;
        //determine dimensions of the new (cropped) raster in cells
        int xSize = round(width/transform[1]);
        int ySize = round(height/transform[1]);
        //std::cout << "xSize " << xSize << ", ySize " << ySize << std::endl;

        // test si la différence entre le raster en entier et le raster croppé est significative, si non on va copier tout au lieu de cropper
        // si 80% ou plus, on garde toute l'image
        if (xSize>0 && ySize>0 && xSize < maxSizePix4Export && ySize<maxSizePix4Export){
            if (getArea(&ext)/getArea(&extGlob)>0.8){
                std::cout << "copie de toute l'image" << std::endl;
                pDriver->CopyFiles(cropPath,inputPath);
                aRes=1;
            } else {
                //create the new (cropped) dataset
                pCroppedRaster = pDriver->Create(cropPath, xSize, ySize, 1, pInputRaster->GetRasterBand(1)->GetRasterDataType(), NULL); //or something similar
                pCroppedRaster->SetProjection( pInputRaster->GetProjectionRef() );
                //pCroppedRaster->SetSpatialRef(pInputRaster->GetSpatialRef());
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
            }
        } else {
            std::cout << " crop du raster a échoué: taille pas correcte " << std::endl;
        }

        GDALClose(pInputRaster);
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
    return aRes;
}

double getArea(OGREnvelope * env){
    return (double) (env->MaxX-env->MinX)*(env->MaxY-env->MinY);
}

/** Fonctions pour gérer les extents sauvés dans DB * */
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
/** FIN extents **/


std::vector<rasterFiles> groupLayers::getSelect4Download(){return mSelectLayers->getSelectedRaster();}
//std::vector<rasterFiles> groupLayers::getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}

//int groupLayers::getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
int groupLayers::getNumSelect4Download(){return mSelectLayers->numSelectedLayer();}
//std::vector<std::shared_ptr<Layer>>groupLayers::getSelectedLayer4Stat(){return mSelect4Stat->getSelectedLayer();}
std::vector<std::shared_ptr<Layer>> groupLayers::getSelectedLayer4Download(){return mSelectLayers->getSelectedLayer();}
//std::vector<Layer *> groupLayers::getAllLayer(){return mSelect4Download->getAllLayer();}


// crée le html en vérifiant que chaque membre de layerMTD soit bien un code html valide
std::string getHtml(LayerMTD * lMTD){

    std::string aRes("");
    aRes+="<h3><strong>"+lMTD->Nom()+"</strong></h3>";
    if (lMTD->Projet()!=""){
        std::string html="<h4>Projet </h4>" +lMTD->Projet();
        if (isValidHtml(html)){aRes+=html;}
    }
    if (lMTD->Descr()!=""){
        std::string html="<h4>Description </h4>" +lMTD->Descr();
        if (isValidHtml(html)){aRes+=html;}
    }
    if (lMTD->Vers()!=""){
        std::string html="<h4>Version  </h4>" +lMTD->Vers();
        if (isValidHtml(html)){aRes+=html;}
    }
    if (lMTD->CopyR()!=""){
        std::string html="<h4>Copyright </h4>" +lMTD->CopyR();
        if (isValidHtml(html)){aRes+=html;}
    }
    if (lMTD->VRefs().size()>0){
        std::string html="<h4>Référence  </h4>" ;
        for (std::string ref : lMTD->VRefs()){
            if (isValidHtml(ref)){html+="<p>"+ref+"</p>";}
        }
        aRes+=html;
    }
    return aRes;
}

bool isValidHtml(std::string text){
    bool aRes(0);
    Wt::WText t(text);
    if (t.textFormat() ==Wt::TextFormat::XHTML){ aRes=1;} else {
        std::cout << " attention, le texte " << text << " n'est pas un code html valide " << std::endl;
    }
    return aRes;
}


GDALDataset * getDSonEnv(std::string inputRaster, OGRGeometry * poGeom){
    OGREnvelope ext;
    poGeom->getEnvelope(&ext);
    GDALDataset * aRes=NULL;
    GDALDataset  * DS = (GDALDataset *) GDALOpen( inputRaster.c_str(), GA_ReadOnly );
    if( DS == NULL )
    {
        std::cout << "je n'ai pas lu l'image " <<  inputRaster << std::endl;
    } else {
        char** papszArgv = nullptr;
        papszArgv = CSLAddString(papszArgv, "-projwin"); //Selects a subwindow from the source image for copying but with the corners given in georeferenced coordinate
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinX).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxY).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxX).c_str());
        papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinY).c_str());

        GDALTranslateOptions * option = GDALTranslateOptionsNew(papszArgv,nullptr);
        if (option){
            //std::cout <<" options parsées " << std::endl;
            GDALDatasetH DScrop = GDALTranslate("/vsimem/out",DS,option,nullptr);

            if (DScrop ){
                // j'ai toujours 2 raster a ouvrir conjointement
                aRes = GDALDataset::FromHandle(DScrop);
            }
        }

        GDALTranslateOptionsFree(option);
        GDALClose(DS);
    }
    return aRes;
}
