#include "caplicarteapt.h"

cApliCarteApt::cApliCarteApt(cDicoApt *aDico):dico(aDico),poDatNH(NULL),poDatZBIO(NULL),poDatTopo(NULL),poDatNT(NULL),poDatCS1(NULL)
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

    CS1Band = poDatCS1->GetRasterBand( 1 );

}

cApliCarteApt::~cApliCarteApt()
{
    std::cout << "destructeur de c appli Carte Apt " << std::endl;
    if( poDatCS1 != NULL ){GDALClose( poDatCS1 );}
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
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" ); // fonctionne pas trop car si je relance un gdal_translate compress==deflate sur le résultat, je descend de 20 à 4Mo
        papszOptions = CSLSetNameValue( papszOptions, "PREDICTOR", "2" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatNH, FALSE, papszOptions,NULL, NULL );
        OGRSpatialReference  * spatialReference=new OGRSpatialReference;
        spatialReference->importFromEPSG(31370);
        poDstDS->SetSpatialRef(spatialReference);
        year_month_day today = year_month_day{floor<days>(std::chrono::system_clock::now())};
        std::string d = "carte de recommandation générée le " + format("%F",today);
        poDstDS->SetMetadataItem("Essence Forestière",aEss->Nom().c_str());
        poDstDS->SetMetadataItem("carte de recommandation pour une essence","Guide des Stations");
        poDstDS->SetMetadataItem("Version",d.c_str());
        poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Toessens Simon, Cubelier Pauline et Claessens Hugues - Gembloux Agro-Bio Tech");
        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        outBand->SetNoDataValue(0);

        float *scanlineCS1 = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline = (float *) CPLMalloc( sizeof( float ) * x );

        int step= y/10;
        // boucle sur les pixels de la RW
        for ( int row = 0; row < y; row++ )
        {
            CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );

            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
#pragma omp parallel num_threads(6)
            {
#pragma omp for
            for (int col = 0; col < x; col++)
            {
                int apt(0), st(0);
                int zbio = scanlineZBIO[ col ];

                if (zbio==1 | zbio==2 |zbio==10 | zbio== 4 ){
                    st = scanlineCS1[ col ];
                    int US=dico->getStationIDFromStatNum(zbio,st);
                    //if (zbio== 4) { std::cout << "stat num pour " << st << " est " << US << std::endl;}
                    apt = aEss->getApt(zbio,US);

                    // version "aptitude CS sans le risque climatique, pour illu dans lettre OWSF
                    //apt = aEss->getApt(zbio,st,"",0);
                }
                scanline[ col ] = apt;
            }
            }
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout<< "-" << std::endl;}
        }

        // set rasterColorInterpretation - après écriture du raster sinon rale - non il rale de toute façon mais ça fonctionne.
        std::shared_ptr<layerBase> lb = dico->getLayerBase("EP_CS");

        outBand->SetColorInterpretation(GCI_PaletteIndex);

        GDALColorTable colors = GDALColorTable();
        GDALColorEntry nd;
        nd.c1=255;
        nd.c2=255;
        nd.c3=255;
        nd.c4=0;
        colors.SetColorEntry(0,&nd);

        // set color for each value
        for (auto kv : lb->getDicoCol()){
        //    int, std::shared_ptr<color>
        std::shared_ptr<color> col = kv.second;
        GDALColorEntry e;
        e.c1=col->mR;
        e.c2=col->mG;
        e.c3=col->mB;
        colors.SetColorEntry(kv.first,&e);
        }
        outBand->SetColorTable(&colors);

        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}
        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleAptCS");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }
}

void cApliCarteApt::carteDeriveCS(){

    std::cout << "création carte dérivées CS" << std::endl;
    // create a copy d'une des couches pour les couches résultats
    const char *pszFormat = "GTiff";
    GDALDriver *poDriver;
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( poDriver == NULL )
        exit( 1 );
    char **papszOptions = NULL;
    papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
    OGRSpatialReference  * spatialReference=new OGRSpatialReference;
    spatialReference->importFromEPSG(31370);
    float *scanlineCS1 = (float *) CPLMalloc( sizeof( float ) * x );
    float *scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
    float *scanline = (float *) CPLMalloc( sizeof( float ) * x );
    int step= y/10;
    year_month_day today = year_month_day{floor<days>(std::chrono::system_clock::now())};
    std::string d = "carte dérivée du catalogue de stations générée le " + format("%F",today);
    GDALRasterBand *outBand;

    /**** sensibilité climatique ****/
    std::cout << "sensibilite climatique" << std::endl;
    std::string aOut=dico->File("SCC");
    GDALDataset* poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Jonathan Lisein , Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );
        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                // valeur pour la sensibilité climatique
                val = dico->getKKCS(dico->ZBIO2CSid(zbio),st).SCC;
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    /**** tassement du sol ****/
    std::cout << "tassement du sol" << std::endl;
    aOut=dico->File("tass_sol");
    poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );
        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                val = dico->getKKCS(dico->ZBIO2CSid(zbio),st).tass_sol;
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    /**** production de bois ****/
    std::cout << "production de bois" << std::endl;
    aOut=dico->File("prod_b");
    poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );

        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                val = dico->getKKCS(dico->ZBIO2CSid(zbio),st).PB;
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    /**** Valeur conservatoire potentielle ****/
    std::cout << "Valeur conservatoire potentielle" << std::endl;
    aOut=dico->File("vcp");
    poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );
        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                val = dico->getKKCS(dico->ZBIO2CSid(zbio),st).VCP;
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    /**** Habitats N2K ****/
    std::cout << "habitats natura 2000" << std::endl;
    aOut=dico->File("N2000_maj");
    poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );

        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                std::string hab=dico->getKKCS(dico->ZBIO2CSid(zbio),st).N2000_maj;
                val = dico->rasterValHabitats(hab);
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    /**** Habitats WalEunis ****/
    std::cout << "habitats WalEunis" << std::endl;
    aOut=dico->File("WalEunis_maj");
    poDstDS = poDriver->CreateCopy( aOut.c_str(), poDatNH, FALSE, papszOptions,NULL, NULL );
    poDstDS->SetSpatialRef(spatialReference);
    poDstDS->SetMetadataItem("Version",d.c_str());
    poDstDS->SetMetadataItem("Crédit","Lisein Jonathan, Simon Toessens et Claessens Hugues Gembloux Agro-Bio Tech");
    outBand = poDstDS->GetRasterBand(1);
    outBand->SetNoDataValue(0);

    for ( int row = 0; row < y; row++ )
    {
        CS1Band->RasterIO( GF_Read, 0, row, x, 1, scanlineCS1, x,1, GDT_Float32, 0, 0 );

        ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
#pragma omp parallel num_threads(6)
        {
#pragma omp for
        for (int col = 0; col < x; col++)
        {
            int val(0), st(0);
            int zbio = scanlineZBIO[ col ];
            if (zbio==1 | zbio==2 |zbio==10 ){
                st = scanlineCS1[ col ];
                //if (zbio==3 | zbio==5) st = scanlineCS3[ col ];
                std::string hab=dico->getKKCS(dico->ZBIO2CSid(zbio),st).Wal_maj;
                val = dico->rasterValHabitats(hab);
            }
            scanline[ col ] = val;
        }
        }
        // écriture du résultat dans le fichier de destination
        outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
        if (row%step==0){std::cout<< "-" << std::endl;}
    }
    if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

    std::cout << " done " << std::endl;
}

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
