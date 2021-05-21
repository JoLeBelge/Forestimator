#ifndef GROUPLAYERS_H
#define GROUPLAYERS_H
#pragma once
#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WSignal.h>
#include <Wt/WText.h>
#include <Wt/WTable.h>
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
#include <Wt/WProgressBar.h>
#include "layerstatchart.h"
#include "Wt/WFileResource.h"
#include "./libzippp/src/libzippp.h"
#include <string.h>
#include "auth.h"
#include "selectlayers.h"
#include "statHdomCompo.h"

class WOpenLayers;
class Layer;
class groupLayers;
class simplepoint;
class ST;
class layerStatChart;
class statHdom;
class statCompo;
class rasterFiles;
class baseSelectLayers;
class selectLayers4Stat;
class selectLayers;
class stackInfoPtr;
class AuthApplication;
class groupStat;

enum TypeClassifST {FEE
                    ,CS
                   };

using namespace Wt;
using namespace libzippp;

bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext);
// pour la comparaison de deux enveloppes lors du crop d'une image
double getArea(OGREnvelope * env);

// retourne le dataset sur l'enveloppe d'un polygone
GDALDataset * getDSonEnv(std::string inputRaster, OGRGeometry *poGeom);

std::string getHtml(LayerMTD * lMTD);
bool isValidHtml(std::string text);

// groupLayers héritera de cette classe , ça me permet d'avoir un meilleur visu des membres dédiés aux clacul des statistiques de surface
class groupStat{
public:
    groupStat(){}
    ~groupStat(){clearStat();}
    std::vector<layerStatChart*> ptrVLStat() {return mVLStat;}
    std::vector<statHdom*> ptrVLStatCont() {return mVLStatCont;}
    std::unique_ptr<statCompo> mCompo;
protected:
    // un vecteur pour les statistique des cartes variables de classes (majoritaire)
    std::vector<layerStatChart*> mVLStat;
    // un autre pour les statistique des cartes variables continu (à commencer à MNH)
    std::vector<statHdom*> mVLStatCont;


    void clearStat(){
        // clear d'un vecteur de pointeur, c'est mal. d'ailleurs ça bug si on calcule plusieur fois des stat dans la mm session, à regler donc
        for (auto p : mVLStat)
        {
            delete p;
        }
        for (auto p : mVLStatCont)
        {
            delete p;
        }
        mVLStatCont.clear();
        mVLStat.clear();
        mCompo.reset();
    }

};

class groupLayers: public WContainerWidget, public groupStat
{
public:
    groupLayers(cDicoApt * aDico,WOpenLayers * aMap, AuthApplication* app,stackInfoPtr * aStackInfoPtr);
    ~groupLayers();
    /*groupLayers(const groupLayers &gl){
        std::cout << "construct by copy group layer -- should never happend\n\n\n" << std::endl;
    }*/
    void clickOnName(std::string aCode, TypeLayer type);

    // update du rendu du nom de la couche qui est sélectionnée
    void update(std::string aCode, TypeLayer type);
    // update pour passer du mode expert au mode non expert et vice et versa
    void updateGL();
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
    std::vector<std::shared_ptr<Layer>> Layers(){ return mVLs;}
    std::shared_ptr<Layer> getActiveLay();

    std::shared_ptr<Layer> getLay(std::string aCode);
    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    //key ; code essence. Value ; code aptitude
    std::map<std::string,int> apts();

    //Wt::WProgressBar *mPBar;
    // pour faire un processEvent, seul moyen de refresh de la progressbar.
    AuthApplication* m_app;

    std::vector<rasterFiles> getSelect4Download();
    //std::vector<rasterFiles> getSelect4Stat();

    // 28 septembre 2020 , Philippe lache l'affaire et on retire les analyses simples qui portent sur l'ajout d'une colonne du shp
    //WContainerWidget * afficheSelect4Stat();
    WContainerWidget * afficheSelect4Download();
    //int getNumSelect4Stat();
    int getNumSelect4Download();
    //std::vector<std::shared_ptr<Layer>> getSelectedLayer4Stat();
    std::vector<std::shared_ptr<Layer>> getSelectedLayer4Download();


    void closeConnection();
    int openConnection();
    bool getExpertModeForUser(std::string id);
    void loadExtents(std::string id);

    // pour changer le curseur quand on clique - public pour avoir accès depuis parcellaire
    WOpenLayers * mMap;

    // gestion de la légende de la carte
    void updateLegende(const std::shared_ptr<Layer> l);
    Wt::WContainerWidget * mLegendDiv;
    Wt::WContainerWidget * mExtentDivGlob; // le glob contient le boutton et le extentDiv
    Wt::WContainerWidget * mExtentDiv;
    Wt::WLineEdit * tb_extent_name;
    Wt::WText * mTitle;
    Wt::WTable * mLegendIndiv;
    stackInfoPtr * mStackInfoPtr;

    void saveExtent(double c_x, double c_y, double zoom);
    void deleteExtent(std::string id);

    //selectLayers4Stat * mSelect4Stat;
    selectLayers * mSelectLayers;

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool>& changeExpertMode() { return expertMode_; }

    OGREnvelope * getMapExtent(){return & mMapExtent;}

private:

    std::vector<std::shared_ptr<Layer>> mVLs;
    TypeClassifST mTypeClassifST; // 2 modes de classification des stations forestières ; FEE et CS. important de savoir le mode pour savoir quel tableau d'aptitude afficher quand on double-click sur une station

    cDicoApt * mDico;
    simplepoint * mAnaPoint;
    sqlite3 *db_;

    // bof finalement c'est mieux le conteneur parent
    Wt::WContainerWidget     * mParent;
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

    /*  Signal pour sauver l'extent de la map dans la DB pour un user connecté     */
    JSlot slotMapCenter;
    JSignal<double, double, double>  sigMapCenter;
};


#endif // GROUPLAYERS_H
