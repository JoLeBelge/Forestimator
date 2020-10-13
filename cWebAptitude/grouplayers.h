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
#include "simplepoint.h"
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
#include "selectlayers.h"
#include "cnsw.h"

class WOpenLayers;
class Layer;
class groupLayers;
class simplepoint;
class ST;
class layerStatChart;
class rasterFiles;
class selectLayers;
class selectLayers4Stat;
class selectLayers4Download;
class stackInfoPtr;

enum TypeClassifST {FEE
                    ,CS
                   };

using namespace Wt;
using namespace libzippp;

bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext);

class groupLayers: public WContainerWidget
{
public:
    groupLayers(cDicoApt * aDico,WOpenLayers * aMap, AuthApplication* app,stackInfoPtr * aStackInfoPtr);
    //~groupLayers();
    /*groupLayers(const groupLayers &gl){
        std::cout << "construct by copy group layer -- should never happend\n\n\n" << std::endl;
    }*/
    void clickOnName(std::string aCode, TypeLayer type);

    // update du rendu du nom de la couche qui est sélectionnée
    void update(std::string aCode, TypeLayer type);
    // update pour passer du mode expert au mode non expert et vice et versa
    void updateGL(bool expertMode=0);
    // click de l'utilisateur sur la carte pour extraire les valeurs des raster pour une position donnée
    void extractInfo(double x, double y);
    cDicoApt * Dico(){return mDico;}
    TypeClassifST TypeClas(){return mTypeClassifST;}
    std::string TypeClasStr(){ // pour afficher dans le titre du tableau d'aptitude
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

    //Wt::WProgressBar *mPBar;
    // pour faire un processEvent, seul moyen de refresh de la progressbar.
    AuthApplication* m_app;

    std::vector<layerStatChart*> ptrVLStat() {return mVLStat;}

    std::vector<rasterFiles> getSelect4Download();
    std::vector<rasterFiles> getSelect4Stat();

    // 28 septembre 2020 , Philippe lache l'affaire et on retire les analyses simples qui portent sur l'ajout d'une colonne du shp
    //WContainerWidget * afficheSelect4Stat();
    WContainerWidget * afficheSelect4Download();
    int getNumSelect4Stat();
    int getNumSelect4Download();
    std::vector<Layer *> getSelectedLayer4Stat();
    std::vector<Layer *> getSelectedLayer4Download();
    //std::vector<Layer *> getAllLayer();
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
    Wt::WContainerWidget * mExtentDivGlob; // le glob contient le boutton et le extentDiv
    Wt::WContainerWidget * mExtentDiv;
    Wt::WLineEdit * tb_extent_name;
    Wt::WText * mTitle;
    Wt::WTable * mLegendIndiv;
    stackInfoPtr * mStackInfoPtr;

    void saveExtent(double c_x, double c_y, double zoom);
    void deleteExtent(std::string id);

    selectLayers4Stat * mSelect4Stat;
    selectLayers4Download * mSelect4Download;

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool>& changeExpertMode() { return expertMode_; }


private:
    TypeClassifST mTypeClassifST; // 2 modes de classification des stations forestières ; FEE et CS. important de savoir le mode pour savoir quel tableau d'aptitude afficher quand on double-click sur une station

    cDicoApt * mDico;
    simplepoint * mLegend;
    sqlite3 *db_;

    // bof finalement c'est mieux le conteneur parent
    Wt::WContainerWidget     * mParent;

    std::vector<layerStatChart*> mVLStat;

    JSignal<double, double, double, double>& getMapExtendSignal(){return mapExtent_; }
    JSlot slot;
    JSignal<double, double, double, double>  mapExtent_;
    OGREnvelope mMapExtent;
    void updateMapExtentAndCropIm(double topX, double topY, double bottomX, double bottomY){
      updateMapExtent(topX, topY, bottomX, bottomY);
      exportLayMapView();
    }

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool> expertMode_;

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
