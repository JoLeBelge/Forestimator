#include "parcellaire.h"


int globSurfMax(2500);// en ha
//https://www.quora.com/What-are-the-risks-associated-with-the-use-of-lambda-functions-in-C-11

parcellaire::parcellaire(WContainerWidget *parent, groupLayers *aGL, Wt::WApplication* app, WContainerWidget *statW):mParent(parent),mGL(aGL),centerX(0.0),centerY(0.0),mClientName(""),mJSfile(""),mName(""),mFullPath(""),m_app(app),fu(NULL),msg(NULL),uploadButton(NULL)
  //,mTopStack(aTopStack)
  ,computeStatButton(NULL)
  ,visuStatButton(NULL)
  ,hasValidShp(0)
  ,downloadRasterBt(NULL)
  ,mStatW(statW)
{
    mDico=aGL->Dico();
    mJSfile=  aGL->Dico()->File("addOLgeojson");

    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);
    //mParent->setMargin(20,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);
    mParent->addWidget(cpp14::make_unique<WText>(tr("anaStep1")));
    //mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoParcellaire")));

    fu =mParent->addNew<Wt::WFileUpload>();
    fu->setFileTextSize(2000); // Set the maximum file size to 50 kB.
    fu->setFilters(".shp, .shx, .dbf, .prj");
    fu->setMultiple(true);
    fu->setInline(0);
    fu->addStyleClass("btn-file");
    //fu->setMargin(20,Wt::Side::Bottom | Wt::Side::Top); // si le parent a des marges et est inline(0) et que je met l'enfant à inline, l'enfant a des marges également

    msg = mParent->addWidget(cpp14::make_unique<Wt::WText>());
    msg->setInline(0);
    //mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    uploadButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Envoyer"));
    uploadButton->setStyleClass("btn btn-success");
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    uploadButton->disable();
    uploadButton->setInline(0);

    mParent->addWidget(cpp14::make_unique<WText>(tr("anaStep2")));
    //mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    //mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoCalculStat")));
    //mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mCB_fusionOT= mParent->addWidget(Wt::cpp14::make_unique<Wt::WCheckBox>(tr("cb_fusionAptOT")));
    mCB_fusionOT->setInline(0);
    mCB_fusionOT->setToolTip(tr("infoCalculStat"));
    //mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoChoixLayerStat")));
    //mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mContSelect4Stat= mParent->addWidget(cpp14::make_unique<Wt::WContainerWidget>());
    //auto * div_4stat = mParent->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Stat()));
    mContSelect4Stat->addStyleClass("div_4stat");

    mParent->addWidget(cpp14::make_unique<WText>(tr("anaStep3")));
    computeStatButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Calcul"));
    computeStatButton->setStyleClass("btn btn-success");
    computeStatButton->setInline(0);
    computeStatButton->disable();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    mGL->mPBar = mParent->addNew<Wt::WProgressBar>();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mGL->mPBar->setRange(0, mGL->getNumSelect4Stat());
    mGL->mPBar->setValue(0);
    mGL->mPBar->setInline(0);

    mParent->addWidget(cpp14::make_unique<WText>(tr("anaStep4")));
    downloadShpBt = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger le shp"));
    downloadShpBt->setStyleClass("btn btn-success");
    downloadShpBt->setWidth(150);
    downloadShpBt->setInline(0);
    downloadShpBt->disable();

    //mParent->addWidget(cpp14::make_unique<Wt::WText>(tr("infoDownloadClippedRaster")));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mContSelect4D= mParent->addWidget(cpp14::make_unique<Wt::WContainerWidget>());

    //mParent->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Download()));
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    downloadRasterBt = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger les cartes"));
    downloadRasterBt->setStyleClass("btn btn-success");
    downloadRasterBt->setWidth(200);
    downloadRasterBt->setInline(0);
    downloadRasterBt->disable();

    fu->fileTooLarge().connect([=] { msg->setText("Le fichier est trop volumineux (max 2000ko).");});
    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);
    uploadButton->clicked().connect(this ,&parcellaire::clickUploadBt);
    computeStatButton->clicked().connect(this,&parcellaire::computeStat);
    downloadShpBt->clicked().connect(this,&parcellaire::downloadShp);
    downloadRasterBt->clicked().connect(this,&parcellaire::downloadRaster);

    // rempli les 2 conteneurs qui présentent les selectLayers
    update();
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
        printf("aSurfha=%d",aSurfha);
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
        //std::string aTmp(aFileIn+".tmp");
        //std::ofstream out(aTmp);
        ss << in.rdbuf();
        in.close();
        std::string JScommand(ss.str());

        std::string aFind1("NAME");
        //std::string line;
        std::string aReplace(geoJsonRelName());
        /*while (getline(in, line))
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
        */
        boost::replace_all(JScommand,aFind1,aReplace);
        mParent->doJavaScript(JScommand);
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
            display();
            mGL->mMap->setToolTip(tr("tooltipMap2"));
        }
    } else {
        msg->setText("Veillez sélectionner les 3 fichiers du shapefile (shp, shx et dbf).");
        cleanShpFile();
    }
}

