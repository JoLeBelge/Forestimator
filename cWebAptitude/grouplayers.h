#ifndef GROUPLAYERS_H
#define GROUPLAYERS_H

#pragma once
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
#include <Wt/WLoadingIndicator.h>

#include "stackinfoptr.h"
#include "layer.h"
#include "legend.h"
#include <fstream>
#include "wopenlayers.h"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <Wt/WProgressBar.h>
#include "layerstatchart.h"
#include "Wt/WFileResource.h"
#include "./libzippp/src/libzippp.h"
#include <sqlite3.h>
#include <string.h>

#include "auth.h"

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
class stackInfoPtr;
//class WCheckBox;
//enum class TypeLayer;


enum TypeClassifST {FEE
                    ,CS
                   };

using namespace Wt;
using namespace libzippp;

extern bool ModeExpert;

bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext);


class selectLayers : public WContainerWidget{
public:
    selectLayers(Wt::WContainerWidget * aParent, std::vector<Layer*> aVpLs,int aMax):mParent(aParent),mVpLs(aVpLs),nbMax(aMax){}
    std::vector<rasterFiles> getSelectedRaster();
    std::map<std::vector<std::string>,Layer*> getSelectedLayer();
    // cas particulier ou je veux toutes les couches, qu'elles soient selectionnées ou pas
    std::map<std::vector<std::string>,Layer*> getAllLayer();
    // retourne l'arbre avec listing des cartes regroupées par groupes (Apt FEE, ect)
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
    groupLayers(cDicoApt * aDico,WOpenLayers * aMap, AuthApplication* app,stackInfoPtr * aStackInfoPtr);
    //~groupLayers();
    /*groupLayers(const groupLayers &gl){
        std::cout << "construct by copy group layer -- should never happend\n\n\n" << std::endl;
    }*/
    void clickOnName(std::string aCode, TypeLayer type);
    void changeClassClick(WText *t);

    // update du rendu du nom de la couche qui est sélectionnée
    void update(std::string aCode, TypeLayer type);
    void updateGL();
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

    void exportLayMapView();

    // clé 1 ; nom de la couche. clé2 : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
    //std::map<std::string,std::map<std::string,int>>
    void computeStatGlob(OGRGeometry *poGeomGlobale);

    // void car on ajoute les résulats à la table d'attribut de la couche
    void computeStatOnPolyg(OGRLayer * lay, bool mergeOT=0);

    ST * mStation;
    std::vector<Layer *> Layers(){ return mVLs;}
    std::vector<Layer*> getVpLs(){ return mVLs;}

    Layer * getActiveLay();
    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    //key ; code essence. Value ; code aptitude
    std::map<std::string,int> apts();

    Wt::WProgressBar *mPBar;
    // pour faire un processEvent, seul moyen de refresh de la progressbar.
    AuthApplication* m_app;

    std::vector<layerStatChart*> ptrVLStat() {return mVLStat;}


    std::vector<rasterFiles> getSelect4Download(){return mSelect4Download->getSelectedRaster();}
    std::vector<rasterFiles> getSelect4Stat(){return mSelect4Stat->getSelectedRaster();}
    WContainerWidget * afficheSelect4Stat(){return mSelect4Stat->affiche();}
    WContainerWidget * afficheSelect4Download(){return mSelect4Download->affiche();}
    int getNumSelect4Stat(){return mSelect4Stat->numSelectedLayer();}
    int getNumSelect4Download(){return mSelect4Download->numSelectedLayer();}
    std::map<std::vector<std::string>,Layer*> getSelectedLayer4Stat(){return mSelect4Stat->getSelectedLayer();}
    std::map<std::vector<std::string>,Layer*> getSelectedLayer4Download(){return mSelect4Download->getSelectedLayer();}
    std::map<std::vector<std::string>,Layer*> getAllLayer(){return mSelect4Download->getAllLayer();}
    std::vector<Layer *> mVLs;
    void closeConnection();
    int openConnection();
    bool getExpertModeForUser(std::string id);
    void loadExtents(std::string id);

    // pour changer le curseur quand on clique - public pour avoir accès depuis parcellaire
    WOpenLayers * mMap;

    // gestion de la légende de la carte
    void updateLegende(const Layer * l);
    Wt::WContainerWidget * mLegendDiv;
    Wt::WContainerWidget * mExtentDiv;
    Wt::WLineEdit * tb_extent_name;
    Wt::WText * mTitle;
    Wt::WTable * mLegendIndiv;
    stackInfoPtr * mStackInfoPtr;

    void saveExtent(double c_x, double c_y, double zoom);
    void deleteExtent(std::string id);
private:
    TypeClassifST mTypeClassifST;
    std::string currentClassifST; // 2 modes de classification des stations forestières ; FEE et CS
    std::vector<WText *> clasLabels_;

    cDicoApt * mDico;

    //Wt::WTable                 *mEssTable;
    //Wt::WTable                 *mClassifTable;
    //Wt::WTable                 *mOtherTable;
    legend * mLegend;
    sqlite3 *db_;

    // bof finalement c'est mieux le conteneur parent
    Wt::WContainerWidget     * mParent;

    std::vector<layerStatChart*> mVLStat;
    selectLayers4Stat * mSelect4Stat;
    selectLayers4Download * mSelect4Download;

    JSignal<double, double, double, double>& getMapExtendSignal(){return mapExtent_; }
    JSlot slot;
    JSignal<double, double, double, double>  mapExtent_;
    OGREnvelope mMapExtent;
    void updateMapExtentAndCropIm(double topX, double topY, double bottomX, double bottomY){
      updateMapExtent(topX, topY, bottomX, bottomY);
      exportLayMapView();
    }

    void updateMapExtent(double topX, double topY, double bottomX, double bottomY){
        mMapExtent.MaxX=topX;
        mMapExtent.MaxY=topY;
        mMapExtent.MinX=bottomX;
        mMapExtent.MinY=bottomY;
        std::cout << "updateMapExtent grouplayer" << std::endl;
    }

    JSlot slot_extent;
    JSignal<double, double, double>& getMapCenterSignal(){return mapExtent2_; }
    JSignal<double, double, double>  mapExtent2_;
};

#endif // GROUPLAYERS_H
