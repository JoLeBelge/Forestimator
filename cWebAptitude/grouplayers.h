#ifndef GROUPLAYERS_H
#define GROUPLAYERS_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WSignal.h>
#include "layer.h"
#include "legend.h"
#include <fstream>

class Layer;
class groupLayers;
class legend;
class ST;

enum TypeClassifST {FEE
                    ,CS
                   };

using namespace Wt;

class ST{
public:
    ST(cDicoApt * aDico);
    void vider();
    std::string NH(){ return mDico->NH(mNH);}
    std::string NT(){ return mDico->NT(mNT);}
    std::string TOPO(){ return mDico->TOPO(mTOPO);}
    std::string ZBIO(){ return mDico->ZBIO(mZBIO);}
    std::string STATION(){return mDico->station(mZBIO,mSt);}

    bool hasNH(){ return mDico->NH()->find(mNH)!=mDico->NH()->end();}
    bool hasNT(){ return mDico->NT()->find(mNT)!=mDico->NT()->end();}
    bool hasZBIO(){ return mDico->ZBIO()->find(mZBIO)!=mDico->ZBIO()->end();}
    bool hasTOPO(){ return mDico->topo()->find(mTOPO)!=mDico->topo()->end();}
    bool hasST(){ return mDico->station(mZBIO,mSt)!="not found";}
    bool readyFEE(){ return hasNH() && hasNT() && hasZBIO() && hasTOPO();}
    bool readyCS(){ return hasZBIO() && hasST();}
    bool hasEss(){ return HaveEss;}

    int mNH,mNT,mZBIO,mTOPO;
    bool HaveEss;
    cEss * mActiveEss; // l'essence qui intéresse l'utilisateur
    cDicoApt * mDico;
    // catalogue de station
    int mSt;
private:


};

class groupLayers: public WContainerWidget
{
public:
    groupLayers(cDicoApt * aDico,WContainerWidget *parent,WContainerWidget *infoW);
    void clickOnName(std::string aCode);
    void changeClassClick(WText *t);
    void update(std::string aCode);
    // click de l'utilisateur sur la carte pour extraire les valeurs des raster pour une position donnée
    void extractInfo(double x, double y);
    cDicoApt * Dico(){return mDico;}
    TypeClassifST TypeClas(){return mTypeClassifST;}
    std::string TypeClasStr(){
        std::string aRes("");
        switch (mTypeClassifST) {
        case FEE:
          aRes="FEE";
            break;
        case CS:
             aRes="CS";
            break;
        }
        return aRes;
    }

    // ne fait pas ce que je veux, il faut apparemment utiliser des anchor pour faire du bookmarking / hashtag
    Wt::Signal<bool>& focusMap(){
        //std::cout << "focus map () dans grouplayer done \n\n\n" << std::endl;
        return focusOnMap_;}
    ST * mStation;
    std::vector<Layer> * Layers(){return & mVLs;}

    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    //key ; code essence. Value ; code aptitude
    std::map<std::string,int> apts();

private:
    TypeClassifST mTypeClassifST;
    std::string currentClassifST; // 2 modes de classification des stations forestières ; FEE et CS
    std::vector<WText *> clasLabels_;
    std::vector<Layer> mVLs;
    cDicoApt * mDico;

    Wt::WTable                 *mEssTable;
    Wt::WTable                 *mClassifTable;
    Wt::WTable                 *mOtherTable;
    legend * mLegend;

    Wt::Signal<bool> focusOnMap_;

    WContainerWidget * mInfoW;
};

#endif // GROUPLAYERS_H
