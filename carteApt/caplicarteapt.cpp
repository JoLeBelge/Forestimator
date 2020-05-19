#include "caplicarteapt.h"
int step(2500);

cApliCarteApt::cApliCarteApt(cDicoApt *aDico):dico(aDico)
{
    std::cout << "constructeur de cApliCarteApt " << std::endl;
    // carte d'aptitude pour FEE - test zone
    std::string aNHpath(dico->Files()->at("NH"));
    std::string aNTpath(dico->Files()->at("NT"));
    std::string aZBIOpath(dico->Files()->at("ZBIO"));
    std::string aTopopath(dico->Files()->at("Topo"));

    GDALAllRegister();

    poDatNH = (GDALDataset *) GDALOpen( aNHpath.c_str(), GA_ReadOnly );
    if( poDatNH == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aNHpath << std::endl;
    }
    //GDALRasterBand * NHBand;
    NHBand = poDatNH->GetRasterBand( 1 );

    poDatNT = (GDALDataset *) GDALOpen( aNTpath.c_str(), GA_ReadOnly );
    if( poDatNT == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aNTpath << std::endl;
    }
    //GDALRasterBand * NTBand;
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

    std::cout << "type nh " <<GDALGetDataTypeName(NHBand->GetRasterDataType()) << " x " << NHBand->GetXSize() <<" y " << NHBand->GetYSize() << std::endl;
    std::cout << "type nt " <<GDALGetDataTypeName(NTBand->GetRasterDataType()) << " x " << NTBand->GetXSize() <<" y " << NTBand->GetYSize() << std::endl;
    std::cout << "type zbio " <<GDALGetDataTypeName(ZBIOBand->GetRasterDataType()) << " x " << ZBIOBand->GetXSize() <<" y " << ZBIOBand->GetYSize() << std::endl;

    x=(NHBand->GetXSize());
    y=(NHBand->GetYSize());

    // même chose pour les CS
    poDatCS1 = (GDALDataset *) GDALOpen(dico->Files()->at("CS1").c_str(), GA_ReadOnly );
    if( poDatCS1 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS1") << std::endl;
    }
    poDatCS2 = (GDALDataset *) GDALOpen(dico->Files()->at("CS2").c_str(), GA_ReadOnly );
    if( poDatCS2 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS2") << std::endl;
    }
    poDatCS3 = (GDALDataset *) GDALOpen(dico->Files()->at("CS3").c_str(), GA_ReadOnly );
    if( poDatCS3 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS3") << std::endl;
    }
    poDatCS10 = (GDALDataset *) GDALOpen(dico->Files()->at("CS10").c_str(), GA_ReadOnly );
    if( poDatCS10 == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->Files()->at("CS10") << std::endl;
    }

    CS1Band = poDatCS1->GetRasterBand( 1 );
    CS2Band = poDatCS2->GetRasterBand( 1 );
    CS3Band = poDatCS3->GetRasterBand( 1 );
    CS10Band = poDatCS10->GetRasterBand( 1 );
}


cApliCarteApt::~cApliCarteApt()
{
    std::cout << "destructeur de c appli Carte Apt " << std::endl;
    if( poDatCS1 != NULL ){GDALClose( poDatCS1 );}
    if( poDatCS2 != NULL ){GDALClose( poDatCS2 );}
    if( poDatCS3 != NULL ){GDALClose( poDatCS3 );}
    if( poDatCS10 != NULL ){GDALClose( poDatCS10 );}
    if( poDatTopo != NULL ){GDALClose( poDatTopo );}
    if( poDatNH != NULL ){GDALClose( poDatNH);}
    if( poDatNT != NULL ){GDALClose(poDatNT);}
    if( poDatZBIO != NULL ){GDALClose( poDatZBIO);}
}


