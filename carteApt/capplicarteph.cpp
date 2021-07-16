#include "capplicarteph.h"
// limite pH pour modelisation carte NT
double lim_m32(3.8);
double lim_m12(4.2);
double lim_p12(7.5);

cAppliCartepH::cAppliCartepH(bool bcarteNT, bool bcartepH)
{
    std::cout << "cApliCarteNT pH PTS " << std::endl;
    std::string adirBD("/home/lisein/Documents/carteApt/Forestimator/carteApt/data/carteFEE_NTpH.db");
    dico= std::make_unique<cDicoCartepH>(adirBD);

    std::string aZBIOpath(dico->File("ZBIO"));
    GDALAllRegister();

    poDatZBIO = (GDALDataset *) GDALOpen( aZBIOpath.c_str(), GA_ReadOnly );
    if( poDatZBIO == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << aZBIOpath << std::endl;
    }

    ZBIOBand = poDatZBIO->GetRasterBand( 1 );

    // si on veux recalculer la carte des pH
    if(bcartepH){
        std::string aTPSpath(dico->File("PTS"));
        //cartePTS(aTPSpath); // créé une seule fois

        poDatPTS = (GDALDataset *) GDALOpen( aTPSpath.c_str(), GA_ReadOnly );
        if( poDatPTS == NULL )
        {
            std::cout << "je n'ai pas lu le fichier " << aTPSpath << std::endl;
        }

        PTSBand = poDatPTS->GetRasterBand(1);

        x=ZBIOBand->GetXSize();
        y=ZBIOBand->GetYSize();

        std::string aOut(dico->File("pH"));
        cartepH(aOut);
    }

    if(bcarteNT){
    std::string aOut(dico->File("NT"));
    carteNT(aOut);
    }
}

cAppliCartepH::~cAppliCartepH()
{
    std::cout << "destructeur de c appli Carte pH " << std::endl;
    if( poDatPTS != NULL ){GDALClose(poDatPTS);}
    if( poDatZBIO != NULL ){GDALClose(poDatZBIO);}
}

void cAppliCartepH::cartepH(std::string aOut, bool force){

    if ((!exists(aOut) | force)){
        std::cout << "création carte pH, fichier " << aOut << std::endl;
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

        int step= y/100;

        float *scanlinePTS;
        scanlinePTS = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanlineZBIO;
        scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );
        // boucle sur les pixels
        int c(0);
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
                scanline[ col ] = pH;
            }
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout << c << " pct"<< std::endl;c++;}

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


void cAppliCartepH::cartePTS(std::string aOut, bool force){

    if ((!exists(aOut) | force)){
        std::cout << "creation de la carte des PTS sur base du dico sigle pédo --> classe pts" << std::endl;
        std::string aCNSWpath(dico->File("CNSW"));
        GDALDataset  * poDatCNSW = (GDALDataset *) GDALOpen( aCNSWpath.c_str(), GA_ReadOnly );
        if( poDatCNSW == NULL )
        {
            std::cout << "je n'ai pas lu le fichier " << aCNSWpath << std::endl;
        }

        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );

        const char * destFile=aOut.c_str();
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatCNSW, FALSE, papszOptions,NULL, NULL );
        GDALRasterBand *outBand, * inBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;
        inBand = poDatCNSW->GetRasterBand(1);

        x =inBand->GetXSize();
        y = inBand->GetYSize();
        int step= y/100;
        float *scanlineCNSW;
        scanlineCNSW = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );
        // boucle sur les pixels
        int c(0);
        for ( int row = 0; row < y; row++ )
        {
            // je ne sais pas pourquoi mais c'est obligé de lire les valeur en float 32 et de les écrires avec le flag FLOAT32 alors que moi je ne manipule que des 8bit

            inBand->RasterIO( GF_Read, 0, row, x, 1, scanlineCNSW, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
            for (int col = 0; col < x; col++)
            {


                int pts(0);
                int cnsw = scanlineCNSW[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la RW
                if (cnsw!=0){
                    //std::cout << " cnsw sigle pedo = " << cnsw << std::endl;
                    pts = dico->getPTS(cnsw);
                    //std::cout << " pts = = " << pts << std::endl;
                }
                scanline[ col ] = pts;
            }
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout << c << " pct"<< std::endl;c++;}
        }
        CPLFree(scanline);
        CPLFree(scanlineCNSW);

        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}
        if( poDatCNSW != NULL ){ GDALClose( (GDALDatasetH) poDatCNSW );}
        std::cout << " done " << std::endl;

    } else {
        std::cout << " la carte des PTS " << aOut << " existe, use force pour écraser " << std::endl;
    }
}


