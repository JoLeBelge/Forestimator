#ifndef CAPPLICARTEPH_H
#define CAPPLICARTEPH_H
#include "cdicocarteph.h"
#include "gdal_priv.h"
#include "gdalwarper.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "cpl_string.h"
#include <iostream>
#include "boost/filesystem.hpp"
// pour les vecteurs
//#include "ogrsf_frmts.h"
//#include "gdal_utils.h"
using namespace std;

inline bool exists (const std::string& name);

class cAppliCartepH
{
public:
    cAppliCartepH();
    ~cAppliCartepH();
    void cartepH(std::string aOut, bool force=true);
private:
    int x,y;

    cDicoCartepH * dico;
    //GDALDataset  * poDatNH;
    //GDALDataset  * poDatNT;
    GDALDataset  * poDatZBIO;
    GDALDataset  * poDatPTS;
    //GDALDataset  * poDatTopo;
    GDALRasterBand * ZBIOBand;
    GDALRasterBand * PTSBand;
    //GDALRasterBand * NHBand;
    //GDALRasterBand * NTBand;
    //GDALRasterBand * TopoBand;
};

#endif // CAPPLICARTEPH_H