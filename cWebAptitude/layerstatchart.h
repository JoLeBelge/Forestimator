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
#include "Magick++.h"
//#include <Wt/WRasterImage.h>

using namespace Wt;
using namespace Wt::Chart;



#include <iomanip>

std::string dToStr(double d);
std::string nth_letter(int n);
std::string getAbbreviation(std::string str);

class layerStat;
class layerStatChart;
class olOneLay;
// pour
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
    staticMap(Layer * aLay,OGRGeometry *poGeom);
    double xGeo2Im(double x);
    double yGeo2Im(double y);
    std::string getFileName(){return mFileName;}
    //Wt::WLink getWLink(){return Wt::WLink(mFileNameRel);}
    // le pdf renderer il aime pas les chemin en relatif pour les image
    Wt::WLink getWLinkRel(){return Wt::WLink(mFileNameRel);}
    Wt::WLink getWLink(){return Wt::WLink(mFileName);}

    void drawPol(OGRPolygon * pol);
    void drawScaleLine(Magick::Image *im);
    std::list<Magick::Drawable> drawList;

private:
    Layer * mLay;
    std::string mFileName,mFileNameRel;
    OGREnvelope * ext;
    // taille de l'image en pixel
    int mSx,mSy;
    // taille de l'emprise de l'image en mètre
    double mWx,mWy;
};

// une carte statique , donc sans interaction avec l'utilisateur
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
// va contenir le titre, le tableau et le pie chart pour permettre une visualisation des statistiques calculé pour chacune des couches, typiquement les aptitudes des essences
class layerStat
{
public:
    layerStat(Layer * aLay,std::map<std::string,int> aStat);
    layerStat(const layerStat &ls){
        std::cout << "construct by copy layerStat " << std::endl;
        mLay=ls.mLay;
        mStat=ls.mStat;
        mStatSimple=ls.mStatSimple;
        mTypeVar=ls.mTypeVar;
        mNbPix=ls.mNbPix;
    }

    void simplifieStat();
    std::string summaryStat();
    int getO(bool mergeOT=false);// proportion en optimum

    int getFieldVal(bool mergeOT=false);
    std::string getFieldValStr();

    Layer * Lay(){return mLay;}

    std::map<std::string, int> StatSimple(){return mStatSimple;}
    std::map<std::string, int> Stat(){return mStat;}
    TypeVar mTypeVar; // pour distinguer le type de variable, continue (MNH) ou classes (aptitude)
    cDicoApt * Dico();//{return mLay->Dico();}
protected:
    Layer * mLay;
    // key ; classe ou valeur, val ; nombre d'occurence
    std::map<std::string, int> mStat;
    std::map<std::string, int> mStatSimple;

    int mNbPix;
    std::string mMode; // fee vs cs

};

class layerStatChart : public layerStat
{
public:
    layerStatChart(Layer * aLay, std::map<std::string,int> aStat, OGRGeometry * poGeom);
    layerStatChart(const layerStatChart &ls):layerStat(ls){
        std::cout << "construct by copy layerStatChart " << std::endl;
        mModel=ls.mModel;
        rowAtMax=ls.rowAtMax;
        mTable=ls.mTable;
        mChart=ls.mChart;
    }
    // chart ; une carte individuelle + tableau + pie Chart
    Wt::WContainerWidget * getChart(bool forRenderingInPdf=0);
    // barstat; pour aptitude, les statistiques des aptitudes résumées sous forme de battonnet
    Wt::WContainerWidget * getBarStat();
    Chart::WPieChart * mChart;
    bool deserveChart(){return mStatSimple.size()>0;}
private:
    std::shared_ptr<WStandardItemModel> mModel;
    //Layer * mLay;

    WTableView * mTable;
    int rowAtMax;

    // j'ai besoin d'une enveloppe carto pour la carte statique openlayer
    //OGREnvelope mExt;
    OGRGeometry * mGeom;

};

#endif // LAYERSTATCHART_H
