#include "caplicarteapt.h"

cApliCarteApt::cApliCarteApt(cDicoApt *aDico):dico(aDico),poDatNH(NULL),poDatZBIO(NULL),poDatTopo(NULL),poDatNT(NULL),poDatCS1(NULL),poDatCS3(NULL)
{
    std::cout << "constructeur de cApliCarteApt " << std::endl;
    // carte d'aptitude pour FEE
    std::string aNHpath(dico->Files()->at("NH"));
    std::string aNTpath(dico->Files()->at("NT"));
    std::string aZBIOpath(dico->Files()->at("ZBIO"));
    std::string aTopopath(dico->Files()->at("Topo"));

    std::cout << " utilise NH " << aNHpath << std::endl;
    std::cout << " utilise NT " << aNTpath << std::endl;
    std::cout << " utilise ZBIO " << aZBIOpath << std::endl;
    std::cout << " utilise topo " << aTopopath << std::endl;

    GDALAllRegister();

    poDatNH = (GDALDataset *) GDALOpen( aNHpath.c_str(), GA_ReadOnly );
    if( poDatNH == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aNHpath << std::endl;
    }
    NHBand = poDatNH->GetRasterBand( 1 );

    poDatNT = (GDALDataset *) GDALOpen( aNTpath.c_str(), GA_ReadOnly );
    if( poDatNT == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aNTpath << std::endl;
    }
    NTBand = poDatNT->GetRasterBand(1);

    poDatZBIO = (GDALDataset *) GDALOpen( aZBIOpath.c_str(), GA_ReadOnly );
    if( poDatZBIO == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aZBIOpath << std::endl;
    }
    ZBIOBand = poDatZBIO->GetRasterBand( 1 );

    poDatTopo = (GDALDataset *) GDALOpen( aTopopath.c_str(), GA_ReadOnly );
    if( poDatTopo == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aZBIOpath << std::endl;
    }
    TopoBand = poDatTopo->GetRasterBand( 1 );

    x=(NHBand->GetXSize());
    y=(NHBand->GetYSize());

    // même chose pour les CS
    poDatCS1 = (GDALDataset *) GDALOpen(dico->Files()->at("CS_A").c_str(), GA_ReadOnly );
    if( poDatCS1 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS_A") << std::endl;
    }
    poDatCS3 = (GDALDataset *) GDALOpen(dico->Files()->at("CS3").c_str(), GA_ReadOnly );
    if( poDatCS3 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS3") << std::endl;
    }
    CS1Band = poDatCS1->GetRasterBand( 1 );
    CS3Band = poDatCS3->GetRasterBand( 1 );
}

cApliCarteApt::~cApliCarteApt()
{
    std::cout << "destructeur de c appli Carte Apt " << std::endl;
    if( poDatCS1 != NULL ){GDALClose( poDatCS1 );}
    if( poDatCS3 != NULL ){GDALClose( poDatCS3 );}
    if( poDatTopo != NULL ){GDALClose( poDatTopo );}
    if( poDatNH != NULL ){GDALClose( poDatNH);}
    if( poDatNT != NULL ){GDALClose(poDatNT);}
    if( poDatZBIO != NULL ){GDALClose( poDatZBIO);}
}

