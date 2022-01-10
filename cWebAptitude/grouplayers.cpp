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

    std::map<std::string, Wt::WTreeNode*> aMNodes;
    // ajout des nodes pour les groupes de couches
    for (std::string gr :mDico->Dico_groupe){

        std::unique_ptr<Wt::WTreeNode> node1 = Wt::cpp14::make_unique<Wt::WTreeNode>(mDico->groupeLabel(gr));
        node1->addStyleClass("tree_node");
        Wt::WTreeNode * n= tree->treeRoot()->addChildNode(std::move(node1));
        n->label()->clicked().connect([=]{
            if(n->isExpanded()){n->collapse();} else {
                n->expand();}});

        if (mDico->groupeExpert(gr)){ this->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);}
        aMNodes.emplace(gr, n);
    }


    // ajout des cartes
    for (auto & pair :mDico->VlayerBase()){
        std::shared_ptr<layerBase> aLB=pair.second;

        if (mDico->lay4Visu(pair.first)){
            // 1) création du mWtText pour cette couche
            WText * wtext=NULL;
            Wt::WTreeNode * n=NULL;
            if (aMNodes.find(mDico->lay2groupe(pair.first))!=aMNodes.end()){
                n = aMNodes.at(mDico->lay2groupe(pair.first))->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""));
                wtext=n->label();

                // 2) création de la couche
                std::shared_ptr<Layer> aL=std::make_shared<Layer>(this,aLB,wtext);
                // 3) ajout des interactions
                std::string aCode=aL->Code();
                wtext->doubleClicked().connect([this,aCode]{clickOnName(aCode);});
                aL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
                mVLs.push_back(aL);
            } else {
                std::cout << "problème pour couche " << pair.first << " car n'as pas de groupe de couche atitré" << std::endl;
            }

        }else {
            mVLs.push_back(std::make_shared<Layer>(this,aLB));
        }

    }

    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep1")));
    mParent->addWidget(std::move(tree));

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


void groupLayers::updateActiveLay(std::string aCode){
    //std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    // désactiver toutes les couches actives et changer le rendu du label
    for (std::shared_ptr<Layer> l : mVLs){
        l->setActive(aCode==l->Code());
    }
}

void groupLayers::clickOnName(std::string aCode){

    //std::cout << " j'ai cliqué sur un label " << aCode <<  "\n\n"<< std::endl;
    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    updateActiveLay(aCode);

    TypeLayer type;
    std::shared_ptr<Layer> layer;
    for (std::shared_ptr<Layer> l : mVLs){
        if(aCode==l->Code()){
            layer=l;
            type=l->getCatLayer();
            break;
        }
    }

    // cacher la fenetre popup
    mParent->doJavaScript("overlay.setVisible(0);");

    // changer le mode CS vs FEE de grouplayer, utilise pour le tableau d'aptitude
    // attention de ne pas prendre la couche "CS_FEE" dans le tas (pour Chêne Sessile).
    if (type == TypeLayer::CS | type == TypeLayer::KK | (type == TypeLayer::Station & aCode.substr(0,2)=="CS")){
        if (globTest){ std::cout << " passe en mode classif CS , couche " << aCode << std::endl;}
        mTypeClassifST=TypeClassifST::CS;
    } else{ mTypeClassifST=TypeClassifST::FEE;
        if (globTest){ std::cout << " passe en mode classif FEE , couche " << aCode << std::endl;}
    }
    // ajouter la couche à la carte
    for (std::shared_ptr<Layer> l : mVLs){
        if (l->IsActive()){
            l->displayLayer();
            break;
        }
    }
    // ajout de la carte après le "diplaylayer" sinon ordre du code js pas bon? ben oui car couche pas encore dans activeLayers[]
    mcWebAptitude->mPanier->addMap(aCode, layer);
    updateLegendeDiv(mcWebAptitude->mPanier->mVLs);
}

/**
 * @brief groupLayers::extractInfo Affiche les stat d'un point clicqué sur la map.
 * @param x
 * @param y
 */
void groupLayers::extractInfo(double x, double y){

    if(!isnan(x) && !isnan(y) && !(x==0 && y==0)){
        if (globTest){std::cout << "groupLayers ; extractInfo " << x << " , " << y << std::endl;}
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
        mAnaPoint->add1InfoRaster(ptPed.displayInfo(PEDO::CHARGE));

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
                                              //+"overlay.setPosition(coordinate);"
                                              );
                    }

                }
            }

            // si la couche active est la CNSW, on affiche les info pédo dans la fenetre "overlay"
            if (( l->IsActive() && l->Code()=="CNSWrest")){
                mParent->doJavaScript("content.innerHTML = '"+ptPed.displayAllInfoInOverlay()+ "';"
                                      +"var coordinate = ["+std::to_string(x) + ","+ std::to_string(y) +"];"
                                      +"overlay.setPosition(coordinate);"
                                      );
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

/**
 * @brief groupLayers::computeStatGlob
 * @param poGeomGlobale
 */
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

        } else if(l->Code()=="MNH2018P95"){
            // calcul des params dendrométriques
            mVLStatCont.push_back(new statDendro(l,poGeomGlobale));

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
     m_app->addLog("compute stat, "+std::to_string(getNumSelect4Download())+" traitements",typeLog::anas); // add some web stats
}

