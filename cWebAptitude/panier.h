#ifndef PANIER_H
#define PANIER_H
#pragma once
#include "cwebaptitude.h"
#include "wopenlayers.h"
#include "layer.h"
#include "grouplayers.h"

class panier: public WContainerWidget
{
public:
    panier(AuthApplication* app, cWebAptitude * cWebApt);

    cDicoApt * mDico;
    AuthApplication * m_app;
    cWebAptitude * mcWebAptitude;
    WOpenLayers * mMap;
    groupLayers * mGroupL;

    // functions
    void addMap(std::string aCode, TypeLayer type, std::shared_ptr<Layer> l);

private:
    std::vector<std::shared_ptr<Layer>> mVLs;
    Wt::WTable * mTable;

    WPushButton *bOrtho,*bIGN;
};

#endif // PANIER_H
