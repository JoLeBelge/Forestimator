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

// renvoie une grille de point, centre d'hexagone
std::vector<OGRPoint> hexbin(GDALDataset * mask);
// renvoie une liste de polgone qui sont des hexagones
std::vector<OGRPolygon*> hexGeombin(GDALDataset *mask);
double predHdom(std::vector<double> aVHs);
double getQ95(std::vector<double> aVHs);

/*
// la méthode Centroid de OGR bugge, alors je fait la mienne - plus nécessaire
OGRPoint *getCentroid(OGRPolygon * hex);
bool IsInsideHexagon(float x0, float y0, float d, float x, float y) ;
bool InsideHexagonB(float x0, float y0, float x, float y, float d);
*/

// modèle reçu de jérome le 9/06/2021
// reçu update du modèle par Adrien. 1) seul le MNH 2018 est suffisament correct. 2) la résolution des couches d'entrainement du modèle est de 5m ET c'est un MNH percentile 95 (on applique donc le percentile à deux reprises)
extern double k1hdom, k2hdom, k1vha,k2vha,k3vha, k1cmoy,k2cmoy;//,k3cmoyk1gha,;

// pour les stat sur un MNH
class statHdom {
public:
    statHdom(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom,bool computeStat=1);
    ~statHdom(){
       for (OGRPolygon * pol: mVaddPol) OGRGeometryFactory::destroyGeometry(pol);
       mVaddPol.clear();
       mStat.clear();
       mDistFrequ.clear();
    }
    std::shared_ptr<layerBase> Lay(){return mLay;}
    bool deserveChart();
    cDicoApt * Dico();

    void predictHdomHex();
    void predictDendro(bool onlyHdomStat=1);
    void prepareResult();
    //std::map<std::string, double> computeDistrH();
    std::vector<std::pair<std::string,double>> computeDistrH();
    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    std::unique_ptr<Wt::WContainerWidget> getResult(){ return std::move(mResult); }

    basicStat bshdom();
    basicStat bsDendro(std::string aVar="hdom");

protected:
    std::shared_ptr<layerBase> mLay;
    //std::vector<double> mStat; // un vecteur ; une valeur par cellule d'un are.
    std::vector<std::unique_ptr<statCellule>> mStat;
    //std::vector<std::string,double>
    std::vector<std::pair<std::string,double>>mDistFrequ;// pair avec range de valeur (genre 3-9) et proportion de la distribution
    OGRGeometry * mGeom;
    // geometrie supplémentaire à afficher sur l'image statique
    std::vector<OGRPolygon *> mVaddPol;
    int mNbOccurence;
    std::unique_ptr<WContainerWidget> mResult;
};

// pour les estimation dendrométriques : je fait une autre classe dérivée. Mais finalement il semblerai que deux classes entièrement séparée serai mieux, surtout à partir du moment ou l'estimation avec une seule cellule par polygone est appliquée (cad modèle pixel au lieu des modèles parcelle)
// en fait statHom doit rester la classe mère car c'est cette classe qui est utilisée par le parcellaire pour la génération du résultat visuel
class statDendro : public statHdom{
public:
    statDendro(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom);
    void predictDendroPix();
    bool deserveChart();
    void prepareResult();
    // equivalent de getChart, conteneur qui sera affiché dans la page de statistique
    //std::unique_ptr<Wt::WContainerWidget> getResult();
};

class statCellule{
public:
    statCellule(std::vector<double> *aVHs, int aSurf,bool computeDendro=0);
    //void computeHdom(){mHdom=k1hdom*mQ95+k2hdom*pow(mQ95,2);}

    // OLD OLD maintenant c'est une approche pixel
    /*
    void computeGha(){ if (mHdom!=0.0){mGha=k1gha*mVHA/mHdom;} else {mGha=0.0;}
                     }
    void computeNha(){mNha=40000.0*M_PI*mGha/pow(mCmoy,2);
                     // peut me renvoyer inf par moment
                      if (isinf(mNha) | isnan(mNha)){mNha=0.0;}
                     }
    void computeCmoy(){mCmoy=(k1cmoy*(mHdom-1.3)+k2cmoy*pow((mHdom-1.3),2))*pow(mMean/mQ95,k3cmoy);}*/

    //SI(Hpixi <= K2; 0; K1*(Hpixi-K2)^K3)



    //void computeHdom(){

    void printDetail();

    double mVHA, mHdom, mGha, mCmoy, mNha;
     int mSurf;
private:
    double mMean,mQ95;

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
