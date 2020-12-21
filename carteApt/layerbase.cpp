#include "layerbase.h"


basicStat::basicStat(std::map<double,int> aMapValandFrequ):mean(0),max(0),min(0),nb(0){
    bool test(0);
    std::vector<double> v;
    for (auto kv : aMapValandFrequ){
        mean += kv.first*kv.second;
        nb+=kv.second;

        // pour pouvoir calculer l'écart type
        for (int i(0) ; i<kv.second;i++){v.push_back(kv.first);}

        if (kv.second>1){
            if (test) {

                if (kv.first>max) {max=kv.first;}
                if (kv.first<min) {min=kv.first;}

            } else {
                max=kv.first;
                min=kv.first;
                test=1;
            }
        }
    }
    mean=mean/nb;

    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    stdev = std::sqrt(sq_sum / nb - mean * mean);
}

basicStat::basicStat(std::vector<double> v):mean(0),max(0),min(0),nb(0){
    bool test(0);
    for (double val : v){
        mean += val;
        nb++;
        if (test) {
            if (val>max) {max=val;}
            if (val<min) {min=val;}
        } else {
            max=val;
            min=val;
            test=1;
        }
    }
    mean=mean/nb;
    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    stdev = std::sqrt(sq_sum / nb - mean * mean);
}

rasterFiles::rasterFiles(std::string aPathTif,std::string aCode):mPathRaster(aPathTif),mPathQml(""),mCode(aCode){
    //std::cout << "rasterFiles " << std::endl;
    checkForQml();
}
// détermine si il y a un fichier de symbologie associé
void rasterFiles::checkForQml(){
    std::string aPathQml = mPathRaster.substr(0,mPathRaster.size()-3)+"qml";
    if (exists(aPathQml)){
        mPathQml=aPathQml;
    }
}

rasterFiles rasterFiles::getRasterfile(){
        return rasterFiles(mPathRaster,mCode);
}

cRasterInfo::cRasterInfo(std::string aCode,cDicoApt * aDico):rasterFiles(aDico->File(aCode),aCode),mDico(aDico),mExpert(0){
    //std::cout << "cRasterInfo constructor " << std::endl;
    mNom=mDico->RasterNom(mCode);
    mExpert=mDico->RasterExpert(mCode);
    mTypeCarte =str2TypeCarte(mDico->RasterType(mCode));
    mTypeVar =str2TypeVar(mDico->RasterVar(mCode));
    mType =str2TypeLayer(mDico->RasterCategorie(mCode));
    mDicoVal=mDico->getDicoRaster(mCode);
    mDicoCol=mDico->getDicoRasterCol(mCode);
}

std::string cRasterInfo::NomFile(){
    boost::filesystem::path p(mPathRaster);
    return p.stem().c_str();}
std::string cRasterInfo::NomFileWithExt(){
    boost::filesystem::path p(mPathRaster);
    return p.filename().c_str();}


layerBase::layerBase(std::string aCode,cDicoApt * aDico):cRasterInfo(aCode,aDico)
{
 //std::cout << "layerbase constructor " << std::endl;

}

int layerBase::getValue(double x, double y){

    int aRes(0);
    if (mType!=TypeLayer::Externe){
        GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
        if( mGDALDat == NULL )
        {
            std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
        } else {
            GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

            double transform[6];
            mGDALDat->GetGeoTransform(transform);
            double xOrigin = transform[0];
            double yOrigin = transform[3];
            double pixelWidth = transform[1];
            double pixelHeight = -transform[5];

            int col = int((x - xOrigin) / pixelWidth);
            int row = int((yOrigin - y ) / pixelHeight);

            if (col<mBand->GetXSize() && row < mBand->GetYSize()){
                float *scanPix;
                scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
                // lecture du pixel
                mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                aRes=scanPix[0];
                CPLFree(scanPix);
                GDALClose( mGDALDat );
                mBand=NULL;
            }
        }
    }
    return aRes;
}