void cAppliCartepH::carteNT(std::string aOut, bool force){

    GDALDataset  * poDatCARBO = (GDALDataset *) GDALOpen( dico->File("CARBO").c_str(), GA_ReadOnly );
    if( poDatCARBO == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->File("CARBO") << std::endl;
    }

    GDALDataset  * poDatEVM = (GDALDataset *) GDALOpen( dico->File("EVM").c_str(), GA_ReadOnly );
    if( poDatEVM == NULL )
    {
        std::cout << "je n'ai pas lu le fichier " << dico->File("EVM") << std::endl;
    }

    if ((!exists(aOut) | force)){
        std::cout << "creation de la carte des NT sur base du dico sigle pédo, de la carte des pH, des territoire ecologique et des zones bioclimatiques" << std::endl;
        std::cout << "  fichier " << aOut << std::endl;
        // input ; zbio (déjà ouvert dans constructeur, ter eco, cnsw
        std::string aCNSWpath(dico->File("CNSW"));
        GDALDataset  * poDatCNSW = (GDALDataset *) GDALOpen( aCNSWpath.c_str(), GA_ReadOnly );
        if( poDatCNSW == NULL )
        {
            std::cout << "je n'ai pas lu le fichier " << aCNSWpath << std::endl;
        }

        std::string aTECOpath(dico->File("TECO"));
        GDALDataset  * poDatTECO = (GDALDataset *) GDALOpen( aTECOpath.c_str(), GA_ReadOnly );
        if( poDatTECO == NULL )
        {
            std::cout << "je n'ai pas lu le fichier " << aTECOpath << std::endl;
        }


        std::string apHpath(dico->File("pH"));
        GDALDataset  * poDatpH = (GDALDataset *) GDALOpen( apHpath.c_str(), GA_ReadOnly );
        if( poDatpH == NULL )
        {
            std::cout << "je n'ai pas lu le fichier " << apHpath << std::endl;
        }

        const char *pszFormat = "GTiff";
        GDALDriver *poDriver;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
            exit( 1 );

        const char * destFile=aOut.c_str();
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
        GDALDataset* poDstDS = poDriver->CreateCopy( destFile, poDatCNSW, FALSE, papszOptions,NULL, NULL );
        GDALRasterBand *outBand, * inBand;
        outBand = poDstDS->GetRasterBand(1);
        std::cout << "copy of raster done" << std::endl;
        inBand = poDatCNSW->GetRasterBand(1);

        x =inBand->GetXSize();
        y = inBand->GetYSize();

        int step= y/100;

        float *scanlineCNSW;
        scanlineCNSW = (float *) CPLMalloc( sizeof( float ) * x );
        float * scanlineTECO = (float *) CPLMalloc( sizeof( float ) * x );
        float * scanlineZBIO = (float *) CPLMalloc( sizeof( float ) * x );
        float * scanlinepH = (float *) CPLMalloc( sizeof( float ) * x );
         float * scanlineEVM = (float *) CPLMalloc( sizeof( float ) * x );
          float * scanlineCARBO = (float *) CPLMalloc( sizeof( float ) * x );
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * x );
        // boucle sur les pixels
        int c(0);
        for ( int row = 0; row < y; row++ )
        {
            // je ne sais pas pourquoi mais c'est obligé de lire les valeur en float 32 et de les écrires avec le flag FLOAT32 alors que moi je ne manipule que des 8bit
           inBand->RasterIO( GF_Read, 0, row, x, 1, scanlineCNSW, x,1, GDT_Float32, 0, 0 );
            poDatpH->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlinepH, x,1, GDT_Float32, 0, 0 );
            poDatZBIO->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlineZBIO, x,1, GDT_Float32, 0, 0 );
            poDatTECO->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlineTECO, x,1, GDT_Float32, 0, 0 );
            poDatEVM->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlineEVM, x,1, GDT_Float32, 0, 0 );
            poDatCARBO->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlineCARBO, x,1, GDT_Float32, 0, 0 );
            // iterate on pixels in row
            for (int col = 0; col < x; col++)
            {

                int NT(0);
                int cnsw = scanlineCNSW[ col ];
                int teco = scanlineTECO[ col ];
                int zbio = scanlineZBIO[ col ];
                int evm = scanlineEVM[ col ];
                int carbo = scanlineCARBO[ col ];
                double pH = scanlinepH[ col ];
                // un test pour tenter de gagner de la vitesse de temps de calcul - masque pour travailler que sur la RW
                if (cnsw!=0){
                    //dbo::ptr<siglePedo> s=dico->getSiglePedoPtr(cnsw);
                    const siglePedo *s=dico->getSiglePedoPtr(cnsw);
                    NT=cleNT(s,zbio,teco,pH,carbo,evm);
                }

                scanline[ col ] = NT;
            }
            // écriture du résultat dans le fichier de destination
            outBand->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            if (row%step==0){std::cout << c << " pct"<< std::endl;c++;}
        }
        CPLFree(scanline);
        CPLFree(scanlineCNSW);
        CPLFree(scanlineZBIO);
        CPLFree(scanlineTECO);
        CPLFree(scanlineEVM);
        CPLFree(scanlineCARBO);

        if( poDstDS != NULL ){ GDALClose( (GDALDatasetH) poDstDS );}
        if( poDatCNSW != NULL ){ GDALClose( (GDALDatasetH) poDatCNSW );}
        if( poDatZBIO != NULL ){ GDALClose( (GDALDatasetH) poDatZBIO );}
        if( poDatTECO != NULL ){ GDALClose( (GDALDatasetH) poDatTECO );}
        if( poDatEVM != NULL ){ GDALClose( (GDALDatasetH) poDatEVM );}
        if( poDatCARBO != NULL ){ GDALClose( (GDALDatasetH) poDatCARBO );}
        std::cout << " done " << std::endl;

    } else {
        std::cout << " la carte des NT " << aOut << " existe, use force pour écraser " << std::endl;
    }
}

