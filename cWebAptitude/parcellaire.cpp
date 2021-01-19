#include "parcellaire.h"

int globSurfMax(3000);// en ha
int globVolMaxShp(10000);// en ko // n'as pas l'air de fonctionner comme je le souhaite

parcellaire::parcellaire(groupLayers *aGL, Wt::WApplication* app, statWindow *statW):mGL(aGL),centerX(0.0),centerY(0.0),mClientName(""),mName(""),mFullPath(""),m_app(app),fu(NULL),msg(NULL)
  ,hasValidShp(0)
  ,downloadRasterBt(NULL)
  ,mStatW(statW)
  ,poGeomGlobale(NULL)

{
    //std::cout << "creation parcellaire " << std::endl;
    mDico=aGL->Dico();
    GDALAllRegister();
    mParcellaireExtent.MinX=0;mParcellaireExtent.MinY=0;mParcellaireExtent.MaxX=0;mParcellaireExtent.MaxY=0;

    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    setInline(0);

    addWidget(cpp14::make_unique<WText>(tr("anaStep1")));
    //mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoParcellaire")));

    fu =addNew<Wt::WFileUpload>();
    fu->setFileTextSize(globVolMaxShp); // Set the maximum file size. il faut également changer param max-request-size dans wt_config
    fu->setFilters(".shp, .shx, .dbf, .prj");
    fu->setMultiple(true);
    fu->setInline(0);
    fu->addStyleClass("btn-file");
    addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    //fu->setMargin(20,Wt::Side::Bottom | Wt::Side::Top); // si le parent a des marges et est inline(0) et que je met l'enfant à inline, l'enfant a des marges également

    msg = addWidget(cpp14::make_unique<Wt::WText>());
    msg->setInline(0);
    addWidget(cpp14::make_unique<WText>(tr("anaStep2")));

    mContSelect4D= addWidget(cpp14::make_unique<Wt::WContainerWidget>());

    downloadRasterBt = addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger les cartes"));
    downloadRasterBt->setStyleClass("btn btn-success");
    downloadRasterBt->setWidth(200);
    downloadRasterBt->setInline(0);
    downloadRasterBt->disable();

    fu->fileTooLarge().connect([=] { msg->setText("Le fichier est trop volumineux (max "+std::to_string(globVolMaxShp)+"Ko).");});
    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);

    downloadRasterBt->clicked().connect(this,&parcellaire::downloadRaster);
    // ouu je pense que c'est mal, car si j'appui sur boutton télécharger les cartes, il me dis que toutes les cartes sont sélectionnées
    mContSelect4D->addWidget(std::unique_ptr<selectLayers4Download>(mGL->mSelect4Download));
}

parcellaire::~parcellaire(){
    //std::cout << "destructeur de parcellaire" << std::endl;
    cleanShpFile();
    fu=NULL;
    //uploadButton=NULL;
    m_app=NULL;
    msg=NULL;
    mGL=NULL;
    mDico=NULL;
}

void parcellaire::cleanShpFile(){
    //std::cout << "clean shp files ..." ;
    std::vector<boost::filesystem::path> aFiles;
    // supprime les éventuels fichiers shp chargé
    for(auto & p : boost::filesystem::recursive_directory_iterator(mDico->File("TMPDIR"))){
        if (p.path().stem().string()==mName){
            aFiles.push_back(p.path());
        }
    }
    for (auto file : aFiles){
        boost::filesystem::remove(file);
    }
    if (poGeomGlobale!=NULL){OGRGeometryFactory::destroyGeometry(poGeomGlobale);}
    hasValidShp=0;
    // je devrait réinitialiser les nom mFullPath et autre ici aussi?
    //std::cout << "done " << std::endl;
}

