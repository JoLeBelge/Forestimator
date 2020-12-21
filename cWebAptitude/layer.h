#ifndef LAYER_H
#define LAYER_H
#include <Wt/WObject.h>
#include <Wt/WTable.h>
#include <Wt/WCheckBox.h>
#include <Wt/WText.h>
#include "cdicoapt.h"
#include <Wt/WLabel.h>
#include <Wt/WWidget.h>
#include <Wt/WString.h>
#include "grouplayers.h"
#include <boost/algorithm/string/replace.hpp>

#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"

#include <numeric>

using namespace Wt;


/* C'est réellement la classe d'objet qui est au centre des différentes technologies et techniques ;
 * -gdal
 * -openlayer
 * -aptitude/ essence
 * -wt
 */
class groupLayers;
class Layer;
class color;
class rasterFiles; // une classe dédiée uniquemnent à l'export et au clip des couches. Layer ne convient pas car contient une grosse partie pour l'affichage (tuiles et autre)
class LayerMTD;
class basicStat;

// ça va être un peu batard vu que je vais mélanger divers type de layer
class Layer : public WMSinfo, public std::enable_shared_from_this<Layer>
{
public:
    Layer(std::string aCode,cDicoApt * aDico,TypeLayer aType);
    Layer(groupLayers * aGroupL, std::string aCode,WText * PWText,TypeLayer aType);

    // constructeur par copie et par déplacement ; indispensable si j'utilise les objets dans un vecteur. http://www-h.eng.cam.ac.uk/help/tpl/languages/C++/morevectormemory.html
    /*
    Layer(const Layer &lay){
        //std::cout << "construct by copy layer " << std::endl;
        mActive=lay.mActive;
        mExpert=lay.mExpert;
        mIsVisible=lay.mIsVisible;
        mGroupL=lay.mGroupL;
        mType=lay.mType;
        mEss=NULL;
        mKK=NULL;
        mRI=NULL;
        mDico=lay.mDico;
        mText=lay.mText;
        mLabel=lay.mLabel;
        mPathTif=lay.mPathTif;
        //mDirTuile=lay.mDirTuile;
        mCode=lay.mCode;
        mDicoCol=lay.mDicoCol;
        mDicoVal=lay.mDicoVal;
        mUrl=lay.mUrl;
        mWMSLayerName=lay.mWMSLayerName;
        switch (mType) {
            case TypeLayer::FEE:
            case TypeLayer::CS:
                mEss=new cEss(mCode,mDico);
                break;
            case TypeLayer::KK:
                mKK=new cKKCS(mCode,mDico);
                break;
            case TypeLayer::Thematique:
                mRI= new cRasterInfo(mCode,mDico);
                break;
            default:{}
        }
    }
    Layer(Layer&& lay) noexcept {
        //std::cout << "construct by move layer " << std::endl;
        mActive=lay.mActive;
        mExpert=lay.mExpert;
        mIsVisible=lay.mIsVisible;
        mGroupL=lay.mGroupL;
        mType=lay.mType;
        mEss=NULL;
        mKK=NULL;
        mRI=NULL;
        mDico=lay.mDico;
        mText=lay.mText;
        mLabel=lay.mLabel;
        mPathTif=lay.mPathTif;
        //mDirTuile=lay.mDirTuile;
        mCode=lay.mCode;
        mDicoCol=lay.mDicoCol;
        mDicoVal=lay.mDicoVal;
        mUrl=lay.mUrl;
        mWMSLayerName=lay.mWMSLayerName;
        switch (mType) {
            case TypeLayer::FEE:
            case TypeLayer::CS:
                mEss=new cEss(mCode,mDico);
                break;
            case TypeLayer::KK:
                mKK=new cKKCS(mCode,mDico);
                break;
            case TypeLayer::Thematique:
                mRI= new cRasterInfo(mCode,mDico);
                break;
            default:{}
        }
    }*/

    ~Layer();

    //void clickOnName(std::string aCode);
    void displayLayer() const;

    std::vector<std::string> displayInfo(double x, double y);
    // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
    std::map<std::string,int> computeStatOnPolyg(OGRGeometry * poGeom);
    GDALDataset * rasterizeGeom(OGRGeometry *poGeom);

    basicStat computeBasicStatOnPolyg(OGRGeometry * poGeom);
    std::string summaryStat(OGRGeometry * poGeom);

    // convertir le wms de la couche au format image en local
    bool wms2jpg(OGREnvelope *extent, int aSx, int aSy, std::string aOut) const;