void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " ;

    mGL->mPBar->setMaximum(mGL->getNumSelect4Stat());
    mGL->mPBar->setValue(0);
    //std::map<std::string,std::map<std::string,int>> stat= mGL->computeStatGlob(poGeomGlobale);
    mGL->computeStatGlob(poGeomGlobale);
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

void parcellaire::visuStat(std::string aTitle){
    std::cout << " parcellaire::visuStat()... " ;
    mGL->mPBar->setValue(0);
    //std::cout << " CLEAR  mStaW... " ;
    mStatW->clear();
    //std::cout << " done... " ;
    mStatW->setOverflow(Wt::Overflow::Auto);
    // un set layout dans un widget qui a déjà un layout en parent, ça peut être bien (cas de page1 ; comportement voulu) ou pas (page2 aka statW ; pas bien)
    // je crée un containeur qui contiendra la page et ça regle mon problème
    Wt::WContainerWidget *cont = mStatW->addNew<Wt::WContainerWidget>();
    // premier lay pour mettre titre et boutton de "retour"
    Wt::WVBoxLayout * layout =  cont->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    //Wt::WVBoxLayout * layout = mStatW->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    auto contTitre = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * contTitre_ = contTitre.get();
    contTitre->addWidget(cpp14::make_unique<WText>(aTitle));
    //Wt::WPushButton * retourButton = contTitre_->addWidget(cpp14::make_unique<Wt::WPushButton>("Retour"));
    //contTitre_->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    contTitre->addWidget(cpp14::make_unique<WText>(tr("infoDansVisuStat")));

    auto contCharts = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * contCharts_ = contCharts.get();
    Wt::WGridLayout * grid = contCharts_->setLayout(Wt::cpp14::make_unique<Wt::WGridLayout>());

    int nbChart=mGL->ptrVLStat().size();
    //std::cout << " nb Chart "<< nbChart << std::endl;
    mGL->mPBar->setMaximum(nbChart);
    int nbColumn=std::min(2,int(std::sqrt(nbChart)));
    int row(0),column(0);
    // mauvaise manière de boucler sur un pointer!!!
    //for (layerStatChart * chart : mGL->ptrVLStat()) {
    for (layerStatChart * chart : mGL->ptrVLStat()) {
        std::cout << " row " << row << " , col " << column << std::endl;
        if (chart->deserveChart()){
            grid->addWidget(std::unique_ptr<Wt::WContainerWidget>(chart->getChart()), row, column);
            column++;
            if (column>nbColumn){row++;column=0;}
        }
        mGL->mPBar->setValue(mGL->mPBar->value()+1);
        mGL->m_app->processEvents();
    }
    layout->addWidget(std::move(contTitre), 0);
    layout->addWidget(std::move(contCharts), 0);
    //std::cout << " change tostack index... " ;
    //mTopStack->setCurrentIndex(1);
    mGL->mPBar->setValue(0);
    std::cout << " ..done " << std::endl;
    m_app->setInternalPath("/resultat",true);
}

// si je click sur un polygone dans ol, calcule les stat et affiche dans une nouvelle page
void parcellaire::computeStatAndVisuSelectedPol(int aId){
    std::cout << " computeStatAndVisuSelectedPol  , id " << aId << std::endl;
    std::string input(mFullPath+ ".shp");
    const char *inputPath=input.c_str();
    GDALAllRegister();
    GDALDataset * DS;

    DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
    if( DS == NULL )
    {
        printf( "Open failed.\n" );
    } else {
        mGL->mMap->decorationStyle().setCursor(Cursor::Wait);
        mGL->mMap->refresh();
        // layer
        OGRLayer * lay = DS->GetLayer(0);
        OGRFeature *poFeature;
        bool find(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            if (poFeature->GetFID()==aId){
                OGRGeometry * poGeom = poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom->flattenTo2D();
                mGL->computeStatGlob(poGeom);
                find=1;
                break;
            }
        }

        GDALClose( DS );
        if (find) {visuStat("<h4>Statistique pour polygone FID " + std::to_string(aId) + " de " + mClientName+ "</h4>");}
        mGL->mMap->decorationStyle().setCursor(Cursor::Auto);
    }

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
    if (exists(mFullPath+".prj"))  zf->addFile(mClientName+"_statForestimator.prj",mFullPath+".prj");

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

    // enveloppe de la géométrie globale
    OGREnvelope ext;
    poGeomGlobale->getEnvelope(&ext);
    aRes=cropIm(inputRaster, aOut, ext);
    return aRes;
}

void parcellaire::update(){
    mContSelect4Stat->clear();
    mContSelect4D->clear();
    mContSelect4Stat->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Stat()));
    mContSelect4D->addWidget(std::unique_ptr<Wt::WContainerWidget>(mGL->afficheSelect4Download()));
}