// le nouveau toGeoJson ; effectue un changement de src
bool parcellaire::to31370AndGeoJson(){
    bool aRes=true;
    std::cout << " parcellaire::toGeoJson() ... " ;
    std::string  output(geoJsonName().c_str());
    const char *outPath=output.c_str();
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    // ouverture en update mode pour pouvoir projeter en bl72
    GDALDatasetH hSrcDS  = GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    char** papszArgv = nullptr;
    //papszArgv = CSLAddString(papszArgv, "-a_srs"); // target src without reprojection
    papszArgv = CSLAddString(papszArgv, "-t_srs"); // target src with reprojection
    papszArgv = CSLAddString(papszArgv, "EPSG:31370");
    GDALVectorTranslateOptions * option = GDALVectorTranslateOptionsNew(papszArgv, nullptr);
    if( option ){
        // 1 on converti en geojson pour ol
        GDALDatasetH hOutDS = GDALVectorTranslate(outPath,nullptr,1,&hSrcDS,option,nullptr);
        if( hOutDS ){
            GDALClose(hOutDS);
        } else { aRes=0; std::cout << "changement de src par GDALVectorTranslate vers geojson; echec" << std::endl;}
    } else {
        std::cout << "options shp to geojson pas correctement parsées " << std::endl;
        aRes=0;
    }
    GDALVectorTranslateOptionsFree(option);
    GDALClose(hSrcDS);
    // la source devient le geojson
    hSrcDS  = GDALOpenEx( outPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    papszArgv = CSLAddString(papszArgv, "-overwrite");
    GDALVectorTranslateOptions * option2 = GDALVectorTranslateOptionsNew(papszArgv, nullptr);
    if( option2 ){
        // 2) on s'assure que le shp soit en BL72
        GDALDatasetH hOutDS = GDALVectorTranslate(inputPath,nullptr,1,&hSrcDS,option2,nullptr);
        if( hOutDS ){
            GDALClose(hOutDS);
        }
    }
    GDALVectorTranslateOptionsFree(option2);
    GDALClose(hSrcDS);
    return aRes;
}

bool parcellaire::computeGlobalGeom(){
    //std::cout << "computeGlobalGeom " << std::endl;
    bool aRes(0);
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    GDALDataset * DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DS != NULL )
    {
        OGRLayer * lay = DS->GetLayer(0);
        // union de tout les polygones du shp
        OGRFeature *poFeature;
        //OGRPolygon * poGeom;
        OGRGeometry * poGeom;
        OGRGeometry * poGeom2;
        std::unique_ptr<OGRMultiPolygon> multi = std::make_unique<OGRMultiPolygon>();
        OGRErr err;
        OGRMultiPolygon *poGeomM;

        int nbValidPol(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            //OGRGeometry * tmp=OGRGeometryFactory::forceTo((poFeature->GetGeometryRef()));
            poFeature->GetGeometryRef()->flattenTo2D();
            switch (poFeature->GetGeometryRef()->getGeometryType()){
            case (wkbPolygon):
            {
                poGeom=poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom = poGeom->Buffer(0.0);
                //poGeom->Simplify(1.0);
                err = multi->addGeometry(poGeom);
                if (err==OGRERR_NONE) nbValidPol++;
                break;
            }

            case wkbMultiPolygon:
            {
                poGeomM= poFeature->GetGeometryRef()->toMultiPolygon();
                int n(poGeomM->getNumGeometries());
                for (int i(0);i<n;i++){
                    poGeom=poGeomM->getGeometryRef(i);
                    poGeom->closeRings();
                    poGeom = poGeom->Buffer(0.0);
                    //if (poFeature->GetGeometryRef()->getGeometryType()==wkbPolygon)
                    err = multi->addGeometry(poGeom);
                    if (err==OGRERR_NONE) nbValidPol++;
                }
                break;
            }
            default:
                std::cout << "Geometrie " << poFeature->GetFID() << ", type de geometrie non pris en charge ; " << poFeature->GetGeometryRef()->getGeometryName() << ", id " << poFeature->GetGeometryRef()->getGeometryType()<< std::endl;

                break;
            }
            if (err!=OGRERR_NONE){
                std::cout << "problem avec ajout de la geometrie " << poFeature->GetFID() << ", erreur : " << err <<  std::endl;
            }
            OGRFeature::DestroyFeature(poFeature);
        }

        // test si
        if (nbValidPol>0){poGeom2 = multi->UnionCascaded();

            poGeom2 =poGeom2->Buffer(1.0);// ça marche bien on dirait! je sais pas si c'est le buffer 1 ou le simplify 1 qui enlève les inner ring (hole) qui restent.
            poGeomGlobale = poGeom2->Simplify(1.0);
            int aSurfha=OGR_G_Area(poGeomGlobale)/10000;
            printf("aSurfha=%d",aSurfha);
            if (aSurfha<globSurfMax){
                poGeomGlobale->getEnvelope(&mParcellaireExtent);
                centerX= (mParcellaireExtent.MaxX+mParcellaireExtent.MinX)/2;
                centerY= (mParcellaireExtent.MaxY+mParcellaireExtent.MinY)/2;
                aRes=1;
            }
        }

    }
    GDALClose(DS);
    return aRes;
}

