#include "layerbase.h"

int seuilClasseMinoritaire(2); // en dessous de ce seuil, les classes sont regroupées dans la catégorie "Autre"
extern bool globTest;
// hexagone ; apotheme et x qui sert pour les longeurs de d
//hexagone de 0.1ha de surface, soit 17 mètre d'apotheme car Aire=1/2*périmetre*apotheme et p=((a/sqrt(3))x2 x6coté, donc a = sqrt(sqrt(3)/6  * aire) et aire = 1000m2, voir https://fr.wikihow.com/calculer-l%27aire-d%27un-hexagone
double globa(17);
double globx=globa/std::sqrt(3);
// distance du centre à un sommet
double globd=2*globx;

basicStat::basicStat(std::map<double,int> aMapValandFrequ, double na):mean(0),max(0),min(0),nb(0),mValFreq(aMapValandFrequ){
    bool test(0);
    std::vector<double> v;
    for (auto kv : aMapValandFrequ){
        if (kv.first!=na){
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
    }
    mean=mean/nb;

    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    stdev = std::sqrt(sq_sum / nb - mean * mean);
}

statHdomBase::statHdomBase(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom, bool computeStat):mLay(aLay),mGeom(poGeom)
{
    if (computeStat){

        // approche rectangle de 1 are. Tellement plus simple,  et plus rapide

            // masque au format raster
            GDALDataset * mask = mLay->rasterizeGeom(mGeom);
            OGREnvelope ext;
            mGeom->getEnvelope(&ext);
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

            GDALDataset  * DS = (GDALDataset *) GDALOpen( mLay->getPathTif().c_str(), GA_ReadOnly );
            if( DS == NULL )
            {
                std::cout << "je n'ai pas lu l'image " <<  mLay->getPathTif() << std::endl;
            } else {

                double transform[6];
                DS->GetGeoTransform(transform);
                double pixelWidth = transform[1];
                double pixelHeight = -transform[5];
                double xOrigin = transform[0];
                double yOrigin = transform[3];
                int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
                int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);

                // découpe du masque en cellule rectangle de 10 mètre, soit un are
                int nbPix= 10/pixelWidth;

                float *scanline, *scanlineMask;
                scanline = (float *) CPLMalloc( sizeof( float ) * nbPix*nbPix );
                scanlineMask = (float *) CPLMalloc( sizeof( float ) * nbPix*nbPix );

                // boucle sur les cellules
                for (int i(0) ; i< ((int) (width/(10.0))); i++){
                    for (int j(0) ; j< ((int)(height/10.0)); j++){

                        std::vector<double> aVHs;

                        // lecture
                        int posU=i*nbPix;
                        int posV=j*nbPix;
                        //std::cout << "i " << i << " , j " << j << " ; row  pos U " << posU << " , posV " << posV << " (width/(10.0)) " << (width/(10.0)) <<  " , (height/10.0) " << (height/10.0)<< std::endl;

                        // lecture du bloc en 1 coup
                        DS->GetRasterBand(1)->RasterIO( GF_Read, posU+xOffset, posV+yOffset, nbPix, nbPix, scanline, nbPix,nbPix, GDT_Float32, 0, 0 );
                        // lecture du masque
                        mask->GetRasterBand(1)->RasterIO( GF_Read, posU , posV, nbPix, nbPix, scanlineMask, nbPix,nbPix, GDT_Float32, 0, 0 );

                        // boucle sur scanline et garder les pixels qui sont dans le polygone
                        for (int pix = 0; pix <  nbPix*nbPix; pix++)
                        {
                            if (scanlineMask[pix]==255){
                                double aVal=scanline[ pix ];
                                // H() c'est pour gain de 0.2
                                aVHs.push_back(Dico()->H(aVal,mLay->Gain()));
                            }
                        }
                        double surf=aVHs.size()*pixelWidth*pixelHeight;
                        // seuil de 80 m2
                        if (surf>80){
                                //std::unique_ptr<basicStat> cel=std::make_unique<basicStat>(&aVHs);
                                //mStat.push_back(std::move(cel));
                        }
                    }
                }

                CPLFree(scanline);
                CPLFree(scanlineMask);
                GDALClose(DS);
            }
            GDALClose(mask);

            // maintenant on fait le résumé de toutes "cellule" de 1 are

        }
}