void cApliCarteApt::carteAptFEE(std::shared_ptr<cEss> aEss, std::string aOut, bool force)
{
    if (aEss->hasFEEApt() && (!exists(aOut) | force)){
        std::cout << "création carte aptitude du FEE pour essence " << aEss->Nom() << std::endl;
        // create a copy d'une des couches pour que ce soit une carte d'aptitude
        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;
        char **papszMetadata;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );
        papszMetadata = poDriver->GetMetadata();

        const char * destFile=aOut.c_str();
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatNH, FALSE, papszOptions,NULL, NULL );
        OGRSpatialReference  * spatialReference=new OGRSpatialReference;
        spatialReference->importFromEPSG(31370);
        poDstDS->SetSpatialRef(spatialReference);
        year_month_day today = year_month_day{floor<days>(std::chrono::system_clock::now())};
        std::string d = "carte aptitude générée le " + format("%F",today);
        poDstDS->SetMetadataItem("Essence Forestière",aEss->Nom().c_str());
        poDstDS->SetMetadataItem("carte d'Aptitude","Fichier Ecologique des Essences");
        poDstDS->SetMetadataItem("Version",d.c_str());
        poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Gembloux Agro-Bio Tech");
        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;

        float *scanlineNH = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineNT = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineTopo= (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline = (float *) CPLMalloc( sizeof( float ) * x );

        int c(0);
        int step= y/10;
        // boucle sur les pixels
        for ( int row = 0; row < y; row++ )
        {
            // je ne sais pas pourquoi mais c'est obligé de lire les valeur en float 32 et de les écrires avec le flag FLOAT32 alors que moi je ne manipule que des 8bit
            NTBand->RasterIO( GF_Read, 0, row, x, 1, scanlineNT, x,1, GDT_Float32, 0, 0 );
            NHBand->RasterIO( GF_Read, 0, row, x, 1, scanlineNH, x,1, GDT_Float32, 0, 0 );
            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            TopoBand->RasterIO( GF_Read, 0, row, x, 1, scanlineTopo, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
#pragma omp parallel num_threads(6)
            {
#pragma omp for
            for (int col = 0; col < x; col++)
            {
                int apt(0);
                int zbio = scanlineZBIO[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la RW
                if (zbio!=0){
                    int nh = scanlineNH[ col ];
                    int nt = scanlineNT[ col ];
                    //apt = aEss->getApt(nt,nh,zbio);
                    // prise en compte des risques liés à la situation topographique
                    int topo = scanlineTopo[ col ];
                    //apt = aEss->corrigAptRisqueTopo(apt,topo,zbio); OLD OLD
                    //apt = aEss->getApt(nt,nh,zbio,1,topo);
                    apt =aEss->getFinalApt(nt,nh,zbio,topo);

                    // fonction menbre de l'essence qui Corrige l'apt sur base de la situtation topographique et des risques
                    //if (scanline[ col ]>20) std::cout << " one value of aptitude more than 20 -- should not exist" << std::endl;
                }
                scanline[ col ] = apt;
            }
            }
            if (row%step==0){std::cout << c*10 << " pct"<< std::endl;c++;}
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        }
        CPLFree(scanline);
        CPLFree(scanlineNT);
        CPLFree(scanlineNH);
        CPLFree(scanlineZBIO);
        CPLFree(scanlineTopo);
        /* Once we're done, close properly the dataset */
        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleApt");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà ou alors aEss->hasFEEApt() false " << std::endl;
    }

    // copie du fichier de style qgis
    std::string aStyleFile=dico->Files()->at("styleApt");
    boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
}

void cApliCarteApt::carteAptCS(std::shared_ptr<cEss> aEss, std::string aOut, bool force)
{
    if (aEss->hasCSApt() && (!exists(aOut) | force)){
        std::cout << "création carte de recommandation CS pour essence " << aEss->Nom() << std::endl;
        // create a copy d'une des couches pour que ce soit une carte d'aptitude
        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );

        const char * destFile=aOut.c_str();
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatNH, FALSE, papszOptions,NULL, NULL );
        OGRSpatialReference  * spatialReference=new OGRSpatialReference;
        spatialReference->importFromEPSG(31370);
        poDstDS->SetSpatialRef(spatialReference);
        year_month_day today = year_month_day{floor<days>(std::chrono::system_clock::now())};
        std::string d = "carte de recommandation générée le " + format("%F",today);
        poDstDS->SetMetadataItem("Essence Forestière",aEss->Nom().c_str());
        poDstDS->SetMetadataItem("carte de recommandation pour une essence","Catalogue de Station");
        poDstDS->SetMetadataItem("Version",d.c_str());
        poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        outBand->SetNoDataValue(0);

        float *scanlineCS1 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineCS3 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline = (float *) CPLMalloc( sizeof( float ) * x );

        int step= y/10;
        // boucle sur les pixels de la RW
        for ( int row = 0; row < y; row++ )
        {
            CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );
            CS3Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS3, x,1, GDT_Float32, 0, 0 );
            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
