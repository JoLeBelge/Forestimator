#ifndef GROUPLAYERS_H
#define GROUPLAYERS_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WSignal.h>
#include <Wt/WIconPair.h>
#include <Wt/WText.h>
#include <Wt/WTree.h>
#include <Wt/WTreeTable.h>
#include <Wt/WTreeTableNode.h>
#include <Wt/WCheckBox.h>
#include <Wt/WMessageBox.h>

#include "layer.h"
#include "legend.h"
#include <fstream>
#include "wopenlayers.h"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <Wt/WProgressBar.h>
#include "layerstatchart.h"

class WOpenLayers;
class Layer;
class groupLayers;
class legend;
class ST;
class layerStatChart;
class rasterFiles;
class selectLayers;
class selectLayers4Stat;
class selectLayers4Download;
//class WCheckBox;
enum class TypeLayer;

enum TypeClassifST {FEE
                    ,CS
                   };

using namespace Wt;


class ST{
public:
    ST(cDicoApt * aDico);
    void vider();
    std::string NH(){ return mDico->NH(mNH);}
    std::string NT(){ return mDico->NT(mNT);}
    std::string TOPO(){ return mDico->TOPO(mTOPO);}
    std::string ZBIO(){ return mDico->ZBIO(mZBIO);}
    std::string STATION(){return mDico->station(mZBIO,mSt);}

    bool hasNH(){ return mDico->NH()->find(mNH)!=mDico->NH()->end();}
    bool hasNT(){ return mDico->NT()->find(mNT)!=mDico->NT()->end();}
    bool hasZBIO(){ return mDico->ZBIO()->find(mZBIO)!=mDico->ZBIO()->end();}
    bool hasTOPO(){ return mDico->topo()->find(mTOPO)!=mDico->topo()->end();}
    bool hasST(){ return mDico->station(mZBIO,mSt)!="not found";}
    bool readyFEE(){ return hasNH() && hasNT() && hasZBIO() && hasTOPO();}
    bool readyCS(){ return hasZBIO() && hasST();}
    bool hasEss(){ return HaveEss;}

    int mNH,mNT,mZBIO,mTOPO;
    bool HaveEss;
    cEss * mActiveEss; // l'essence qui intéresse l'utilisateur
    cDicoApt * mDico;
    // catalogue de station
    int mSt;
private:

};


class selectLayers : public WContainerWidget{
public:
    selectLayers(Wt::WContainerWidget * aParent, std::vector<Layer*> aVpLs,int aMax):mParent(aParent),mVpLs(aVpLs),nbMax(aMax){}
    // retourne l'arbre avec listing des cartes regroupées par groupes (Apt FEE, ect)
    std::vector<rasterFiles> getSelectedRaster();
    std::map<std::vector<std::string>,Layer*> getSelectedLayer();
    WContainerWidget * affiche(){return cont;}
    int numSelectedLayer(){
        int aRes(0);
        for (auto & kv: mSelectedLayers){
            if (kv.second==true){aRes++;}
        }
        return aRes;
    }
protected: // les classes qui héritent peuvent avoir accès

    // une map qui permet de lister les couches selectionnées.
    // vu que pour les apt j'ai deux couches possibles, cs et fee, la clé est un vecteur de deux elements , le premier le code layer, le deuxième le mode FEEvsCS
    // value ; boolean pour dire si la couche est sélectionnée par l'utilisateur
    std::map<std::vector<std::string>,bool> mSelectedLayers;
    // une map de pointeur vers checkbox qui est liée aux couches selectionnée
    std::map<std::vector<std::string>,Wt::WCheckBox*> mLayersCBox;

    Layer* getLayerPtr(std::vector<std::string> aCode);

    void SelectLayer(bool select,std::string aCode, std::string aMode="",bool afficheMsg=true);
    void SelectLayerGroup(bool select,TypeLayer aType,std::string aMode="");

    bool isSelected(std::string aCode, std::string aMode=""){
        bool aRes(0);
        std::vector<std::string> aKey={aCode,aMode};
        if (mSelectedLayers.find(aKey)!=mSelectedLayers.end()){
            aRes=mSelectedLayers.at(aKey);
        }
        return aRes;
    }
    std::vector<Layer*> mVpLs;// je vais faire une copie de ce vecteur , qui existera en 3 exemplaires donc (2 select layer + 1 grouplayer).mauvais idée non?
    WContainerWidget * cont;
    int nbMax;
    Wt::WContainerWidget * mParent;// il en faut un pour les messages box
};