bool statHdomBase::deserveChart(){
    bool aRes=mStat.size()>0;
    if (!aRes){ std::cout << "la couche " << mLay->getLegendLabel() << " ne mérite pas de graphique pour ses statistiques " << std::endl;}
    return aRes;
}


// cette fonctione ne fonctionne pas de manière identique sur le serveur qu'en local chez moi.
// chez moi ; 99 point. serveur ; 66 points, et manifestement pas au même endroit
std::vector<OGRPoint> hexbin(GDALDataset * mask){
    //std::cout << " hexbin sur le masque "<< std::endl;
    std::vector<OGRPoint> aRes;
    // nid d'abeille de 0.1 ha de surface.
    //double distEntreLignes=globd*sqrt(2);
    double distEntreLignes=globa;
    double distEntreColonnes=globd*2+globa;
    double transform[6];
    mask->GetGeoTransform(transform);
    double MaxY=transform[3];
    double MinX=transform[0];
    double MinY=transform[3]+transform[5]*mask->GetRasterBand(1)->GetYSize();
    double MaxX=transform[0]+transform[1]*mask->GetRasterBand(1)->GetXSize();

    //std::cout << " MaxY " <<MaxY << " MinX " <<MinX << " MinY " <<MinY << " MaxX " << MaxX << std::endl;
    //int c(0);
    double xOrigin = transform[0];
    double yOrigin = transform[3];
    double pixelWidth = transform[1];
    double pixelHeight = -transform[5];

    for (int i(0) ; i< ((MaxX-MinX)/(distEntreColonnes)); i++){
        for (int j(0) ; j< ((MaxY-MinY)/distEntreLignes); j++){
            double x=MinX+i*distEntreColonnes;
            // décallage horizontal une ligne sur 2. attention de ne pas sortir du raster masque alors.
            if ( j%2==0){x+=distEntreColonnes/2;}
            double y=MinY+j*distEntreLignes;

            // check que la valeur du mask est bien à 1 pour cette position
            int col = int((x - xOrigin) / pixelWidth);
            int row = int((yOrigin - y ) / pixelHeight);

            if (col<mask->GetRasterBand(1)->GetXSize() && row < mask->GetRasterBand(1)->GetYSize()){
                float *scanPix;
                scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
                // lecture du pixel
                mask->GetRasterBand(1)->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                int maskVal=scanPix[0];
                // c++;
                if (maskVal==255){

                    OGRPoint pt(x, y);
                    aRes.push_back(pt);
                    //std::cout << pt.getX() << "," << pt.getY() << std::endl;
                }
            }
        }
    }
    //std::cout << " c =" << c << ".." << std::endl;
    //std::cout << " hexbin generate " << aRes.size() << "points" << std::endl;
    return aRes;
}

std::vector<OGRPolygon *> hexGeombin(GDALDataset *mask){
    std::cout << " hexGeombin sur le masque "<< std::endl;
    std::vector<OGRPoint> VCentreHexagone=hexbin(mask);
    std::vector<OGRPolygon *> aRes(VCentreHexagone.size());
    int c(0);
    for (OGRPoint p : VCentreHexagone){
        //std::cout << " création d'un hexagone "<< std::endl;
        // création d'un hexagone
        OGRLinearRing * ring = new OGRLinearRing();
        OGRPolygon * hex= new OGRPolygon();
        // liste des 6 sommets ;
        // dimension de d= distance du centre de l'hex au sommet (pas la mm chose que apotheme) = 2x
        ring->addPoint(p.getX()+globx, p.getY()+globa);
        ring->addPoint(p.getX()+globd, p.getY());
        ring->addPoint(p.getX()+globx, p.getY()-globa);
        ring->addPoint(p.getX()-globx, p.getY()-globa);
        ring->addPoint(p.getX()-globd, p.getY());
        ring->addPoint(p.getX()-globx, p.getY()+globa);
        ring->closeRings();
        //std::cout << " ring done "<< std::endl;
        hex->addRingDirectly(ring);
        // delete ring;
        /* pour vérification dans Qgis
        char *toto;
        toto = hex.exportToJson();
        std::cout << toto << std::endl;
        */
        aRes[c]=hex;
        c++;
    }
    std::cout << "hexGeombin generate " << aRes.size() << " hexagones" << std::endl;
    return aRes;

}

cDicoApt * statHdomBase::Dico(){return mLay->Dico();}