void parcellaire::display(){
    //std::cout << " parcellaire::display " << std::endl;
    boost::system::error_code ec;
    std::string JSfile(mDico->File("addOLgeojson"));
    if (boost::filesystem::exists(JSfile,ec)){
        assert(!ec);
        std::cout << " ... " << std::endl;
        std::stringstream ss;
        std::string aFileIn(JSfile);

        std::ifstream in(aFileIn);
        ss << in.rdbuf();
        in.close();
        std::string JScommand(ss.str());

        std::string aFind1("NAME");

        std::string aReplace(geoJsonRelName());

        boost::replace_all(JScommand,aFind1,aReplace);

        // extent du parcellaire
        boost::replace_all(JScommand,"MAXX",std::to_string(mParcellaireExtent.MaxX));
        boost::replace_all(JScommand,"MAXY",std::to_string(mParcellaireExtent.MaxY));
        boost::replace_all(JScommand,"MINX",std::to_string(mParcellaireExtent.MinX));
        boost::replace_all(JScommand,"MINY",std::to_string(mParcellaireExtent.MinY));

        doJavaScript(JScommand);
        // centrer la map sur le shp
        doJavaScript("map.getView().setCenter(["+std::to_string(centerX)+","+std::to_string(centerY)+" ]);");
    } else {
        std::cout << " ne trouve pas le fichier script de js " << std::endl;
    }
    //std::cout << " done " << std::endl;
}

/*
void parcellaire::clickUploadBt(){
    uploadButton->disable();
    fu->upload();
    uploadButton->enable();
}*/

void parcellaire::fuChanged(){
    // si on a changé le contenu, on tente de le télécharger
    fu->upload();
    //uploadButton->enable();
}

void parcellaire::upload(){
    std::cout << "upload commence.. " ;
    //computeStatButton->disable();
    downloadRasterBt->disable();
    cleanShpFile();
    boost::filesystem::path p(fu->clientFileName().toUTF8()), p2(this->fu->spoolFileName());
    this->mClientName = p.stem().c_str();

    mName = ((std::string) p2.filename().c_str()) + "-"+ mClientName;
    mFullPath = this->mDico->File("TMPDIR")+ mName;

    int nbFiles(0);
    for (Http::UploadedFile file : fu->uploadedFiles()){
        boost::filesystem::path a(file.clientFileName());
        //boost::filesystem::rename(file.spoolFileName(),mFullPath+a.extension().c_str());
        // sur server boost::filesystem::rename: Invalid cross-device link:  car /data1 et tmp sont sur des différents volumes.
        //solution ; copy! de toute manière tmp/ est pugé souvent
        boost::filesystem::copy(file.spoolFileName(),mFullPath+a.extension().c_str());
        //std::cout << "réception " << mFullPath+a.extension().c_str() << std::endl;
        //if ((a.extension().string()==".shp") | (a.extension().string()==".shx" )| (a.extension().string()==".dbf") | (a.extension().string()==".qpj") | (a.extension().string()==".prj")) nbFiles++;
        if ((a.extension().string()==".shp") | (a.extension().string()==".shx" )| (a.extension().string()==".dbf")) nbFiles++;
    }

    // ici je converti en json et affichage dans ol
    if (nbFiles==3){
        msg->setText("Téléchargement du shp effectué avec succès.");
        if (to31370AndGeoJson()){
            if (computeGlobalGeom()){
                hasValidShp=true;
                //computeStatButton->enable();
                downloadRasterBt->enable();
                display();
                mGL->mMap->setToolTip(tr("tooltipMap2"));
            }
        }
    } else {
        msg->setText("Veillez sélectionner les 3 fichiers du shapefile (shp, shx et dbf).");
        cleanShpFile();
    }
}

