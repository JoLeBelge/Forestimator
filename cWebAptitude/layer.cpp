#include "layer.h"

Layer::Layer(groupLayers * aGroupL, std::string aCode,WText * PWText):
   layerBase(aCode,aGroupL->Dico())
  ,mGroupL(aGroupL)
  ,mWtText(PWText)
  ,mIsVisible(1)
  ,mLay4Stat(1)
  ,mLay4Visu(1)
{
    //std::cout << "création de layer pour " << aCode << std::endl;
    mLabel=mNom;
    mWtText->setText(mLabel);

    /*
    case TypeLayer::KK:
        // construction de la caractéristique stationnelle
        mKK=new cKKCS(mCode,mDico);
        mLabel= "Catalogue de Station - "+ mKK->Nom();
        mWtText->setText(mLabel);
        mPathTif=mKK->NomCarte();
        mDicoVal=mKK->getDicoValPtr();
        mDicoCol=mKK->getDicoCol();
        mExpert=1;
        break;
    */

    mLay4Stat=mDico->lay4Stat(mCode);
    mLay4Visu=mDico->lay4Visu(mCode);

    setActive(false);
    //std::cout << "done" << std::endl;
}

Layer::Layer(groupLayers * aGroupL, std::string aCode):
   layerBase(aCode,aGroupL->Dico())
  ,mGroupL(aGroupL)
  ,mIsVisible(0)
  ,mLay4Stat(1)
  ,mLay4Visu(0)
{
    mLabel=mNomCourt;
    mWtText=NULL;
    mLay4Stat=mDico->lay4Stat(mCode);
    mLay4Visu=mDico->lay4Visu(mCode);
    setActive(false);
}

Layer::Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB, WText * PWText):
    layerBase(aLB)
   ,mGroupL(aGroupL)
   ,mWtText(PWText)
   ,mIsVisible(1)
   ,mLay4Stat(1)
   ,mLay4Visu(1)
 {
     //std::cout << "création de layer en copiant layerbase " << aLB->Code() << std::endl;
     mLabel=mNomCourt;
     mWtText->setText(mLabel);
     mLay4Stat=mDico->lay4Stat(mCode);
     mLay4Visu=mDico->lay4Visu(mCode);
     setActive(false);
     //std::cout << "done" << std::endl;
 }

Layer::Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB):
    layerBase(aLB)
   ,mGroupL(aGroupL)
   ,mWtText(NULL)
   ,mIsVisible(0)
   ,mLay4Stat(1)
   ,mLay4Visu(0)
 {
     //std::cout << "création de layer pour " << aCode << std::endl;
     mLabel=mNomCourt;
     mLay4Stat=mDico->lay4Stat(mCode);
     mLay4Visu=mDico->lay4Visu(mCode);
     setActive(false);
    //std::cout << "done" << std::endl;
 }



void Layer::setActive(bool b){
    // ici je peux également changer le style du rendu du label
    mActive=b;
    if (mWtText!=NULL){
        mWtText->setStyleClass(mActive ? "currentEss" : "ess");
        if (mActive) {mWtText->setToolTip(WString::tr("toolTipActiveLayer"));} else {mWtText->setToolTip(getLegendLabel(0));}
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
    mWtText->doJavaScript(JScommand);
    //std::cout << JScommand << std::endl;
}

std::vector<std::string> Layer::displayInfo(double x, double y){
    std::vector<std::string> aRes;
    aRes.push_back(getLegendLabel(false));
    std::string val("");
    // on va affichier uniquement les informations de la couches d'apt qui est sélectionnée, et de toutes les couches thématiques (FEE et CS)
    if ((mType==TypeLayer::KK )| (mType==TypeLayer::Station) |( this->IsActive())){
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
        if(mDicoVal.find(aVal)!=mDicoVal.end()){
            val=mDicoVal.at(aVal);
        }
    }

    if ((mType==TypeLayer::FEE || mType==TypeLayer::CS) && (this->IsActive())){
        mGroupL->mStation->mActiveEss=Ess();
        mGroupL->mStation->HaveEss=1;
    }

    aRes.push_back(val);
    if (this->IsActive()) {aRes.push_back("bold");}
    return aRes;
}



