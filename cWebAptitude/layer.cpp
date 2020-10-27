#include "layer.h"

// j'ai besoin d'un objet layer pour faire des cartes statiques dans la fenetre pour les stat, mais ces objets sont totalement lié à groupLayer ET à un WText, donc sont encombrant. Je crée un constructeur "allégé" pour une utilisation plus souple
Layer::Layer(std::string aCode,cDicoApt * aDico,TypeLayer aType):
    mDico(aDico)
  ,mGroupL(NULL)
  ,mCode(aCode)
  ,mText(NULL)
  ,mType(aType)
  ,mDicoVal(NULL)
  ,mRI(NULL)
  ,mEss(NULL)
  ,mKK(NULL)
  ,mTypeVar(TypeVar::Classe)
  ,mExpert(0)
  ,mIsVisible(1)
{
    //std::cout << "création de layer en dehors de wt pour " << aCode << std::endl;
    switch (mType) {
    case TypeLayer::FEE:
        // construction de l'essence
        mEss=new cEss(mCode,mDico);
        mLabel=mEss->Code() + " - "+ mEss->Nom();
        //mText->setText(mLabel);
        mDicoVal=mDico->code2AptFull();
        mDicoCol=mDico->codeApt2col();
        break;
    case TypeLayer::CS:
        // construction de l'essence
        mEss=new cEss(mCode,mDico);
        mLabel=mEss->Code() + " - "+ mEss->Nom();
        //mText->setText(mLabel);
        mDicoVal=mDico->code2AptFull();
        mDicoCol=mDico->codeApt2col();
        setExpert(1);
        break;
    case TypeLayer::KK:
        // construction de la caractéristique stationnelle
        mKK=new cKKCS(mCode,mDico);
        mLabel= "Catalogue de Station - "+ mKK->Nom();
        //mText->setText(mLabel);
        mPathTif=mKK->NomCarte();
        mDicoVal=mKK->getDicoValPtr();
        mDicoCol=mKK->getDicoCol();
        setExpert(1);
        break;
    case TypeLayer::Thematique:
        // creation de l'objet cRasterInfo
        mRI= new cRasterInfo(mCode,mDico);
        mLabel= mRI->Nom();
        //mText->setText(mLabel);
        mPathTif=mRI->NomCarte();
        mDicoVal=mRI->getDicoVal();
        mDicoCol=mRI->getDicoCol();
        mTypeVar=mRI->getTypeVar();
        mType=mRI->getCatLayer();
        setExpert(mRI->Expert());
        break;
        /*case TypeLayer::Externe: les cartes de type externe sont créé à partir de cRasterInfo (type thématique qui est écrasé en fonction du cRasterInfo)
        }*/
    }

    mUrl=MapServerURL();
    mWMSLayerName=NomMapServerLayer();

}

Layer::Layer(groupLayers * aGroupL, std::string aCode, WText *PWText, TypeLayer aType):
    mDico(aGroupL->Dico())
  ,mGroupL(aGroupL)
  ,mCode(aCode)
  ,mText(PWText)
  ,mType(aType)
  ,mDicoVal(NULL)
  //,mDicoCol(NULL)
  ,mRI(NULL)
  ,mEss(NULL)
  ,mKK(NULL)
  ,mTypeVar(TypeVar::Classe)
  ,mExpert(0)
  ,mIsVisible(1)
{
    //std::cout << "création de layer pour " << aCode << std::endl;
    // constructeur qui dépend du type de layer
    switch (mType) {
    case TypeLayer::FEE:
        // construction de l'essence
        mEss=new cEss(mCode,mDico);
        mLabel=mEss->Code() + " - "+ mEss->Nom();
        mText->setText(mLabel);
        mDicoVal=mDico->code2AptFull();
        mDicoCol=mDico->codeApt2col();
        break;
    case TypeLayer::CS:
        // construction de l'essence
        mEss=new cEss(mCode,mDico);
        mLabel=mEss->Code() + " - "+ mEss->Nom();
        mText->setText(mLabel);
        mDicoVal=mDico->code2AptFull();
        mDicoCol=mDico->codeApt2col();
        mExpert=1;
        break;
    case TypeLayer::KK:
        // construction de la caractéristique stationnelle
        mKK=new cKKCS(mCode,mDico);
        mLabel= "Catalogue de Station - "+ mKK->Nom();
        mText->setText(mLabel);
        mPathTif=mKK->NomCarte();
        mDicoVal=mKK->getDicoValPtr();
        mDicoCol=mKK->getDicoCol();
        mExpert=1;
        break;
    case TypeLayer::Thematique:
        // creation de l'objet cRasterInfo
        mRI= new cRasterInfo(mCode,mDico);
        mLabel= mRI->Nom();
        mText->setText(mLabel);
        mPathTif=mRI->NomCarte();
        mDicoVal=mRI->getDicoVal();
        mDicoCol=mRI->getDicoCol();
        mTypeVar=mRI->getTypeVar();
        mType=mRI->getCatLayer();
        mExpert=mRI->Expert();
        break;
    }

    mUrl=MapServerURL();
    mWMSLayerName=NomMapServerLayer();

    setActive(false);
    //std::cout << "done" << std::endl;
}

