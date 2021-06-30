#ifndef CAPPLICARTEPH_H
#define CAPPLICARTEPH_H
#include "cdicocarteph.h"
#include "gdal_priv.h"
#include "gdalwarper.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "cpl_string.h"
#include <iostream>
#include "boost/filesystem.hpp"

using namespace std;

inline bool exists (const std::string& name);

int cleNT(const siglePedo * s, int ZBIO, int TECO, double pH);
//int cleNT(dbo::ptr<siglePedo> s, int ZBIO, int TECO, double pH);

class cAppliCartepH
{
public:
    cAppliCartepH(bool bcarteNT=0, bool bcartepH=0);
    ~cAppliCartepH();
    void cartepH(std::string aOut, bool force=true);

    // je recrée un raster PTS car celui que j'ai fait avec gdal depuis le shp PTS n'est pas le même que celui de FR en utilisant la table qui lie chaque sigle pédo à un PTS
    void cartePTS(std::string aOut, bool force=true);

    void carteNT(std::string aOut, bool force=true);
private:
    int x,y;

    std::unique_ptr<cDicoCartepH> dico;
    GDALDataset  * poDatZBIO;
    GDALDataset  * poDatPTS;
    GDALRasterBand * ZBIOBand;
    GDALRasterBand * PTSBand;

};

#endif // CAPPLICARTEPH_H