#pragma omp parallel num_threads(6)
            {
#pragma omp for
            for (int col = 0; col < x; col++)
            {
                int apt(0), st(0);
                int zbio = scanlineZBIO[ col ];

                if (zbio==1 | zbio==2 |zbio==10 ){
                    st = scanlineCS1[ col ];
                    //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                    apt = aEss->getApt(zbio,st);
                }
                scanline[ col ] = apt;
            }
            }
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout<< "-" << std::endl;}
        }

        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}
        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleAptCS");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }
}

/*
void cApliCarteApt::carteKKCS(cKKCS * aKK, std::string aOut, bool force)
{

    if (!exists(aOut) | force){
        std::cout << "création carte potentiel/risque sylvicole/environnement du CS pour KK " << aKK->Nom() << std::endl;
        // create a copy d'une des couches pour que ce soit une carte d'aptitude
        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );

        const char * destFile=aOut.c_str();

        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatNH, FALSE, papszOptions,NULL, NULL );

        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;

        float *scanlineCS1;
        scanlineCS1 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineCS2;
        scanlineCS2 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineCS3;
        scanlineCS3 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineCS10;
        scanlineCS10 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO;
        scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );

        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );

        int c(0);
         int step= y/100;
        // boucle sur les pixels de la RW
        for ( int row = 0; row < y; row++ )
        {
            CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );
            CS2Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS2, x,1, GDT_Float32, 0, 0 );
            CS3Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS3, x,1, GDT_Float32, 0, 0 );
            CS10Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS10, x,1, GDT_Float32, 0, 0 );
            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
            for (int col = 0; col < x; col++)
            {
                //std::cout << "start col " << col << std::endl;
                int apt(0), st(0);
                int zbio = scanlineZBIO[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la partie pour laquelle on a des CS
                if (zbio==1 | zbio==2 |zbio==3 |zbio==5|zbio==10 ){
                    if (zbio==1) st = scanlineCS1[ col ];
                    if (zbio==2) st = scanlineCS2[ col ];
                    if (zbio==3) st = scanlineCS3[ col ];
                    if (zbio==5) st = scanlineCS3[ col ];
                    if (zbio==10) st = scanlineCS10[ col ];

                        apt = aKK->getEchelle(zbio,st);


                }
                if ((col%step==0) && row%step==0){
                    std::cout << ".." ;
                    /* if (aMEss.at("EP").getApt(nt,nh,zbio)!=0){
                    std::cout << " row " << row << " col " << col << "  ";
                    std::cout << " nh =" << nh << " soit " << dico.NH(nh) << " nt " <<nt << " soit " << dico.NT(nt) << " zbio " << zbio << " soit " << dico.ZBIO(zbio) ;
                    std::cout << " aptitude pour Epicea commun " << dico.code2Apt(aMEss.at("EP").getApt(nt,nh,zbio)) << std::endl;
                }

                }
                scanline[ col ] = apt;
            }
            // écriture du résultat dans le fichier de destination
            //if (row%step==0){std::cout << "write row " << row << " in dest file" << std::endl;}
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout<< std::endl;}
        }


        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleKK");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);

        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }

}*/