std::map<std::string,int> layerBase::computeStat1(OGRGeometry *poGeom){

    std::map<std::string,int> aRes;
    if (mType!=TypeLayer::Externe){
        // préparation du containeur du résultat
        for (auto &kv : mDicoVal){
            aRes.emplace(std::make_pair(kv.second,0));
        }
        // création d'une classe nd? quitte à la supprimer par après si pas d'occurence de nd
        aRes.emplace(std::make_pair("ND",0));
        int nbPix(0);

        // c'est mon masque au format raster
        GDALDataset * mask = rasterizeGeom(poGeom);

        if (mask!=NULL){
            OGREnvelope ext;
            poGeom->getEnvelope(&ext);
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
            // std::cout << " x " << width<< " y " << height << std::endl;

            GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
            if( mGDALDat == NULL )
            {
                std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
            } else {
                GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

                double transform[6];
                mGDALDat->GetGeoTransform(transform);
                double xOrigin = transform[0];
                double yOrigin = transform[3];
                double pixelWidth = transform[1];
                double pixelHeight = -transform[5];

                //determine dimensions of the tile
                int xSize = round(width/pixelWidth);
                int ySize = round(height/pixelHeight);
                int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
                int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);

                float *scanline, *scanlineMask;
                scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
                scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
                // boucle sur chaque ligne
                for ( int row = 0; row < ySize; row++ )
                {
                    // lecture
                    mBand->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                    // lecture du masque
                    mask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
                    // boucle sur scanline et garder les pixels qui sont dans le polygone
                    for (int col = 0; col <  xSize; col++)
                    {
                        if (scanlineMask[col]==255){
                            double aVal=scanline[ col ];
                            if (mDicoVal.find(aVal)!=mDicoVal.end()){
                                aRes.at(mDicoVal.at(aVal))++;
                                nbPix++;
                                // et les no data dans tout ça??? il faut les prendre en compte également! les ajouter dans le dictionnaire? dangereux aussi
                                //} else if (aVal==0) {nbPix++;}
                            } else {
                                aRes.at("ND")++;
                                nbPix++;}// ben oui sinon si les nodata n'ont pas la valeur 0 ça ne va pas (cas du masque forestier)
                        }
                    }
                }
                CPLFree(scanline);
                CPLFree(scanlineMask);
            }
            GDALClose(mask);
            GDALClose(mGDALDat);
        }
    }
    return aRes;
}


std::pair<int,double> layerBase::valMajoritaire(OGRGeometry * poGeom){
    std::pair<int,double> aRes;
    std::map<int,double> aStat=computeStat2(poGeom);
    // chercher le pct maximum
    double aMax(0);
    for (auto  kv : aStat){
        if (kv.second>aMax){
            aMax=kv.second;
            aRes=kv;}
    }
    return aRes;
}

std::map<int,double> layerBase::computeStat2(OGRGeometry * poGeom){
    std::map<int,double> aRes;
    if (mType!=TypeLayer::Externe){
        // préparation du containeur du résultat
        for (auto const & kv : mDicoVal){
            aRes.emplace(std::make_pair(kv.first,0.0));
        }
        int nbPix(0);

        GDALDataset * mask = rasterizeGeom(poGeom);

        if (mask!=NULL){
            OGREnvelope ext;
            poGeom->getEnvelope(&ext);
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

            GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
            if( mGDALDat == NULL )
            {
                std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
            } else {
                GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

                double transform[6];
                mGDALDat->GetGeoTransform(transform);
                double xOrigin = transform[0];
                double yOrigin = transform[3];
                double pixelWidth = transform[1];
                double pixelHeight = -transform[5];

                //determine dimensions of the tile
                int xSize = round(width/pixelWidth);
                int ySize = round(height/pixelHeight);
                int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
                int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);

                float *scanline, *scanlineMask;
                scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
                scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
                // boucle sur chaque ligne
                for ( int row = 0; row < ySize; row++ )
                {
                    // lecture
                    mBand->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                    // lecture du masque
                    mask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
                    // boucle sur scanline et garder les pixels qui sont dans le polygone
                    for (int col = 0; col <  xSize; col++)
                    {
                        if (scanlineMask[col]==255){
                            int aVal=scanline[ col ];
                            nbPix++;

                            if (aRes.find(aVal)!=aRes.end()){
                                aRes.at(aVal)++;
                            } else {
                              if (aRes.find(-1)==aRes.end()){   aRes.emplace(-1,0);}
                                aRes.at(-1)++;
                            }
                        }
                    }
                }
                CPLFree(scanline);
                CPLFree(scanlineMask);
            }
            GDALClose(mask);
            GDALClose(mGDALDat);
        }
        // calcul des pct
        if (nbPix!=0){
        for (auto  kv : aRes){
            aRes.at(kv.first)=100.0*aRes.at(kv.first)/((double) nbPix);
        }
        }
    }
    return aRes;
}

