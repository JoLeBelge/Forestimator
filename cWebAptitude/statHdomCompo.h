#ifndef LSTATCONTCHART_H
#define LSTATCONTCHART_H

#include <Wt/WItemDelegate.h>
#include <Wt/Chart/WDataSeries.h>
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WTableView.h>
#include "layer.h"
//#include "grouplayers.h"
class statHdom; // similaire à layerSatChart mais dédié au HDOM
class statCompo; // différent car va devoir utiliser plusieurs layers.

class basicStat; // forward declaration

// renvoie une grille de point, centre d'hexagone
std::vector<OGRPoint> hexbin(GDALDataset * mask);
// renvoie une liste de polgone qui sont des hexagones
std::vector<OGRPolygon*> hexGeombin(GDALDataset *mask);
double predHdom(std::vector<double> aVHs);

/*
// la méthode Centroid de OGR bugge, alors je fait la mienne - plus nécessaire
OGRPoint *getCentroid(OGRPolygon * hex);
bool IsInsideHexagon(float x0, float y0, float d, float x, float y) ;
bool InsideHexagonB(float x0, float y0, float x, float y, float d);
*/


// pour les stat sur un MNH
class statHdom {
public:
    statHdom(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom);
    ~statHdom(){
       for (OGRPolygon * pol: mVaddPol) OGRGeometryFactory::destroyGeometry(pol);
       mVaddPol.clear();
       mStat.clear();
       mDistFrequ.clear();
    }
    std::shared_ptr<layerBase> Lay(){return mLay;}
    bool deserveChart();
    cDicoApt * Dico();

    void predictHdom();
    //std::map<std::string, double> computeDistrH();
    std::vector<std::pair<std::string,double>> computeDistrH();
    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    std::unique_ptr<Wt::WContainerWidget> getResult();

    basicStat bshdom(){return basicStat(mStat);}

private:
    std::shared_ptr<layerBase> mLay;
    std::vector<double> mStat;
    //std::vector<std::string,double>
    std::vector<std::pair<std::string,double>>mDistFrequ;// pair avec range de valeur (genre 3-9) et proportion de la distribution
    OGRGeometry * mGeom;
    // geometrie supplémentaire à afficher sur l'image statique
    std::vector<OGRPolygon *> mVaddPol;
    int mNbOccurence;
};

class statCompo{
public:
    statCompo(cDicoApt * aDico, OGRGeometry * poGeom);
    // utilisation API ; pas spécialement toutes les essences
    statCompo(std::vector<std::shared_ptr<layerBase>> VlayCompo, cDicoApt * aDico, OGRGeometry * poGeom);

    basicStat computeStatWithMasq(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom);
    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    std::unique_ptr<Wt::WContainerWidget> getResult();
    std::string getAPIresult();
private:
    std::vector<std::shared_ptr<layerBase>> mVLay;
    OGRGeometry * mGeom;
    cDicoApt * mDico;
};
#endif // LSTATCONTCHART_H