class selectLayers4Stat : public selectLayers{
public:
    selectLayers4Stat(groupLayers * aGL);
private:
      Wt::WTreeTable * treeTable;
      groupLayers * mGL;
};
class selectLayers4Download : public selectLayers{
public:
    selectLayers4Download(groupLayers * aGL);
private:
      Wt::WTreeTable * treeTable;
      groupLayers * mGL;
};

class groupLayers: public WContainerWidget
{
public:
    groupLayers(cDicoApt * aDico,WContainerWidget *parent,WContainerWidget *infoW,WOpenLayers * aMap, Wt::WApplication* app);
    ~groupLayers(){std::cout << " destructeur de group layer " << std::endl;}
    void clickOnName(std::string aCode);
    void changeClassClick(WText *t);
    void update(std::string aCode);
    // click de l'utilisateur sur la carte pour extraire les valeurs des raster pour une position donnée
    void extractInfo(double x, double y);
    cDicoApt * Dico(){return mDico;}
    TypeClassifST TypeClas(){return mTypeClassifST;}
    std::string TypeClasStr(){
        std::string aRes("");
        switch (mTypeClassifST) {
        case FEE:
          aRes="FEE";
            break;
        case CS:
             aRes="CS";
            break;
        }
        return aRes;
    }

    // clé 1 ; nom de la couche. clé2 : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
    std::map<std::string,std::map<std::string,int>> computeStatGlob(OGRGeometry *poGeomGlobale);
    //void visuStat();
    // void car on ajoute les résulats à la table d'attribut de la couche
    void computeStatOnPolyg(OGRLayer * lay, bool mergeOT=0);

    // ne fait pas ce que je veux, il faut apparemment utiliser des anchor pour faire du bookmarking / hashtag
    Wt::Signal<bool>& focusMap(){
        //std::cout << "focus map () dans grouplayer done \n\n\n" << std::endl;
        return focusOnMap_;}
    ST * mStation;
    std::vector<Layer> * Layers(){return & mVLs;}

    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    //key ; code essence. Value ; code aptitude
    std::map<std::string,int> apts();

    Wt::WProgressBar *mPBar;
    // pour faire un processEvent, seul moyen de refresh de la progressbar.
    Wt::WApplication* m_app;

    std::vector<layerStatChart*> ptrVLStat() {return mVLStat;}

    std::vector<Layer*> getVpLs(){
     std::vector<Layer*> aRes;
     for (auto & l : mVLs){
         aRes.push_back(&l);
     }
     return aRes;
    }

    std::vector<rasterFiles> getSelect4Download(){return mSelect4Download->getSelectedRaster();}
    std::vector<rasterFiles> getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}
    WContainerWidget * afficheSelect4Stat(){return mSelect4Stat->affiche();}
    WContainerWidget * afficheSelect4Download(){return mSelect4Download->affiche();}
    int getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
    int getNumSelect4Download(){return mSelect4Download->numSelectedLayer();}
    std::map<std::vector<std::string>,Layer*> getSelectedLayer4Stat(){return mSelect4Stat->getSelectedLayer();}
    std::map<std::vector<std::string>,Layer*> getSelectedLayer4Download(){return mSelect4Download->getSelectedLayer();}

private:
    TypeClassifST mTypeClassifST;
    std::string currentClassifST; // 2 modes de classification des stations forestières ; FEE et CS
    std::vector<WText *> clasLabels_;
    std::vector<Layer> mVLs;
    cDicoApt * mDico;

    Wt::WTable                 *mEssTable;
    Wt::WTable                 *mClassifTable;
    Wt::WTable                 *mOtherTable;
    legend * mLegend;

    Wt::Signal<bool> focusOnMap_;

    WContainerWidget * mInfoW;
    //WWidget * mInfoW;

    // pour changer le curseur quand on clique
    WOpenLayers * mMap;
    // bof finalement c'est mieux le conteneur parent
    Wt::WContainerWidget     * mParent;


    std::vector<layerStatChart*> mVLStat;
    selectLayers4Stat * mSelect4Stat;
    selectLayers4Download * mSelect4Download;
};

#endif // GROUPLAYERS_H
