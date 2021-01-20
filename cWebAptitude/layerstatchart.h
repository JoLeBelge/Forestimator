#ifndef LAYERSTATCHART_H
#define LAYERSTATCHART_H
#include <Wt/Chart/WPieChart.h>
#include <Wt/Chart/WCartesianChart.h>
#include <Wt/WItemDelegate.h>
#include <Wt/Chart/WDataSeries.h>
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WEnvironment.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WTableView.h>
#include "layer.h"

using namespace Wt;
using namespace Wt::Chart;

#include <iomanip>

std::string dToStr(double d);
std::string nth_letter(int n);


class layerStat;
class layerStatChart;
//class olOneLay;// plus utilisé, remplacé par staticMap
class staticMap;
class batonnetApt;

class batonnetApt : public Wt::WPaintedWidget
{
public:
    batonnetApt(layerStat * alayStat,std::map<std::string,std::string>  Dico_AptFull2AptAcro)
        : WPaintedWidget(),mLayStat(alayStat),mW(500),mH(40),aptFull2aptAcro(Dico_AptFull2AptAcro)
    {
    resize(mW, mH);
    update();
    }

protected:
    void paintEvent(Wt::WPaintDevice *paintDevice);

private:
    layerStat * mLayStat;
    std::map<std::string,std::string> aptFull2aptAcro;
    int mW,mH;
};


// une carte statique mais généré au format img en local (server side) car ol ça commence à fair ch*
// en remplacement de olOneLay, plus de flexibilité et format img server-side pour export en pdf, avec choix de la résolution
class  staticMap
{
public:
    // constructeur ; a besoin d'un extend et de quoi créer la variable js de la carte, donc pointer vers le layer
    staticMap(std::shared_ptr<layerBase> aLay,OGRGeometry *poGeom, OGREnvelope * env=NULL);
    double xGeo2Im(double x);
    double yGeo2Im(double y);
    std::string getFileName(){return mFileName;}
    //Wt::WLink getWLink(){return Wt::WLink(mFileNameRel);}
    // le pdf renderer il aime pas les chemin en relatif pour les image
    Wt::WLink getWLinkRel(){return Wt::WLink(mFileNameRel);}
    Wt::WLink getWLink(){return Wt::WLink(mFileName);}

    void drawPol(OGRPolygon * pol, Wt::WPainter * painter);
    // ajoute un polygone par après, l'image existe déjà
    void addPols(std::vector<OGRPolygon *> vpol, Wt::WColor col= Wt::StandardColor::DarkYellow);
    // ajout d'un logo en bas à doite - consigne IGN
    void addImg(std::string afileName);
    void drawScaleLine(WPainter *painter);

private:
    std::shared_ptr<layerBase> mLay;
    std::string mFileName,mFileNameRel;
    OGREnvelope * ext;
    // taille de l'image en pixel
    int mSx,mSy;
    // taille de l'emprise de l'image en mètre
    double mWx,mWy;
};

// une carte statique , donc sans interaction avec l'utilisateur
/*
class  olOneLay: public Wt::WContainerWidget
{
public:
    // constructeur ; a besoin d'un extend et de quoi créer la variable js de la carte, donc pointer vers le layer
    olOneLay(Layer * aLay,OGRGeometry *poGeom);

    virtual void layoutSizeChanged(int width, int height)
    {
        WContainerWidget::layoutSizeChanged(width, height);
        // Force a recalculation of the map viewport size. This should be called when third-party code changes the size of the map viewport
        // je ne connais pas la portée des variable mapStat (il y en a +ieur map du mm nom) mais j'essaie
        std::cout << "layoutSizeChanged \n\n" << std::endl;
        doJavaScript("mapStat"+id()+".updateSize();");
    }
private:
    Layer * mLay;
};
*/
// va contenir le titre, le tableau et le pie chart pour permettre une visualisation des statistiques calculé pour chacune des couches, typiquement les aptitudes des essences

class layerStatChart : public layerStat
{
public:
    layerStatChart(std::shared_ptr<Layer> aLay, std::map<std::string,int> aStat, OGRGeometry * poGeom);
    /*layerStatChart(const layerStatChart &ls):layerStat(ls){
        std::cout << "construct by copy layerStatChart " << std::endl;
        mModel=std::move(ls.mModel);
        rowAtMax=ls.rowAtMax;
        mTable=ls.mTable;
        mChart=ls.mChart;
    }*/
    // chart ; une carte individuelle + tableau
    std::unique_ptr<WContainerWidget> getChart(bool forRenderingInPdf=0);
    // barstat; pour aptitude, les statistiques des aptitudes résumées sous forme de battonnet
    std::unique_ptr<WContainerWidget> getBarStat();
    bool deserveChart();
private:
    std::shared_ptr<WStandardItemModel> mModel; // est partagé avec le conteneur de résultat qui est envoyé à la fenetre statwindow
    int rowAtMax;
    // j'ai besoin de la géometrie pour la carte statique openlayer
    OGRGeometry * mGeom;
};


#endif // LAYERSTATCHART_H
