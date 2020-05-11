#include "parcellaire.h"


int globSurfMax(2500);// en ha
//https://www.quora.com/What-are-the-risks-associated-with-the-use-of-lambda-functions-in-C-11

parcellaire::parcellaire(WContainerWidget *parent, groupLayers *aGL, Wt::WApplication* app, WStackedWidget *aTopStack, WContainerWidget *statW):mParent(parent),mStatW(statW),mGL(aGL),centerX(0.0),centerY(0.0),mClientName(""),mJSfile(""),mName(""),mFullPath(""),m_app(app),fu(NULL),msg(NULL),uploadButton(NULL),mTopStack(aTopStack)
  ,computeStatButton(NULL)
  ,visuStatButton(NULL)
  ,hasValidShp(0)
  ,downloadRasterBt(NULL)
{
    mDico=aGL->Dico();
    mJSfile=  aGL->Dico()->File("addOLgeojson");

    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    mParent->setMargin(20,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoParcellaire")));
    mParent->addWidget(cpp14::make_unique<WText>("<h4>Charger votre parcellaire</h4>"));
    fu =mParent->addNew<Wt::WFileUpload>();

    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    fu->setFileTextSize(2000); // Set the maximum file size to 50 kB.
    fu->setFilters(".shp, .shx, .dbf, .prj");
    fu->setMultiple(true);
    fu->setInline(0);
    //fu->setMargin(20,Wt::Side::Bottom | Wt::Side::Top); // si le parent a des marges et est inline(0) et que je met l'enfant à inline, l'enfant a des marges également

    msg = mParent->addWidget(cpp14::make_unique<Wt::WText>());
    msg->setInline(0);
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    uploadButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger"));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    uploadButton->disable();
    uploadButton->setInline(0);

    mParent->addWidget(cpp14::make_unique<WText>("<h4>Calcul de statistique</h4>"));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoCalculStat")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mCB_fusionOT= mParent->addWidget(Wt::cpp14::make_unique<Wt::WCheckBox>(tr("cb_fusionAptOT")));
    mCB_fusionOT->setInline(0);
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoChoixLayerStat")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mParent->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Stat()));

    computeStatButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Calcul"));
    computeStatButton->setInline(0);
    computeStatButton->disable();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    mGL->mPBar = mParent->addNew<Wt::WProgressBar>();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mGL->mPBar->setRange(0, mGL->getNumSelect4Stat());
    mGL->mPBar->setValue(0);
    mGL->mPBar->setInline(0);
    /*mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoVisuStat")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    visuStatButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Visualiser les statistiques"));
    visuStatButton->setInline(0);
    visuStatButton->disable();
    est buggé pour le moment
    */

    downloadShpBt = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger le shp"));
    downloadShpBt->setInline(0);
    downloadShpBt->disable();
    //visuStatButton->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/StatistiqueParcellaire"));
    mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoDownloadClippedRaster")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mParent->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Download()));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    downloadRasterBt = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger les cartes"));
    downloadRasterBt->setInline(0);
    downloadRasterBt->disable();

    fu->fileTooLarge().connect([=] { msg->setText("Le fichier est trop volumineux (max 2000ko).");});
    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);
    uploadButton->clicked().connect(this ,&parcellaire::clickUploadBt);
    computeStatButton->clicked().connect(this,&parcellaire::computeStat);
    //visuStatButton->clicked().connect(this,&parcellaire::visuStat);
    downloadShpBt->clicked().connect(this,&parcellaire::downloadShp);
    downloadRasterBt->clicked().connect(this,&parcellaire::downloadRaster);
}

parcellaire::~parcellaire(){
    //std::cout << "destructeur de parcellaire" << std::endl;
    cleanShpFile();
    mParent=NULL;
    fu=NULL;
    uploadButton=NULL;
    m_app=NULL;
    msg=NULL;
    mGL=NULL;
    mDico=NULL;
    delete poGeomGlobale;
    visuStatButton=NULL;
    downloadShpBt=NULL;
    mCB_fusionOT=NULL;
}

void   parcellaire::cleanShpFile(){
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
    // je devrait réinitialiser les nom mFullPath et autre ici aussi?
    //std::cout << "done " << std::endl;
}