    // raster value
    int getValue(double x, double y);
    // signification pour ce raster value
    std::string getValueTxt(double x, double y);
    // crop du raster avec un shp parcellaire
    //bool cropIm(std::string aOut);
    void setActive(bool b=true);
    bool IsActive() const {return mActive;}
    std::string getCode() const{return mCode;}

    std::string getFieldName() const{
        std::string aRes=mCode;
        if (aRes=="MNH2019"){aRes="TreeCover";}
        if (aRes=="MF"){aRes="MasqForet";}
        if (aRes=="COMPO"){aRes="Compo";}
        return aRes;
    }
    std::string getFieldType() const{
        std::string aRes("int");
        if (mCode=="COMPO"){aRes="str";}
        return aRes;
    }

    std::string getPathTif(){return mPathTif;}
    std::string getLegendLabel(bool escapeChar=true) const;
    std::string getShortLabel() const {return mLabel;}

    std::string NomMapServerLayer()const;
    std::string MapServerURL()const;

    // les info utile pour manipuler les fichiers (avec une méthode pour sélectionner le fichier de symbologie Qgis)
    rasterFiles getRasterfile();

    std::vector<std::string> getCode(std::string aMode);
    std::string getLegendLabel(std::string aMode);

    TypeLayer Type() const {return mType;}
    TypeVar Var() const {return mTypeVar;}

    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> * mDicoVal;
    // le dictionnaire des valeurs raster vers leur couleur, pour la légende
    std::map<int, color> mDicoCol;

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool>& changeExpertMode() { return expertMode_; }
    Wt::Signal<bool> expertMode_;
    void ExpertMode(bool GlobalExpertMode){
        // va envoyer le signal à setNodeVisible
        if (GlobalExpertMode) {
            mIsVisible=1;
        } else {
            if (mExpert) {
            mIsVisible=0;
            } else {
            mIsVisible=1;
            }
        }
         expertMode_.emit(mIsVisible);
    }
    // utilisé dans les construteurs pour changer en un coup mIsVisible et mExpert
    void setExpert(bool expert){
        mExpert=expert;
        mIsVisible=!expert;
    }
    // pour savoir distinguer mode expert et mode normal au niveau de chaque layer
    bool mIsVisible;
    bool isVisible(){return mIsVisible;}

    bool hasColor(int aCode) const{
        bool aRes(0);
        if (mDicoCol.find(aCode)!=mDicoCol.end()){aRes=true;}
        return aRes;
    }
    color getColor(int aCode) const{
        color aRes(0,0,0);
        if (mDicoCol.find(aCode)!=mDicoCol.end()){
            aRes=mDicoCol.at(aCode);}
        return aRes;
    }

    color getColor(std::string aStrCode) const{
        int aCode(0);
        bool test(0);
        color aRes(255,255,255);
        std::map<int, std::string>::iterator it = mDicoVal->begin();
        // Iterate through the map
        while(it != mDicoVal->end())
        {
            // Check if value of this entry matches with given value
            if(it->second == aStrCode)
            {
                // Yes found
                test = true;
                // Push the key in given map
                aCode=it->first;
            }
            // Go to next entry in map
            it++;
        }
        //if (!test) std::cout << " warn, pas de label correspondant à " << aStrCode << " dans le dictionnaire des valeurs de cette layer" << std::endl;
        if ((!test) & (aStrCode=="Sans données")){ }else
        {aRes=getColor(aCode);}
        return aRes;
    }
    cDicoApt * Dico(){return mDico;}
    //cEss * Ess(){return mEss;}

    std::shared_ptr<cEss> Ess(){return mDico->getEss(mCode);}
    bool l4Stat(){return mLay4Stat;}
        //WMSinfo * mWMS;
private:
    bool mActive;
    // les couches sont taggées comme étant expert ou non expert
    bool mExpert;
    // est ce une couche qu'on veux pouvoir visulaliser, qu'on veux pouvoir calculer des stats dessus?
    bool mLay4Visu;
    bool mLay4Stat;
    groupLayers * mGroupL;
    TypeLayer mType;
    TypeVar mTypeVar;
    cEss * mEss;
    cKKCS * mKK;
    cRasterInfo * mRI;
    cDicoApt * mDico;
    WText * mText; // ça c'est le lien entre l'objet et l'affichage dans la page web
    // le texte affiché dans le Wtext
    std::string mLabel;
    std::string mPathTif;
    //std::string mDirTuile;
    std::string mCode;

};

#endif // LAYER_H
