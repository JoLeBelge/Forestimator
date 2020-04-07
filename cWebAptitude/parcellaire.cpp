#include "parcellaire.h"

parcellaire::parcellaire(WContainerWidget *parent, groupLayers *aGL):mParent(parent),mGL(aGL),centerX(0.0),centerY(0.0),mName(""),mFullPath(""),mClientName(""),mJSfile(NULL)
{

    mDico=aGL->Dico();
    mJSfile= new std::string(aGL->Dico()->File("addOLgeojson"));
    std::cout <<"mJSfile   " << *mJSfile << std::endl;
    // boutton pour l'upload du shp
    fu = mParent->addNew<Wt::WFileUpload>();
    fu->setFileTextSize(500); // Set the maximum file size to 50 kB.
    fu->setFilters(".shp, .shx, .dbf, .qpj, .prj, .cpg");
    fu->setMultiple(true);
    //fu->setProgressBar(Wt::cpp14::make_unique<Wt::WProgressBar>());
    fu->setMargin(10, Wt::Side::Right);
    //fu->enableAjax();

    // Provide a button to start uploading.
    uploadButton = mParent->addNew<Wt::WPushButton>("charger le shp parcellaire");
    uploadButton->setMargin(10, Wt::Side::Left | Wt::Side::Right);
    uploadButton->disable();

    msg = mParent->addNew<Wt::WText>();

    // Upload when the button is clicked.
    uploadButton->clicked().connect(std::bind(&parcellaire::clickUploadBt,this));

    /* J'ai testé 3 type de liaison ; std:bind, lambda et connect sans rien. beaucoup de soucis. std::bind et lambda avec this semble "perdre" les membres de l'objet this si il ne s'agit pas de pointeur.
     * ainsi, un pointeur vers un string (mJSfile) semble être bien géré par bind et lambda. la valeur d'un string (mJSfile) est modifié dès la première liaison, à savoir ici fuchanged()
     * A priori c'est un malfonctionnement.
     * attention, j'ai encore des string comme membre ; mName, mClientName. Mais il sont redéfini au cours du temps.
     * attention, même les pointeurs semblent souffrir par moment ; l'accès à mDico est des fois compromis !! mais il n'est utilisé que pour instancier l'objet donc ok
     *
     * ça je pense que c'est la syntaxe qui fonctionne avec des slot et signaux : uploadButton->clicked().connect(this,&parcellaire::clickUploadBt);
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

    fu->changed().connect(std::bind(&parcellaire::fuChanged,this));

    // ok gros souci avec ces lambda fonction
    /*fu->changed().connect([this,mJSfile = mJSfile]() {
        //fu->upload();
        std::cout << "fu changed connect mJSfile " << mJSfile <<std::endl;
        std::cout << "fu changed connect this->mJSfile " << this->mJSfile <<std::endl;
        uploadButton->enable();
    });*/


    fu->uploaded().connect(std::bind(&parcellaire::upload,this));

    // React to a file upload problem.
    fu->fileTooLarge().connect([=] {
        msg->setText("Le fichier est trop volumineux.");
    });

    //

    Wt::WPushButton *computeStatButton = mParent->addNew<Wt::WPushButton>("Calcul");



    computeStatButton->clicked().connect(std::bind(&parcellaire::computeStat,this));


}

parcellaire::~parcellaire(){
    cleanShpFile();
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
     std::cout << "done " << std::endl;
}

bool parcellaire::toGeoJson(){
    bool aRes=true;

    //std::cout << " parcellaire::toGeoJson() ... " ;
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
    //std::cout << lay->GetName() << std::endl;
    OGREnvelope ext;
    OGRFeature * fet =lay->GetFeature(0);
    fet->GetGeometryRef()->getEnvelope(&ext);

    centerX= (ext.MaxX+ext.MinX)/2;
    centerY= (ext.MaxY+ext.MinY)/2;

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

void parcellaire::display(){
    //std::cout << " parcellaire::display " << std::endl;
    std::stringstream ss;
    std::string aFileIn(*mJSfile);
    //std::cout << "input js file " << *mJSfile << std::endl;
    std::ifstream in(aFileIn);
    std::string aTmp(aFileIn+".tmp");
    std::ofstream out(aTmp);
    // remplace l'url des tuiles par celui de l'essence actuelle:
    std::string aFind1("NAME");
    std::string line;

    while (getline(in, line))
    {
        boost::replace_all(line,aFind1,geoJsonRelName());
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

    //std::cout << " done " << std::endl;
}

void parcellaire::clickUploadBt(){
    //std::cout <<"clickUploadBt mJSfile " << mJSfile << std::endl;
    uploadButton->disable();
    fu->upload();
    uploadButton->enable();
}

void parcellaire::fuChanged(){
    std::cout <<"fuChanged() " << *(this->mJSfile) << std::endl;
    std::cout <<"fuChanged() " << mDico->File("addOLgeojson") << std::endl;
    uploadButton->enable();
}


void parcellaire::upload(){
    std::cout << "upload commence.. " ;
    //std::cout <<"mJSfile" << mJSfile << std::endl;
    cleanShpFile();
    boost::filesystem::path p(fu->clientFileName().toUTF8()), p2(fu->spoolFileName());
    mClientName = p.stem().c_str();
    mName = ((std::string) p2.filename().c_str()) + "-"+ mClientName;
    mFullPath = mDico->File("TMPDIR")+ mName;

    int nbFiles(0);
    for (Http::UploadedFile file : fu->uploadedFiles()){
        boost::filesystem::path a(file.clientFileName());
        boost::filesystem::rename(file.spoolFileName(),mFullPath+a.extension().c_str());
        //std::cout << "réception " << mFullPath+a.extension().c_str() << std::endl;
        if (a.extension().string()==".shp" | a.extension().string()==".shx" | a.extension().string()==".dbf") nbFiles++;
    }

    // je fais ça car de toute manière j'ai déjà renommé les fichier donc plus rien à supprimer
    fu->stealSpooledFile();
    // ici je converti en json et affichage dans ol
    if (nbFiles==3){
        msg->setText("Téléchargement du shp effectué avec succès.");
        if (toGeoJson()){display();}
    } else {
        msg->setText("Veillez sélectionner les 6 fichiers du shapefile.");
        cleanShpFile();
    }
}

// sur toute les couches de groupLayers?
void parcellaire::computeStat(){
    std::cout << " parcellaire::computeStat()... " << std::endl;
    std::string input(mFullPath+ ".shp");
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

    // statistique pour l'entièretée du parcellaire.
    std::map<std::string,std::map<std::string,int>> stat= mGL->computeStatGlob(lay);

    for (auto kv : stat){
        std::cout << "\n statistique pour la couche " << kv.first << std::endl;
        for (auto kv2 : kv.second){
            std::cout << kv2.first  << " --> " << kv2.second << "%" << std::endl;
        }
    }

    }
    std::cout << " ..done " << std::endl;
}
