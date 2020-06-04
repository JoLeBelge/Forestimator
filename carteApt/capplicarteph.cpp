#include "capplicarteph.h"
    int astep(2500);

cAppliCartepH::cAppliCartepH()
{
    std::cout << "constructeur de cApliCartepH " << std::endl;
    std::string adirBD("/home/lisein/Documents/Carto/pH/2020/cartepH.db");
    dico= new cDicoCartepH(adirBD);

    std::string aZBIOpath(dico->File("ZBIO"));
    std::string aTPSpath(dico->File("PTS"));

    GDALAllRegister();

    poDatZBIO = (GDALDataset *) GDALOpen( aZBIOpath.c_str(), GA_ReadOnly );
    if( poDatZBIO == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aZBIOpath << std::endl;
    }

    ZBIOBand = poDatZBIO->GetRasterBand( 1 );

    poDatPTS = (GDALDataset *) GDALOpen( aTPSpath.c_str(), GA_ReadOnly );
    if( poDatPTS == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aTPSpath << std::endl;
    }

    PTSBand = poDatPTS->GetRasterBand(1);

    x=ZBIOBand->GetXSize();
    y=ZBIOBand->GetYSize();

    std::string aOut(dico->File("OUTDIR")+"cartepH2020.tif");
    cartepH(aOut);

}

cAppliCartepH::~cAppliCartepH()
{
    std::cout << "destructeur de c appli Carte pH " << std::endl;
    if( poDatPTS != NULL ){GDALClose(poDatPTS);}
    if( poDatZBIO != NULL ){GDALClose( poDatZBIO);}
    delete dico;
}

void cAppliCartepH::cartepH(std::string aOut, bool force){


    if ((!exists(aOut) | force)){
        std::cout << "création carte pH" << std::endl;
        // create a copy d'une des couches pour que ce soit une carte d'aptitude
        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;

        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );

        const char * destFile=aOut.c_str();

        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        //GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatZBIO, FALSE, papszOptions,NULL, NULL );
        GDALDataset * poDstDS= poDriver->Create(destFile, x,y, 1, GDT_Float32 , papszOptions);
        poDstDS->SetProjection( poDatZBIO->GetProjectionRef() );
        double transform[6];
        poDatZBIO->GetGeoTransform(transform);
        poDstDS->SetGeoTransform( transform );

        GDALRasterBand *outBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;

        float *scanlinePTS;
        scanlinePTS = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO;
        scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );
        // boucle sur les pixels
        for ( int row = 0; row < y; row++ )
        {
            // je ne sais pas pourquoi mais c'est obligé de lire les valeur en float 32 et de les écrires avec le flag FLOAT32 alors que moi je ne manipule que des 8bit

            ZBIOBand->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            PTSBand->RasterIO( GF_Read, 0, row, x, 1, scanlinePTS, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
            for (int col = 0; col < x; col++)
            {

                double pH(0);
                int zbio = scanlineZBIO[ col ];
                int pts = scanlinePTS[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la RW
                if (zbio!=0){

                    pH = dico->getpH(pts,zbio);
                    //std::cout << "ph est de " << pH << std::endl;

                }
                if ((col%astep==0) && row%astep==0){
                    std::cout << ".." ;

                }
                scanline[ col ] = pH;
            }
            // écriture du résultat dans le fichier de destination

            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%astep==0){std::cout<< std::endl;}
        }
        CPLFree(scanline);
        CPLFree(scanlineZBIO);
        CPLFree(scanlinePTS);

        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}
        std::cout << " done " << std::endl;

    }else {
        std::cout << aOut << " existe déjà " << std::endl;
    }

}