// création d'un raster masque pour un polygone. Valeur de 255 pour l'intérieur du polygone
GDALDataset * layerBase::rasterizeGeom(OGRGeometry *poGeom){

    std::string output(mDico->File("TMPDIR")+"tmp.tif");
    const char *out=output.c_str();
    GDALDriver *pDriver;
    GDALDataset *pRaster=NULL, * pShp;

    if (mType!=TypeLayer::Externe){
        // driver et dataset shp -- creation depuis la géométrie
        GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        pShp = pShpDriver->Create("/vsimem/blahblah.shp", 0, 0, 0, GDT_Unknown, NULL );
        // he bien c'est le comble, sur le serveur j'arrive à avoir le comportement adéquat si JE NE MET PAS de src. j'ai des warnings mais tout vas mieux!!
        OGRLayer * lay = pShp->CreateLayer("toto",nullptr,wkbPolygon,NULL);

        OGRFeature * feat = new OGRFeature(lay->GetLayerDefn());
        feat->SetGeometry(poGeom);

        lay->CreateFeature(feat);
        delete feat;
        const char *pszFormat = "MEM";
        // sauver le masque pour vérification
        //const char *pszFormat = "GTiff";

        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( pDriver == NULL )
        {
            printf( "%s driver not available.\n", pszFormat );
        } else {

            OGREnvelope ext;
            poGeom->getEnvelope(&ext);
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

            GDALDataset * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
            if( mGDALDat == NULL )
            {
                std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
            } else {
                double transform[6];
                mGDALDat->GetGeoTransform(transform);

                double pixelWidth = transform[1];
                double pixelHeight = -transform[5];
                //determine dimensions of the tile
                int xSize = round(width/pixelWidth);
                int ySize = round(height/pixelHeight);

                double tr2[6];
                tr2[0]=ext.MinX;
                tr2[3]=ext.MaxY;
                tr2[1]=transform[1];
                tr2[2]=transform[2];
                tr2[4]=transform[4];
                tr2[5]=transform[5];
                // création du raster en mémoire - on dois lui donner un out mais il ne l'utile pas car MEM driver

                pRaster = pDriver->Create(out, xSize, ySize, 1, GDT_Byte,NULL);
                pRaster->SetGeoTransform(tr2);

                // on en avait besoin que pour l'extent et resol
                pRaster->SetProjection(mGDALDat->GetProjectionRef());
                GDALClose(mGDALDat);
                GDALRasterize(NULL,pRaster,pShp,NULL,NULL);
            }
        }
        GDALClose(pShp);
    }
    return pRaster;
}

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {
        aRes=std::to_string(d+0.5).substr(0, std::to_string(d+0.5).find("."));}
      return aRes;
}

// pour les couches des variables continues
basicStat layerBase::computeBasicStatOnPolyg(OGRGeometry * poGeom){
    //std::cout << "compute BasicStat On Polyg" << std::endl;
    std::map<double,int> aMapValandFrequ;

    if (mTypeVar==TypeVar::Continu){
        // préparation du containeur du résultat
        for (auto &kv : mDicoVal){
            try {
                aMapValandFrequ.emplace(std::make_pair(std::stod(kv.second),0));
            }
            catch (const std::invalid_argument& ia) {
                std::cerr << "Invalid argument pour stod computeBasicStatOnPolyg: " << ia.what() << '\n';
            }
        }

        // c'est mon masque au format raster
        GDALDataset * mask = rasterizeGeom(poGeom);

        OGREnvelope ext;
        poGeom->getEnvelope(&ext);
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
        // std::cout << " x " << width<< " y " << height << std::endl;

        GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
        if( mGDALDat == NULL )
        {
            std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
        } else {
            GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

            double transform[6];
            mGDALDat->GetGeoTransform(transform);
            double xOrigin = transform[0];
            double yOrigin = transform[3];
            double pixelWidth = transform[1];
            double pixelHeight = -transform[5];

            //determine dimensions of the tile
            int xSize = round(width/pixelWidth);
            int ySize = round(height/pixelHeight);
            int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
            int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);

            float *scanline, *scanlineMask;
            scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
            scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
            // boucle sur chaque ligne
            for ( int row = 0; row < ySize; row++ )
            {
                // lecture
                mBand->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                // lecture du masque
                mask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
                // boucle sur scanline et garder les pixels qui sont dans le polygone
                for (int col = 0; col <  xSize; col++)
                {
                    // élégant mais trop lent!!
                    //OGRPoint op1(ext.MinX+col*pixelWidth,ext.MaxY-row*pixelWidth);
                    //if ( op1.Intersect(poGeom)/ within()){
                    if (scanlineMask[col]==255){
                        double aVal=scanline[ col ];
                        if (mDicoVal.find(aVal)!=mDicoVal.end()){

                            try {
                                aMapValandFrequ.at(std::stod(mDicoVal.at(aVal)))++;
                            }
                            catch (const std::invalid_argument& ia) {
                                std::cerr << "Invalid argument pour stod computeBasicStatOnPolyg, part2: " << ia.what() << '\n';
                            }
                        }
                    }
                }
            }
            CPLFree(scanline);
            CPLFree(scanlineMask);

            mBand=NULL;
        }
        GDALClose(mask);
        GDALClose(mGDALDat);
    }

    if (aMapValandFrequ.size()>0) {return basicStat(aMapValandFrequ);} else {return  basicStat();}

}
