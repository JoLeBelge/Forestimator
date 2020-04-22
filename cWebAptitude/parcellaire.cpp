#include "parcellaire.h"

//https://www.quora.com/What-are-the-risks-associated-with-the-use-of-lambda-functions-in-C-11

parcellaire::parcellaire(WContainerWidget *parent, groupLayers *aGL, Wt::WApplication* app, WStackedWidget *aTopStack, WContainerWidget *statW):mParent(parent),mStatW(statW),mGL(aGL),centerX(0.0),centerY(0.0),mClientName(""),mJSfile(""),mName(""),mFullPath(""),m_app(app),fu(NULL),msg(NULL),uploadButton(NULL),mTopStack(aTopStack),computeStatButton(NULL),visuStatButton(NULL)
{
    mDico=aGL->Dico();
    mJSfile=  aGL->Dico()->File("addOLgeojson");

    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Left);

    mParent->setMargin(20,Wt::Side::Bottom | Wt::Side::Top);
    mParent->setInline(0);

    mParent->addWidget(cpp14::make_unique<WText>("<h4>Charger votre parcellaire</h4>"));
    fu =mParent->addNew<Wt::WFileUpload>();

    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    fu->setFileTextSize(500); // Set the maximum file size to 50 kB.
    fu->setFilters(".shp, .shx, .dbf, .qpj, .prj, .cpg");
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

    computeStatButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Calcul"));
    computeStatButton->setInline(0);
    computeStatButton->disable();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());

    mGL->mPBar = mParent->addNew<Wt::WContainerWidget>()->addNew<Wt::WProgressBar>();
    mParent->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    mGL->mPBar->setRange(0, 50);
    mGL->mPBar->setValue(0);
    mGL->mPBar->setInline(0);

    visuStatButton = mParent->addWidget(cpp14::make_unique<Wt::WPushButton>("Visualiser les statistiques"));
    visuStatButton->setInline(0);
    visuStatButton->disable();
    //visuStatButton->setLink(Wt::WLink(Wt::LinkType::InternalPath, "/StatistiqueParcellaire"));

    fu->fileTooLarge().connect([=] { msg->setText("Le fichier est trop volumineux.");});
    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);
    uploadButton->clicked().connect(this ,&parcellaire::clickUploadBt);
    computeStatButton->clicked().connect(this,&parcellaire::computeStat);
    visuStatButton->clicked().connect(this,&parcellaire::visuStat);
}

parcellaire::~parcellaire(){
    //std::cout << "destructeur de parcellaire" << std::endl;
    cleanShpFile();
    //delete mParent;
    delete fu;
    delete uploadButton;
    //delete m_app;
    delete msg;
    //delete mGL;
    //delete mDico;
    delete poGeomGlobale;
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
            computeGlobalGeom(lay);
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

            GDALClose( DS );
            GDALClose( DS2 );

        }}

    //std::cout << " done " << std::endl;

    return aRes;
}

void parcellaire::computeGlobalGeom(OGRLayer * lay){

    // union de tout les polygones du shp
    OGRFeature *poFeature;
    //OGRPolygon * poGeom;
    OGRGeometry * poGeom;
    OGRGeometry * poGeom2;
    OGRMultiPolygon *multi = new OGRMultiPolygon();
    OGRErr err;
    OGRMultiPolygon *poGeomM;

    lay->ResetReading();
    while( (poFeature = lay->GetNextFeature()) != NULL )
    {

        switch (poFeature->GetGeometryRef()->getGeometryType()){
        case wkbPolygon:
        {
            poGeom=poFeature->GetGeometryRef();
            poGeom->closeRings();
            poGeom = poGeom->Buffer(0.0);
            //poGeom->Simplify(1.0);
            err = multi->addGeometry(poGeom);
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
            }
            break;
        }
        default:
            std::cout << "Geometrie " << poFeature->GetFID() << ", type de geometrie non pris en charge ; " << poFeature->GetGeometryRef()->getGeometryName() << std::endl;
            break;
        }
        if (err!=OGRERR_NONE){
            std::cout << "problem avec ajout de la geometrie " << poFeature->GetFID() << ", erreur : " << err <<  std::endl;
        }

    }

    poGeom2 = multi->UnionCascaded();
    poGeom2 =poGeom2->Buffer(1.0);// ça marche bien on dirait! je sais pas si c'est le buffer 1 ou le simplify 1 qui enlève les inner ring (hole) qui restent.
    poGeomGlobale =poGeom2->Simplify(1.0);

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

    delete multi;
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
    visuStatButton->disable();
    cleanShpFile();
    boost::filesystem::path p(fu->clientFileName().toUTF8()), p2(this->fu->spoolFileName());
    this->mClientName = p.stem().c_str();

    mName = ((std::string) p2.filename().c_str()) + "-"+ mClientName;
    mFullPath = this->mDico->File("TMPDIR")+ mName;

    int nbFiles(0);
    for (Http::UploadedFile file : fu->uploadedFiles()){
        boost::filesystem::path a(file.clientFileName());
        boost::filesystem::rename(file.spoolFileName(),mFullPath+a.extension().c_str());
        //std::cout << "réception " << mFullPath+a.extension().c_str() << std::endl;
        if ((a.extension().string()==".shp") | (a.extension().string()==".shx" )| (a.extension().string()==".dbf") | (a.extension().string()==".qpj") | (a.extension().string()==".prj")) nbFiles++;
    }

    // ici je converti en json et affichage dans ol
    if (nbFiles==5){
        msg->setText("Téléchargement du shp effectué avec succès.");
        if (toGeoJson()){
            computeStatButton->enable();
            display();}
    } else {
        msg->setText("Veillez sélectionner les 6 fichiers du shapefile.");
        cleanShpFile();
    }
}

void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " ;
    mGL->mPBar->setValue(0);
    std::map<std::string,std::map<std::string,int>> stat= mGL->computeStatGlob(poGeomGlobale);
    mGL->mPBar->setValue(0);
    visuStatButton->enable();
    std::cout << " ..done " << std::endl;
}


void parcellaire::visuStat(){
    std::cout << " parcellaire::visuStat()... " ;
    mGL->mPBar->setValue(0);
    mStatW->clear();
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
    retourButton->clicked().connect([this] {mTopStack->setCurrentIndex(0);});

    auto contCharts = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    WContainerWidget * contCharts_ = contCharts.get();
    Wt::WGridLayout * grid = contCharts_->setLayout(Wt::cpp14::make_unique<Wt::WGridLayout>());

    int nbChart=mGL->ptrVLStat().size();
    int nbColumn=std::min(2,int(std::sqrt(nbChart)));
    int row(0),column(0);
    for (layerStatChart * chart : mGL->ptrVLStat()) {
            //std::cout << " row " << row << " , col " << column << std::endl;
            grid->addWidget(std::unique_ptr<Wt::WContainerWidget>(chart->getChart()), row, column);
            column++;
            if (column>nbColumn){row++;column=0;}

            mGL->mPBar->setValue(mGL->mPBar->value()+(50.0/double(nbChart)));
            mGL->m_app->processEvents();
    }

    layout->addWidget(std::move(contTitre), 0);
    layout->addWidget(std::move(contCharts), 1);

    mTopStack->setCurrentIndex(1);
    mGL->mPBar->setValue(0);
    std::cout << " ..done " << std::endl;
}

std::string parcellaire::geoJsonName(){return mFullPath + ".geojson";}
std::string parcellaire::geoJsonRelName(){ return "tmp/" + mName +".geojson";}