/**
 * @brief groupLayers::apts
 * @return
 */
std::map<std::string,int> groupLayers::apts(){
    std::map<std::string,int> aRes;
    switch (mTypeClassifST){
    case FEE:{
        if (globTest){std::cout << " GL get apt pour mode FEE " << std::endl;}
        if (mStation->readyFEE()){
            if (globTest){std::cout << "station a NT NH ZBIO et Topo " << std::endl;}
            for (std::shared_ptr<Layer> l : mVLs){
                if (l->getCatLayer()==TypeLayer::FEE ){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    std::shared_ptr<cEss> Ess= l->Ess();
                    int apt = Ess->getFinalApt(mStation->mNT,mStation->mNH, mStation->mZBIO, mStation->mTOPO);
                    aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    }
    case CS:{
        if (globTest){std::cout << " GL get apt pour mode CS " << std::endl;}
        if (mStation->readyCS()){
            if (globTest){std::cout << "station a bien une station du catalogue " << std::endl;}
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
    }
    return aRes;
}

/**
 * @brief groupLayers::updateGL
 */
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

/**
 * @brief groupLayers::updateLegendeDiv Refresh les legendes pour les couches du panier
 * @param layers
 */
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

/**
 * @brief groupLayers::updateLegende Refresh la legende pour une couche
 * @param l
 */
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

/**
 * @brief groupLayers::getActiveLay
 * @return
 */
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

/**
 * @brief groupLayers::getLay Getter d'une couche selon son code
 * @param aCode
 * @return
 */
std::shared_ptr<Layer> groupLayers::getLay(std::string aCode){
    std::shared_ptr<Layer> aRes=NULL;
    for (std::shared_ptr<Layer> l : mVLs){
        if (( l->Code()==aCode)){aRes=l;break;}
    }
    return aRes;
}

/**
 * @brief groupLayers::exportLayMapView Exporte la vue actuelle de la top couche du panier
 */
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
            m_app->addLog(l->Code(),typeLog::dsingle);
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

/**
 * @brief cropIm
 * @param inputRaster
 * @param aOut
 * @param ext
 * @return
 */
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
        // si 65% ou plus, on garde toute l'image
        if (xSize>0 && ySize>0 && xSize < maxSizePix4Export && ySize<maxSizePix4Export){
            if (getArea(&ext)/getArea(&extGlob)>0.65){
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

/**
 * @brief groupLayers::loadExtents Charge/recharge les extents du user
 * @param id
 */
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
    button_s->addStyleClass("extent_button");
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
    m_app->addLog("save an extent",typeLog::extend); // add some web stats
}

void groupLayers::deleteExtent(std::string id_extent){
    openConnection();

    std::string sql = "DELETE FROM user_extent WHERE id="+id_extent;
    std::cout << sql << std::endl;
    sqlite3_exec(db_, sql.c_str(),NULL,NULL,NULL);
    closeConnection();

    loadExtents(m_app->getUser().id());
    m_app->addLog("delete an extent",typeLog::extend); // add some web stats
}
/** FIN extents **/


std::vector<rasterFiles> groupLayers::getSelect4Download(){return mSelectLayers->getSelectedRaster();}
//std::vector<rasterFiles> groupLayers::getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}

//int groupLayers::getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
int groupLayers::getNumSelect4Download(){return mSelectLayers->numSelectedLayer();}

std::vector<std::shared_ptr<Layer>> groupLayers::getSelectedLayer4Download(){return mSelectLayers->getSelectedLayer();}



// crée le html en vérifiant que chaque membre de layerMTD soit bien un code html valide
std::string getHtml(LayerMTD * lMTD){

    std::string aRes("");


    aRes+="<h3><strong>"+lMTD->Nom()+"</strong></h3>";

    std::string proj(WString::tr(lMTD->code()+".projet").toUTF8());
    if (proj.substr(0,2)=="??" && lMTD->Projet()!=""){
        std::string proj="<h4>Projet </h4>" +lMTD->Projet();

    }
     if (proj!="" && isValidHtml(proj)){aRes+=proj;}
    // si il y a un message avec le bon id, on le prend

     std::string descr(WString::tr(lMTD->code()+".description").toUTF8());
     if (descr.substr(0,2)=="??"){
         descr="<h4>Description </h4>" + lMTD->Descr();
    }
     if (descr!="" && isValidHtml(descr)){aRes+=descr;}

    std::string version( "<h4>Version  </h4>" + WString::tr(lMTD->code()+".version").toUTF8());
    if (version.substr(0,2)=="??"){
        if (lMTD->Vers()!=""){

        version="<h4>Version  </h4>" +lMTD->Vers();
        }
    }
    if (version!="" && isValidHtml(version)){aRes+=version;}

    // test si il existe un message dans le xml qui contient les logs de changements pour cette carte
    std::string logs(WString::tr(lMTD->code()+".logs").toUTF8());
    if (logs.substr(0,2)!="??"){
        std::string html="<h4>Information de modification </h4>" +logs;
        if (isValidHtml(logs)){aRes+=html;}
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