bool parcellaire::toGeoJson(){
    bool aRes=true;
    std::cout << " parcellaire::toGeoJson() ... " ;
    std::string input(mFullPath+ ".shp"), output(geoJsonName().c_str());
    const char *inputPath=input.c_str();
    const char *outPath=output.c_str();
    GDALDriver  *jsonDriver;
    GDALAllRegister();

    //std::cout << "input shp : " << inputPath << " , output " << outPath << std::endl;
    //OGRSpatialReference  * spatialReference=new OGRSpatialReference;
    //spatialReference->importFromEPSG(31370);

    // datasource
    jsonDriver =GetGDALDriverManager()->GetDriverByName("GeoJSON");

    if( jsonDriver == NULL )
    {
        printf( "%s driver not available.\n", "GeoJSON" );
        msg->setText("GeoJSO driver not available!");
        aRes=false;

    } else {
        GDALDataset * DS;

        DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR, NULL, NULL, NULL );
        if( DS == NULL )
        {
            printf( "Open failed.\n" );
            aRes=false;

        } else {
            // layer
            OGRLayer * lay = DS->GetLayer(0);
            if (computeGlobalGeom(lay)){

            //std::cout << lay->GetName() << std::endl;
            char **papszOptions = NULL; // il faut utiliser le driver GeoJSONSeq si je veux pouvoir utiliser les options ci-dessous
            //papszOptions = CSLSetNameValue( papszOptions, "ATTRIBUTES_SKIP", "YES" );
            //papszOptions = CSLSetNameValue( papszOptions, "WRITE_BBOX", "YES" );
            /*papszOptions = CSLSetNameValue( papszOptions, "COORDINATE_PRECISION", "2" );
    std::cout << "papszOptions " <<papszOptions << std::endl;
    papszOptions = CSLSetNameValue( papszOptions, "SIGNIFICANT_FIGURES", "5" );
     std::cout << "papszOptions " <<papszOptions << std::endl;
    //papszOptions = CSLSetNameValue( papszOptions, "RFC7946", "YES" );
    std::cout << "papszOptions " <<papszOptions << std::endl;
    */
            GDALDataset * DS2;
            DS2 = jsonDriver->CreateCopy(outPath, DS, FALSE, papszOptions,NULL, NULL );
            GDALClose( DS2 );
            GDALClose( DS );
            } else {
                aRes=0;
                msg->setText("vérifiez que la surface totale de vos parcelles est bien inférieur à " + std::to_string(globSurfMax) + " ha.");
            }
        }}

    //std::cout << " done " << std::endl;

    return aRes;
}

bool parcellaire::computeGlobalGeom(OGRLayer * lay){
    bool aRes(0);
    // union de tout les polygones du shp
    OGRFeature *poFeature;
    //OGRPolygon * poGeom;
    OGRGeometry * poGeom;
    OGRGeometry * poGeom2;
    OGRMultiPolygon *multi = new OGRMultiPolygon();
    OGRErr err;
    OGRMultiPolygon *poGeomM;

    lay->ResetReading();
    int nbValidPol(0);
    while( (poFeature = lay->GetNextFeature()) != NULL )
    {
        // allez les gros bourrin

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
    }

    // test si
    if (nbValidPol>0){poGeom2 = multi->UnionCascaded();

        poGeom2 =poGeom2->Buffer(1.0);// ça marche bien on dirait! je sais pas si c'est le buffer 1 ou le simplify 1 qui enlève les inner ring (hole) qui restent.

        // devrait je pense eêtre créé avec new, car si je ferme le dataset qui contient le layer, poGeomGlobal fait une fuite
        poGeomGlobale =poGeom2->Simplify(1.0);
        int aSurfha=OGR_G_GetArea(poGeomGlobale)/10000;
        if (aSurfha<globSurfMax){
        //OGRPolygon * pol=poGeom2->toPolygon();
        //std::ofstream out("/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude/tmp/test.geojson");
        //out << poGeom->exportToJson();
        //out.close();
        OGRPoint * aPt=NULL;
        err = poGeomGlobale->Centroid(aPt);
        if (err!=OGRERR_NONE){
            std::cout << "problem avec le calcul du centroid, erreur : " << err <<  std::endl;
            OGREnvelope ext;
            poGeomGlobale->getEnvelope(&ext);
            centerX= (ext.MaxX+ext.MinX)/2;
            centerY= (ext.MaxY+ext.MinY)/2;
        } else {

            centerX=aPt->getX();
            centerY=aPt->getY();
        }
        aRes=1;

    }
    }

    delete multi;

    return aRes;
}