void cApliCarteApt::toPol(std::string input, std::string output)
{
    std::cout << " to polygon " << std::endl;
    const char *inputPath=input.c_str();
    const char *outPath=output.c_str();
    GDALDataset *pInputRaster;
    GDALDriver *pDriver, *shpDriver;
    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);

    OGRSpatialReference  * spatialReference=new OGRSpatialReference;
    spatialReference->importFromEPSG(31370);

    // datasource
    shpDriver =GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if( shpDriver == NULL )
    {
        printf( "%s driver not available.\n", pszFormat );
        exit( 1 );
    }
    GDALDataset *poDS;
    poDS = shpDriver->Create(outPath, 0, 0, 0, GDT_Unknown, NULL );
    // layer
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("pol_out", spatialReference, wkbPolygon, NULL );
    // creation du champs
    OGRFieldDefn oField("Aptitude", OFTString );
    oField.SetWidth(32);
    poLayer->CreateField(&oField);

    GDALPolygonize(pInputRaster->GetRasterBand(1),nullptr,poLayer,0,nullptr,nullptr,nullptr);

    delete spatialReference;
    GDALClose( poDS );
    GDALClose(pInputRaster);
}

/*void cApliCarteApt::compressTif(std::string input){

    if (exists(input)){
        std::cout << "compression deflate pour  " << input << std::endl;
        std::string aCommand= std::string("gdal_translate -co 'COMPRESS=DEFLATE' ")
                +input+ "  "
                +input+".tif";
        std::cout << aCommand << "\n";
        system(aCommand.c_str());
        aCommand= "mv " +input+".tif " + input;
        system(aCommand.c_str());
    }else {
        std::cout << input << " n'existe pas. pas de compression donc. " << std::endl;
    }
}*/

void cApliCarteApt::codeMapServer(std::string inputData, std::string layerName, std::string layerFullName, std::string output, std::map<int, string> * DicoVal, std::map<int, color> DicoCol){
    std::cout << " code MapServer " << std::endl;

    std::string aCMS=std::string("LAYER\n")
            +std::string("  NAME \"")+ layerName +std::string("\"\n")+
            std::string("   TYPE RASTER\n")+
            std::string("   STATUS ON\n") +
            //std::string("   MAXSCALEDENOM 200000.00\n") +
            std::string("   PROJECTION\n") +
            std::string("      \"init=epsg:31370\"\n")+
            std::string("   END\n")+
            std::string("   DATA \"")+inputData+std::string("\" \n")+
            std::string("   PROCESSING \"BANDS=1\" \n")+
            std::string("   CLASSITEM \"[pixel]\" \n");

    for (auto kv : *DicoVal){

        if (DicoCol.find(kv.first)!=DicoCol.end()){
            color col = DicoCol.at(kv.first);
            aCMS+=MSClass(kv.second,std::to_string(kv.first),col);
        }

        // pour mnt, je dois mettre des expressions qui couvrent un range de valeur. attention, il faudra alors enlever les guillemets qui entoure l'expression!!
        /*if (DicoCol.find(kv.first)!=DicoCol.end()){
            color col = DicoCol.at(kv.first);
            std::string expression="([pixel] > "+std::to_string(kv.first)+" AND [pixel] <= "+std::to_string(kv.first+30)+")";
            aCMS+=MSClass(kv.second,expression,col);
        }*/
        // je modifie à la main les premières et dernières classes et j'enlève les guillemets. même pas besoin de faire restart apache, le fichier .map est lu à chaque requête


    }

    aCMS+=std::string(" METADATA\n")+
            // ici je peux mettre des accents dans le nom, et une balise description de la couche
            std::string("      \"wms_title\"           \"")+layerFullName+std::string("\"\n")+
            std::string("      \"wms_srs\"             \"EPSG:31370\"\n")+
            std::string("   END\n")+
            std::string("   TEMPLATE \"../template.html\"\n")+
            std::string("END\n");


    // on écrit le résultat dans le fichier de sortie

    std::ofstream outfile;

    outfile.open(output, std::ios_base::app); // append instead of overwrite
    outfile << aCMS;
    outfile.close();

}



std::string MSClass(std::string label, std::string expression, color col){

    std::string aRes=
            std::string("   CLASS \n")+
            std::string("      NAME \"")+removeAccents(label)+std::string("\"\n")+
            std::string("      EXPRESSION \"")+expression + std::string("\"\n")+
            std::string("      STYLE \n")+
            std::string("          COLOR ")+ col.cat2() + std::string("\n ")+
            std::string("      END\n")+
            std::string("   END\n");
    return aRes;
}