// le probleme des map c'est qu'elles sont ordonnées automatiquement avec leur clé, je veux pas.
std::vector<std::pair<std::string,double>> statHdomBase::computeDistrH(){

    std::vector<std::pair<std::string,double>> aRes;
    std::vector<double> seuilClasses{0,3,9,15,21,27,33,39,45,51};
    // vecteur plus long que le nombre de seuil de classe de 1
    std::vector<int> clasOcc{0,0,0,0,0,0,0,0,0,0,0};
    std::vector<double> aVHauteur;

    // c'est mon masque au format raster
    GDALDataset * mask = mLay->rasterizeGeom(mGeom);

    OGREnvelope ext;
    mGeom->getEnvelope(&ext);
    double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( mLay->getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << mLay->getPathTif() << std::endl;
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
                    aVHauteur.push_back(Dico()->H(aVal,mLay->Gain()));
                }
            }
        }
        CPLFree(scanline);
        CPLFree(scanlineMask);

        mBand=NULL;
        GDALClose(mask);
        GDALClose(mGDALDat);

        // calcul de la distribution
        for (std::vector<double>::iterator it = aVHauteur.begin(); it < aVHauteur.end(); it++)
        {
            if (*it <= seuilClasses.at(0))
            {
                clasOcc.at(0)++;
            } else if (*it> seuilClasses.at(seuilClasses.size()-1)){
                clasOcc.at(clasOcc.size()-1)++;
            } else {
                for (int i(0) ; i+1 < seuilClasses.size() ; i++){
                    if (*it > seuilClasses.at(i) && *it <= seuilClasses.at(i+1)){
                        clasOcc.at(i+1)++;
                    }
                }
            }
        }

        /*for (int occ : clasOcc){
            std::cout << " occurence classe est " << occ << std::endl;
        }*/

        int nbPix=aVHauteur.size();
        // std::cout << " nombre de hauteur ;  " << nbPix<< std::endl;
        std::string aRange("<3m");
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(0)/((double)nbPix))));
        for (int i(1) ; i+1 < seuilClasses.size() ; i++){
            aRange=roundDouble(seuilClasses.at(i),0)+"m-"+roundDouble(seuilClasses.at(i+1),0)+"m";
            aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(i)/((double)nbPix))));
        }
        aRange=">51m";
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(clasOcc.size()-1)/((double)nbPix))));
    }

    return aRes;
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

rasterFiles::rasterFiles(std::string aPathTif,std::string aCode):mPathRaster(aPathTif),mPathQml(""),mCode(aCode), mResolution(0){
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

layerBase::layerBase(std::string aCode,cDicoApt * aDico):rasterFiles(aDico->File(aCode),aCode),mDico(aDico),mExpert(0),mGain(1){
    //if (globTest){std::cout << "layerBase constructor " << aCode << std::endl;}
    mNom=mDico->RasterNom(mCode);
    mNomCourt=mDico->RasterNomCourt(mCode);
    mExpert=mDico->RasterExpert(mCode);
    mGain=mDico->RasterGain(mCode);
    mTypeCarte =str2TypeCarte(mDico->RasterType(mCode));
    mTypeVar =str2TypeVar(mDico->RasterVar(mCode));
    mType =str2TypeLayer(mDico->RasterCategorie(mCode));
    mDicoVal=mDico->getDicoRaster(mCode);
    // si pas de dictionnaire mais un gain et 8 bits je peux m'en créer un
    if (mDicoVal.size()==0){
        for (int i(0);i<255;i++){
        mDicoVal.emplace(std::make_pair(i,std::to_string(i*mGain)));
        }
    }

    mDicoCol=mDico->getDicoRasterCol(mCode);
    mUrl=mDico->getWMSinfo(this->Code())->WMSURL();
    mWMSLayerName=mDico->getWMSinfo(this->Code())->WMSLayerName();
    mTypeWMS=mDico->getWMSinfo(this->Code())->getTypeWMS();
    mResolution=mDico->RasterResolution(mCode);
}

layerBase::layerBase(std::shared_ptr<layerBase> aLB):rasterFiles(aLB->Dico()->File(aLB->Code()),aLB->Code()){
    //std::cout << "layerBase constructor by copy " << aLB->Code() << std::endl;
    mDico=aLB->Dico();
    mExpert=aLB->Expert();
    mGain=aLB->Gain();
    mNom=aLB->Nom();
    mNomCourt=aLB->NomCourt();
    mTypeCarte =aLB->TypeCart();
    mTypeVar =aLB->getTypeVar();
    mType=aLB->getCatLayer();
    mDicoVal=aLB->getDicoVal();
    mDicoCol=aLB->getDicoCol();
    mUrl=aLB->WMSURL();
    mWMSLayerName=aLB->WMSLayerName();
    mTypeWMS=aLB->getTypeWMS();
    mResolution=aLB->mResolution;
}

std::string layerBase::NomFile(){
    boost::filesystem::path p(mPathRaster);
    return p.stem().c_str();}
std::string layerBase::NomFileWithExt(){
    boost::filesystem::path p(mPathRaster);
    return p.filename().c_str();}

int rasterFiles::getValue(double x, double y){

    int aRes(0);
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

        if (col<mBand->GetXSize() && col>=0  && row < mBand->GetYSize() && row>=0){
            float *scanPix;
            scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
            // lecture du pixel
            mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
            aRes=scanPix[0];//*mBand->GetScale(); non car c'est un integer qui est renvoyé par ma fonction getVal, donc si j'applique un gain pour avoir un float, je vais perdre les chiffres après la virgule
            CPLFree(scanPix);

            mBand=NULL;
        }
        GDALClose( mGDALDat );
    }
    return aRes;
}

