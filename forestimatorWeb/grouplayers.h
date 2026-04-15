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
#include <Wt/WAnchor.h>
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
#include <memory>

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


using namespace Wt;
using namespace libzippp;

//bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext);

// retourne le dataset sur l'enveloppe d'un polygone
GDALDataset *getDSonEnv(std::string inputRaster, OGRGeometry *poGeom);

std::string getHtml(std::string);
bool isValidHtml(const std::string &);
bool isValidXmlIdentifier(const std::string &);


class groupLayers : public WContainerWidget
{
public:
    groupLayers(cWebAptitude * cWebApt);
    ~groupLayers();

    void clickOnName(std::string aCode);

    // update du rendu du nom de la couche qui est sélectionnée
    void updateActiveLay(std::string aCode);
    // update pour passer du mode expert au mode non expert et vice et versa
    void updateGL();

    cDicoApt *Dico() { return mDico; }
    void exportLayMapView();


    //  pour faire un processEvent, seul moyen de refresh de la progressbar.
    //cWebAptitude *m_app;

    WOpenLayers *mMap;
    cWebAptitude * m_app;

    // gestion de la légende de la carte

    void updateLegende(const std::shared_ptr<Layer> l);

    Wt::WContainerWidget *mLegendDiv;
    Wt::WContainerWidget *mExtentDivGlob; // le glob contient le boutton et le extentDiv
    Wt::WContainerWidget *mExtentDiv;
    Wt::WLineEdit *tb_extent_name;

    void saveExtent(double c_x, double c_y, double zoom);
    void deleteExtent(std::string id_extent);

    //selectLayers *mSelectLayers;

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool> &changeExpertMode() { return expertMode_; }

    OGREnvelope *getMapExtent() { return &mMapExtent; }


    //std::vector<std::shared_ptr<layerBase>> Layers() {return mVLs; }
    std::shared_ptr<Layer> getActiveLay();

    std::vector<std::shared_ptr<Layer>> mVLs;
    std::shared_ptr<Layer> getLay(std::string aCode);


    void closeConnection();
    int openConnection();
    bool getExpertModeForUser(std::string id);
    void loadExtents(std::string id);

    void updateLegendeDiv(std::vector<std::shared_ptr<Layer>> layers);

    JSlot slotMapExport;


private:

    cDicoApt *mDico;
    sqlite3 *db_;
    JSignal<double, double, double, double> sigMapExport;

    OGREnvelope mMapExtent;
    void updateMapExtentAndCropIm(double topX, double topY, double bottomX, double bottomY)
    {
        updateMapExtent(topX, topY, bottomX, bottomY);
        exportLayMapView();
    }
    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool> expertMode_;

    void updateMapExtent(double topX, double topY, double bottomX, double bottomY)
    {
        mMapExtent.MaxX = topX;
        mMapExtent.MaxY = topY;
        mMapExtent.MinX = bottomX;
        mMapExtent.MinY = bottomY;
        std::cout << "updateMapExtent grouplayer" << std::endl;
    }

    /*  Signal pour sauver l'extent de la map dans la DB pour un user connecté     */
    JSlot slotMapCenter;
    JSignal<double, double, double> sigMapCenter;
};

#endif // GROUPLAYERS_H
