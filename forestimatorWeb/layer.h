#ifndef LAYER_H
#define LAYER_H
#pragma once
#include <Wt/WText.h>
#include "cdicoapt.h"
#include <Wt/WString.h>
//#include "grouplayers.h"
#include "cwebaptitude.h"

using namespace Wt;

// C'est la classe qui hérite de layerbase et qui intègre tout le volêt Wt - site internet avec interface graphique
// c'est également la classe qui fait le lien avec la classe Ess et les aptitudes qui vont avec

class groupLayers;
class Layer;

class Layer : public layerBase, public std::enable_shared_from_this<Layer>
{
public:

    Layer(std::shared_ptr<layerBase> aLB,WText * PWText);
    Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB,WText * PWText);


    // JS and layerbase
    void displayLayer() const;



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

    bool isVisible(){return mIsVisible;}
    std::shared_ptr<cEss> Ess(){ return mDico->getEss(EssCode());}

private:
    bool mIsVisible;
    bool mActive;

    //groupLayers * mGroupL;

    WText * mWtText;
};

#endif // LAYER_H
