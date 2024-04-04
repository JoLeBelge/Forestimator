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

class statCellule; // info dendrométrique stoquée pour une cellule d'un are. on fait tourner les modèles dendro pour chaque cellule et puis effectue une aggrégation.

class basicStat; // forward declaration

/*
// la méthode Centroid de OGR bugge, alors je fait la mienne - plus nécessaire
OGRPoint *getCentroid(OGRPolygon * hex);
bool IsInsideHexagon(float x0, float y0, float d, float x, float y) ;
bool InsideHexagonB(float x0, float y0, float x, float y, float d);
*/


// pour les stat sur un MNH
class statHdom :public statHdomBase {
public:
    statHdom(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom,bool computeStat=1);
    bool deserveChart();
    void prepareResult();

    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    std::unique_ptr<Wt::WContainerWidget> getResult(){ return std::move(mResult); }
protected:
    std::unique_ptr<WContainerWidget> mResult;
};

// pour les estimation dendrométriques : je fait une autre classe dérivée. Mais finalement il semblerai que deux classes entièrement séparée serai mieux, surtout à partir du moment ou l'estimation avec une seule cellule par polygone est appliquée (cad modèle pixel au lieu des modèles parcelle)
// en fait statHom doit rester la classe mère car c'est cette classe qui est utilisée par le parcellaire pour la génération du résultat visuel
class statDendro : public statDendroBase{
public:
    statDendro(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom);

    void prepareResult();
    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique

    std::unique_ptr<Wt::WContainerWidget> getResult(){ return std::move(mResult); }
protected:
    std::unique_ptr<WContainerWidget> mResult;

};

// OLD ; plus utilisé car carte essence à été mise à jour, maintenant on a la carte essense majoritaire

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
