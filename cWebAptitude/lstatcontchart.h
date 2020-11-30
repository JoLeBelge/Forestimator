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
class lStatContChart; // similaire à layerSatChart mais dédié au var continue type hdom prédit

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

enum TypeStat {HDOM
               ,COMPO
               };

// pour les stat des var continue, à commencer par hdom prédit sur grille
class lStatContChart {
public:
    lStatContChart(std::shared_ptr<Layer> aLay, OGRGeometry * poGeom, TypeStat aType);
    ~lStatContChart(){
       for (OGRPolygon * pol: mVaddPol) delete pol;
       mVaddPol.clear();
       mStat.clear();
       mDistFrequ.clear();
    }
    std::shared_ptr<Layer> Lay(){return mLay;}
    bool deserveChart(){return mStat.size()>0;}
    cDicoApt * Dico();

    // pour hdom
    void predictHdom();
    //std::map<std::string, double> computeDistrH();
    std::vector<std::pair<std::string,double>> computeDistrH();

    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    std::unique_ptr<Wt::WContainerWidget> getResult();

private:
    std::shared_ptr<Layer> mLay;
    std::vector<double> mStat;
    //std::vector<std::string,double>
    std::vector<std::pair<std::string,double>>mDistFrequ;// pair avec range de valeur (genre 3-9) et proportion de la distribution
    OGRGeometry * mGeom;
    // geometrie supplémentaire à afficher sur l'image statique
    std::vector<OGRPolygon *> mVaddPol;
    int mNbOccurence;
    TypeStat mType;
};
#endif // LSTATCONTCHART_H
