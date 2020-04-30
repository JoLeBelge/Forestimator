#ifndef PARCELLAIRE_H
#define PARCELLAIRE_H
#include "grouplayers.h"

#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WFileUpload.h>
#include <Wt/WProgressBar.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WProgressBar.h>
#include <Wt/WSignal.h>
#include <Wt/WEnvironment.h>
#include <Wt/WGridLayout.h>
#include "Wt/WFileResource.h"

#include "boost/filesystem.hpp"
#include <functional> //--> pour bind
#include "libzippp.h"
using namespace libzippp;

// objet qui comprend le shp d'un parcellaire DNF que l'on va afficher dans openlayer
// cet objet contient également l'interface graphique wt avec les bouttons qui permettent de charger le shp, les bouttons pour démarrer les calcul, ect

class parcellaire: public WContainerWidget
{
public:
    parcellaire(WContainerWidget *parent, groupLayers * aGL, Wt::WApplication* app,Wt::WStackedWidget * aTopStack,WContainerWidget * statW);
    ~parcellaire();
    void cleanShpFile();
    // conversion shp esri vers geoJson
    bool toGeoJson();
    void display();
    // effectue des vérification du shp (polygone, src)
    void checkShp();
    // merge de tout les polygones pour avoir une géométrie globale
    void computeGlobalGeom(OGRLayer * lay);
    // rasterize une géométrie
    //void rasterizeGeom(OGRGeometry *poGeom);
    void computeStat();
    void visuStat();
    void upload();
    void clickUploadBt();
    void fuChanged();

    bool hasShp(){return hasValidShp;}

    std::string geoJsonName();
    std::string geoJsonRelName();

    Wt::Signal<int>& changePage() { return page_; }

    void downloadShp();

private:

    // Full path ; là ou est sauvé le shp localement, mName ; le nom du shp tels qu'il était chez le client
    std::string mFullPath, mName,mClientName;
    std::string mJSfile;
    Wt::WContainerWidget     * mParent;
    Wt::WContainerWidget * mStatW;
    Wt::WFileUpload *fu;
    Wt::WPushButton *uploadButton,*computeStatButton, *visuStatButton, *downloadShpBt;
    Wt::WApplication* m_app;
    Wt::WText * msg;
    groupLayers * mGL;
    cDicoApt  * mDico;
    double centerX,centerY;
    bool hasValidShp;

    OGRGeometry *poGeomGlobale;

    Wt::Signal<int> page_;
    Wt::WStackedWidget * mTopStack;

    Wt::WCheckBox *mCB_fusionOT;

};

#endif // PARCELLAIRE_H
