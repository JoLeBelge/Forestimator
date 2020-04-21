#include "parcellaire.h"
std::string JSfile;
std::string mFullPath;
std::string mName;

//https://www.quora.com/What-are-the-risks-associated-with-the-use-of-lambda-functions-in-C-11

parcellaire::parcellaire(WContainerWidget *parent, groupLayers *aGL, Wt::WApplication* app):mParent(parent),mGL(aGL),centerX(0.0),centerY(0.0),mClientName(""),mJSfile(NULL),m_app(app),fu(NULL),msg(NULL),uploadButton(NULL)
{
    this->addStyleClass("table form-inline");
    //this->enableAjax();
    mDico=aGL->Dico();
    mJSfile= new std::string(aGL->Dico()->File("addOLgeojson"));
    JSfile=*mJSfile;

    mParent->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Left);
    // organiser les éléments dans un layout vbox, ou dans un tableau pour ne pas devoir faire un layout (j'aime pas la syntaxe)
    //addWidget(Wt::cpp14::make_unique<Wt::WBreak>());    // insert a line break
    mParent->setOffsets(20);
    mParent->setPadding(20);
    mParent->setInline(0);
    Wt::WTable * tab =  mParent->addWidget(cpp14::make_unique<Wt::WTable>());
    //tab->addCssRule("border-spacing: 5px;");
    int row(0);
    //std::cout <<"mJSfile   " << *mJSfile << std::endl;
    // boutton pour l'upload du shp
    //auto cont1 = mParent->addNew<Wt::WContainerWidget>();

    tab->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Charger votre parcellaire</h4>"));
    row++;
    //cont1->addWidget(cpp14::make_unique<WText>("<h4>Charger votre parcellaire</h4>"));
    //fu =cont1->addNew<Wt::WFileUpload>();
    fu =tab->elementAt(row,0)->addWidget(cpp14::make_unique<Wt::WFileUpload>());
    row++;
    fu->setFileTextSize(500); // Set the maximum file size to 50 kB.
    fu->setFilters(".shp, .shx, .dbf, .qpj, .prj, .cpg");
    fu->setMultiple(true);
    //fu->setProgressBar(Wt::cpp14::make_unique<Wt::WProgressBar>());
    //fu->setMargin(10, Wt::Side::Right);
    //fu->enableAjax();
    // Provide a button to start uploading.



    msg = tab->elementAt(row,0)->addWidget(cpp14::make_unique<Wt::WText>());
    row++;
    uploadButton = tab->elementAt(row,0)->addWidget(cpp14::make_unique<Wt::WPushButton>("Télécharger"));
    row++;
    //uploadButton->setMargin(10, Wt::Side::Left | Wt::Side::Right);
    uploadButton->disable();




    // Upload when the button is clicked.
    //uploadButton->clicked().connect(std::bind(&parcellaire::clickUploadBt,this));


    //fu->changed().connect(this,&parcellaire::fuChanged);
    //fu->changed().connect();

    //fu->uploaded().connect(std::bind(&parcellaire::upload,this));
    //uploadButton->clicked().connect([this] {clickUploadBt(); });
    // connect jslot ; plus prudent que connect tout court?
    //uploadButton->clicked().connect([this = move(this)] { clickUploadBt(); });
    //uploadButton->clicked().connectStateless()

    //fu->changed().connect([this] {fuChanged(); });
    //fu->uploaded().connect([this] {upload(); });

    //fu->uploaded().connect([this] {upload(); });


    // React to a file upload problem.
    fu->fileTooLarge().connect([=] {
        msg->setText("Le fichier est trop volumineux.");
    });


    //auto cont3 = mParent->addNew<Wt::WContainerWidget>();
    tab->elementAt(row,0)->addWidget(cpp14::make_unique<WText>("<h4>Calcul de statistique</h4>"));
    row++;

    Wt::WPushButton *computeStatButton = tab->elementAt(row,0)->addWidget(cpp14::make_unique<Wt::WPushButton>("Calcul"));
    //computeStatButton->clicked().connect(std::bind(&parcellaire::computeStat,this));

    mGL->mPBar = mParent->addNew<Wt::WContainerWidget>()->addNew<Wt::WProgressBar>();
    mGL->mPBar->setRange(0, 50);
    mGL->mPBar->setValue(0);

    fu->changed().connect([=] {fuChanged(); });
    fu->uploaded().connect([=] {upload();});
    uploadButton->clicked().connect([=] {clickUploadBt();});
    computeStatButton->clicked().connect([=] {computeStat();});

    /*
    fu->changed().connect(std::bind(&parcellaire::fuChanged,this));
    fu->uploaded().connect(std::bind(&parcellaire::upload,this));
    uploadButton->clicked().connect(std::bind(&parcellaire::clickUploadBt,this));
    computeStatButton->clicked().connect([this]{computeStat();});
    */

    // fonctionne pas!!!!! pourquoi!!
    /*
    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);
    uploadButton->clicked().connect(this ,&parcellaire::clickUploadBt);
    computeStatButton->clicked().connect(this,&parcellaire::computeStat);
    this->enable();


    fu->changed().connect(this,&parcellaire::fuChanged);
    fu->uploaded().connect(this,&parcellaire::upload);
    uploadButton->clicked().connect(this ,&parcellaire::clickUploadBt);
    computeStatButton->clicked().connect(this,&parcellaire::computeStat);
*/

    // ça fonctionne, mais ça bug avec le wt_config.xml perso que j'ai changé pour permettre l'activation des signaux ajax. peut-être un conflit ?
    // donc bien garder les params global (etc/wt/wt_config.xml)
    //fu->changed().connect([=]() {fuChanged(); });
    //fu->uploaded().connect([=]() {upload();});
    //uploadButton->clicked().connect([=]() {clickUploadBt();});
    //computeStatButton->clicked().connect([=]() {computeStat();});

    /*
    fu->changed().connect([this] {fuChanged(); });
    fu->uploaded().connect([this] {upload();});
    uploadButton->clicked().connect([this] {clickUploadBt();});
    computeStatButton->clicked().connect([this] {computeStat();});
    */

    // fonctionne pas
    /*
     fu->changed().connect(bindSafe([this] {fuChanged(); }));
     fu->uploaded().connect(bindSafe([this] {upload(); }));
     uploadButton->clicked().connect(bindSafe([this] {clickUploadBt();}));
    computeStatButton->clicked().connect(bindSafe([this]{computeStat();}));
    */

    /* fonctionne mais avec risque de dandling pointer
    fu->changed().connect(std::bind(&parcellaire::fuChanged,this));
    fu->uploaded().connect(std::bind(&parcellaire::upload,this));
     uploadButton->clicked().connect(std::bind(&parcellaire::clickUploadBt,this));
    computeStatButton->clicked().connect([this]{computeStat();});
    */

    //fu->changed().connect(boost::bind(&parcellaire::fuChanged,this));
    //fu->changed().connect("fuChanged");
    //fu->uploaded().connect("upload");
    //uploadButton->clicked().connect(bbind(&parcellaire::clickUploadBt,this));
    //computeStatButton->clicked().connect(boost::bind(&parcellaire::computeStat,this));



    /* J'ai testé 3 type de liaison ; std:bind, lambda et connect sans rien. beaucoup de soucis. std::bind et lambda avec this semble "perdre" les membres de l'objet this si il ne s'agit pas de pointeur.
     * ainsi, un pointeur vers un string (mJSfile) semble être bien géré par bind et lambda. la valeur d'un string (mJSfile) est modifié dès la première liaison, à savoir ici fuchanged()
     * A priori c'est un malfonctionnement.
     * attention, j'ai encore des string comme membre ; mName, mClientName. Mais il sont redéfini au cours du temps.
     * attention, même les pointeurs semblent souffrir par moment ; l'accès à mDico est des fois compromis !! mais il n'est utilisé que pour instancier l'objet donc ok
     *
     * lambda [=] passe tout par copie. La copie d'un pointeur, ça pointe toujour sur l'objet originel donc ça fonctionne.
     * mais pour les non-pointeur, ça fait une grosse différence.
     *
     *
     * DANGLING REF!!!
     *
     * https://www.quora.com/What-are-the-risks-associated-with-the-use-of-lambda-functions-in-C-11
     *
     *
     * ça ça marche avec phytospy mais pas avec webatp. et je ne sais pas pourquoi. : uploadButton->clicked().connect(this,&parcellaire::clickUploadBt);
    // https://openclassrooms.com/forum/sujet/pointeur-de-fonction-membre-ou-pas-membre-de-class
    //auto lambda = [this](){clickUploadBt();};
      //auto& mJSFile = mJSFile;
       //auto lambda = [this,&mJSfile = mJSfile](){clickUploadBt();};
      // auto lambda = [&](){clickUploadBt();};
    //auto lambda = [this,mJSfile = mJSfile]{std::cout << "mJSfile  uploadbutton click" << mJSfile <<std::endl;};
    //

    auto lambda = [this,mJSfile = mJSfile](){std::cout << "mJSfile  uploadbutton click " << mJSfile <<std::endl;
         std::cout << "mJSfile  uploadbutton click " << this->mJSfile <<std::endl;

        clickUploadBt();};
    uploadButton->clicked().connect(lambda);
    */


    // ok gros souci avec ces lambda fonction
    /*fu->changed().connect([this,mJSfile = mJSfile]() {
        //fu->upload();
        std::cout << "fu changed connect mJSfile " << mJSfile <<std::endl;
        std::cout << "fu changed connect this->mJSfile " << this->mJSfile <<std::endl;
        uploadButton->enable();
    });*/
}