void cApliCarteApt::carteAptFEE(cEss * aEss, std::string aOut, bool force)
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
        if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATE, FALSE ) )
            printf( "Driver %s supports Create() method.\n", pszFormat );
        if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATECOPY, FALSE ) )
            printf( "Driver %s supports CreateCopy() method.\n", pszFormat );

        //std::string destFile("/home/lisein/Documents/carteApt/tutoGDAL/output/test.tif");

        // 2 méthodes de création; createCopy() et creaty(). create plus complexe. ne fonctionne pas avec tout les drivers

        //char **Options = NULL;
        //char * destFile="/home/lisein/Documents/carteApt/tutoGDAL/output/test.tif";
        const char * destFile=aOut.c_str();
        //GDALDataset* poDstDS = poDriver->Create(destFile, x,y, 1, GDT_Byte, Options);
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatNH, FALSE, papszOptions,NULL, NULL );
        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;

        float *scanlineNH;
        scanlineNH = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineNT;
        scanlineNT = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO;
        scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineTopo;
        scanlineTopo = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );

        //poBandR->RasterIO( GF_Write, 0, row, nXSize, 1, scanline, nXSize, 1,GDT_Byte, 0, 0 );

        // boucle sur les pixels
        for ( int row = 0; row < y; row++ )
        {
            //std::cout << "start row " << row << std::endl;
            // je ne sais pas pourquoi mais c'est obligé de lire les valeur en float 32 et de les écrires avec le flag FLOAT32 alors que moi je ne manipule que des 8bit

            NTBand->RasterIO( GF_Read, 0, row, x, 1, scanlineNT, x,1, GDT_Float32, 0, 0 );
            NHBand->RasterIO( GF_Read, 0, row, x, 1, scanlineNH, x,1, GDT_Float32, 0, 0 );
            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            TopoBand->RasterIO( GF_Read, 0, row, x, 1, scanlineTopo, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
            for (int col = 0; col < x; col++)
            {
                //std::cout << "start col " << col << std::endl;
                int apt(0);
                int zbio = scanlineZBIO[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la RW
                if (zbio!=0){
                    int nh = scanlineNH[ col ];
                    int nt = scanlineNT[ col ];
                    apt = aEss->getApt(nt,nh,zbio);

                    // prise en compte des risques liés à la situation topographique
                    int topo = scanlineTopo[ col ];
                    apt = aEss->corrigAptRisqueTopo(apt,topo,zbio);
                    // fonction menbre de l'essence qui Corrige l'apt sur base de la situtation topographique et des risques
                    //if (scanline[ col ]>20) std::cout << " one value of aptitude more than 20 -- should not exist" << std::endl;
                }
                if ((col%step==0) && row%step==0){
                    std::cout << ".." ;
                    /* if (aMEss.at("EP").getApt(nt,nh,zbio)!=0){
                    std::cout << " row " << row << " col " << col << "  ";
                    std::cout << " nh =" << nh << " soit " << dico.NH(nh) << " nt " <<nt << " soit " << dico.NT(nt) << " zbio " << zbio << " soit " << dico.ZBIO(zbio) ;
                    std::cout << " aptitude pour Epicea commun " << dico.code2Apt(aMEss.at("EP").getApt(nt,nh,zbio)) << std::endl;
                }
                */
                }
                scanline[ col ] = apt;
            }
            // écriture du résultat dans le fichier de destination
            //if (row%step==0){std::cout << "write row " << row << " in dest file" << std::endl;}
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );

            if (row%step==0){std::cout<< std::endl;}
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
        std::cout << aOut << " existe déjà " << std::endl;
    }

}