/*
void cApliCarteApt::tiletoPNG(std::string aDir,TypeCarte aType){

    std::vector<std::string> aVFiles;
    // en deux étape car si j'ajoute des fichiers durant la boucle recursive dir it, il peut planter (bug)
    for(auto & p : boost::filesystem::recursive_directory_iterator(aDir)){
        std::string ext= p.path().extension().string();
        if (ext==".tif"){
            //std::string fname=p.path().filename().string();
            // std::cout << fname << std::endl;
            //this->toPNG(p.path().string(),p.path().string()+".png");
            aVFiles.push_back(p.path().string());
        }
    }
    for (auto & imFile : aVFiles){
        this->toPNG(imFile,imFile+".png",aType);
    }
}

void cApliCarteApt::createTile(std::string input, std::string outDir, TypeCarte aType, bool force){

    if (exists(input)){
        std::cout << "création des tuiles pour  " << input << std::endl;
        int count(0);
        // préhalable; check si les png existent déjà, car si oui, pas nécessaire de refaire tourner le tuilage
        if(exists(outDir)){
            for(auto & p : boost::filesystem::recursive_directory_iterator(outDir)){
                std::string ext= p.path().extension().string();
                if (ext==".png"){
                    count++;
                }
            }
        }
        // si plus de 50 png, je juge que le job est déjà fait.
        if (count<50 | force){
            // 0 clean et create le directory output
            if(exists(outDir))
            {
                boost::filesystem::remove_all(outDir);
            }
            boost::filesystem::create_directory(outDir);

            // 1 gdal_retile
            std::string aCommand= std::string("gdal_retile.py -ot Byte -ps 512 512 -overlap 0 -levels 5 ")
                    + "-targetDir "+outDir+" "
                    + ""+input+ "";
            std::cout << aCommand << "\n";
            system(aCommand.c_str());
            // 2 conversion to png
            this->tiletoPNG(outDir,aType);
            // 3 effacer les tif
            std::vector<std::string> aVFiles;
            // en deux étape car si j'ajoute des fichiers durant la boucle recursive dir it, il peut planter (bug)
            for(auto & p : boost::filesystem::recursive_directory_iterator(outDir)){
                std::string ext= p.path().extension().string();
                if (ext==".tif"){
                    aVFiles.push_back(p.path().string());
                }
            }
            for (auto & imFile : aVFiles){
                boost::filesystem::remove(imFile);
            }
            std::cout << " done " << std::endl;
        } else {
            std::cout << "Il semblerai que les tuiles existent déjà, utiliser argument force pour refaire le tuilage. " << std::endl;
        }

    }else {
        std::cout << input << " n'existe pas. pas de tuilage. " << std::endl;
    }
}
*/
/*
void cApliCarteApt::cropImWithPol(std::string inputRaster, std::string inputVector, std::string aOut){
    std::cout << " cropImWithPol" << std::endl;
    if (exists(inputRaster) && exists(inputVector)){
        // lecture du shp
        GDALDriver *shpDriver;
        const char *pszFormat = "ESRI Shapefile";
        GDALDataset *shp;
        // datasource
        shpDriver =GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( shpDriver == NULL )
        {
            printf( "%s driver not available.\n", pszFormat );
            exit( 1 );
        }
        //std::cout << " driver ok" << std::endl;
        shp = (GDALDataset*) GDALOpenEx( inputVector.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
        // layer
        OGRLayer * lay = shp->GetLayer(0);
        //std::cout << lay->GetName() << std::endl;
        OGREnvelope ext;
        OGRFeature * fet =lay->GetFeature(0);
        fet->GetGeometryRef()->getEnvelope(&ext);

        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
        // std::cout << " x " << width<< " y " << height << std::endl;
        cropIm(inputRaster, aOut, ext.MinX,ext.MaxY, width, height);

    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << " ou " << inputVector << std::endl;
    }
}*/