int cleNT(const siglePedo *s, int ZBIO, int TECO, double pH, bool carbo, bool evm){
    int aRes(0);
    if(pH>=lim_p12){aRes=12;}
    if(pH<lim_p12 && s->calcaire() && (s->podzol() | s->podzolique())){aRes=9;}
    // 2021 07 12 Andyne me dis que les kEbb et autres sols en calestienne sont en -1 (cause du pH) mais on aimerai qu'ils soient en +1
    if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH<5 && TECO!=13){aRes=9;}
    if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH<5 && TECO==13){aRes=11;}
    //if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH<5 && !s->profond()){aRes=9;}
    //if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH<5 && s->profond()){aRes=9;}
    if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH>=5 && s->profond()){aRes=10;}
    if(pH<lim_p12 && s->calcaire() && (!s->podzol() & !s->podzolique()) && pH>=5 && !s->profond()){aRes=11;}
    if(pH<lim_p12 && s->calcaire() && s->superficiel() && (!s->podzol() & !s->podzolique())){aRes=12;}

    // calcaire non détecté

    if(pH<lim_p12 && !s->calcaire() &&  s->alluvion()){aRes=10;}
    if(pH<lim_p12 && !s->calcaire() &&  s->podzolique()){aRes=8;}
    if(pH<lim_p12 && !s->calcaire() &&  s->podzol()){aRes=7;}

    if(pH<lim_p12 && !s->calcaire() &&  !s->podzol() && !s->podzolique() && !s->alluvion() && pH>=5){aRes=10;}
    if(pH<lim_p12 && !s->calcaire() &&  !s->podzol() && !s->podzolique() && !s->alluvion() && pH<5 && pH>lim_m12){aRes=9;}
    if(pH<lim_p12 && !s->calcaire() &&  !s->podzol() && !s->podzolique() && !s->alluvion() && pH<=lim_m12 && pH>lim_m32){aRes=8;}
    if(pH<lim_p12 && !s->calcaire() &&  !s->podzol() && !s->podzolique() && !s->alluvion() && pH<=lim_m32){aRes=7;}

    if(pH<lim_m32){aRes=7;}

    // limon en ardenne condruzienne - Andyne me dis de mettre tout en -1 en 2021
    //if(s->limon() &&  TECO==14 && !s->calcaire() && !s->alluvion() && !s->podzol() ){aRes=8;}

    // limon en r?gion limoneuse
    if(s->limon() &&  (TECO==6 | TECO==7) && !s->calcaire() && !s->alluvion() && !s->podzol() ){aRes=9;}

    // 2021 07 12 ; andyne souhaite que la bande de sol gbbf en ardenne qui longe la famenne soit en -1 plutôt que -2. Sophie et Hugues confirme que c'est plus riche, malgré que le pH de ce pts soit à 4.15 pour l'Ardenne
     if( s->chargeSchisteux() &&  ( ZBIO==1 | ZBIO==2 | ZBIO==10) ){aRes=9;}
    // 2021 07 Andyne
     // limon calestienne sans calcaire détecté.
     if(s->limon() &&  TECO==13 && !s->calcaire() && !s->substratSchisteux()){aRes=10;}
     // nord du Condroz, Entre Vesdre et Meuse
     if(evm && s->getDEV_PROFIL()=="a" &&  s->getCHARGE()=="x"){aRes=8;}
     if(evm && s->getDEV_PROFIL()=="b" &&  s->getCHARGE()=="x"){aRes=10;}
     // reste du condroz avec et sans formation carbonaté ;
     if(ZBIO==8 && !evm && !carbo &&  s->getCHARGE()=="x"){aRes=9;}
     if(ZBIO==8 && !evm && carbo &&  s->getCHARGE()=="x"){aRes=10;}
     if(s->getCHARGE()=="r"){aRes=8;}

     // substrat schisteux : en -1, surtout pour la famenne
     if (s->substratSchisteux()){aRes=9;}

    // attention, si na sur carte pH, attribue mauvaise classe trophique.

    // en 2017 je mettais cette ligne de code en fin de clé, du coup certains sols en zone de tourbe qui n'avait pas de valeur de pH se voyais attribuer un nodata.
    // corrigé en 2020

    if(pH==0.0 | pH==1.0){aRes=0;}

    // ajout Claessens 10/02: tourbe en Ardenne
    if(s->tourbe() &&  ( ZBIO==1 | ZBIO==2 | ZBIO==10)){aRes=7;}

    // hors ardenne : indetermin?
    if(s->tourbe() &&  ( ZBIO!=1 && ZBIO!=2 && ZBIO!=10)){aRes=0;}

    if(s->ssriche()){aRes=10;}

    return aRes;
}
