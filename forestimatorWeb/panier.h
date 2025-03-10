#ifndef PANIER_H
#define PANIER_H
#pragma once
#include "cwebaptitude.h"
#include "wopenlayers.h"
#include "layer.h"
#include "grouplayers.h"
#include <algorithm>

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
    std::vector<std::shared_ptr<Layer>> mVLs;

    // functions
    void addMap(std::string aCode, std::shared_ptr<Layer> l);
private:

    Wt::WTable * mTable;

    WPushButton *bOrtho,*bIGN;
};

#endif // PANIER_H