void parcellaire::display(){
    //std::cout << " parcellaire::display " << std::endl;
    boost::system::error_code ec;
    if (boost::filesystem::exists(mJSfile,ec)){
        assert(!ec);
        std::cout << " ... " << std::endl;
        std::stringstream ss;
        std::string aFileIn(mJSfile);

        std::ifstream in(aFileIn);
        std::string aTmp(aFileIn+".tmp");
        std::ofstream out(aTmp);

        std::string aFind1("NAME");
        std::string line;
        std::string aReplace(geoJsonRelName());
        while (getline(in, line))
        {
            boost::replace_all(line,aFind1,aReplace);
            out << line << "\n";
        }
        in.close();
        out.close();

        in.open(aFileIn+".tmp");
        ss << in.rdbuf();
        in.close();
        boost::filesystem::remove(aFileIn+".tmp");
        //std::cout << " do js script " << std::endl;
        mParent->doJavaScript(ss.str());
        // centrer la map sur le shp
        mParent->doJavaScript("map.getView().setCenter(["+std::to_string(centerX)+","+std::to_string(centerY)+" ]);");
    } else {
        std::cout << " ne trouve pas le fichier script de js " << std::endl;
    }
    //std::cout << " done " << std::endl;
}

void parcellaire::clickUploadBt(){
    uploadButton->disable();
    fu->upload();
    uploadButton->enable();
}

void parcellaire::fuChanged(){
    uploadButton->enable();
}


void parcellaire::upload(){
    //std::cout << "upload commence.. " ;
    computeStatButton->disable();
    downloadRasterBt->disable();

    //visuStatButton->disable();
    hasValidShp=false;
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
        if (toGeoJson()){
            hasValidShp=true;
            computeStatButton->enable();
            downloadRasterBt->enable();
            display();}
    } else {
        msg->setText("Veillez sélectionner les 3 fichiers du shapefile (shp, shx et dbf).");
        cleanShpFile();
    }
}

void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " ;
    mGL->mPBar->setMaximum(mGL->getNumSelect4Stat());
    mGL->mPBar->setValue(0);
    std::map<std::string,std::map<std::string,int>> stat= mGL->computeStatGlob(poGeomGlobale);
    mGL->mPBar->setValue(0);
    //visuStatButton->enable();

    // ici j'ouvre le shp
    std::cout << " prépare le calcul sur chacun des polygones ... " ;
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    GDALAllRegister();
    GDALDataset * DS;


    DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
    if( DS == NULL )
    {
        printf( "Open failed.\n" );
    } else {
        // layer
        OGRLayer * lay = DS->GetLayer(0);
        mGL->computeStatOnPolyg(lay,mCB_fusionOT->isChecked());
        // sauve le résultat
        GDALClose( DS );
        downloadShpBt->enable();
    }
    std::cout << " ..done " << std::endl;
}

void parcellaire::visuStat(){
    std::cout << " parcellaire::visuStat()... " ;
    mGL->mPBar->setValue(0);
    std::cout << " CLEAR  mStaW... " ;
    mStatW->clear();
    std::cout << " done... " ;
    mStatW->setOverflow(Wt::Overflow::Auto);
    // premier lay pour mettre titre et boutton de "retour"
    auto layout = mStatW->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    auto contTitre = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * contTitre_ = contTitre.get();
    contTitre->addWidget(cpp14::make_unique<WText>("<h4>Statistique globale pour "+ mClientName+ "</h4>"));
    Wt::WPushButton * retourButton = contTitre_->addWidget(cpp14::make_unique<Wt::WPushButton>("Retour"));
    //retourButton->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/Aptitude"));
    //retourButton->clicked().connect([&] {topStack->setCurrentIndex(0);});// avec &, ne tue pas la session mais en recrée une. avec =, tue et recrée, c'est car le lambda copie plein de variable dont this, ça fout la merde
    // non c'est pas la faute du lambda, c'est les internal path qui font qu'une nouvelle session est créée.
    std::cout << " config retour button "<< std::endl;
    //retourButton->clicked().connect(this,mTopStack->setCurrentIndex(0);});
    retourButton->clicked().connect([=] {mTopStack->setCurrentIndex(0);});
    std::cout << " done "<< std::endl;

    auto contCharts = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * contCharts_ = contCharts.get();
    Wt::WGridLayout * grid = contCharts_->setLayout(Wt::cpp14::make_unique<Wt::WGridLayout>());

    int nbChart=mGL->ptrVLStat().size();
    std::cout << " nb Chart "<< nbChart << std::endl;
    mGL->mPBar->setMaximum(nbChart);
    int nbColumn=std::min(2,int(std::sqrt(nbChart)));
    int row(0),column(0);
    // mauvaise manière de boucler sur un pointer!!!
    //for (layerStatChart * chart : mGL->ptrVLStat()) {
    for (const auto & chart : mGL->ptrVLStat()) {
        std::cout << " row " << row << " , col " << column << std::endl;
        grid->addWidget(std::unique_ptr<Wt::WContainerWidget>(chart->getChart()), row, column);
        column++;
        if (column>nbColumn){row++;column=0;}

        mGL->mPBar->setValue(mGL->mPBar->value()+1);
        mGL->m_app->processEvents();
    }

    layout->addWidget(std::move(contTitre), 0);
    layout->addWidget(std::move(contCharts), 1);
    std::cout << " change tostack index... " ;
    mTopStack->setCurrentIndex(1);
    mGL->mPBar->setValue(0);
    std::cout << " ..done " << std::endl;
}

