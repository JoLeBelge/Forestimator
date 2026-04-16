#ifndef STATWINDOW_H
#define STATWINDOW_H
#include "cwebaptitude.h"
#include "layerstatchart.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <Wt/Render/WPdfRenderer.h>
#include <Wt/WApplication.h>

class statWindow;
class cWebAptitude;
class layerStatChart;

class statWindow : public Wt::WContainerWidget
{
public:
    // a besoin du dictionnaire pour créer le layer qui contient la carte IGN pour faire la carte de situation globale
    statWindow(cWebAptitude * aWebApp);

    // click de l'utilisateur sur la carte pour extraire les valeurs des raster pour une position donnée
    void extractInfo(double x, double y);
    void vider();
    void add1Aptitude(std::shared_ptr<layerStatChart> lstat);
    void add1layerStat(std::shared_ptr<layerStatChart> layerStat);
    void add1layerStat(std::unique_ptr<Wt::WContainerWidget> cont);
    void genIndivCarteAndAptT();
    void generateGenCarte(OGRFeature *poFeature);

    WText * mTitre;
    WTable * mAptTable;
    WPushButton * createPdfBut;
    WContainerWidget * mCarteGenCont;
    WContainerWidget * mAllStatIndivCont;

    cDicoApt * mDico;
    cWebAptitude * m_app;
private:


    Wt::WVBoxLayout * layout;

    // pour la carte de localisation
    std::shared_ptr<layerBase> mIGN;
    // pour les information générales
    std::shared_ptr<layerBase> mMNT, mZBIO, mPente;

};

class surfPdfResource : public Wt::WResource
{
public:

surfPdfResource(statWindow * aStatW) : WResource(), mSW(aStatW){}

    ~surfPdfResource()
    {
        beingDeleted();
    }
void handleRequest(const Http::Request &request, Http::Response &response);

private:
statWindow * mSW;

};

#endif // STATWINDOW_H