/*
void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " ;

    m_app->loadingIndicator()->setMessage(tr("LoadingI2"));
    m_app->loadingIndicator()->show();

    //visuStatButton->enable();

    // ici j'ouvre le shp
    //std::cout << " prépare le calcul sur chacun des polygones ... " ;
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        // layer
        OGRLayer * lay = mDS->GetLayer(0);
        mGL->computeStatOnPolyg(lay,mCB_fusionOT->isChecked());
        //downloadShpBt->enable();
        GDALClose(mDS);
    }
    m_app->loadingIndicator()->hide();
    m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));
    std::cout << " ..done " << std::endl;
}  
*/


void parcellaire::visuStat(OGRFeature *poFeature){
    std::cout << " parcellaire::visuStat()... " ;

    mStatW->vider();
    mStatW->titre("<h4>Statistique pour polygone FID " + std::to_string(poFeature->GetFID()) + " de " + mClientName+ "</h4>");
    mStatW->generateGenCarte(poFeature);
    mStatW->genIndivCarteAndAptT();

    std::cout << " ..done " << std::endl;
    m_app->setInternalPath("/resultat",true);
    //auto pdf = std::make_shared<ReportResource>(mStatW);
    //mStatW->createPdfBut->setLink(WLink(pdf));
}

// si je click sur un polygone dans ol, calcule les stat et affiche dans une nouvelle page
void parcellaire::computeStatAndVisuSelectedPol(int aId){
    std::cout << " computeStatAndVisuSelectedPol  , id " << aId << std::endl;
    m_app->loadingIndicator()->setMessage(tr("LoadingI1"));
    m_app->loadingIndicator()->show();
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        // layer
        OGRLayer * lay = mDS->GetLayer(0);
        OGRFeature *poFeature;
        bool find(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            if (poFeature->GetFID()==aId){
                OGRGeometry * poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom->flattenTo2D();// sinon la géométrie est genre polygone3D et pas pris en charge dans staticMap par ex
                mGL->computeStatGlob(poGeom);
                find=1;
                break;
            }
        }
        if (find) {visuStat(poFeature);}
        GDALClose(mDS);
    }
    m_app->loadingIndicator()->hide();
    m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));
}

std::string parcellaire::geoJsonName(){return mFullPath + ".geojson";}
std::string parcellaire::geoJsonRelName(){ return "tmp/" + mName +".geojson";}

/*
void parcellaire::downloadShp(){

    // créer une nouvelle archive et y mettre tout les fichiers du shp
    // attention, pour l'instant l'archive crée n'est pas supprimée
    ZipArchive* zf = new ZipArchive(mFullPath+".zip");
    zf->open(ZipArchive::WRITE);
    zf->addFile(mClientName+"_statForestimator.shp",mFullPath+".shp");
    zf->addFile(mClientName+"_statForestimator.dbf",mFullPath+".dbf");
    zf->addFile(mClientName+"_statForestimator.shx",mFullPath+".shx");
    if (exists(mFullPath+".prj"))  zf->addFile(mClientName+"_statForestimator.prj",mFullPath+".prj");

    zf->close();
    delete zf;

     //std::unique_ptr<WFileResource> fileResource = std::make_unique<Wt::WFileResource>("plain/text",mFullPath+".zip");
     WFileResource *fileResource = new Wt::WFileResource("plain/text",mFullPath+".zip");
     fileResource->suggestFileName(mClientName+"_statForestimator.zip");
    m_app->redirect(fileResource->url());
    // quand est-ce que je supprime fileResource? et l'archive? l'objet wt et les fichiers?
    // pas simple on dirai : https://redmine.webtoolkit.eu/boards/2/topics/16990?r=16992#message-16992
    //https://redmine.webtoolkit.eu/boards/2/topics/11758?r=11774
}
*/

