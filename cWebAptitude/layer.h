#ifndef LAYER_H
#define LAYER_H
#include <Wt/WObject.h>
#include <Wt/WTable.h>
#include <Wt/WCheckBox.h>
#include <Wt/WText.h>
#include "cdicoapt.h"
#include <Wt/WLabel.h>
#include "grouplayers.h"
#include <boost/algorithm/string/replace.hpp>


#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"

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

// ça va être un peu batard vu que je vais mélanger divers type de layer

enum TypeLayer {Apti
                ,KK // les cartes dérivées des CS
                ,Thematique // toutes les autres ; NH NT ZBIO IGN
                ,Externe // toutes les cartes qui ne sont pas en local ; carte IGN pour commencer
               };



class Layer
{
public:
    Layer(groupLayers * aGroupL, std::string aCode,WText * PWText,TypeLayer aType=Apti);
    Layer(groupLayers * aGroupL, cEss aEss ,WText * PWText);

    //void clickOnName(std::string aCode);
    void displayLayer();

    std::vector<std::string> displayInfo(double x, double y);
    // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
    std::map<std::string,int> computeStatOnPolyg(OGRGeometry * poGeom);
    GDALDataset * rasterizeGeom(OGRGeometry *poGeom);

    // raster value
    int getValue(double x, double y);
    // signification pour ce raster value
    std::string getValueTxt(double x, double y);
    // crop du raster avec un shp parcellaire
    bool cropIm(std::string aOut);
    int setActive(bool b=true){
        // ici je peux également changer le style du rendu du label
        mActive=b;
        mText->setStyleClass(mActive ? "currentEss" : "ess");
    }
    bool IsActive(){return mActive;}
    std::string getCode(){return mCode;}
    std::string getPathTif();
    std::string getLegendLabel();
    TypeLayer Type(){return mType;}

    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> * mDicoVal;

    // le dictionnaire des valeurs raster vers leur couleur, pour la légende
    std::map<int, color> * mDicoCol;

    color getColor(int aCode){
        color aRes(0,0,0);
            if (mDicoCol && mDicoCol->find(aCode)!=mDicoCol->end()){
                aRes=mDicoCol->at(aCode);}
        return aRes;
    }
    cEss * Ess(){return mEss;}
private:
    bool mActive;
    groupLayers * mGroupL;
    TypeLayer mType;
    cEss * mEss;
    cKKCS * mKK;
    cRasterInfo * mRI;
    Wt::WCheckBox *mCheck;
    cDicoApt * mDico;
    WText * mText; // ça c'est le lien entre l'objet et l'affichage dans la page web
    // le texte affiché dans le Wtext
    std::string mLabel;
    std::string mPathTif;
    std::string mDirTuile;
    std::string mCode;
    //std::string mCodeTuile;


    GDALDataset  * mGDALDat;
    GDALRasterBand * mBand;
};

#endif // LAYER_H
