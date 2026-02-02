#ifndef PARCELLAIRE_H
#define PARCELLAIRE_H
#pragma once

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
#include "Wt/WLoadingIndicator.h"
#include "statwindow.h"
#include "widgetcadastre.h"
#include "tools/tools.hpp"
#include <Wt/Mail/Message.h>
#include <Wt/Mail/Client.h>
#include <memory>

#include "boost/filesystem.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional> //--> pour bind
#include "./libzippp/src/libzippp.h"
using namespace libzippp;

#include "threadpool/ThreadPool.hpp"

class groupLayers;
class statWindow;
class cDicoApt;

extern Wt::WServer *globServer;

// objet qui comprend le shp d'un parcellaire DNF que l'on va afficher dans openlayer
// cet objet contient également l'interface graphique wt avec les bouttons qui permettent de charger le shp, les bouttons pour démarrer les calcul, ect

class parcellaire : public WContainerWidget
{
public:
    parcellaire(groupLayers *aGL, cWebAptitude *app, statWindow *statW);
    ~parcellaire();
    void cleanShpFile();
    bool to31370AndGeoJson(); // boite de dialogue pour choisir src si jamais la couche n'en a pas de défini
    void to31370AndGeoJsonGDAL();
    void display();
    // effectue des vérification du shp (polygone, src)
    void checkShp();
    // merge de tout les polygones pour avoir une géométrie globale et y calculer la surface totale
    bool computeGlobalGeom(std::string extension = "", bool limitSize = 1);

    void visuStat(OGRFeature *poFeature);
    void upload();
    void clickUploadBt();
    void fuChanged();

    void selectPolygon(double x, double y);

    void computeStatAndVisuSelectedPol(int aId);

    bool hasShp() { return hasValidShp; }

    std::string geoJsonName();
    std::string geoJsonRelName();
    std::string fileName();

    // télécharger plusieurs carte à la fois
    void downloadRaster();
    void anaAllPol();
    bool cropImWithShp(std::shared_ptr<layerBase> l, std::string aOut);

    // une méthode pour utiliser le polygone issu d'une recherche sur le cadastre. Un seul polygone, format geojson - et les signaux qui vont avec pour la communicaiton avec widgetCadastre.
    void polygoneCadastre(std::string aFileGeoJson, std::string aLabelName = "");

    statWindow *mStatW;

    void doComputingTask();

    class TaskComputing : public Task
    {
        std::string geoJsonName;
        groupLayers *mGL;
        std::string path;
        cWebAptitude **app;
        void run() override;

    public:
        TaskComputing(std::string geoJsonName, groupLayers *mGL, std::string path, cWebAptitude **app) : geoJsonName(geoJsonName), mGL(mGL), path(path), app(app)
        {}
    };

private:
    // Full path ; là ou est sauvé le shp localement, mName ; le nom du shp tels qu'il était chez le client
    std::string mFullPath, mName, mClientName, mExtention;
    std::string mLabelName; // visible pour rapport
    Wt::WContainerWidget *mContSelect4D;

    Wt::WFileUpload *fu;
    Wt::WPushButton *downloadRasterBt;
    Wt::WPushButton *anaOnAllPolygBt;
    // Wt::WPushButton  *anaOnAllPolygBt;
    cWebAptitude *m_app;
    Wt::WText *msg;
    groupLayers *mGL;
    cDicoApt *mDico;
    double centerX, centerY;
    OGREnvelope mParcellaireExtent;
    bool hasValidShp;
    OGRGeometry *poGeomGlobale;
};

#endif // PARCELLAIRE_H