Layer::~Layer(){
    //std::cout << "destrutor layer" << std::endl;
    // delete ; only with new
    mDicoVal=NULL;
    //mDicoCol=NULL;
    mGroupL=NULL;
    mDico=NULL;
    //delete mText;
    mText=NULL;

    // créé avec new, donc je baque avec delte? et le constructeur numéro2, il utilise new mais pas dans l'objet layer.
    delete mEss;
    delete mKK;
    delete mRI;
    mEss=NULL;
    mKK=NULL;
    mRI=NULL;
}

void Layer::setActive(bool b){
    // ici je peux également changer le style du rendu du label
    mActive=b;
    mText->setStyleClass(mActive ? "currentEss" : "ess");
    if (mActive) {mText->setToolTip(WString::tr("toolTipActiveLayer"));} else {mText->setToolTip("");}
}


void Layer::displayLayer() const{ 
    std::string JScommand;
    //std::cout << "display layer " << std::endl;
    wms2jpg();

    std::string aFileIn(mDico->File("displayWMS"));
    std::ifstream in(aFileIn);
    std::stringstream ss;
    ss << in.rdbuf();
    in.close();
    JScommand=ss.str();
    boost::replace_all(JScommand,"MYTITLE",this->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",mUrl);
    mText->doJavaScript(JScommand);
    //std::cout << JScommand << std::endl;
}

std::vector<std::string> Layer::displayInfo(double x, double y){
    std::vector<std::string> aRes;
    aRes.push_back(getLegendLabel(false));
    std::string val("");
    // on va affichier uniquement les informations de la couches d'apt qui est sélectionnée, et de toutes les couches thématiques (FEE et CS)
    if ((mType==TypeLayer::KK )| (mType==TypeLayer::Thematique) |( this->IsActive())){
        // 1 extraction de la valeur
        int aVal=getValue(x,y);
        if (mCode=="NT"){ mGroupL->mStation->mNT=aVal;}
        if (mCode=="NH"){ mGroupL->mStation->mNH=aVal;}
        if (mCode=="ZBIO"){ mGroupL->mStation->mZBIO=aVal;}
        if (mCode=="Topo"){ mGroupL->mStation->mTOPO=aVal;}

        // station du CS
        if (mCode.substr(0,2)=="CS" && aVal!=0){
            mGroupL->mStation->mSt=aVal;}
        //std::cout << " la valeur de la couche " << mLabel << " est de " << aVal << std::endl;
        if((mDicoVal!=NULL) && (mDicoVal->find(aVal)!=mDicoVal->end())){
            val=mDicoVal->at(aVal);
        }
    }

    if ((mType==TypeLayer::FEE || mType==TypeLayer::CS) && (this->IsActive())){
        mGroupL->mStation->mActiveEss=mEss;
        mGroupL->mStation->HaveEss=1;
    }

    aRes.push_back(val);
    if (this->IsActive()) {aRes.push_back("bold");}
    return aRes;
}

int Layer::getValue(double x, double y){

    int aRes(0);
    if (mType!=TypeLayer::Externe){
        // gdal
        GDALAllRegister();

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

            //std::cout << " x : " << x << " x Origin " << xOrigin << " pixelwidth " << pixelWidth << " col " << col << std::endl;
            //std::cout << " y : " << y << " y Origin " << yOrigin << " pixelHeigh " << pixelHeight <<  " row " << row << std::endl;

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

std::map<std::string,int> Layer::computeStatOnPolyg(OGRGeometry *poGeom){
    //std::cout << " groupLayers::computeStatOnPolyg " << std::endl;
    std::map<std::string,int> aRes;

    if (mType!=TypeLayer::Externe){
        // préparation du containeur du résultat
        for (auto &kv : *mDicoVal){
            aRes.emplace(std::make_pair(kv.second,0));
        }
        // création d'une classe nd? quitte à la supprimer par après si pas d'occurence de nd
        aRes.emplace(std::make_pair("ND",0));
        int nbPix(0);

        // c'est mon masque au format raster
        GDALDataset * mask = Layer::rasterizeGeom(poGeom);

        OGREnvelope ext;
        poGeom->getEnvelope(&ext);
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
        // std::cout << " x " << width<< " y " << height << std::endl;

        // gdal
        GDALAllRegister();
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
                        if (mDicoVal->find(aVal)!=mDicoVal->end()){
                            aRes.at(mDicoVal->at(aVal))++;
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

            // maintenant on calcule les pourcentage
            /*if (nbPix>0){
                for (auto & kv : aRes){
                    kv.second=(100*kv.second)/nbPix;
                }
            }*/
            mBand=NULL;
        }
        GDALClose(mask);
        GDALClose(mGDALDat);
    }
    return aRes;
}



std::string Layer::getPathTif(){
    std::string aRes;
    switch (mType) {
    case TypeLayer::FEE:
        aRes=mEss->NomCarteAptFEE();break;
    case TypeLayer::CS:
        aRes=mEss->NomCarteAptCS();break;
    default:
        aRes=mPathTif;
    }
    return aRes;
}

std::string Layer::getLegendLabel(bool escapeChar) const{
    std::string aRes;
    switch (mType) {
    case TypeLayer::FEE:
        aRes="Aptitude FEE du "+mLabel;break;
    case TypeLayer::CS:
        aRes="Aptitude CS du "+mLabel;break;
    default:
        aRes=mLabel;
    }

    if (escapeChar) {
        boost::replace_all(aRes,"'","\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
    }

    return aRes;
}

std::string Layer::getLegendLabel(std::string aMode){
    std::string aRes;
    switch (mType) {
    case TypeLayer::FEE:
        aRes="Aptitude FEE du "+mLabel;break;
    case TypeLayer::CS:
        aRes="Aptitude CS du "+mLabel;break;
    default:
        aRes=mLabel;
    }
    return aRes;
}


std::vector<std::string> Layer::getCode(std::string aMode){
    std::vector<std::string> aRes;
    switch (mType) {
    case TypeLayer::FEE:
        aRes={mCode,"FEE"};break;
    case TypeLayer::CS:
        aRes={mCode,"CS"};break;
    default:
        aRes={mCode,""};
    }
    return aRes;
}

GDALDataset * Layer::rasterizeGeom(OGRGeometry *poGeom){

    std::string output(mDico->File("TMPDIR")+"tmp");
    const char *out=output.c_str();
    //std::cout << " Layer::rasterizeGeom " << std::endl;
    GDALAllRegister();
    GDALDriver *pDriver;
    GDALDataset *pRaster=NULL, * pShp;

    if (mType!=TypeLayer::Externe){
        OGRSpatialReference  * spatialReference=new OGRSpatialReference;
        OGRErr err;
        err=spatialReference->importFromEPSG(31370);
        char *src;
        spatialReference->exportToWkt(&src);

        // driver et dataset shp -- creation depuis la géométrie
        GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        //std::cout << " create shp dataset " << std::endl;
        pShp = pShpDriver->Create( "/vsimem/blahblah.shp", 0, 0, 0, GDT_Unknown, NULL );
        //std::cout << " create layer " << std::endl;
        // pas nécessaire pShp->SetProjection(src);
        OGRLayer * lay = pShp->CreateLayer("toto",spatialReference,wkbPolygon,NULL);
        OGRFeature * feat = new OGRFeature(lay->GetLayerDefn());
        feat->SetGeometry(poGeom);
        //std::cout << " create feature " << std::endl;
        lay->CreateFeature(feat);

        // driver et dataset raster in memory
        const char *pszFormat = "MEM";
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
                GDALClose(mGDALDat);
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
                pRaster->SetProjection(src);
                //std::cout << " rasterize " << std::endl;
                GDALRasterize(NULL,pRaster,pShp,NULL,NULL);
            }
        }
        GDALClose(pShp);
    }
    return pRaster;
}

std::string Layer::NomMapServerLayer()const{
    std::string aRes;
    if (mDico->hasWMSinfo(this->getCode())){
          aRes=mDico->getWMSinfo(this->getCode())->mWMSLayerName;
    }else{
    switch (mType) {
    case TypeLayer::FEE:
        aRes="Aptitude_FEE_"+mCode;
        break;
    case TypeLayer::CS:
        aRes="Aptitude_CS_"+mCode;
        break;
    case TypeLayer::Thematique:
        aRes=mCode;
        break;
    case TypeLayer::KK:
        aRes="KK_CS_"+mCode;
        break;
    default:
        aRes=mCode;
    }
    }
    return aRes;
}

std::string Layer::MapServerURL()const{
    std::string aRes;
    if (mDico->hasWMSinfo(this->getCode())){
          aRes=mDico->getWMSinfo(this->getCode())->mUrl;
    }else{

    switch (mType) {
    case TypeLayer::FEE:
        aRes="https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/aptitude_fee";
        break;
    case TypeLayer::CS:
        aRes="https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/aptitude_cs";
        break;
    case TypeLayer::Thematique:
        aRes="https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/station_fee";
        break;
    case TypeLayer::KK:
        aRes="https://gxgfservcarto.gxabt.ulg.ac.be/cgi-bin/station_cs";
        break;
    default:
        aRes=mCode;
    }
    }
    return aRes;
}

rasterFiles Layer::getRasterfile(){

    //rasterFiles aRes;
    switch (this->Type()){
    case TypeLayer::FEE:{
        rasterFiles aRes(getPathTif(),"Aptitude_"+getCode()+"_FEE");
        return aRes;
    }
    case TypeLayer::CS:{
        rasterFiles aRes(rasterFiles(getPathTif(),"Aptitude_"+getCode()+"_CS"));
        return aRes;
    }
    default:{
        rasterFiles aRes(getPathTif(),getCode());
        return aRes;
    }
    }

}


rasterFiles::rasterFiles(std::string aPathTif,std::string aCode):mPathTif(aPathTif),mPathQml(""),mCode(aCode){
    // <-- initialize with the map's default c'tor
    //boost::filesystem::path p(mPathTif);
    // détermine si il y a un fichier de symbologie associé
    std::string aPathQml = mPathTif.substr(0,mPathTif.size()-3)+"qml";
    if (exists(aPathQml)){
        mPathQml=aPathQml;
    }
}

basicStat::basicStat(std::map<double,int> aMapValandFrequ):mean(0),max(0),min(0){
    bool test(0);
    int tot(0);
    for (auto kv : aMapValandFrequ){
        mean += kv.first*kv.second;
        tot+=kv.second;

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
    mean=mean/tot;

}

// pour les couches des variables continues
basicStat Layer::computeBasicStatOnPolyg(OGRGeometry * poGeom){
    std::cout << "compute BasicStat On Polyg" << std::endl;
    std::map<double,int> aMapValandFrequ;

    if (mTypeVar==TypeVar::Continu){
        // préparation du containeur du résultat
        for (auto &kv : *mDicoVal){
            try {
                aMapValandFrequ.emplace(std::make_pair(std::stod(kv.second),0));
            }
            catch (const std::invalid_argument& ia) {
                std::cerr << "Invalid argument pour stod computeBasicStatOnPolyg: " << ia.what() << '\n';
            }
        }

        // c'est mon masque au format raster
        GDALDataset * mask = Layer::rasterizeGeom(poGeom);

        OGREnvelope ext;
        poGeom->getEnvelope(&ext);
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
        // std::cout << " x " << width<< " y " << height << std::endl;

        // gdal
        GDALAllRegister();
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
                        if (mDicoVal->find(aVal)!=mDicoVal->end()){

                            try {
                                aMapValandFrequ.at(std::stod(mDicoVal->at(aVal)))++;
                            }
                            catch (const std::invalid_argument& ia) {
                                std::cerr << "Invalid argument pour stod computeBasicStatOnPolyg, part2: " << ia.what() << '\n';
                            }

                        }   }
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

// pour les couches des variables de classe
std::string Layer::summaryStat(OGRGeometry * poGeom){

    std::cout << "summary stat" << std::endl;
    layerStat ls(this,computeStatOnPolyg(poGeom));
    return ls.summaryStat();

}


//bool Layer::wms2jpg(OGREnvelope extent,double aGsd){
bool Layer::wms2jpg() const{

    std::cout << "Layer::wms2jpg()" << std::endl;
    bool aRes(0);

        GDALAllRegister();

        /*
        const char *connStr = CPLSPrintf("<GDAL_WMS><Service name=\"WMS\">"
                "<ServerUrl>%s</ServerUrl></Service><DataWindow>"
                "<UpperLeftX>%f</UpperLeftX><UpperLeftY>%f</UpperLeftY>"
                "<LowerRightX>%f</LowerRightX><LowerRightY>%f</LowerRightY>"
                "<TileLevel>%d</TileLevel><TileCountX>1</TileCountX>"
                "<TileCountY>1</TileCountY><YOrigin>top</YOrigin></DataWindow>"
                "<Projection>EPSG:31370</Projection><BlockSizeX>256</BlockSizeX>"
                "<BlockSizeY>256</BlockSizeY><BandsCount>%d</BandsCount>"
                "<Cache><Type>file</Type>"
                "</Cache><ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes></GDAL_WMS>",
                                             wms.mUrl.c_str(), extent.MinX,
                                             extent.MaxY, extent.MaxX,
                                             extent.MinY, z_max,
                                            // y_origin_top ? "top" : "bottom",
                                             3);
                                         //, cacheExpires,
                                          //   cacheMaxSize);
                                          */

        //const char *connStr = "<GDAL_WMS>"
        const char *connStr = CPLSPrintf("<GDAL_WMS>"
                                         "<Service name=\"WMS\">"
                                         "<Version>1.1.1</Version>"
                                         "<ServerUrl>%s?</ServerUrl>"
                                         "<SRS>EPSG:31370</SRS>"
                                         "<ImageFormat>image/jpeg</ImageFormat>"
                                         "<Layers>%s</Layers>"
                                         "<Styles></Styles>"
                                         "</Service>"
                                         "<DataWindow>"
                                         "<UpperLeftX>200000.00</UpperLeftX>"
                                         "<UpperLeftY>100000.00</UpperLeftY>"
                                         "<LowerRightX>201000.00</LowerRightX>"
                                         "<LowerRightY>99000.00</LowerRightY>"
                                         "<SizeX>500</SizeX>"
                                         "<SizeY>500</SizeY>"
                                         "</DataWindow>"
                                         "<Projection>EPSG:31370</Projection>"
                                         "<BandsCount>3</BandsCount>"
                                         "<ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>"
                                         "<ZeroBlockOnServerException>true</ZeroBlockOnServerException>"
                                         "</GDAL_WMS>",
                                         mUrl.c_str(),
                                         mWMSLayerName.c_str()
                                         );

        std::cout << connStr << std::endl;

        GDALDataset *pDS = static_cast<GDALDataset*>(GDALOpenEx(
                                                         connStr, GDAL_OF_RASTER, nullptr, nullptr, nullptr));

        if( pDS != NULL ){

            std::cout << " X size is " << pDS->GetRasterBand( 1 )->GetXSize() << " , Y size is " << pDS->GetRasterBand( 1 )->GetYSize()<< std::endl;

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

            pOutRaster = pDriverPNG->CreateCopy( "/home/lisein/Documents/carteApt/Forestimator/data/tmp/toto.png", pDS, FALSE, NULL,NULL, NULL );
            if( pOutRaster != NULL ){ GDALClose( pOutRaster );}
            GDALClose( pDS );
        }

    return aRes;
}
