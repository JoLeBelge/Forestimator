#ifndef PLAIPRFW_H
#define PLAIPRFW_H

#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include "boost/filesystem.hpp"
#include <unistd.h>
#include <cmath>
#include <sqlite3.h>

// classe pour lire la requete de l'IPRFW, calculer gha tot, gha en ep, generer un polygone disque depuis la position du centre de placette
class plaIPRFW
{
public:
    plaIPRFW(int ign, int npl, sqlite3 *db_);

    bool isPessiere(){return mPctGHA>=80;}
    std::string summary();

private:
    int mIGN,mNPL;
    int mAge; //en 2020
    double mGHA,mPctGHA,mSI;
    OGRPolygon * mPolygon;
    double mX,mY;
};

#endif // PLAIPRFW_H
