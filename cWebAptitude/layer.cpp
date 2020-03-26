#include "layer.h"


Layer::Layer(groupLayers * aGroupL, std::string aCode, WText *PWText, TypeLayer aType):
    mDico(aGroupL->Dico())
  ,mGroupL(aGroupL)
  ,mCode(aCode)
  ,mText(PWText)
  ,mType(aType)
  ,mDicoVal(0)
  ,mDicoCol(0)

{
    // constructeur qui dépend du type de layer
    switch (mType) {
    case Apti:
        // construction de l'essence
        mEss=new cEss(mCode,mDico);
        mLabel=mEss->Code() + " - "+ mEss->Nom();
        mText->setText(mLabel);
        mDicoVal=mDico->code2AptFull();
        mDicoCol=mDico->codeApt2col();
        break;
    case KK:
        // construction de la caractéristique stationnelle
        mKK=new cKKCS(mCode,mDico);
        mLabel= "Catalogue de Station - "+ mKK->Nom();
        mText->setText(mLabel);
        mPathTif=mKK->NomCarte();
        if (mKK->IsHabitat()){
             mDicoVal=mDico->id2Hab();
        } else if (mKK->IsFact()){
            mDicoVal=mDico->echelleFactNom() ;
        } else if (mKK->IsPot()){
             mDicoVal=mDico->echellePotCat() ;
        }
        mDicoCol=mKK->getDicoCol();
        break;
    case Thematique:
        // creation de l'objet cRasterInfo
        mRI= new cRasterInfo(mCode,mDico);
        mLabel= mRI->Nom();
        mText->setText(mLabel);
        mPathTif=mRI->NomCarte();
        mDicoVal=mRI->getDicoVal();
        mDicoCol=mRI->getDicoCol();
        break;
    case Externe:
       if (mCode=="IGN"){
            mLabel="Carte topographique IGN";
            mText->setText(mLabel);
            mPathTif="";
        }
    }

    // ici ça bugge pk????? je pense qu'il crée un nouveau objet Layer depuis rien et c'est une mauvaise idée en fait
    // le connect et le bind fonctionne pour des objets dérivés des classes wt, mais ici mon layer n'est pas une classe wt donc ça bug
    mText->clicked().connect(std::bind(&groupLayers::clickOnName, mGroupL, mCode));
    //mText->clicked().connect(this, &Layer::clickOnName);

    // ajout d'un ancrage qui permet de remonter la page vers le haut, càd la carte
    //Wt::WLink link = Wt::WLink("/WebAptitude");
    //link.setTarget(Wt::LinkTarget::NewWindow);

    //std::unique_ptr<Wt::WAnchor> anchor = Wt::cpp14::make_unique<Wt::WAnchor>(link);
    //WAnchor * bookmark = new WAnchor( WLink( WLink::InternalPath, "/" ), "toto");

    //mText->addChild();

    setActive(false);
}

/*
void Layer::clickOnName(std::string aCode){
    std::cout << " j'ai cliqué sur " << aCode << " \n\n\n" << std::endl;
    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    //mGroupL->update(mCode);
    // ajouter la couche à la carte
    //displayLayer();
    //setActive(true);
}
*/

void Layer::displayLayer(){

    switch (mType) {
    case Externe:
    {
        std::string JScommand("groupe = new ol.layer.Group({layers:[TOREPLACE, communes]});TOREPLACE.setVisible(true);map.setLayerGroup(groupe);");
        boost::replace_all(JScommand,"TOREPLACE",mCode);
        mText->doJavaScript(JScommand);
        break;
    }
    default:
    {
        std::stringstream ss;
        std::string aFileIn(mDico->Files()->at("addOLraster"));
        std::ifstream in(aFileIn);
        std::string aTmp(aFileIn+".tmp");
        std::ofstream out(aTmp);
        // remplace l'url des tuiles par celui de l'essence actuelle:
        std::string aFind1("CODE1");
        std::string aFind2("CODE2");
        std::string line;
        std::string Replace1(""),Replace2("");

        switch (mType) {
        case Apti:
            switch (mGroupL->TypeClas()) {
            case FEE: Replace1="FEE_"+mCode;Replace2="aptitudeFEE_"+mCode;break;
            case CS: Replace1="CS_"+mCode;Replace2="aptitudeCS_"+mCode;break;
            }
            break;
        case KK:
            Replace1="KK_CS_"+mCode;
            Replace2=Replace1;
            break;
        case Thematique:
            Replace1=mRI->NomTuile();
            Replace2=mRI->NomFile();
        default:
            break;
        }

        while (getline(in, line))
        {
            boost::replace_all(line,aFind1,Replace1);
            boost::replace_all(line,aFind2,Replace2);
            out << line << "\n";
        }
        in.close();
        out.close();

        in.open(aFileIn+".tmp");
        ss << in.rdbuf();
        mText->doJavaScript(ss.str());

        break;
    }

    }
}

std::vector<std::string> Layer::displayInfo(double x, double y){
    std::vector<std::string> aRes;
    aRes.push_back(getLegendLabel());
    std::string val("");
    // on va affichier uniquement les informations de la couches d'apt qui est sélectionnée, et de toutes les couches thématiques (FEE et CS)
    if (mType==KK | mType==Thematique | this->IsActive()){
    // 1 extraction de la valeur
    int aVal=getValue(x,y);
    if (mCode=="NT"){ mGroupL->mStation->mNT=aVal;}
    if (mCode=="NH"){ mGroupL->mStation->mNH=aVal;}
    if (mCode=="ZBIO"){ mGroupL->mStation->mZBIO=aVal;}
    if (mCode=="Topo"){ mGroupL->mStation->mTOPO=aVal;}
    if (mCode.substr(0,2)=="CS" && aVal!=0){
        //std::cout << "totoooo \n\n\n" << std::endl;
        mGroupL->mStation->mSt=aVal;}
    //std::cout << " la valeur de la couche " << mLabel << " est de " << aVal << std::endl;
    if(mDicoVal->find(aVal)!=mDicoVal->end()){
        val=mDicoVal->at(aVal);
    }
    }

    if (mType==Apt && this->IsActive()){
        mGroupL->mStation->mActiveEss=mEss;
        mGroupL->mStation->HaveEss=1;
    }

    aRes.push_back(val);
    if (this->IsActive()) {aRes.push_back("bold");}
    return aRes;
}

int Layer::getValue(double x, double y){

    int aRes(0);
    if (mType!=Externe){
    // gdal
    GDALAllRegister();

    mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
    } else {
    mBand = mGDALDat->GetRasterBand( 1 );

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

    GDALClose( mGDALDat );
    }
    }
    }
    return aRes;
}

std::string Layer::getPathTif(){
    std::string aRes;
    switch (mType) {
    case Apti:
        switch (mGroupL->TypeClas()) {
        case FEE: aRes=mEss->NomCarteAptFEE();break;
        case CS: aRes=mEss->NomCarteAptCS();break;
        }
        break;
    default:
        aRes=mPathTif;
    }
    return aRes;
}

std::string Layer::getLegendLabel(){
    std::string aRes;
    switch (mType) {
    case Apti:
        switch (mGroupL->TypeClas()) {
        case FEE: aRes="Aptitude FEE du "+mLabel;break;
        case CS: aRes="Aptitude CS du "+mLabel;break;
        }
        break;
    default:
        aRes=mLabel;
    }
    return aRes;
}