void layerBase::createRasterColorInterpPalette(GDALRasterBand * aBand){

    aBand->SetColorInterpretation(GCI_PaletteIndex);

    GDALColorTable colors = GDALColorTable();
    GDALColorEntry nd;
    nd.c1=255;
    nd.c2=255;
    nd.c3=255;
    nd.c4=0;
    colors.SetColorEntry(0,&nd);

    // set color for each value
    for (auto kv : mDicoCol){
    //    int, std::shared_ptr<color>
    std::shared_ptr<color> col = kv.second;
    GDALColorEntry e;
    e.c1=col->mR;
    e.c2=col->mG;
    e.c3=col->mB;
    colors.SetColorEntry(kv.first,&e);
    }
    aBand->SetColorTable(&colors);
}


double rasterFiles::getValueDouble(double x, double y){

    double aRes(0);
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

        if (col<mBand->GetXSize() && col>=0  && row < mBand->GetYSize() && row>=0){
            float *scanPix;
            scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
            // lecture du pixel
            mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
            aRes=scanPix[0];
            CPLFree(scanPix);
            //mBand=NULL;
        }
        /*if (Code()=="ETP_30aire"){
        std::cout << "rasterFiles::getValueDouble pour raster " << Code() << " et position " << x <<  " , " << y << " donne comme résultat " << aRes << " qui est en position pixel de " << col << " , " << row <<std::endl;
        }*/
        if( mGDALDat != NULL){GDALClose(mGDALDat);}
    }
    return aRes;
}