parcellaire::~parcellaire(){
    std::cout << "destructeur de parcellaire" << std::endl;
    cleanShpFile();
    delete mJSfile;
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

    std::cout << "clean shp files ..." ;
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
    std::cout << "done " << std::endl;
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
    std::cout << " parcellaire::display " << std::endl;
   //std::cout << "input js file " << *mJSfile << std::endl;
    std::cout << "input js file " << JSfile << std::endl;
        boost::system::error_code ec;
        if (boost::filesystem::exists(JSfile,ec)){
        assert(!ec);
        std::cout << " ... " << std::endl;
        std::stringstream ss;
        std::string aFileIn(JSfile);

        std::ifstream in(aFileIn);
        std::string aTmp(aFileIn+".tmp");
        std::ofstream out(aTmp);
        // remplace l'url des tuiles par celui de l'essence actuelle:
        std::string aFind1("NAME");
        std::string line;
         std::cout << " toto " << std::endl;
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

    std::cout << " done " << std::endl;
}

void parcellaire::clickUploadBt(){
    std::cout <<"clickUploadBt mJSfile " << *(this->mJSfile) << std::endl;
    uploadButton->disable();
    fu->upload();
    uploadButton->enable();
}

void parcellaire::fuChanged(){
    // invalid read avec memcheck
    //std::cout <<"fuChanged() " << *(this->mJSfile) << std::endl;
    //std::cout <<"fuChanged() " << this->mDico->File("addOLgeojson") << std::endl;
    uploadButton->enable();
}


void parcellaire::upload(){
    std::cout << "upload commence.. " ;
    //std::cout <<"mJSfile" << mJSfile << std::endl;
    cleanShpFile();
    boost::filesystem::path p(fu->clientFileName().toUTF8()), p2(this->fu->spoolFileName());
    //this->mClientName = p.stem().c_str();

    mName = ((std::string) p2.filename().c_str()) + "-"+ p.stem().c_str();
    mFullPath = this->mDico->File("TMPDIR")+ mName;

    int nbFiles(0);
    for (Http::UploadedFile file : fu->uploadedFiles()){
        boost::filesystem::path a(file.clientFileName());
        boost::filesystem::rename(file.spoolFileName(),mFullPath+a.extension().c_str());
        //std::cout << "réception " << mFullPath+a.extension().c_str() << std::endl;
        if ((a.extension().string()==".shp") | (a.extension().string()==".shx" )| (a.extension().string()==".dbf")) nbFiles++;
    }

    // je fais ça car de toute manière j'ai déjà renommé les fichier donc plus rien à supprimer
    //fu->stealSpooledFile();
    // ici je converti en json et affichage dans ol
    if (nbFiles==3){
        msg->setText("Téléchargement du shp effectué avec succès.");
         //std::cout << "geoJsonName 1 " << geoJsonName() << std::endl;
         std::cout << " parcellaire::toGeoJson() ... " ;
         std::string input(mFullPath+ ".shp"), output(geoJsonName().c_str());
         const char *inputPath=input.c_str();
         const char *outPath=output.c_str();
         GDALDriver  *jsonDriver;
         GDALAllRegister();
         jsonDriver =GetGDALDriverManager()->GetDriverByName("GeoJSON");

         if( jsonDriver == NULL )
         {
             printf( "%s driver not available.\n", "GeoJSON" );


         } else {
             GDALDataset * DS;

             DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR, NULL, NULL, NULL );
             if( DS == NULL )
             {
                 printf( "Open failed.\n" );

             } else {
                 OGRLayer * lay = DS->GetLayer(0);
                 computeGlobalGeom(lay);
                 char **papszOptions = NULL;
                 GDALDataset * DS2;
                 DS2 = jsonDriver->CreateCopy(outPath, DS, FALSE, papszOptions,NULL, NULL );

                 GDALClose( DS );
                 GDALClose( DS2 );

             }




            //std::cout << "geoJsonName 2 " << geoJsonName() << std::endl;
            display();}
    } else {
        msg->setText("Veillez sélectionner les 6 fichiers du shapefile.");
        cleanShpFile();
    }
}

// sur toute les couches de groupLayers?
void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " << std::endl;
        mGL->mPBar->setValue(0);
    /*std::string input(this->mFullPath+ ".shp");
    const char *inputPath=input.c_str();



    GDALAllRegister();
    GDALDataset * DS;
    DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( DS == NULL )
    {
        std::cout << "Open " << inputPath << " failed.\n" ;
        //exit( 1 );
    } else {
        // layer
        OGRLayer * lay = DS->GetLayer(0);
        */

        std::map<std::string,std::map<std::string,int>> stat= mGL->computeStatGlob(poGeomGlobale);

        for (auto kv : stat){
            std::cout << "\n statistique pour la couche " << kv.first << std::endl;
            for (auto kv2 : kv.second){
                std::cout << kv2.first  << " --> " << kv2.second << "%" << std::endl;
            }
        }

    //}
    // fait planter l'appli!! connerie
    //mGL->mPBar->setValue(0);
    std::cout << " ..done " << std::endl;
}

std::string parcellaire::geoJsonName(){return mFullPath + ".geojson";}
std::string parcellaire::geoJsonRelName(){ return "tmp/" + mName +".geojson";}


