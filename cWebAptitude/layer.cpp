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
  ,mLay4Stat(1)
  ,mLay4Visu(1)
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
    mLay4Stat=mDico->lay4Stat(mCode);
    mLay4Visu=mDico->lay4Visu(mCode);

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
  ,mLay4Stat(1)
  ,mLay4Visu(1)
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
    mLay4Stat=mDico->lay4Stat(mCode);
    mLay4Visu=mDico->lay4Visu(mCode);

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
    if (mText!=NULL){
        mText->setStyleClass(mActive ? "currentEss" : "ess");
        if (mActive) {mText->setToolTip(WString::tr("toolTipActiveLayer"));} else {mText->setToolTip("");}
    }
}


void Layer::displayLayer() const{ 
    std::string JScommand;
    //std::cout << "display layer " << std::endl;

    std::string aFileIn(mDico->File("displayWMS"));
    std::ifstream in(aFileIn);
    std::stringstream ss;
    ss << in.rdbuf();
    in.close();
    JScommand=ss.str();
    boost::replace_all(JScommand,"MYTITLE",this->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",mUrl);
    boost::replace_all(JScommand,"MYATTRIBUTION",mWMSattribution);
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
                //mBand=NULL;
            }
            GDALClose(mask);
            GDALClose(mGDALDat);
        }
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

// création d'un raster masque pour un polygone. Valeur de 255 pour l'intérieur du polygone
GDALDataset * Layer::rasterizeGeom(OGRGeometry *poGeom){

    std::string output(mDico->File("TMPDIR")+"tmp");
    const char *out=output.c_str();
    //std::cout << " Layer::rasterizeGeom " << std::endl;

    GDALDriver *pDriver;
    GDALDataset *pRaster=NULL, * pShp;

    if (mType!=TypeLayer::Externe){
        OGRSpatialReference  * spatialReference=new OGRSpatialReference;
        OGRErr err;
        err=spatialReference->importFromEPSG(31370);
        char * src;
        //spatialReference->exportToWkt(&src);

        // driver et dataset shp -- creation depuis la géométrie
        GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        //std::cout << " create shp dataset " << std::endl;
        pShp = pShpDriver->Create("/vsimem/blahblah.shp", 0, 0, 0, GDT_Unknown, NULL );
        //std::cout << " create layer " << std::endl;
        // pas nécessaire pShp->SetProjection(src);
        OGRLayer * lay = pShp->CreateLayer("toto",spatialReference,wkbPolygon,NULL);
        OGRFeature * feat = new OGRFeature(lay->GetLayerDefn());
        feat->SetGeometry(poGeom);
        //std::cout << " create feature " << std::endl;
        lay->CreateFeature(feat);

        delete spatialReference;
        delete feat;


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


            // openshared pour pouvoir fermer
            GDALDataset * mGDALDat = (GDALDataset *) GDALOpenShared( getPathTif().c_str(), GA_ReadOnly );
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
                //pRaster->SetProjection(src);

                // on en avait besoin que pour l'extent et resol
                //pRaster->SetSpatialRef(mGDALDat->GetSpatialRef());
                pRaster->SetProjection(mGDALDat->GetProjectionRef());
                GDALClose(mGDALDat);

                //std::cout << " rasterize " << std::endl;
                GDALRasterize(NULL,pRaster,pShp,NULL,NULL);
            }
        }
        //delete src;
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

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {aRes=std::to_string(d+0.5).substr(0, std::to_string(d).find("."));}
    return aRes;
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

// pour les couches des variables de classe
std::string Layer::summaryStat(OGRGeometry * poGeom){
    //std::cout << "summary stat" << std::endl;
    layerStat ls(shared_from_this(),computeStatOnPolyg(poGeom));
    return ls.summaryStat();
}

bool Layer::wms2jpg(OGREnvelope  * extent, int aSx, int aSy, std::string aOut) const{

    //std::cout << "Layer::wms2jpg()" << std::endl;
    bool aRes(0);
    std::string layerName=mWMSLayerName;
    // urlify le nom de couche, enlever les espaces
    boost::replace_all(layerName," ","%20");
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
                                     "<UpperLeftX>%f</UpperLeftX>"
                                     "<UpperLeftY>%f</UpperLeftY>"
                                     "<LowerRightX>%f</LowerRightX>"
                                     "<LowerRightY>%f</LowerRightY>"
                                     "<SizeX>%d</SizeX>"
                                     "<SizeY>%d</SizeY>"
                                     "</DataWindow>"
                                     "<Projection>EPSG:31370</Projection>"
                                     "<BandsCount>3</BandsCount>"
                                     "<ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>"
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
    //std::cout << connStr << std::endl;
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