std::map<std::string,int> layerBase::computeStat1(OGRGeometry *poGeom){

    std::map<std::string,int> aRes;
    if (mType!=TypeLayer::Externe){

        /*if (mCode="MNH2019"){
            // statistique hdom
            statHdomBase hdomStat = statHdomBase(this,poGeom,1);

        } else {*/

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
       // }
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
    // suppression des paires si valeur est == 0
    for (auto it = aRes.cbegin(); it != aRes.cend() /* not hoisted */; /* no increment */)
    {
      if (it->second==0)
      {
        aRes.erase(it++);    // or "it = m.erase(it)" since C++11
      }
      else
      {
        ++it;
      }
    }

    return aRes;
}


std::shared_ptr<color> layerBase::getColor(std::string aStrCode) const{
    int aCode(0);
    bool test(0);
    //color aRes(255,255,255);
    std::shared_ptr<color> aRes=std::make_shared<color>(0,0,0);

    for (auto & kv : mDicoVal){
        if(kv.second == aStrCode)
        {
            // Yes found
            test = true;
            // Push the key in given map
            aCode=kv.first;
        }
    }
    if ((!test) & (aStrCode=="Sans données")){ }else
    {aRes=getColor(aCode);}
    return aRes;
}

std::string layerBase::getDicoValStr(){
    std::string aRes("");
    for (auto & kv :mDicoVal){
        aRes+=std::to_string(kv.first)+";"+kv.second+"\n";
    }
    return aRes;
}

// création d'un raster masque pour un polygone. Valeur de 255 pour l'intérieur du polygone
GDALDataset * rasterFiles::rasterizeGeom(OGRGeometry *poGeom){
    std::string output("/vsimem/tmp.tif");
    const char *out=output.c_str();
    GDALDriver *pDriver;
    GDALDataset *pRaster=NULL, * pShp;

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

            const char *pszWkt = mGDALDat->GetProjectionRef();
            OGRSpatialReference oSRS;
            oSRS.importFromWkt(&pszWkt);

            // driver et dataset shp -- creation depuis la géométrie
            GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("Memory");
            char name[L_tmpnam];
            boost::filesystem::path tmpPath = boost::filesystem::path("/vsimem/") / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%");

            std::string name1 = tmpPath.string();
            std::string name2 = name1.substr(5,name1.size()-5);
            //pShp = pShpDriver->Create("/vsimem/blahblah.shp", 0, 0, 0, GDT_Unknown, NULL );
            pShp = pShpDriver->Create(name1.c_str(), 0, 0, 0, GDT_Unknown, NULL );// avoir des noms unique si je veux fontionner en parrallel computing
            // he bien c'est le comble, sur le serveur j'arrive à avoir le comportement adéquat si JE NE MET PAS de src. j'ai des warnings mais tout vas mieux!!
            //mais c'est parceque avant j'utilisais  OGRErr err=src.SetWellKnownGeogCS( "EPSG:31370" );

            OGRLayer * lay = pShp->CreateLayer(name2.c_str(),&oSRS,wkbPolygon,NULL);

            OGRFeature * feat = new OGRFeature(lay->GetLayerDefn());
            feat->SetGeometry(poGeom);
            lay->CreateFeature(feat);
            //delete feat;

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
            OGRFeature::DestroyFeature(feat);
            pShp->DeleteLayer(0);
            GDALClose(pShp);
        }
    }

    return pRaster;
}


// pour les couches des variables continues
basicStat layerBase::computeBasicStatOnPolyg(OGRGeometry * poGeom){
    if (globTest){std::cout << "compute BasicStat On Polyg" << std::endl;
    std::cout << getDicoValStr() << std::endl;
    }
    std::map<double,int> aMapValandFrequ;
    int nbNA(0);

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
                        } else {
                            // pour compter les no data et autres valeurs en dehors du dictionnaire
                            nbNA++;
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

    if (aMapValandFrequ.size()>0) {

        basicStat bs(aMapValandFrequ);
        bs.nbNA=nbNA;// je veux fournir le nombre de pixel en ND
        bs.resolution=mResolution;// je veux une somme par hectare donc j'ai besoin de la résolution des pixels
        return bs;} else {return  basicStat();}

}

// pour les couches des variables de classe
std::string layerBase::summaryStat(OGRGeometry * poGeom){
    //std::cout << "summary stat" << std::endl;
    layerStat ls(shared_from_this(),computeStat1(poGeom));
    return ls.summaryStat();
}

basicStat rasterFiles::computeBasicStatOnPolyg(OGRGeometry * poGeom){
    //std::cout << "rasterFile computeBasicStat" << std::endl;
    std::map<double,int> aMapValandFrequ;

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
                    if (aMapValandFrequ.find(aVal)!=aMapValandFrequ.end()){aMapValandFrequ.at(aVal)++;} else {
                        aMapValandFrequ.emplace(std::make_pair(aVal,1));}

                }
            }
        }
        CPLFree(scanline);
        CPLFree(scanlineMask);

        mBand=NULL;
    }
    GDALClose(mask);
    GDALClose(mGDALDat);

    if (aMapValandFrequ.size()>0) {return basicStat(aMapValandFrequ);} else {return  basicStat();}

}

cDicoApt * layerStat::Dico(){return mLay->Dico();}

void layerStat::simplifieStat(){

    mNbPix=0;
    for (auto & kv : mStat){
        mNbPix+=kv.second;
    }

    switch (mTypeVar){
    case TypeVar::Classe:{
        // calcul des pourcentages au lieu du nombre de pixel
        if (mNbPix>0){
            for (auto & kv : mStat){
                double pct = (100.0*kv.second)/mNbPix;
                kv.second=round(pct);
            }
        }

        int autres(0);
        int tot(0);
        for (auto & kv : mStat){
            if (kv.second>seuilClasseMinoritaire){
                mStatSimple.emplace(kv);
                tot+=kv.second;
            } else {
                autres+=kv.second;
            }
        }
        if (autres>0) {
            tot+=autres;
            // correction de l'erreur d'arrondi si elle est de 2 pct max
            if ((tot>97) & (tot <100)) { autres+= 100-tot; tot=100;}
            //std::cout << "ajout classe autre dans simplify stat " << autres << ", layer " << mLay->getLegendLabel() << std::endl;
            mStatSimple.emplace(std::make_pair("Autre",autres));
        }

        break;}

    case TypeVar::Continu:{

        // pour l'instant, copie juste le mStat
        mStatSimple=mStat;
        break;
    }
    }
}

