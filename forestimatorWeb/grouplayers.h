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
#include <Wt/WPanel.h>
#include "cwebaptitude.h"
#include "layer.h"
#include "simplepoint.h"
#include <fstream>
#include "wopenlayers.h"
#include <Wt/WProgressBar.h>
#include "layerstatchart.h"
#include "Wt/WFileResource.h"
#include "./libzippp/src/libzippp.h"
#include <string.h>
#include "selectlayers.h"


class WOpenLayers;
class Layer;
class groupLayers;
class simplepoint;
class ST;
class layerStatChart;
class rasterFiles;
class baseSelectLayers;
class selectLayers4Stat;
class selectLayers;
class cWebAptitude;
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

std::string getHtml(std::string);
bool isValidHtml(std::string);
bool isValidXmlIdentifier(std::string);

// groupLayers héritera de cette classe , ça me permet d'avoir un meilleur visu des membres dédiés aux clacul des statistiques de surface
class groupStat{
public:
    groupStat(){}
    ~groupStat(){clearStat();}
    std::vector<std::shared_ptr<layerStatChart>> ptrVLStat() {return mVLStat;}
    // j'y met directement le conteneur résultats
    std::vector<std::unique_ptr<Wt::WContainerWidget>> mVLStatCont;
protected:
    // un vecteur pour les statistique des cartes variables de classes (majoritaire)
    std::vector<std::shared_ptr<layerStatChart>> mVLStat;

    void clearStat(){
        for (std::shared_ptr<layerStatChart> p : mVLStat)
        {
            p.reset();
        }
        mVLStatCont.clear();
        mVLStat.clear();
    }
};

class groupLayers: public WContainerWidget, public groupStat
{
public:
    groupLayers(cWebAptitude * cWebApt);
    ~groupLayers();

    void clickOnName(std::string aCode);

    // update du rendu du nom de la couche qui est sélectionnée
    // gestion de la carte active ; celle qui est en haut du panier
    void updateActiveLay(std::string aCode);
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

    // avec un retour qui est un fichier texte à télécharger
    void computeStatAllPol(OGRLayer * lay, WFileResource *fileResource);

    ST * mStation;
    std::vector<std::shared_ptr<Layer>> Layers(){ return mVLs;}
    std::shared_ptr<Layer> getActiveLay();

    std::shared_ptr<Layer> getLay(std::string aCode);
    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    //key ; code essence. Value ; code aptitude
    std::map<std::string,int> apts();

    //Wt::WProgressBar *mPBar;
    // pour faire un processEvent, seul moyen de refresh de la progressbar.
    cWebAptitude* m_app;

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
    void updateLegendeDiv(std::vector<std::shared_ptr<Layer>> layers);
    void updateLegende(const std::shared_ptr<Layer> l);

    Wt::WContainerWidget * mLegendDiv;
    Wt::WContainerWidget * mExtentDivGlob; // le glob contient le boutton et le extentDiv
    Wt::WContainerWidget * mExtentDiv;
    Wt::WLineEdit * tb_extent_name;
    Wt::WText * mTitle;
    cWebAptitude * mcWebAptitude;

    void saveExtent(double c_x, double c_y, double zoom);
    void deleteExtent(std::string id_extent);

    //selectLayers4Stat * mSelect4Stat;
    selectLayers * mSelectLayers;

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool>& changeExpertMode() { return expertMode_; }

    OGREnvelope * getMapExtent(){return & mMapExtent;}

    JSlot slotMapExport;
    simplepoint * mAnaPoint;

    // bof finalement c'est mieux le conteneur parent
    Wt::WContainerWidget     * mParent;

    TypeClassifST mTypeClassifST; // 2 modes de classification des stations forestières ; FEE et CS. important de savoir le mode pour savoir quel tableau d'aptitude afficher quand on double-click sur une station

private:

    std::vector<std::shared_ptr<Layer>> mVLs;
    cDicoApt * mDico;
    sqlite3 *db_;
    JSignal<double, double, double, double>  sigMapExport;
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
