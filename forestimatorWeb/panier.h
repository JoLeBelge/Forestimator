#ifndef PANIER_H
#define PANIER_H
#pragma once
#include <Wt/WPanel.h>
#include "cwebaptitude.h"
#include "wopenlayers.h"
#include "layerbase.h"
#include "grouplayers.h"
#include <algorithm>

class WOpenLayers;
class groupLayers;

class panier: public WContainerWidget
{
public:
    panier(cWebAptitude * cWebApt);

    // var globales
    cDicoApt * mDico;
    cWebAptitude * m_app;
    WOpenLayers * mMap;
    groupLayers * mGroupL;

    // var de classe
    std::vector<std::shared_ptr<layerBase>> mVLs;

    // functions
    void addMap(std::string aCode);

    virtual void refresh();

    std::string activeLayerCode;
    // gestion de la légende de la carte
    void updateLegendeDiv();
    void updateLegende(const std::shared_ptr<layerBase> l);

private:

    Wt::WTable * mTable;
    Wt::WContainerWidget *mLegendDiv;

    WPushButton *bOrtho,*bIGN;
};

#endif // PANIER_H