std::string layerStat::summaryStat(){
    std::string aRes("");
    if (mLay->getTypeVar()==TypeVar::Classe){
        // on concatene toutes les essences
        for (auto & kv : mStat){
            if (kv.second>1){
                if (kv.second>99){ aRes+=kv.first;break;} else {
                    aRes+=kv.first+": "+std::to_string(kv.second)+"% ";
                }
            }
        }

    } else {
        std::cout << "  summaryStat sur une couche de variable de classes? " << mLay->Nom() << std::endl;
    }
    return aRes;

}

int layerStat::getO(bool mergeOT){
    unsigned int aRes(0);
    if (mLay->getCatLayer()==TypeLayer::FEE || mLay->getCatLayer()==TypeLayer::CS){
        // il faudrait plutôt faire le calcul sur les statistique mStat, car StatSimple peut avoir regroupé des classe trop peu représentées!
        // mais attention alors car le % n'est pas encore calculé, c'est le nombre de pixels.
        for (auto & kv : mStatSimple){
            int codeApt(666);
            for (auto & kv2 : mLay->getDicoVal()){
                if (kv2.second==kv.first){codeApt=kv2.first;}
            }
            int aptContr=mLay->Dico()->AptContraignante(codeApt);
            if (mergeOT) {if (aptContr<3) aRes+=kv.second;} else { if (aptContr<2) aRes+=kv.second; }
        }}
    else {
        std::cout << " problem, on me demande de calculer la proportion d'optimum pour une carte qui n'est pas une carte d'aptitude" << std::endl;
    }
    return aRes;
}

layerStat::layerStat(std::shared_ptr<layerBase> aLay, std::map<std::string,int> aStat):mLay(aLay),mStat(aStat),mTypeVar(aLay->getTypeVar()){
    simplifieStat();
}