void parcellaire::downloadRaster(){

    std::vector<rasterFiles> vRs=mGL->getSelect4Download();
    if (vRs.size()>0){

        m_app->loadingIndicator()->setMessage(tr("LoadingI4"));
        m_app->loadingIndicator()->show();
        // crée l'archive
        ZipArchive* zf = new ZipArchive(mFullPath+"_raster.zip");
        zf->open(ZipArchive::WRITE);
        // crop les raster selectionnés

        //mGL->mPBar->setValue(0);
        //mGL->mPBar->setMaximum(vRs.size());
        for (const rasterFiles & r : vRs){
            std::string aCroppedRFile=mFullPath+"_"+r.Code()+".tif";
            //mGL->mPBar->setToolTip("découpe de la carte " + r.code() + "...");
            if (cropImWithShp(r.getPathTif(),aCroppedRFile)){
                zf->addFile(mClientName+"_"+r.Code()+".tif",aCroppedRFile);
                if (r.hasSymbology()){zf->addFile(mClientName+"_"+r.Code()+".qml",r.symbology());}
            }
            //mGL->mPBar->setValue(mGL->mPBar->value()+1);
            //m_app->processEvents();
        }
        // mGL->mPBar->setToolTip("");
        zf->close();
        delete zf;
        m_app->loadingIndicator()->hide();
        m_app->loadingIndicator()->setMessage(tr("defaultLoadingI"));

        // bof ça marche paaaaas avec unique ptr, je sais pas pk wt renvoi vers une page "nothing to say about that"
        //std::unique_ptr<WFileResource> fileResource = cpp14::make_unique<Wt::WFileResource>("plain/text",mFullPath+"_raster.zip");
        //WFileResource * fileResource_ = fileResource->get();

        WFileResource * fileResource = new Wt::WFileResource("plain/text",mFullPath+"_raster.zip");
        fileResource->suggestFileName(mClientName+"_raster.zip");
        m_app->redirect(fileResource->url());

    } else {
        auto messageBox =
                addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                             "Sélection des couches à exporter",
                              tr("download.lay.error.noLay")
                             ,
                             Wt::Icon::Information,
                             Wt::StandardButton::Ok));

        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            removeChild(messageBox);
        });
        messageBox->show();
    }
}


bool parcellaire::cropImWithShp(std::string inputRaster, std::string aOut){
    bool aRes(0);
    std::cout << " cropImWithShp" << std::endl;

    // enveloppe de la géométrie globale
    OGREnvelope ext;
    poGeomGlobale->getEnvelope(&ext);
    aRes=cropIm(inputRaster, aOut, ext);
    return aRes;
}

void parcellaire::selectPolygon(double x, double y){
    if(!isnan(x) && !isnan(y) && !(x==0 && y==0)){
        std::cout << "parcellaire::selectPolygon " << std::endl;
        std::string input(mFullPath+ ".shp");
        const char *inputPath=input.c_str();
        GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
        if( mDS != NULL )
        {
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            lay->ResetReading();

            OGRPoint pt(x,y);
            //pt.assignSpatialReference(lay->GetSpatialRef());
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                OGRGeometry * poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom->flattenTo2D();
                if ( pt.Within(poGeom)){
                    computeStatAndVisuSelectedPol(poFeature->GetFID());
                    break;
                }
            }
            GDALClose(mDS);
        } else { std::cout << "select Polygone mDS is null " << std::endl;}
    }
}
