#ifndef PARCELLAIRE_H
#define PARCELLAIRE_H
#include "grouplayers.h"

#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WFileUpload.h>
#include <Wt/WProgressBar.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

#include "boost/filesystem.hpp"

// objet qui comprend le shp d'un parcellaire DNF que l'on va afficher dans openlayer
// cet objet contient également l'interface graphique wt avec les bouttons qui permettent de charger le shp, les bouttons pour démarrer les calcul, ect

class parcellaire: public WContainerWidget
{
public:
    parcellaire(WContainerWidget *parent, groupLayers * aGL);
    ~parcellaire();
    void cleanShpFile();
    // conversion shp esri vers geoJson
    bool toGeoJson();
    void display();
    // effectue des vérification du shp (polygone, src)
    void checkShp();
    // merge de tout les polygones pour avoir une géométrie globale
    void computeGlobalGeom();
    // rasterize une géométrie
    //void rasterizeGeom(OGRGeometry *poGeom);
    void computeStat();
    void upload();
    void clickUploadBt();
    void fuChanged();

    std::string geoJsonName(){return mFullPath + ".geojson";}
    std::string geoJsonRelName(){ return "tmp/" + mName +".geojson";}

private:


    // Full path ; là ou est sauvé le shp localement, mName ; le nom du shp tels qu'il était chez le client
    std::string mFullPath, mName,mClientName;
    std::string * mJSfile;
    Wt::WContainerWidget     * mParent;
    Wt::WFileUpload *fu;
    Wt::WPushButton *uploadButton;
    Wt::WText * msg;
    groupLayers * mGL;
    cDicoApt  * mDico;
    double centerX,centerY;

    OGRGeometry *poGeomGlobale;
};

#endif // PARCELLAIRE_H
