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
class rasterFiles; // une classe dédiée uniquemnent à l'export et au clip des couches. Layer ne convient pas car contient une grosse partie pour l'affichage (tuiles et autre) et surtout qu'il y a deux couches par layer pour les apti


inline bool exists (const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
};



enum class TypeLayer {Apti
                ,KK // les cartes dérivées des CS
                ,Thematique // toutes les autres ; NH NT ZBIO
                ,Externe // toutes les cartes qui ne sont pas en local ; carte IGN pour commencer
               };


class rasterFiles{

public:
    rasterFiles(std::string aPathTif);
    // constructeur par copie et par déplacement ; indispensible si j'utilise les objets dans un vecteur. http://www-h.eng.cam.ac.uk/help/tpl/languages/C++/morevectormemory.html
    rasterFiles(const rasterFiles &rf){
        mPathTif=rf.mPathTif;
        mPathQml=rf.mPathQml;
    }
    rasterFiles(rasterFiles&& rf) noexcept {mPathTif=rf.mPathTif;mPathQml=rf.mPathQml;}

    std::string tif(){return mPathTif;}
    std::string symbology(){return mPathQml;}
    bool hasSymbology(){return mPathQml!="";}
private:
    std::string mPathTif, mPathQml;
    // pour exporter le dictionnaire dans un fichier texte si nécessaire
    std::map<int, std::string> mDicoVal;
};

// ça va être un peu batard vu que je vais mélanger divers type de layer
class Layer
{
public:
    Layer(groupLayers * aGroupL, std::string aCode,WText * PWText,TypeLayer aType=TypeLayer::Apti);
    Layer(groupLayers * aGroupL, cEss aEss ,WText * PWText);

    //void clickOnName(std::string aCode);
    void displayLayer();

    std::vector<std::string> displayInfo(double x, double y);
    // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
    std::map<std::string,int> computeStatOnPolyg(OGRGeometry * poGeom, std::string aMode="FEE");
    GDALDataset * rasterizeGeom(OGRGeometry *poGeom);

    // raster value
    int getValue(double x, double y);
    // signification pour ce raster value
    std::string getValueTxt(double x, double y);
    // crop du raster avec un shp parcellaire
    bool cropIm(std::string aOut);
    void setActive(bool b=true){
        // ici je peux également changer le style du rendu du label
        mActive=b;
        mText->setStyleClass(mActive ? "currentEss" : "ess");
    }
    bool IsActive(){return mActive;}
    std::string getCode(){return mCode;}
    std::string getPathTif();
    std::string getLegendLabel();
    std::string getShortLabel(){return mLabel;}

    // à cause de ma superbe idée de merde de mettre deux couches raster par layer, je dois surcharger ces méthodes pour pouvoir spécifier le mode Fee vs Cs
    std::vector<std::string> getCode(std::string aMode);
    std::string getPathTif(std::string aMode);
    std::string getLegendLabel(std::string aMode);

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

    color getColor(std::string aStrCode){
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