void cApliCarteApt::carteAptCS(cEss * aEss, std::string aOut, bool force)
{

    if (aEss->hasCSApt() && (!exists(aOut) | force)){
        std::cout << "création carte aptitude du CS pour essence " << aEss->Nom() << std::endl;
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
                    apt = aEss->getApt(zbio,st);
                }
                if ((col%step==0) && row%step==0){
                    std::cout << ".." ;
                    /* if (aMEss.at("EP").getApt(nt,nh,zbio)!=0){
                    std::cout << " row " << row << " col " << col << "  ";
                    std::cout << " nh =" << nh << " soit " << dico.NH(nh) << " nt " <<nt << " soit " << dico.NT(nt) << " zbio " << zbio << " soit " << dico.ZBIO(zbio) ;
                    std::cout << " aptitude pour Epicea commun " << dico.code2Apt(aMEss.at("EP").getApt(nt,nh,zbio)) << std::endl;
                }
                */
                }
                scanline[ col ] = apt;
            }
            // écriture du résultat dans le fichier de destination
            //if (row%step==0){std::cout << "write row " << row << " in dest file" << std::endl;}
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout<< std::endl;}
        }

        /* Once we're done, close properly the dataset */
        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleApt");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);

        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }

}

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
                    if (aKK->IsHabitat()) {apt = aKK->getHab(zbio,st);}else{
                        apt = aKK->getEchelle(zbio,st);
                    }

                }
                if ((col%step==0) && row%step==0){
                    std::cout << ".." ;
                    /* if (aMEss.at("EP").getApt(nt,nh,zbio)!=0){
                    std::cout << " row " << row << " col " << col << "  ";
                    std::cout << " nh =" << nh << " soit " << dico.NH(nh) << " nt " <<nt << " soit " << dico.NT(nt) << " zbio " << zbio << " soit " << dico.ZBIO(zbio) ;
                    std::cout << " aptitude pour Epicea commun " << dico.code2Apt(aMEss.at("EP").getApt(nt,nh,zbio)) << std::endl;
                }
                */
                }
                scanline[ col ] = apt;
            }
            // écriture du résultat dans le fichier de destination
            //if (row%step==0){std::cout << "write row " << row << " in dest file" << std::endl;}
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout<< std::endl;}
        }

        /* Once we're done, close properly the dataset */
        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}

        // copie du fichier de style qgis
        std::string aStyleFile=dico->Files()->at("styleKK");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);

        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }

}

// attention, widht et height en mètres!!!
void cApliCarteApt::cropIm(std::string input, std::string output, double topLeftX, double topLeftY,
                           double width, double height)
{
    const char *inputPath=input.c_str();
    const char *cropPath=output.c_str();
    GDALDataset *pInputRaster, *pCroppedRaster;
    GDALDriver *pDriver;
    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);

    double transform[6], tr1[6];
    pInputRaster->GetGeoTransform(transform);
    pInputRaster->GetGeoTransform(tr1);
    //adjust top left coordinates
    transform[0] = topLeftX;
    transform[3] = topLeftY;
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
    if( pCroppedRaster != NULL ){GDALClose( (GDALDatasetH) pCroppedRaster );}
    GDALClose( pInputRaster );
}

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

    //GDALDatasetH hDstDS = nullptr;
    //hDS = GDALOpenEx(,GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    //OGR_G_Simplify(poLayer->GetFeature(1),20);
    /* std::string aOut2=output+"2.shp";
    const char *outPath2=aOut2.c_str();
    GDALDataset *poDS2;
    //poDS2=GDALVectorTranslate(outPath2,NULL,1,tmp,NULL,NULL);
    */
    delete spatialReference;
    GDALClose( poDS );
    GDALClose(pInputRaster);
}


void cApliCarteApt::shptoGeoJSON(std::string input, std::string output)
{
    std::cout << " shptoGeoJSON " << std::endl;
    const char *inputPath=input.c_str();
    const char *outPath=output.c_str();
    //GDALDataset *pInputRaster;
    GDALDriver  *shpDriver,*jsonDriver;
    GDALAllRegister();

    //OGRSpatialReference  * spatialReference=new OGRSpatialReference;
    //spatialReference->importFromEPSG(31370);

    // datasource

    jsonDriver =GetGDALDriverManager()->GetDriverByName("GeoJSON");

    if( jsonDriver == NULL )
    {
        printf( "%s driver not available.\n", "GeoJSON" );
        exit( 1 );
    }

    GDALDataset * DS;

    //DS = GDALOpen( inputPath, GDAL_OF_VECTOR, NULL, NULL, NULL );
    DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( DS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }
    char **papszOptions = NULL;

    GDALDataset * DS2;
    DS2 = jsonDriver->CreateCopy(outPath, DS, FALSE, papszOptions,NULL, NULL );


    GDALClose( DS );
    GDALClose( DS2 );

}