std::string parcellaire::geoJsonName(){return mFullPath + ".geojson";}
std::string parcellaire::geoJsonRelName(){ return "tmp/" + mName +".geojson";}

void parcellaire::downloadShp(){

    // créer une nouvelle archive et y mettre tout les fichiers du shp
    // attention, pour l'instant l'archive crée n'est pas supprimée
    ZipArchive* zf = new ZipArchive(mFullPath+".zip");
    zf->open(ZipArchive::WRITE);
    zf->addFile(mClientName+"_statForestimator.shp",mFullPath+".shp");
    zf->addFile(mClientName+"_statForestimator.dbf",mFullPath+".dbf");
    zf->addFile(mClientName+"_statForestimator.shx",mFullPath+".shx");
    zf->close();
    delete zf;

    WFileResource *fileResource = new Wt::WFileResource("plain/text",mFullPath+".zip");
    fileResource->suggestFileName(mClientName+"_statForestimator.zip");
    m_app->redirect(fileResource->url());
    // quand est-ce que je supprime fileResource? l'objet wt et les fichiers?
    // pas simple on dirai : https://redmine.webtoolkit.eu/boards/2/topics/16990?r=16992#message-16992
}

void parcellaire::downloadRaster(){

    // crée l'archive
    ZipArchive* zf = new ZipArchive(mFullPath+"_raster.zip");
    zf->open(ZipArchive::WRITE);
    // crop les raster selectionnés

    std::vector<rasterFiles> vRs=mGL->getSelect4Download();
    mGL->mPBar->setValue(0);
    mGL->mPBar->setMaximum(vRs.size());
    for (const rasterFiles & r : vRs){
        std::string aCroppedRFile=mFullPath+"_"+r.code()+".tif";
        mGL->mPBar->setToolTip("découpe de la carte " + r.code() + "...");
        if (cropImWithShp(r.tif(),aCroppedRFile)){
           zf->addFile(mClientName+"_"+r.code()+".tif",aCroppedRFile);
           if (r.hasSymbology()){zf->addFile(mClientName+"_"+r.code()+".qml",r.symbology());}
        }
        mGL->mPBar->setValue(mGL->mPBar->value()+1);
        m_app->processEvents();
    }
    mGL->mPBar->setToolTip("");
    zf->close();
    delete zf;

    WFileResource *fileResource = new Wt::WFileResource("plain/text",mFullPath+"_raster.zip");
    fileResource->suggestFileName(mClientName+"_raster.zip");
    m_app->redirect(fileResource->url());
}


bool parcellaire::cropImWithShp(std::string inputRaster, std::string aOut){
    bool aRes(0);
    std::cout << " cropImWithShp" << std::endl;
    if (exists(inputRaster)){

        // enveloppe de la géométrie globale
        OGREnvelope ext;
        poGeomGlobale->getEnvelope(&ext);
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

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
        //adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;
        //determine dimensions of the new (cropped) raster in cells
        int xSize = round(width/transform[1]);
        int ySize = round(height/transform[1]);
        //std::cout << "xSize " << xSize << ", ySize " << ySize << std::endl;
        //create the new (cropped) dataset
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
        if( pCroppedRaster != NULL ){GDALClose( (GDALDatasetH) pCroppedRaster );}
        GDALClose(pInputRaster);
        aRes=1;
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
    return aRes;
}

