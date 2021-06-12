#ifndef LAYER_H
#define LAYER_H

#pragma once
#include <Wt/WText.h>
#include "cdicoapt.h"
#include <Wt/WString.h>
//#include "grouplayers.h"
#include "cwebaptitude.h"

using namespace Wt;

//C'est la classe qui hérite de layerbase et qui intègre tout le volêt Wt - site internet avec interface graphique
// c'est également la classe qui fait le lien avec la classe Ess et les aptitudes qui vont avec

class groupLayers;
class Layer;

class Layer : public layerBase, public std::enable_shared_from_this<Layer>
{
public:

    Layer(groupLayers * aGroupL, std::string aCode,WText * PWText);
    // objet layer équivalent de layerBase car sans interface mais permet de disposer dans GroupLayer d'un vecteur unique avec toutes les couches, celles visibles ou pas
    Layer(groupLayers * aGroupL, std::string aCode);
    // ce qui m'ennuie c'est que je dois réouvrir la connection à la BD car je recrée mes layerbase.
    //il faudrait pouvoir copier ma baseclass, comme ceci;
    Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB,WText * PWText);
    Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB);


    void displayLayer() const;
    std::vector<std::string> displayInfo(double x, double y);

    void setActive(bool b=true);
    bool IsActive() const {return mActive;}

    // signal pour cacher les nodes qui sont en mode expert
    Wt::Signal<bool>& changeExpertMode() { return expertMode_; }
    Wt::Signal<bool> expertMode_;
    void ExpertMode(bool GlobalExpertMode){
        // va envoyer le signal à setNodeVisible
        if (GlobalExpertMode) {
            mIsVisible=1;
        } else {
            if (mExpert) {
            mIsVisible=0;
            } else {
            mIsVisible=1;
            }
        }
         expertMode_.emit(mIsVisible);
    }
    // utilisé dans les construteurs pour changer en un coup mIsVisible et mExpert
    void setExpert(bool expert){
        mExpert=expert;
        mIsVisible=!expert;
    }
    // pour savoir distinguer mode expert et mode normal au niveau de chaque layer

    bool isVisible(){return mIsVisible;}

    std::shared_ptr<cEss> Ess(){
        //code == 'EP_FEE'
        //std::cout <<" layer code " << mCode << ", substr" <<mCode.substr(0,2) << std::endl;
        return mDico->getEss(mCode.substr(0,2));}
    bool l4Stat(){return mLay4Stat;}

private:
    bool mIsVisible;
    bool mActive;
    // est ce une couche qu'on veux pouvoir visulaliser, qu'on veux pouvoir calculer des stats dessus?
    bool mLay4Visu;
    bool mLay4Stat;
    groupLayers * mGroupL;

    WText * mWtText; // ça c'est le lien entre l'objet et l'affichage dans la page web
    std::string mLabel;
};

#endif // LAYER_H