void cApliCarteApt::toPNG(std::string input, std::string output,TypeCarte aType){
    //std::cout << " to png " << std::endl;
    const char *inputPath=input.c_str();
    const char *out=output.c_str();
    GDALDataset *pInputRaster, *pOutRaster;
    GDALDriver *pDriver, *pDriverPNG;

    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    const char *pszFormat2 = "PNG";
    pDriverPNG = GetGDALDriverManager()->GetDriverByName(pszFormat2);
    if( pDriverPNG == NULL )
    {
        printf( "%s driver not available.\n", pszFormat2 );
        exit( 1 );
    }
    pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);
    //pOutRaster = pDriverPNG->CreateCopy(out, pInputRaster, FALSE, NULL, NULL, NULL);
    // creation d'un raster temporaire avec driver tiff qui a la résolution voulue, càd 512x512. Car certaines tuiles ne font pas 512 et ça fout la merde.
    GDALDataset *pTmpRaster;
    std::string atmp="./tmp.tif";
    const char *tmpPath = atmp.c_str() ;
    pTmpRaster = pDriver->Create(tmpPath, 512, 512, 4, GDT_Byte,NULL);
    pTmpRaster->SetProjection( pInputRaster->GetProjectionRef() );
    double transform[6];
    pInputRaster->GetGeoTransform(transform);
    pTmpRaster->SetGeoTransform( transform );

    GDALRasterBand *inBand;
    inBand = pInputRaster->GetRasterBand(1);
    // gdal_retile n'as pas considéré les ND, bug corrigé en 2019 mais pas dans ma version hélas
    inBand->SetNoDataValue(0); // n'as pas l'air de fonctionner des masse.
    //inBand->DeleteNoDataValue();
    GDALRasterBand *outBand1,*outBand2,*outBand3;
    outBand1 = pTmpRaster->GetRasterBand(1);
    outBand2 = pTmpRaster->GetRasterBand(2);
    outBand3 = pTmpRaster->GetRasterBand(3);
    int aX=inBand->GetXSize();

    pTmpRaster->CreateMaskBand(4);

    GDALRasterBand *maskBand = pTmpRaster->GetRasterBand(4);

    //outBand->SetNoDataValue(0);
    //outBand->DeleteNoDataValue();
    float *scanline, * scanline2,*scanline3,*scanline4;
    scanline = (float *) CPLMalloc( sizeof( float ) *   aX);
    scanline2 = (float *) CPLMalloc( sizeof( float ) *   aX);
    scanline3 = (float *) CPLMalloc( sizeof( float ) *   aX);
    scanline4 = (float *) CPLMalloc( sizeof( float ) *   aX);

    // boucle sur les pixels
    for ( int row = 0; row < inBand->GetYSize(); row++ )
    {
        // il va retourner une erreur si il essaie de lire une ligne qui n'existe pas dans la tuile input. si erreur, tout plante.
        inBand->RasterIO( GF_Read, 0, row,  aX, 1, scanline,  aX,1, GDT_Float32, 0, 0 );
        // iterate on pixels in row
        for (int col = 0; col <  aX; col++)
        {
            // je met les nd à 0, c'est pas vraiment ce que je veux faire.
            int mask(255);
            int aRes1 = scanline[ col ];
            int aRes2 = scanline[ col ];
            int aRes3 = scanline[ col ];
            int aVal = scanline[ col ];
            switch(aType){

            case Apt:{
                aVal=dico->AptContraignante(aVal);
                // optimum
                if (aVal==1) {dico->getColor("O").set(aRes1,aRes2,aRes3);}
                // tolérance
                if (aVal==2) {dico->getColor("T").set(aRes1,aRes2,aRes3);}
                // tol élargie
                if (aVal==3) {dico->getColor("TE").set(aRes1,aRes2,aRes3);}
                // exclusion
                if (aVal==4) {dico->getColor("E").set(aRes1,aRes2,aRes3);}
                // zone batie
                if (aVal==12) {dico->getColor("grisc1").set(aRes1,aRes2,aRes3);}
                // indeterminé
                if (aVal==11) {dico->getColor("grisc1").set(aRes1,aRes2,aRes3);}

                //Apt2col permettrai de résumer tout ça en une ligne
                break;
            }

            case AE:{
                // pas d'apport
                if (aVal==1) {dico->colors.at("grisc0").set(aRes1,aRes2,aRes3);}
                // apport var
                if (aVal==2) {dico->colors.at("bleum2").set(aRes1,aRes2,aRes3);}
                // apport perm
                if (aVal==3) {dico->colors.at("bleuf").set(aRes1,aRes2,aRes3);}
                break;
            }
            case Topo:{
                // froid
                if (aVal==1) {dico->colors.at("bleuf").set(aRes1,aRes2,aRes3);}
                // plateau et faible pente
                if (aVal==2) {dico->colors.at("grisc0").set(aRes1,aRes2,aRes3);}
                // chaud
                if (aVal==3) {dico->colors.at("rouge").set(aRes1,aRes2,aRes3);}
                // fond de vallon
                if (aVal==4) {dico->colors.at("noir").set(aRes1,aRes2,aRes3);}
                break;
            }
            case SS:{
                // froid
                if (aVal==1) {dico->colors.at("bleuf").set(aRes1,aRes2,aRes3);}
                // neutre
                if (aVal==2) {dico->colors.at("grisc0").set(aRes1,aRes2,aRes3);}
                // chaud
                if (aVal==3) {dico->colors.at("rouge").set(aRes1,aRes2,aRes3);}
                break;
            }

            case NT:{

                if (aVal==7) {dico->colors.at("ntm3").set(aRes1,aRes2,aRes3);}
                if (aVal==8) {dico->colors.at("ntm2").set(aRes1,aRes2,aRes3);}
                if (aVal==9) {dico->colors.at("ntm1").set(aRes1,aRes2,aRes3);}
                if (aVal==10) {dico->colors.at("nt0").set(aRes1,aRes2,aRes3);}
                if (aVal==11) {dico->colors.at("nt1").set(aRes1,aRes2,aRes3);}
                if (aVal==12) {dico->colors.at("nt2").set(aRes1,aRes2,aRes3);}
                break;
            }

            case NH:{
                if (aVal==7) {dico->colors.at("nhm4").set(aRes1,aRes2,aRes3);}
                if (aVal==7) {dico->colors.at("nhm3").set(aRes1,aRes2,aRes3);}
                if (aVal==8) {dico->colors.at("nhm2").set(aRes1,aRes2,aRes3);}
                if (aVal==9) {dico->colors.at("nhm1").set(aRes1,aRes2,aRes3);}
                if (aVal==10) {dico->colors.at("nh0").set(aRes1,aRes2,aRes3);}
                if (aVal==11) {dico->colors.at("nh1").set(aRes1,aRes2,aRes3);}
                if (aVal==12) {dico->colors.at("nh2").set(aRes1,aRes2,aRes3);}
                if (aVal==13) {dico->colors.at("nh3").set(aRes1,aRes2,aRes3);}
                if (aVal==14) {dico->colors.at("nh4").set(aRes1,aRes2,aRes3);}
                if (aVal==15) {dico->colors.at("nh5").set(aRes1,aRes2,aRes3);}

                if (aVal==16) {dico->colors.at("grisc0").set(aRes1,aRes2,aRes3);}
                if (aVal==17) {dico->colors.at("grisc1").set(aRes1,aRes2,aRes3);}
                if (aVal==18) {dico->colors.at("noir").set(aRes1,aRes2,aRes3);}
                break;
            }

            case Potentiel:{
                // faible
                if (aVal==1) {dico->colors.at("faible").set(aRes1,aRes2,aRes3);}
                // moyen
                if (aVal==2) {dico->colors.at("moyen").set(aRes1,aRes2,aRes3);}
                // élevé
                if (aVal==3) {dico->colors.at("eleve").set(aRes1,aRes2,aRes3);}
                break;
            }

            case ZBIO:{
                if (aVal==3) {dico->colors.at("mauvec1").set(aRes1,aRes2,aRes3);}
                if (aVal==5) {dico->colors.at("mauvec2").set(aRes1,aRes2,aRes3);}

                if (aVal==10) {dico->colors.at("O").set(aRes1,aRes2,aRes3);}
                if (aVal==1) {dico->colors.at("vertc1").set(aRes1,aRes2,aRes3);}
                if (aVal==2) {dico->colors.at("vertc2").set(aRes1,aRes2,aRes3);}

                if (aVal==4) {dico->colors.at("bleum2").set(aRes1,aRes2,aRes3);}
                if (aVal==8) {dico->colors.at("nh5").set(aRes1,aRes2,aRes3);}
                if (aVal==6) {dico->colors.at("vertcaca").set(aRes1,aRes2,aRes3);}

                if (aVal==7) {dico->colors.at("rouge").set(aRes1,aRes2,aRes3);}

                if (aVal==9) {dico->colors.at("bleuf").set(aRes1,aRes2,aRes3);}

                break;
            }

            case Station1: case Habitats: case CSArdenne: case CSLorraine:{
                // j'ouvre le fichier de style qgis et je converti le code couleur en code RGB pour les stations ardennes
                if (aVal==1) {aRes1=4; aRes2=6;aRes3=2;}
                if (aVal==2) {aRes1=25; aRes2=30;aRes3=160;}
                if (aVal==3) {aRes1=116; aRes2=113;aRes3=115;}
                if (aVal==4) {aRes1=203; aRes2=207;aRes3=194;}
                if (aVal==5) {aRes1=195; aRes2=161;aRes3=39;}
                if (aVal==6) {aRes1=75; aRes2=145;aRes3=195;}
                if (aVal==7) {aRes1=37; aRes2=161;aRes3=214;}
                if (aVal==9) {aRes1=236; aRes2=36;aRes3=42;}
                if (aVal==10) {aRes1=233; aRes2=142;aRes3=226;}
                if (aVal==11) {aRes1=199; aRes2=84;aRes3=182;}
                if (aVal==12) {aRes1=123; aRes2=135;aRes3=144;}
                if (aVal==14) {aRes1=49; aRes2=158;aRes3=54;}
                if (aVal==15) {aRes1=213; aRes2=165;aRes3=255;}
                if (aVal==16) {aRes1=76; aRes2=188;aRes3=138;}
                if (aVal==17) {aRes1=138; aRes2=219;aRes3=250;}
                if (aVal==18) {aRes1=215; aRes2=222;aRes3=216;}
                break;
            }
                // fin select case Type rendu visuel
            }

            if (aVal==0) {mask=0;}
            scanline[ col ] = aRes1;
            scanline2[ col ] = aRes2;
            scanline3[ col ] = aRes3;
            scanline4[ col ] = mask;

        }
        outBand1->RasterIO( GF_Write, 0, row, aX, 1, scanline, aX, 1,GDT_Float32, 0, 0 );
        outBand2->RasterIO( GF_Write, 0, row, aX, 1, scanline2, aX, 1,GDT_Float32, 0, 0 );
        outBand3->RasterIO( GF_Write, 0, row, aX, 1, scanline3, aX, 1,GDT_Float32, 0, 0 );
        maskBand->RasterIO( GF_Write, 0, row, aX, 1, scanline4, aX, 1,GDT_Float32, 0, 0 );
    }


    // maintenant je crée une copy png du résultat
    pOutRaster = pDriverPNG->CreateCopy(out, pTmpRaster, FALSE, NULL, NULL, NULL);

    CPLFree(scanline);
    CPLFree(scanline2);
    CPLFree(scanline3);
    CPLFree(scanline4);
    GDALClose( pTmpRaster );
    GDALClose( pOutRaster );
    GDALClose( pInputRaster );
}

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

void cApliCarteApt::compressTif(std::string input){

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
}