std::string layerBase::getLegendLabel(bool escapeChar) const{
    std::string aRes=mNom;
    if (escapeChar) {
        boost::replace_all(aRes,"'","\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
    }
    return aRes;
}


bool layerBase::wms2jpg(OGREnvelope  * extent, int aSx, int aSy, std::string aOut) const{

    //std::cout << "Layer::wms2jpg()" << std::endl;
    bool aRes(0);
    std::string layerName=mWMSLayerName;
    // urlify le nom de couche, enlever les espaces
    boost::replace_all(layerName," ","%20");
    const char *connStr = CPLSPrintf("<GDAL_WMS>"
                                     "<Service name=\"WMS\">"
                                     "<Version>1.1.1</Version>"
                                     //"<Version>1.3.0</Version>"
                                     "<ServerUrl>%s?</ServerUrl>"
                                     "<SRS>EPSG:31370</SRS>"
                                     //"<CRS>EPSG:31370</CRS>" //pour version 1.3.0
                                     "<ImageFormat>image/jpeg</ImageFormat>"
                                     "<Layers>%s</Layers>"
                                     "<Styles></Styles>"
                                     "</Service>"
                                     "<DataWindow>"
                                     "<UpperLeftX>%f</UpperLeftX>"
                                     "<UpperLeftY>%f</UpperLeftY>"
                                     "<LowerRightX>%f</LowerRightX>"
                                     "<LowerRightY>%f</LowerRightY>"
                                     "<SizeX>%d</SizeX>"
                                     "<SizeY>%d</SizeY>"
                                     "</DataWindow>"
                                     "<Projection>EPSG:31370</Projection>"
                                     "<BandsCount>3</BandsCount>"
                                     "<ZeroBlockHttpCodes>204,404,500</ZeroBlockHttpCodes>" // la couche topo de l'ign retourne le code 500 depuis leurs mise à jour de avril 2021,vers WMS version 1.3 qui n'utilise pas les même balises xml (ajout crs entre autre, mais ne fonctionne pas)
                                     "<ZeroBlockOnServerException>true</ZeroBlockOnServerException>"
                                     "</GDAL_WMS>",
                                     mUrl.c_str(),
                                     layerName.c_str(),
                                     extent->MinX,
                                     extent->MaxY,
                                     extent->MaxX,
                                     extent->MinY,
                                     aSx,
                                     aSy
                                     );

    //"<Cache><Type>file</Type><Expires>%d</Expires></Cache>"
    /*if (globTest){
        std::cout <<mUrl << std::endl;
        std::cout << connStr << std::endl;}*/
    GDALDataset *pDS = static_cast<GDALDataset*>(GDALOpenEx(connStr, GDAL_OF_RASTER, nullptr, nullptr, nullptr));
    if( pDS != NULL ){

        //std::cout << " X size is " << pDS->GetRasterBand( 1 )->GetXSize() << " , Y size is " << pDS->GetRasterBand( 1 )->GetYSize()<< std::endl;
        // conversion vers jpg
        GDALDataset *pOutRaster;
        GDALDriver *pDriverPNG;
        const char *pszFormat2 = "PNG";
        pDriverPNG = GetGDALDriverManager()->GetDriverByName(pszFormat2);
        if( pDriverPNG == NULL )
        {
            printf( "%s driver not available.\n", pszFormat2 );
            exit( 1 );
        }
        pOutRaster = pDriverPNG->CreateCopy( aOut.c_str(),pDS,FALSE, NULL,NULL, NULL );
        if( pOutRaster != NULL ){ GDALClose( pOutRaster );}
        GDALClose(pDS);
        //GDALClose( (GDALDatasetH) pDS);
        aRes=1;
    }
    return aRes;
}

std::string getAbbreviation(std::string str)
{
    std::string aRes("");
    std::vector<std::string> words;
    std::string word("");
    for (auto x : str)
    {
        if (x == ' ' | x=='/')
        {
            word=removeAccents(word);// si je n'enlève pas les accents maintenant, les accents sont codé sur deux charachtère et en gardant les 2 premiers du mot je tronque l'accent en deux ce qui donne ququch d'illisible type ?
            if (word.size()>1){words.push_back(word);}
            //std::cout << "word is " << word << std::endl;
            word = "";
        }
        else
        {
            word = word + x;
        }
    }
    // pour le dernier mot :
    word=removeAccents(word);
    if (word.size()>1){words.push_back(word);}
    //std::cout << "word is " << word << std::endl;
    for (auto w : words){
        aRes+=w.substr(0,2);
    }

    return aRes;
}

GDALDataset * rasterizeGeom(OGRGeometry *poGeom, GDALDataset * aGDALDat){
    std::string output("/vsimem/tmp.tif");
    //std::string output("/home/lisein/Documents/tmp.tif");

    const char *out=output.c_str();
    GDALDriver *pDriver;
    GDALDataset *pRaster=NULL, * pShp;

    const char *pszFormat = "MEM";
    // sauver le masque pour vérification
    // const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( pDriver == NULL )
    {
        printf( "%s driver not available.\n", pszFormat );
    } else {

        OGREnvelope ext;
        poGeom->getEnvelope(&ext);
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

        if( aGDALDat == NULL )
        {
            std::cout << "rasterizeGeom ; on m'as donnée un pointeur GDALDataset qui est nul" << std::endl;
        } else {
            const char *pszWkt = aGDALDat->GetProjectionRef();
            OGRSpatialReference oSRS;
            oSRS.importFromWkt(&pszWkt);
            // driver et dataset shp -- creation depuis la géométrie
            GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("Memory");
            pShp = pShpDriver->Create("/vsimem/blahblah.shp", 0, 0, 0, GDT_Unknown, NULL );
            OGRLayer * lay = pShp->CreateLayer("toto",&oSRS,wkbPolygon,NULL);
            OGRFeature * feat = new OGRFeature(lay->GetLayerDefn());
            feat->SetGeometry(poGeom);
            lay->CreateFeature(feat);

            double transform[6];
            aGDALDat->GetGeoTransform(transform);

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
            pRaster->SetProjection(aGDALDat->GetProjectionRef());
            // on en avait besoin que pour l'extent et resol
            GDALClose(aGDALDat);
            GDALRasterize(NULL,pRaster,pShp,NULL,NULL);
            OGRFeature::DestroyFeature(feat);
            pShp->DeleteLayer(0);
            GDALClose(pShp);
        }
    }

    return pRaster;
}
